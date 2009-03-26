#include "port.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <string.h>
#include "g_ddraw.h"
#include "g_dsound.h"
#include "psg.h"
#include "ym2612.h"
#include "mem_m68k.h"
#include "vdp_io.h"
#include "g_main.h"
#include "gens.h"
#include "rom.h"
#include "wave.h"
#include "pcm.h"
#include "misc.h"		// for Have_MMX flag
#ifdef WITH_GTK
#include "glade/support.h"
#endif
void End_Sound(void);

int Seg_L[882], Seg_R[882];
int Seg_Lenght, SBuffer_Lenght;
int Sound_Rate = 44100, Sound_Segs = 8;
int Bytes_Per_Unit = 4;
int Sound_Enable;
int Sound_Stereo = 1;
int Sound_Is_Playing = 0;
int Sound_Initialised = 0;
int WAV_Dumping = 0;
int GYM_Playing = 0;
int WP, RP;
unsigned int Sound_Interpol[882];
unsigned int Sound_Extrapol[312][2];

char Dump_Dir[1024] = "";
char Dump_GYM_Dir[1024] = "";

static int audio_len=0;
unsigned char *pMsndOut = 0;

unsigned char* audiobuf = 0;

void proc(void *user, Uint8 *buffer, int len)
{
	if(audio_len < (int)len)
	{
		memcpy(buffer, user, audio_len);
		audio_len = 0;
		return;
	}
	memcpy(buffer, user, len);
	audio_len -= len;
	memcpy(user, (unsigned char*)user + len, audio_len);
}

int Init_Sound(int hWnd)
{
	int i;
 	
	if (Sound_Initialised)
	{
		fprintf(stderr, "sound already init\n");
		return 0;
	}
	End_Sound();
	
	switch (Sound_Rate)
	{
		case 11025:
			if (CPU_Mode)
				Seg_Lenght = 220;
			else
				Seg_Lenght = 184;
			break;
			
		case 22050:
			if (CPU_Mode)
				Seg_Lenght = 441;
			else
				Seg_Lenght = 368;
			break;
			
		case 44100:
			if (CPU_Mode)
				Seg_Lenght = 882;
			else
				Seg_Lenght = 735;
			break;
	}

	if (CPU_Mode)
	{
		for(i = 0; i < 312; i++)
		{
			Sound_Extrapol[i][0] = ((Seg_Lenght * i) / 312);
			Sound_Extrapol[i][1] = (((Seg_Lenght * (i + 1)) / 312) - Sound_Extrapol[i][0]);
		}

		for(i = 0; i < Seg_Lenght; i++)
			Sound_Interpol[i] = ((312 * i) / Seg_Lenght);
	}
	else
	{
		for(i = 0; i < 262; i++)
		{
			Sound_Extrapol[i][0] = ((Seg_Lenght * i) / 262);
			Sound_Extrapol[i][1] = (((Seg_Lenght * (i + 1)) / 262) - Sound_Extrapol[i][0]);
		}

		for(i = 0; i < Seg_Lenght; i++)
			Sound_Interpol[i] = ((262 * i) / Seg_Lenght);
	}

	memset(Seg_L, 0, Seg_Lenght << 2);
	memset(Seg_R, 0, Seg_Lenght << 2);

	WP = 0;
	RP = 0;
	
	if(-1 == SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		return 0;
	}
	
	pMsndOut= (unsigned char *)malloc(Seg_Lenght<<2);
	
	SDL_AudioSpec spec;
	
	spec.freq = Sound_Rate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = 1024;
	spec.callback = proc;
	audiobuf=(unsigned char *)malloc((spec.samples*spec.channels*2*4)*sizeof(short));

	spec.userdata = audiobuf;
	
	memset(audiobuf,0,(spec.samples*spec.channels*2*4)*sizeof(short));
	if(SDL_OpenAudio(&spec, 0) != 0)
	{
		return 0;
	}
	SDL_PauseAudio(0);
	return(Sound_Initialised = 1);
}


void End_Sound()
{
	free(audiobuf); audiobuf = 0;
	free(pMsndOut); pMsndOut = 0;
	if (Sound_Initialised)
	{
		Sound_Is_Playing = 0;
		Sound_Initialised = 0;
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int Get_Current_Seg(void)
{
	return 0;
}


int Check_Sound_Timing(void)
{
	return 0;
}

 
void Write_Sound_Stereo(short *Dest, int lenght)
{
	int i, out_L, out_R;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Lenght; i++)
	{
		out_L = Seg_L[i];
		Seg_L[i] = 0;

		if (out_L < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_L;
						
		out_R = Seg_R[i];
		Seg_R[i] = 0;

		if (out_R < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_R;
	}
}


void Dump_Sound_Stereo(short *Dest, int lenght)
{
	int i, out_L, out_R;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Lenght; i++)
	{
		out_L = Seg_L[i];

		if (out_L < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_L > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_L;
						
		out_R = Seg_R[i];

		if (out_R < -0x7FFF) *dest++ = -0x7FFF;
		else if (out_R > 0x7FFF) *dest++ = 0x7FFF;
		else *dest++ = (short) out_R;
	}
}


void Write_Sound_Mono(short *Dest, int lenght)
{
	int i, out;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Lenght; i++)
	{
		out = Seg_L[i] + Seg_R[i];
		Seg_L[i] = Seg_R[i] = 0;

		if (out < -0xFFFF) *dest++ = -0x7FFF;
		else if (out > 0xFFFF) *dest++ = 0x7FFF;
		else *dest++ = (short) (out >> 1);
	}
}


void Dump_Sound_Mono(short *Dest, int lenght)
{
	int i, out;
	short *dest = Dest;
	
	for(i = 0; i < Seg_Lenght; i++)
	{
		out = Seg_L[i] + Seg_R[i];

		if (out < -0xFFFF) *dest++ = -0x7FFF;
		else if (out > 0xFFFF) *dest++ = 0x7FFF;
		else *dest++ = (short) (out >> 1);
	}
}


int Write_Sound_Buffer(void *Dump_Buf)
{
	struct timespec rqtp = {0,1000000};
	
	SDL_LockAudio();
	
	if (Sound_Stereo)
	{
		if (Have_MMX) Write_Sound_Stereo_MMX(Seg_L, Seg_R, (short *) pMsndOut, Seg_Lenght);
		else Write_Sound_Stereo((short *) pMsndOut, Seg_Lenght); 
	}
	else
	{
		if (Have_MMX) Write_Sound_Mono_MMX(Seg_L, Seg_R, (short *) pMsndOut, Seg_Lenght);
		else Write_Sound_Mono((short *) pMsndOut, Seg_Lenght); 
	}
	memcpy(audiobuf+audio_len,pMsndOut,Seg_Lenght*4);

#if NESVIDEOS_LOGGING /*** AUDIO LOGGING ***/
    static FILE* out = NULL;
    if(!out) out = fopen("s.log", "wb");
    if(out && LoggingEnabled)
    {
        fwrite(pMsndOut, 1, Seg_Lenght*4, out);
        fflush(out);
    }
#endif
	
	audio_len+=Seg_Lenght*4;
	SDL_UnlockAudio();
	while(audio_len>1024*2*2*4) nanosleep(&rqtp,NULL);//SDL_Delay(1);
	return 1;
}


int Clear_Sound_Buffer(void)
{
#if 0
	LPVOID lpvPtr1;
	DWORD dwBytes1;
	HRESULT rval;
	int i;

	if (!Sound_Initialised) return 0;
		
	rval = lpDSBuffer->Lock(0, Seg_Lenght * Sound_Segs * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);

    if (rval == DSERR_BUFFERLOST)
	{
        lpDSBuffer->Restore();
		rval = lpDSBuffer->Lock(0, Seg_Lenght * Sound_Segs * Bytes_Per_Unit, &lpvPtr1, &dwBytes1, NULL, NULL, 0);

    }

	if (rval == DS_OK)
	{
		signed short *w = (signed short *)lpvPtr1;

		for(i = 0; i < Seg_Lenght * Sound_Segs * Bytes_Per_Unit; i+= 2)
			*w++ = (signed short)0;

		rval = lpDSBuffer->Unlock(lpvPtr1, dwBytes1, NULL, NULL);

		if (rval == DS_OK) return 1;
    }
#endif
    return 0;
}


int Play_Sound(void)
{
#if 0
	HRESULT rval;

	if (Sound_Is_Playing) return 1;
	
	rval = lpDSBuffer->Play(0, 0, DSBPLAY_LOOPING);

	Clear_Sound_Buffer();

	if (rval != DS_OK) return 0;
#endif
	return(Sound_Is_Playing = 1);
}


int Stop_Sound(void)
{
#if 0
	HRESULT rval;

	rval = lpDSBuffer->Stop();

	if (rval != DS_OK) return 0;
	
	Sound_Is_Playing = 0;
#endif
	return 1;
}


int Start_WAV_Dump(void)
{
#if 0
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
#endif
	return 1;
}


int Update_WAV_Dump(void)
{
#if 0
	unsigned char Buf_Tmp[882 * 4 + 16];
	unsigned int lenght, Writted;
	
	if (!WAV_Dumping) return 0;

	Write_Sound_Buffer(Buf_Tmp);

	lenght = Seg_Lenght << 1;

	if (Sound_Stereo) lenght *= 2;
	
	if (WaveWriteFile(MMIOOut, lenght, &Buf_Tmp[0], &CkOut, &Writted, &MMIOInfoOut))
	{
		Put_Info("Error in WAV dumping", 1000);
		return 0;
	}
#endif
	return(1);
}


int Stop_WAV_Dump(void)
{
#if 0
	if (!WAV_Dumping)
	{
		Put_Info("Already stopped", 1000);
		return 0;
	}

	if (WaveCloseWriteFile(&MMIOOut, &CkOut, &CkRIFF, &MMIOInfoOut, 0))
		return 0;

	Put_Info("WAV dump stopped", 1000);
	WAV_Dumping = 0;
#endif
	return 1;
}


int Start_GYM_Dump(void)
{
	char Name[1024], Name2[1024];
	char ext[12] = "_000.gym";
	unsigned char YM_Save[0x200], t_buf[4];
	int num = -1, i, j;
#ifndef __PORT__
	unsigned long bwr;
#endif
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
	}
#ifdef __PORT__
		while(Exists(Name2));
#else
		while ((GYM_File = CreateFile(Name2, GENERIC_WRITE, NULL,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE);
#endif
		
#ifdef __PORT__
	GYM_File = fopen(Name2, "w");
#endif

	YM2612_Save(YM_Save);

	for(i = 0x30; i < 0x90; i++)
	{
		t_buf[0] = 1;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif
		t_buf[0] = 2;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i + 0x100];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif
	}


	for(i = 0xA0; i < 0xB8; i++)
	{
		t_buf[0] = 1;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif

		t_buf[0] = 2;
		t_buf[1] = i;
		t_buf[2] = YM_Save[i + 0x100];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif
	}

	t_buf[0] = 1;
	t_buf[1] = 0x22;
	t_buf[2] = YM_Save[0x22];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif

	t_buf[0] = 1;
	t_buf[1] = 0x27;
	t_buf[2] = YM_Save[0x27];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif

	t_buf[0] = 1;
	t_buf[1] = 0x28;
	t_buf[2] = YM_Save[0x28];
#ifdef __PORT__
		fwrite(t_buf,3,1,GYM_File);
#else
		WriteFile(GYM_File, t_buf, 3, &bwr, NULL);
#endif


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

#ifdef __PORT__
	if (GYM_File) fclose(GYM_File);
#else
	if (GYM_File) CloseHandle(GYM_File);
#endif
	Clear_Sound_Buffer();

	Put_Info("GYM dump stopped", 1000);
	GYM_Dumping = 0;

	return 1;
}


int Start_Play_GYM(void)
{
	char Name[1024];
#ifndef __PORT__
	OPENFILENAME ofn;
#endif
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
#ifdef __PORT__
#ifdef WITH_GTK
	GtkWidget* filesel;
	gint res;
	gchar* filename=NULL;

	filesel = create_file_selector_private(NULL/*Filter*/,0);
	res = gtk_dialog_run(GTK_DIALOG(filesel));
	switch(res)
	{
		case GTK_RESPONSE_OK:
			filename = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
			break;
		case GTK_RESPONSE_CANCEL:
			break;
	}

	gtk_widget_destroy(filesel);
	
	if (filename)
	{
		GYM_File = fopen(filename, "r");
		if (!GYM_File) return 0;
	}
	else return 0;
#endif
#else
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
	ofn.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		GYM_File = CreateFile(Name, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (GYM_File == INVALID_HANDLE_VALUE) return 0;
	}
	else return 0;
#endif
	
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
#ifdef __PORT__
	if (GYM_File) fclose(GYM_File);
#else
	if (GYM_File) CloseHandle(GYM_File);
#endif
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
#ifdef __PORT__
		l = fread(&c,1,1,GYM_File);
#else
		ReadFile(GYM_File, &c, 1, &l, NULL);
#endif
		if (l == 0) return 0;

		switch(c)
		{
			case 0:
				PSG_Update(buf, Seg_Lenght);
				if (YM2612_Enable) YM2612_Update(buf, Seg_Lenght);
				break;

			case 1:
#ifdef __PORT__
				fread(&c2,1,1,GYM_File);
#else
				ReadFile(GYM_File, &c2, 1, &l, NULL);
#endif
				YM2612_Write(0, c2);
#ifdef __PORT__
				fread(&c2,1,1,GYM_File);
#else
				ReadFile(GYM_File, &c2, 1, &l, NULL);
#endif

				YM2612_Write(1, c2);
				break;

			case 2:
#ifdef __PORT__
				fread(&c2,1,1,GYM_File);
#else
				ReadFile(GYM_File, &c2, 1, &l, NULL);
#endif

				YM2612_Write(2, c2);
#ifdef __PORT__
				fread(&c2,1,1,GYM_File);
#else
				ReadFile(GYM_File, &c2, 1, &l, NULL);
#endif

				YM2612_Write(3, c2);
				break;

			case 3:
#ifdef __PORT__
				fread(&c2,1,1,GYM_File);
#else
				ReadFile(GYM_File, &c2, 1, &l, NULL);
#endif

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
