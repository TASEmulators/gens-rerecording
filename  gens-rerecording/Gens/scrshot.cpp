#include <windows.h>
#include "G_Main.h"
#include "G_ddraw.h"
#include "rom.h"
#include "AVIWrite.h"
#include "save.h"
#include "movie.h"
#include "scrshot.h"
#include "mem_M68K.h"
#include <stdio.h>
#include "g_dsound.h"
int AVIRecording=0;
int AVISound=1;
int AVIWaitMovie=0;
AVIWrite *AVIRecorder=NULL;
int AVIFrame=0;
char ScrShot_Dir[1024] = "\\";
int AVIBreakMovie=0;
bool AVIFileOk;
char AVIFileName[1024];


int Save_Shot(unsigned char *Screen, int mode, int X, int Y, int Pitch)
{
	HANDLE ScrShot_File;
	unsigned char *Src = NULL, *Dest = NULL;
	int i, j, tmp, offs, num = -1;
	unsigned long BW;
	char Name[1024], ext[16];

	SetCurrentDirectory(Gens_Path);

	i = (X * Y * 3) + 54;
	
	if (!Game) return(0);
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);

	memset(Dest, 0, i);
	
	do
	{
		if (num++ > 99999)
		{
			free(Dest);
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
		ext[i++] = 'b';
		ext[i++] = 'm';
		ext[i++] = 'p';
		ext[i] = 0;

		strcpy(Name, ScrShot_Dir);
		strcat(Name, Rom_Name);
		strcat(Name, ext);

		ScrShot_File = CreateFile(Name, GENERIC_WRITE, NULL,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	} while(ScrShot_File == INVALID_HANDLE_VALUE);

	Dest[0] = 'B';
	Dest[1] = 'M';

	Dest[2] = (unsigned char) ((i >> 0) & 0xFF);
	Dest[3] = (unsigned char) ((i >> 8) & 0xFF);
	Dest[4] = (unsigned char) ((i >> 16) & 0xFF);
	Dest[5] = (unsigned char) ((i >> 24) & 0xFF);

	Dest[6] = Dest[7] = Dest[8] = Dest[9] = 0;

	Dest[10] = 54;
	Dest[11] = Dest[12] = Dest[13] = 0;

	Dest[14] = 40;
	Dest[15] = Dest[16] = Dest[17] = 0;

	Dest[18] = (unsigned char) ((X >> 0) & 0xFF);
	Dest[19] = (unsigned char) ((X >> 8) & 0xFF);
	Dest[20] = (unsigned char) ((X >> 16) & 0xFF);
	Dest[21] = (unsigned char) ((X >> 24) & 0xFF);

	Dest[22] = (unsigned char) ((Y >> 0) & 0xFF);
	Dest[23] = (unsigned char) ((Y >> 8) & 0xFF);
	Dest[24] = (unsigned char) ((Y >> 16) & 0xFF);
	Dest[25] = (unsigned char) ((Y >> 24) & 0xFF);

	Dest[26] = 1;
	Dest[27] = 0;
	
	Dest[28] = 24;
	Dest[29] = 0;

	Dest[30] = Dest[31] = Dest[32] = Dest[33] = 0;

	i -= 54;
	
	Dest[34] = (unsigned char) ((i >> 0) & 0xFF);
	Dest[35] = (unsigned char) ((i >> 8) & 0xFF);
	Dest[36] = (unsigned char) ((i >> 16) & 0xFF);
	Dest[37] = (unsigned char) ((i >> 24) & 0xFF);

	Dest[38] = Dest[42] = 0xC4;
	Dest[39] = Dest[43] = 0x0E;
	Dest[40] = Dest[44] = Dest[41] = Dest[45] = 0;

	Dest[46] = Dest[47] = Dest[48] = Dest[49] = 0;
	Dest[50] = Dest[51] = Dest[52] = Dest[53] = 0;

	Src = (unsigned char *)(Screen);
	Src += Pitch * (Y - 1);

	if (mode & 2)
	{
		for(offs = 54, j = 0; j < Y; j++, Src -= Pitch, offs += (3 * X))
		{
			for(i = 0; i < X; i++)
			{
				tmp = *(unsigned int *) &(Src[4 * i]);
				Dest[offs + (3 * i) + 2] = ((tmp >> 16) & 0xFF);
				Dest[offs + (3 * i) + 1] = ((tmp >> 8) & 0xFF);
				Dest[offs + (3 * i) + 0] = ((tmp) & 0xFF);
			}
		}
	}
	else
	{
		if (mode)
		{
			for(offs = 54, j = 0; j < Y; j++, Src -= Pitch, offs += (3 * X))
			{
				for(i = 0; i < X; i++)
				{
					tmp = (unsigned int) (Src[2 * i + 0] + (Src[2 * i + 1] << 8));
					Dest[offs + (3 * i) + 2] = ((tmp >> 7) & 0xF8);
					Dest[offs + (3 * i) + 1] = ((tmp >> 2) & 0xF8);
					Dest[offs + (3 * i) + 0] = ((tmp << 3) & 0xF8);
				}
			}
		}	
		else
		{
			for(offs = 54, j = 0; j < Y; j++, Src -= Pitch, offs += (3 * X))
			{
				for(i = 0; i < X; i++)
				{
					tmp = (unsigned int) (Src[2 * i + 0] + (Src[2 * i + 1] << 8));
					Dest[offs + (3 * i) + 2] = ((tmp >> 8) & 0xF8);
					Dest[offs + (3 * i) + 1] = ((tmp >> 3) & 0xFC);
					Dest[offs + (3 * i) + 0] = ((tmp << 3) & 0xF8);
				}
			}
		}
	}
	WriteFile(ScrShot_File, Dest, (X * Y * 3) + 54, &BW, NULL);

	CloseHandle(ScrShot_File);
	if (Dest)
	{
		free(Dest);
		Dest = NULL;
	}

	wsprintf(Name, "Screen shot %d saved", num);
	Put_Info(Name, 1500);

	return(1);
}

int Save_Shot_AVI(void* VideoBuf, int mode ,int Hmode, int Vmode,HWND hWnd)
{
	unsigned char *Src = NULL, *Dest = NULL;
	int i, j, tmp, offs, num = -1;
//	unsigned long FileSize; //Modif N
	int fnlen;

	SetCurrentDirectory(Gens_Path);

	i = (320 * 240 * 3);
	
	if (!Game) return(0);
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	
	if(AVIRecorder)
	{

		if(AVIRecorder->GetSize() > 0x7B9ACA00)
		{
			if(AVIBreakMovie)
			{
				Close_AVI();
				AVIBreakMovie=2;
			}
			else
			{
				Close_AVI();
				AVIBreakMovie=1;
			}
		}
	}

    //if(AVIRecorder == NULL) {
	else 
	{
		AVIFrame = 0;
		AVIRecorder = new AVIWrite();

		if (CPU_Mode)
			AVIRecorder->SetFPS(50);
		else
			AVIRecorder->SetFPS(60);
		BITMAPINFOHEADER bi;
		memset(&bi, 0, sizeof(bi));      
		bi.biSize = 0x28;    
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biWidth = 320;
		bi.biHeight = 240;
		bi.biSizeImage = 3*320*240;
		AVIRecorder->SetVideoFormat(&bi);
		if(AVIBreakMovie)
		{
			fnlen=strlen(AVIFileName);
			if(AVIBreakMovie==2)
			{
				AVIFileName[fnlen-5]=AVIFileName[fnlen-5]++;
				if(AVIFileName[fnlen-5]==90)
					AVIBreakMovie=1;
			}
			else
			{
				AVIFileName[fnlen+1]=AVIFileName[fnlen];
				AVIFileName[fnlen]=AVIFileName[fnlen-1];
				AVIFileName[fnlen-1]=AVIFileName[fnlen-2];
				AVIFileName[fnlen-2]=AVIFileName[fnlen-3];
				AVIFileName[fnlen-3]=AVIFileName[fnlen-4];
				AVIFileName[fnlen-4]=65;
			}
		}
		else
		{
			strcpy(AVIFileName,Rom_Name);
			strcat(AVIFileName,".avi");
			if (Change_File_S(AVIFileName, Movie_Dir, "Save AVI", "AVI\0*.avi*\0All Files\0*.*\0\0", "avi", hWnd)==0)
			{
				Close_AVI();
				AVIRecording=0;
				return 0;
			}
		}
		if(AVIRecorder->Open(AVIFileName,hWnd,AVIBreakMovie?false:true)==false)
		{
			Close_AVI();
			AVIRecording=0;
			return 0;
		}
		if (CleanAvi)
			return Update_WAV_Dump_AVI();
    }
  
	memset(Dest, 0, i);

	Src = (unsigned char *)(VideoBuf);

	if (mode & 2)
	{
		if(Vmode)
			Src += (336 * 239 * 4);
		else
			Src += (336 * 223 * 4);
//		Dest -= 3;
		for(offs = Vmode ? 0 : (3*320*8), j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * 4, offs += (3 * (Hmode ? 320 : 288)))
		{
			if (Hmode==0) offs+=96;
			for(i = Hmode ? 319 : 256; i >= 0; i--)
			{
				tmp = (unsigned int) (Src[4 * i + 32] + (Src[4 * i + 33] << 8) + (Src[4 * i + 34] << 16));
				Dest[offs + (3 * i) + 2] = ((tmp >> 16) & 0xFF);
				Dest[offs + (3 * i) + 1] = ((tmp >> 8) & 0xFF);
				Dest[offs + (3 * i)    ] = (tmp & 0xFF);
			}
		}

	}
	else
	{
		if(Vmode)
			Src += (336 * 239 * 2);
		else
			Src += (336 * 223 * 2);
		if (mode)
		{
			for(offs = Vmode ? 0 : (3*320*8), j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * 2, offs += (3 * (Hmode ? 320 : 288)))
			{
				if (Hmode==0) offs+=96;
				for(i = Hmode ? 320 : 256; i > 0; i--)
				{
					tmp = (unsigned int) (Src[2 * i + 16] + (Src[2 * i + 17] << 8));
					Dest[offs + (3 * i) + 2] = ((tmp >> 7) & 0xF8);
					Dest[offs + (3 * i) + 1] = ((tmp >> 2) & 0xF8);
					Dest[offs + (3 * i)    ] = ((tmp << 3) & 0xF8);
				}
			}
		}
		else
		{
			for(offs = Vmode ? 0 : (3*320*8), j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * 2, offs += (3 * (Hmode ? 320 : 288)))
			{
				if (Hmode==0) offs+=96;
				for(i = Hmode ? 320 : 256; i >0 ;i--)
				{
					tmp = (unsigned int) (Src[2 * i + 16] + (Src[2 * i + 17] << 8));
					Dest[offs + (3 * i) + 2] = ((tmp >> 8) & 0xF8);
					Dest[offs + (3 * i) + 1] = ((tmp >> 3) & 0xFC);
					Dest[offs + (3 * i)    ] = ((tmp << 3) & 0xF8);
				}
			}
		}
	}

	AVIRecorder->AddFrame(AVIFrame, (char*)Dest);
	AVIFrame++;

	if (Dest)
	{
		free(Dest);
		Dest = NULL;
	}

	return(1);
	
}

int Close_AVI()
{
	if(AVIRecorder!=NULL)
	{
		delete AVIRecorder;
		AVIRecorder = NULL;
	}
	AVIBreakMovie=0;
	return 1;
}

int InitAVI()
{
	AVIBreakMovie=0;
	return 1;
}


int UpdateSoundAVI(unsigned char * buf,unsigned int length, int Rate, int Stereo)
{
	if (AVIRecorder == NULL)
		return 0;
	if(!AVIRecorder->IsSoundAdded())
	{
		WAVEFORMATEX format;
		format.cbSize=0;
		format.nChannels= Stereo ? 2 : 1;
		format.nSamplesPerSec=Rate;
		format.wBitsPerSample=16;
		format.wFormatTag=WAVE_FORMAT_PCM;
		format.nBlockAlign=format.nChannels*format.wBitsPerSample/8;
		format.nAvgBytesPerSec=format.nBlockAlign*format.nSamplesPerSec;
		AVIRecorder->SetSoundFormat(&format);
	}
	AVIRecorder->AddSound((char *)buf,length);
	return 1;
}