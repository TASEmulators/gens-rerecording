#include <stdio.h>
#include "G_ddraw.h"
#include "G_dsound.h"
#include "psg.h"
#include "ym2612.h"
#include "mem_M68K.h"
#include "vdp_io.h"
#include "G_Main.h"
#include "gens.h"
#include "Rom.h"
#include "wave.h"
#include "pcm.h"
#include "misc.h"		// for Have_MMX flag
#include "scrshot.h"
#include <math.h> // Nitsuja includes this for his softsound filter

LPDIRECTSOUND lpDS;
WAVEFORMATEX MainWfx;
DSBUFFERDESC dsbdesc;
LPDIRECTSOUNDBUFFER lpDSPrimary, lpDSBuffer;
HMMIO MMIOOut;
MMCKINFO CkOut;
MMCKINFO CkRIFF;
MMIOINFO MMIOInfoOut;

void End_Sound(void);

int Seg_L[882] = {0}, Seg_R[882] = {0};
int Last_Seg_L[882] = {0}, Last_Seg_R[882] = {0};
int Seg_Length, SBuffer_Length;
int Sound_Rate = 44100, Sound_Segs = 8; //Sound defaults to 44100, instead of 22050, nitsuja did this, I agree
int Bytes_Per_Unit;
int Sound_Enable;
int Sound_Stereo = 1;
int Sound_Soften = 0; // Modif N.
int Sound_Is_Playing = 0;
int Sound_Initialised = 0;
int WAV_Dumping = 0;
int GYM_Playing = 0;
int WP, RP;
unsigned short MastVol = 128;
extern unsigned long FrameCount;
unsigned long FrameCountAtLastAudioOutput = -1;

unsigned int Sound_Interpol[882];
unsigned int Sound_Extrapol[312][2];

char Dump_Dir[1024] = "";
char Dump_GYM_Dir[1024] = "";


int Init_Sound(HWND hWnd)
{
   	HRESULT rval;
	WAVEFORMATEX wfx;
	int i;
 
	if (Sound_Initialised) return 0;
	End_Sound();
	
	switch (Sound_Rate)
	{
		case 11025:
			if (CPU_Mode)
				Seg_Length = 220;
			else
				Seg_Length = 184;
			break;
			
		case 22050:
			if (CPU_Mode)
				Seg_Length = 441;
			else
				Seg_Length = 368;
			break;
			
		case 44100:
			if (CPU_Mode)
				Seg_Length = 882;
			else
				Seg_Length = 735;
			break;
	}

	if (CPU_Mode)
	{
		for(i = 0; i < 312; i++)
		{
			Sound_Extrapol[i][0] = ((Seg_Length * i) / 312);
			Sound_Extrapol[i][1] = (((Seg_Length * (i + 1)) / 312) - Sound_Extrapol[i][0]);
		}

		for(i = 0; i < Seg_Length; i++)
			Sound_Interpol[i] = ((312 * i) / Seg_Length);
	}
	else
	{
		for(i = 0; i < 262; i++)
		{
			Sound_Extrapol[i][0] = ((Seg_Length * i) / 262);
			Sound_Extrapol[i][1] = (((Seg_Length * (i + 1)) / 262) - Sound_Extrapol[i][0]);
		}

		for(i = 0; i < Seg_Length; i++)
			Sound_Interpol[i] = ((262 * i) / Seg_Length);
	}

	memset(Seg_L, 0, Seg_Length << 2);
	memset(Seg_R, 0, Seg_Length << 2);

	WP = 0;
	RP = 0;

	rval = DirectSoundCreate(NULL, &lpDS, NULL);

	if (rval != DS_OK) return 0;

	rval = lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
//	rval = lpDS->SetCooperativeLevel(hWnd, DSSCL_WRITEPRIMARY);

	if(rval != DS_OK) 
	{
		lpDS->Release();
		lpDS = NULL;
		return 0;
	}

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

	rval = lpDS->CreateSoundBuffer(&dsbdesc, &lpDSPrimary, NULL);

	if(rval != DS_OK) 
	{
		lpDS->Release();
		lpDS = NULL;
		return 0;
	}

    memset(&wfx, 0, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM; 
	if (Sound_Stereo) wfx.nChannels = 2;
	else wfx.nChannels = 1;
	wfx.nSamplesPerSec = Sound_Rate;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = Bytes_Per_Unit = (wfx.wBitsPerSample / 8) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * Bytes_Per_Unit;

	rval = lpDSPrimary->SetFormat(&wfx);
 
	if(rval != DS_OK) 
	{
		lpDSPrimary->Release();
		lpDSPrimary = NULL;
		lpDS->Release();
		lpDS = NULL;
		return 0;
	}

    memset(&MainWfx, 0, sizeof(WAVEFORMATEX));
    MainWfx.wFormatTag = WAVE_FORMAT_PCM; 
	if (Sound_Stereo) MainWfx.nChannels = 2;
	else MainWfx.nChannels = 1;
	MainWfx.nSamplesPerSec = Sound_Rate;
	MainWfx.wBitsPerSample = 16;
	MainWfx.nBlockAlign = Bytes_Per_Unit = (MainWfx.wBitsPerSample / 8) * MainWfx.nChannels;
	MainWfx.nAvgBytesPerSec = MainWfx.nSamplesPerSec * Bytes_Per_Unit;

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
    dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
	dsbdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS;
	dsbdesc.dwBufferBytes = SBuffer_Length = Seg_Length * Sound_Segs * Bytes_Per_Unit;
	dsbdesc.lpwfxFormat = &MainWfx;

//	sprintf(STR, "Seg l : %d   Num Seg : %d   Taille : %d", Seg_Length, Sound_Segs, Bytes_Per_Unit);
//	MessageBox(HWnd, STR, "", MB_OK);

	rval = lpDS->CreateSoundBuffer(&dsbdesc, &lpDSBuffer, NULL);

	if(rval != DS_OK) 
	{
		lpDS->Release();
		lpDS = NULL;
		return 0;
	}

	return(Sound_Initialised = 1);
}


void End_Sound()
{
	if(Sound_Initialised)
	{
		Sound_Initialised = 0;

		if (lpDSPrimary)
		{
			lpDSPrimary->Release();
			lpDSPrimary = NULL;
		}

		if (lpDSBuffer)
		{
			lpDSBuffer->Stop();
			Sound_Is_Playing = 0;
			lpDSBuffer->Release();
			lpDSBuffer = NULL;
		}

		if (lpDS)
		{
			lpDS->Release();
			lpDS = NULL;
		}
	}
}

int Get_Current_Seg(void)
{
	unsigned long R;
	
	lpDSBuffer->GetCurrentPosition(&R, NULL);

	return(R / (Seg_Length * Bytes_Per_Unit));
}


int Check_Sound_Timing(void)
{
	unsigned long R;
	
	lpDSBuffer->GetCurrentPosition(&R, NULL);

	RP = R / (Seg_Length * Bytes_Per_Unit);

	if (RP == ((WP + 1) & (Sound_Segs - 1))) return 2;

	if (RP != WP) return 1;

	return 0;
}


static int lowpass (int* buf, int i, int len) //Nitsuja's sound softening filter
{
	//return (8*buf[i]
	//+ 2*buf[(i+1)%len] + 2*buf[(i-1+len)%len]
	//+ 1*buf[(i+2)%len] + 1*buf[(i-2+len)%len]) / 14;

	return (3*buf[i]
	+ 2*buf[min(i+1,len-1)] + 2*buf[max(i-1,0)]
	+ 1*buf[min(i+2,len-1)] + 1*buf[max(i-2,0)]) / 9;
}

void Write_Sound_Stereo(short *Dest, int length)
{
	int i, out_L, out_R;
	short *dest = Dest;
	
	if(Sound_Soften) // Modif N.
	{
		for(i = 0; i < Seg_Length; i++)
		{
			out_L = lowpass(Seg_L,i,Seg_Length);

			if (out_L < -0x8000) *dest++ = -0x8000;
			else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
			else *dest++ = (short) out_L;
							
			out_R = lowpass(Seg_R,i,Seg_Length);

			if (out_R < -0x8000) *dest++ = -0x8000;
			else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
			else *dest++ = (short) out_R;
		}
		for(i = 0; i < Seg_Length; i++)
			Seg_L[i] = 0;
		for(i = 0; i < Seg_Length; i++)
			Seg_R[i] = 0;
	}
	else
	{
		for(i = 0; i < Seg_Length; i++)
		{
			out_L = Seg_L[i];
			Seg_L[i] = 0;

			if (out_L < -0x8000) *dest++ = -0x8000;
			else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
			else *dest++ = (short) out_L;
							
			out_R = Seg_R[i];
			Seg_R[i] = 0;

			if (out_R < -0x8000) *dest++ = -0x8000;
			else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
			else *dest++ = (short) out_R;
		}
	}
}


void Dump_Sound_Stereo(short *Dest, int length)
{
	int i, out_L, out_R;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Length; i++)
	{
		out_L = Seg_L[i];

		if (out_L < -0x8000) *dest++ = -0x8000;
		else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_L;
						
		out_R = Seg_R[i];

		if (out_R < -0x8000) *dest++ = -0x8000;
		else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_R;
	}
	out_R = (int)(((out_R * MastVol) >> 8) / 255.0);
	out_L = (int)(((out_L * MastVol) >> 8) / 255.0);
}


void Write_Sound_Mono(short *Dest, int length)
{
	int i, out;
	short *dest = Dest;

	if(Sound_Soften) // Modif N.
	{
		for(i = 0; i < Seg_Length; i++)
		{
			out = lowpass(Seg_L,i,Seg_Length) + lowpass(Seg_R,i,Seg_Length);
			if (out < -0x10000) *dest++ = -0x8000;
			else if (out > 0xFFFF) *dest++ = 0x7FFF;
			else *dest++ = (short) (out >> 1);
		}
		for(i = 0; i < Seg_Length; i++)
			Seg_L[i] = Seg_R[i] = 0;
	}
	else
	{
		for(i = 0; i < Seg_Length; i++)
		{
			out = Seg_L[i] + Seg_R[i];
			Seg_L[i] = Seg_R[i] = 0;

			if (out < -0x10000) *dest++ = -0x8000;
			else if (out > 0xFFFF) *dest++ = 0x7FFF;
			else *dest++ = (short) (out >> 1);
		}
	}
}


void Dump_Sound_Mono(short *Dest, int length)
{
	int i, out;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Length; i++)
	{
		out = Seg_L[i] + Seg_R[i];

		if (out < -0x10000) *dest++ = -0x8000;
		else if (out > 0xFFFF) *dest++ = 0x7FFF;
		else *dest++ = (short) (out >> 1);
	}
	out = (int)(((out * MastVol) >> 8) / 255.0);
}


int Write_Sound_Buffer(void *Dump_Buf)
{
	LPVOID lpvPtr1;
	DWORD dwBytes1; 
	HRESULT rval;

	if (Dump_Buf)
	{
		if (Sound_Stereo) Dump_Sound_Stereo((short *) Dump_Buf, Seg_Length);
		else Dump_Sound_Mono((short *) Dump_Buf, Seg_Length);
	}
	else
	{
		rval = lpDSBuffer->Lock(WP * Seg_Length * Bytes_Per_Unit, Seg_Length * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);

		if (rval == DSERR_BUFFERLOST)
		{
	        lpDSBuffer->Restore();
			rval = lpDSBuffer->Lock(WP * Seg_Length * Bytes_Per_Unit, Seg_Length * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);
		}

		if (rval == DSERR_BUFFERLOST || !lpvPtr1) return 0;

		if(SlowDownMode && FrameCount == FrameCountAtLastAudioOutput)
		{
			for(int i = 0; i < Seg_Length; i++)
				Seg_L[i] = Last_Seg_L[i];
			for(int i = 0; i < Seg_Length; i++)
				Seg_R[i] = Last_Seg_R[i];
		}
		else
		{
			for(int i = 0; i < Seg_Length; i++)
				Seg_L[i] = (Seg_L[i] * MastVol) >> 8;
			for(int i = 0; i < Seg_Length; i++)
				Seg_R[i] = (Seg_R[i] * MastVol) >> 8;

			if(SlowDownMode)
			{
				for(int i = 0; i < Seg_Length; i++)
					Last_Seg_L[i] = Seg_L[i];
				for(int i = 0; i < Seg_Length; i++)
					Last_Seg_R[i] = Seg_R[i];
			}

			FrameCountAtLastAudioOutput = FrameCount;
		}

		if (Sound_Stereo)
		{
			if (Have_MMX && !Sound_Soften) Write_Sound_Stereo_MMX(Seg_L, Seg_R, (short *) lpvPtr1, Seg_Length); //Nitsuja changed this
			else Write_Sound_Stereo((short *) lpvPtr1, Seg_Length);
		}
		else
		{
			if (Have_MMX && !Sound_Soften) Write_Sound_Mono_MMX(Seg_L, Seg_R, (short *) lpvPtr1, Seg_Length); //Nitsuja changed this
			else Write_Sound_Mono((short *) lpvPtr1, Seg_Length);
		}

		lpDSBuffer->Unlock(lpvPtr1, dwBytes1, NULL, NULL);
	}

	return 1;
}


int Clear_Sound_Buffer(void)
{
	LPVOID lpvPtr1;
	DWORD dwBytes1; 
	HRESULT rval;
	int i;

	if (!Sound_Initialised) return 0;
		
	rval = lpDSBuffer->Lock(0, Seg_Length * Sound_Segs * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);

    if (rval == DSERR_BUFFERLOST)
	{
        lpDSBuffer->Restore();
		rval = lpDSBuffer->Lock(0, Seg_Length * Sound_Segs * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);

    }

	if (rval == DS_OK)
	{
		signed short *w = (signed short *)lpvPtr1;

		for(i = 0; i < Seg_Length * Sound_Segs * Bytes_Per_Unit; i+= 2)
			*w++ = (signed short)0;

		rval = lpDSBuffer->Unlock(lpvPtr1, dwBytes1, NULL, NULL);

		if (rval == DS_OK) return 1;
    }
	
    return 0;
}


int Play_Sound(void)
{
	HRESULT rval;

	if (Sound_Is_Playing) return 1;
	
	rval = lpDSBuffer->Play(0, 0, DSBPLAY_LOOPING);

	Clear_Sound_Buffer();

	if (rval != DS_OK) return 0;
	
	return(Sound_Is_Playing = 1);
}


int Stop_Sound(void)
{
	HRESULT rval;

	rval = lpDSBuffer->Stop();

	if (rval != DS_OK) return 0;
	
	Sound_Is_Playing = 0;

	return 1;
}


int Start_WAV_Dump(void)
{
	char Name[1024] = "";

	if (!(Sound_Is_Playing) || !(Game)) return(0);
		
	if (WAV_Dumping)
	{
		Put_Info("WAV sound is already dumping", 1000);
		return(0);
	}

	strcpy(Name, Dump_Dir);
	strcat(Name, Rom_Name);
	
	if (WaveCreateFile(Name, &MMIOOut, &MainWfx, &CkOut, &CkRIFF))
	{
		Put_Info("Error in WAV dumping", 1000);
		return(0);
	}

	if (WaveStartDataWrite(&MMIOOut, &CkOut, &MMIOInfoOut))
	{
		Put_Info("Error in WAV dumping", 1000);
		return(0);
	}

	Put_Info("Starting to dump WAV sound", 1000);
	WAV_Dumping = 1;


	return 1;
}


int Update_WAV_Dump(void)
{
	unsigned char Buf_Tmp[882 * 4 + 16];
	unsigned int length, Writted;
	
	if (!WAV_Dumping) return 0;

	Write_Sound_Buffer(Buf_Tmp);

	length = Seg_Length << 1;
	
	if (Sound_Stereo) length *= 2;
	
	if (WaveWriteFile(MMIOOut, length, &Buf_Tmp[0], &CkOut, &Writted, &MMIOInfoOut))
	{
		Put_Info("Error in WAV dumping", 1000);
		return 0;
	}

	return(1);
}

int Update_WAV_Dump_AVI(void)
{
	unsigned char Buf_Tmp[882 * 4 + 16];
	unsigned int length;

	Write_Sound_Buffer(Buf_Tmp);

	length = Seg_Length << 1;
	
	if (Sound_Stereo) length *= 2;
	
	UpdateSoundAVI(Buf_Tmp,length,Sound_Rate,Sound_Stereo);

	return(1);
}

int Stop_WAV_Dump(void)
{
	if (!WAV_Dumping)
	{
		Put_Info("Already stopped", 1000);
		return 0;
	}

	if (WaveCloseWriteFile(&MMIOOut, &CkOut, &CkRIFF, &MMIOInfoOut, 0))
		return 0;

	Put_Info("WAV dump stopped", 1000);
	WAV_Dumping = 0;

	return 1;
}


int Start_GYM_Dump(void)
{
	char Name[1024], Name2[1024];
	char ext[12] = "_000.gym";
	unsigned char YM_Save[0x200], t_buf[4];
	int num = -1, i, j;
	unsigned long bwr;

	SetCurrentDirectory(Gens_Path);

	if (!Game) return 0;
		
	if (GYM_Dumping)
	{
		Put_Info("GYM sound is already dumping", 1000);
		return 0;
	}

	strcpy(Name, Dump_GYM_Dir);
	strcat(Name, Rom_Name);

	do
	{
		if (num++ > 99999)
		{
			Put_Info("Too much GYM files in your GYM directory", 1000);
			GYM_File = NULL;
			return(0);
		}

		ext[0] = '_';
		i = 1;

		j = num / 10000;
		if (j) ext[i++] = '0' + j;
		j = (num / 1000) % 10;
		if (j) ext[i++] = '0' + j;
		j = (num / 100) % 10;
		ext[i++] = '0' + j;
		j = (num / 10) % 10;
		ext[i++] = '0' + j;
		j = num % 10;
		ext[i++] = '0' + j;
		ext[i++] = '.';
		ext[i++] = 'g';
		ext[i++] = 'y';
		ext[i++] = 'm';
		ext[i] = 0;

		if ((strlen(Name) + strlen(ext)) > 1023) return(0);
		
		strcpy(Name2, Name);
		strcat(Name2, ext);
	} while ((GYM_File = CreateFile(Name2, GENERIC_WRITE, NULL,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE);

	YM2612_Save(YM_Save);

	for(i = 0x30; i < 0x90; i++)
	{
		t_buf[0] = 1;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i];
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);

		t_buf[0] = 2;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i + 0x100];
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
	}


	for(i = 0xA0; i < 0xB8; i++)
	{
		t_buf[0] = 1;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i];
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);

		t_buf[0] = 2;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i + 0x100];
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
	}

	t_buf[0] = 1;
	t_buf[1] = 0x22;
	t_buf[2] = YM_Save[0x22];
	WriteFile(GYM_File, t_buf, 3, &bwr, NULL);

	t_buf[0] = 1;
	t_buf[1] = 0x27;
	t_buf[2] = YM_Save[0x27];
	WriteFile(GYM_File, t_buf, 3, &bwr, NULL);

	t_buf[0] = 1;
	t_buf[1] = 0x28;
	t_buf[2] = YM_Save[0x28];
	WriteFile(GYM_File, t_buf, 3, &bwr, NULL);


	Put_Info("Starting to dump GYM sound", 1000);
	GYM_Dumping = 1;

	return 1;
}


int Stop_GYM_Dump(void)
{
	if (!GYM_Dumping)
	{
		Put_Info("Already stopped", 1000);
		return 0;
	}

	if (GYM_File) CloseHandle(GYM_File);
//
	Clear_Sound_Buffer();

	Put_Info("GYM dump stopped", 1000);
	GYM_Dumping = 0;

	return 1;
}


int Start_Play_GYM(const char* filename)
{
	char Name[1024];
	OPENFILENAME ofn;

	if (Game || !(Sound_Enable)) return(0);
		
	if (GYM_Playing)
	{
		Put_Info("Already playing GYM", 1000);
		return 0;
	}

	End_Sound();
	CPU_Mode = 0;

	if (!Init_Sound(HWnd)) 
	{
		Sound_Enable = 0;
		Put_Info("Can't initialise DirectSound", 1000);
		return 0;
	}

	Play_Sound();
	
	memset(Name, 0, 1024);
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = HWnd;
	ofn.hInstance = ghInstance;
	ofn.lpstrFile = Name;
	ofn.nMaxFile = 1023;
	ofn.lpstrFilter = "GYM files\0*.gym\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = Dump_GYM_Dir;
	ofn.lpstrDefExt = "gym";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if(filename)
		strcpy(Name, filename);

	if (filename || GetOpenFileName(&ofn))
	{
		GYM_File = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (GYM_File == INVALID_HANDLE_VALUE) return 0;
	}
	else return 0;

	YM2612_Init(CLOCK_NTSC / 7, Sound_Rate, YM2612_Improv);
	PSG_Init(CLOCK_NTSC / 15, Sound_Rate);
	GYM_Playing = 1;

	Put_Info("Starting to play GYM", 1000);

	return 1;
}


int Stop_Play_GYM(void)
{
	if (!GYM_Playing)
	{
		Put_Info("Already stopped", 1000);
		return 0;
	}

	if (GYM_File) CloseHandle(GYM_File);
	Clear_Sound_Buffer();
	GYM_Playing = 0;

	Put_Info("Stop playing GYM", 1000);

	return 1;
}


int GYM_Next(void)
{
	unsigned char c, c2;
	unsigned long l;
	int *buf[2];

	buf[0] = Seg_L;
	buf[1] = Seg_R;

	if (!(GYM_Playing) || !(GYM_File)) return 0;
	
	do
	{
		ReadFile(GYM_File, &c, 1, &l, NULL);
		if (l == 0) return 0;

		switch(c)
		{
			case 0:
				PSG_Update(buf, Seg_Length);
				if (YM2612_Enable) YM2612_Update(buf, Seg_Length);
				break;

			case 1:
				ReadFile(GYM_File, &c2, 1, &l, NULL);
				YM2612_Write(0, c2);
				ReadFile(GYM_File, &c2, 1, &l, NULL);
				YM2612_Write(1, c2);
				break;

			case 2:
				ReadFile(GYM_File, &c2, 1, &l, NULL);
				YM2612_Write(2, c2);
				ReadFile(GYM_File, &c2, 1, &l, NULL);
				YM2612_Write(3, c2);
				break;

			case 3:
				ReadFile(GYM_File, &c2, 1, &l, NULL);
				PSG_Write(c2);
				break;
		}

	} while (c);

	return 1;
}


int Play_GYM(void)
{
	if (!GYM_Next())
	{
		Stop_Play_GYM();
		return 0;
	}

	while (WP == Get_Current_Seg());
			
	RP = Get_Current_Seg();

	while (WP != RP)
	{
		Write_Sound_Buffer(NULL);
		WP = (WP + 1) & (Sound_Segs - 1);

		if (WP != RP)
		{
			if (!GYM_Next())
			{
				Stop_Play_GYM();
				return 0;
			}
		}
	}

	return 1;
}

int Play_GYM_Bench(void)
{
	if (!GYM_Next())
	{
		Stop_Play_GYM();
		return 0;
	}

	Write_Sound_Buffer(NULL);
	WP = (WP + 1) & (Sound_Segs - 1);

	return 1;
}