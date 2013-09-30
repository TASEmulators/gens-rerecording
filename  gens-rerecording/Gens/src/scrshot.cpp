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
#include "drawutil.h"
#include "png.h"
#include <set>
#include <assert.h>
int AVIRecording=0;
int AVISound=1;
int AVISplit=1953; // this should be a safe enough amount below 2048 since VFW tends to corrupt AVIs around 2GB
int AVIWaitMovie=0;
AVIWrite *AVIRecorder=NULL;
int AVIFrame=0;
char ScrShot_Dir[1024] = "\\";
int AVIBreakMovie=0;
bool AVIFileOk;
char AVIFileName[1024];
int AVIHeight224IfNotPAL=1;
int AVICurrentY=240;
int ShotPNGFormat=1;

#define WRITE_FRAME_TO_SRC(pixbits) do{ \
	int topbar = (Y - (Vmode ? 240 : 224)) >> 1; \
	if(Hmode || !Correct_256_Aspect_Ratio) \
	{ \
		int sidebar = (X - (Hmode ? 320 : 256)) >> 1; \
		for(offs = 3*X*topbar, j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * sizeof(pix##pixbits), offs += (3 * (X - sidebar))) \
		{ \
			offs += sidebar * 3; \
			for(i = Hmode ? 319 : 255; i >= 0; i--) \
			{ \
				tmp = READ_PIXEL(i); \
				WRITE_PIXEL(i); \
			} \
		} \
	} \
	else /* 256 across, but output equally wide as 320 across */ \
	{ \
		for(offs = 3*320*topbar, j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * sizeof(pix##pixbits), offs += (3 * 320)) \
		{ \
			int iDst, iSrc = 255, err = 0; \
			unsigned int tmp1 = READ_PIXEL(iSrc); \
			unsigned int tmp2 = READ_PIXEL(iSrc-1); \
			for(iDst = 319; iDst > 0; --iDst) \
			{ \
				int weight = 255 - err * 256 / 320; \
				tmp = DrawUtil::Blend((pix32)tmp1, (pix32)tmp2, weight); \
				WRITE_PIXEL(iDst); \
				err += 256; \
				if(err >= 320) \
				{ \
					err -= 320; \
					iSrc--; \
					tmp1 = tmp2; \
					tmp2 = READ_PIXEL(iSrc-1); \
				} \
			} \
			tmp = tmp2; \
			WRITE_PIXEL(iDst); \
		} \
	} }while(0)

// 24-bit output
#define WRITE_PIXEL(i) Dest[offs + (3 * (i)) + 2] = ((tmp >> 16) & 0xFF); \
                       Dest[offs + (3 * (i)) + 1] = ((tmp >> 8) & 0xFF); \
                       Dest[offs + (3 * (i))    ] = (tmp & 0xFF);

// allocates a png palette, or returns NULL if there are too many colors to put in a palette
// input data is assumed to be 24-bit color
png_colorp MakePalette(void* data, int X, int Y, int* numColorsOut, png_structp png_ptr)
{
	typedef std::set<std::pair<unsigned char,std::pair<unsigned char,unsigned char> > > RGBSet;
	RGBSet found;
	for(unsigned char* pix = (unsigned char*)data+((X*Y)-1)*3; pix >= data; pix -= 3)
		found.insert(std::make_pair(pix[0],std::make_pair(pix[1],pix[2])));
	int numColors = found.size();

	png_colorp palette = NULL;
	if(numColors <= PNG_MAX_PALETTE_LENGTH)
	{
		palette = (png_colorp)png_malloc(png_ptr, numColors * sizeof(png_color));
		if(palette)
		{
			int i = 0;
			for(RGBSet::iterator iter = found.begin(); iter != found.end(); ++iter)
			{
				png_color color = {iter->second.second, iter->second.first, iter->first};
				palette[i++] = color;
			}
		}
	}

	if(numColorsOut)
		*numColorsOut = numColors;

	return palette;
}

// write a png file (PNG8 if possible to do so losslessly, PNG24 otherwise)
// input data is assumed to be 24-bit color
bool write_png(void* data, int X, int Y, FILE* fp)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,  NULL);
		return false;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	png_init_io(png_ptr, fp);

	int numColors = 0;
	png_colorp palette = MakePalette(data, X, Y, &numColors, png_ptr);

	png_set_IHDR(png_ptr, info_ptr, X, Y, 8, palette ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	if(palette)
		png_set_PLTE(png_ptr, info_ptr, palette, numColors);

	png_write_info(png_ptr, info_ptr);

	if(!palette)
	{
		assert(Y <= 240);
		png_bytep row_pointers [240];
		for(int y = 0; y < Y; y++)
			row_pointers[y] = (png_bytep)data + ((Y-y)-1) * X * 3;
		png_set_bgr(png_ptr);
		png_write_image(png_ptr, row_pointers);
	}
	else
	{
		assert(X <= 320);
		png_byte row [320];
		for(int y = 0; y < Y; y++)
		{
			png_bytep src = (png_bytep)data + ((Y-y)-1) * X * 3;
			for(int x = 0; x < X; x++)
			{
				png_byte b = *src++;
				png_byte g = *src++;
				png_byte r = *src++;
				int i;
				for(i = 0; i < numColors; i++)
					if(palette[i].red == r && palette[i].green == g && palette[i].blue == b)
						break;
				assert(i != numColors);
				row[x] = i;
			}
			png_write_row(png_ptr, row);
		}
	}

	png_write_end(png_ptr, info_ptr);
	png_free(png_ptr, palette);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}



int Save_Shot(void* Screen,int mode, int Hmode, int Vmode)
{
	HANDLE Temp_ScrShot_File_Handle;
	FILE* ScrShot_File;
	unsigned char *Src = NULL, *Dest = NULL;
	int j, tmp, offs, num = -1;
	char Name[1024], Message[1024], ext[16];

	SetCurrentDirectory(Gens_Path);
	
	if (!Game) return(0);
	
	do
	{
		if (num++ > 99999)
		{
			free(Dest);
			return(0);
		}

		ext[0] = '_';
		int i = 1;

		int j = num / 10000;
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
		if(ShotPNGFormat)
		{
			ext[i++] = 'p'; // ".png"
			ext[i++] = 'n';
			ext[i++] = 'g';
		}
		else
		{
			ext[i++] = 'b'; // ".bmp"
			ext[i++] = 'm';
			ext[i++] = 'p';
		}
		ext[i] = 0;

		strcpy(Name, ScrShot_Dir);
		strcat(Name, Rom_Name);
		strcat(Name, ext);

		Temp_ScrShot_File_Handle = CreateFile(Name, GENERIC_WRITE, NULL,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	} while(Temp_ScrShot_File_Handle == INVALID_HANDLE_VALUE);

	CloseHandle(Temp_ScrShot_File_Handle);
	ScrShot_File = fopen(Name, "wb");
	if(!ScrShot_File) return 0;

	int X = (Hmode || Correct_256_Aspect_Ratio) ? 320 : 256;
	int Y = Vmode ? 240 : 224;
	int i = (X * Y * 3) + 54;
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	memset(Dest, 0, i);

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
	Dest += 54;

	Src = (unsigned char *)(Screen);
	Src += ((336 * (Vmode ? 239 : 223) + 8) * ((mode&2) ? 4 : 2));

	if(mode & 2) // 32-bit:
	{
		#define READ_PIXEL(i) *(pix32*)&(Src[4 * (i)]);
		WRITE_FRAME_TO_SRC(32);
		#undef READ_PIXEL
	}
	else if(!mode) // 16-bit 565:
	{
		#define READ_PIXEL(i) DrawUtil::Pix16To32((pix16)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
		WRITE_FRAME_TO_SRC(16);
		#undef READ_PIXEL
	}
	else // 16-bit 555:
	{
		#define READ_PIXEL(i) DrawUtil::Pix15To32((pix15)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
		WRITE_FRAME_TO_SRC(15);
		#undef READ_PIXEL
	}

	if(!ShotPNGFormat)
		fwrite(Dest-54, (X * Y * 3) + 54, 1, ScrShot_File); // save BMP
	else
		write_png(Dest, X, Y, ScrShot_File); // save PNG

	fclose(ScrShot_File);

	free(Dest-54);

	wsprintf(Message, "Screen shot %d saved (%s)", num, Name);
	Put_Info(Message, 1500);

	return(1);
}

int Save_Shot_AVI(void* VideoBuf, int mode ,int Hmode, int Vmode,HWND hWnd)
{
	unsigned char *Src = NULL, *Dest = NULL;
	int i, j, tmp, offs, num = -1;

	SetCurrentDirectory(Gens_Path);
	
	if (!Game) return(0);
	
	if(AVIRecorder && AVISplit > 0)
	{
		if((AVIRecorder->GetSize() >> 10) > ((unsigned int)AVISplit << 10))
		{
			int PrevAVIBreakMovie = AVIBreakMovie;
			Close_AVI();
			AVIBreakMovie = PrevAVIBreakMovie + 1;
		}
	}

	int X = 320;
	int Y = AVIRecorder ? AVICurrentY : ((Vmode || !AVIHeight224IfNotPAL) ? 240 : 224);
	AVICurrentY = Y;

    if(AVIRecorder == NULL) 
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
		bi.biWidth = X;
		bi.biHeight = Y;
		bi.biSizeImage = 3*X*Y;
		AVIRecorder->SetVideoFormat(&bi);

		char AVIFileName2 [1024];
		const char* AVIFileNamePtr = AVIFileName;

		if(AVIBreakMovie)
		{
			strcpy(AVIFileName2, AVIFileName);
			char* dot = strrchr(AVIFileName2, '.');
			if(dot)
				*dot = 0;
			else
				dot = AVIFileName2 + strlen(AVIFileName2);
			_snprintf(dot, 1024 - strlen(AVIFileName2), "%03d%s", AVIBreakMovie + 1, dot - AVIFileName2 + AVIFileName);
			AVIFileNamePtr = AVIFileName2;
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
		if(!AVIRecorder->Open(AVIFileNamePtr,hWnd,AVIBreakMovie==0))
		{
			Close_AVI();
			AVIRecording=0;

			char Message[1024];
			sprintf(Message, "Failed to open AVI file \"%s\".\nIt might be read-only or locked by another program.", AVIFileNamePtr);
			DialogsOpen++;
			int answer = MessageBox(hWnd, Message, "Error", MB_ICONERROR);
			DialogsOpen--;

			return 0;
		}
		if (CleanAvi)
			return Update_WAV_Dump_AVI();
    }

	if(VideoBuf == NULL)
		return 1;

	i = (X * Y * 3);
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	memset(Dest, 0, i);

	Src = (unsigned char *)(VideoBuf);
	Src += ((336 * (Vmode ? 239 : 223) + 8) * ((mode&2) ? 4 : 2));

	if(mode & 2) // 32-bit:
	{
		#define READ_PIXEL(i) *(pix32*)&(Src[4 * (i)]);
		WRITE_FRAME_TO_SRC(32);
		#undef READ_PIXEL
	}
	else if(!mode) // 16-bit 565:
	{
		#define READ_PIXEL(i) DrawUtil::Pix16To32((pix16)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
		WRITE_FRAME_TO_SRC(16);
		#undef READ_PIXEL
	}
	else // 16-bit 555:
	{
		#define READ_PIXEL(i) DrawUtil::Pix15To32((pix15)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
		WRITE_FRAME_TO_SRC(15);
		#undef READ_PIXEL
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

int InitAVI(HWND hWnd)
{
	AVIBreakMovie=0;

	return Save_Shot_AVI(NULL, 0,0,0, hWnd);
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