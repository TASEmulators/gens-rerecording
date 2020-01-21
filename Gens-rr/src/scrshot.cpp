#include <windows.h>
#include "gens.h"
#include "G_Main.h"
#include "G_ddraw.h"
#include "guidraw.h"
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
int AVICurrentX=320;
int AVICurrentY=240;
int ShotPNGFormat=1;

#define WRITE_FRAME_TO_SRC(pixbits, bytes) do{ \
	int topbar = (Y - (Vmode ? 240 : 224)) >> 1; \
	if(Hmode || !Correct_256_Aspect_Ratio) \
	{ \
		int sidebar = (X - (Hmode ? 320 : 256)) >> 1; \
		for(offs = bytes*X*topbar, j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * sizeof(pix##pixbits), offs += (bytes * (X - sidebar))) \
		{ \
			offs += sidebar * bytes; \
			for(i = Hmode ? 319 : 255; i >= 0; i--) \
			{ \
				tmp = READ_PIXEL(i); \
				WRITE_PIXEL(i); \
			} \
		} \
	} \
	else /* 256 across, but output equally wide as 320 across */ \
	{ \
		for(offs = bytes*320*topbar, j = Vmode ? 240 : 224; j > 0; j--, Src -= 336 * sizeof(pix##pixbits), offs += (bytes * 320)) \
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

// allocates a png palette, or returns NULL if there are too many colors to put in a palette
// input data is assumed to be 32-bit color BGRA
png_colorp MakePalette(void* data, int X, int Y, int* numColorsOut, png_structp png_ptr, png_bytep *alpha)
{
	typedef std::set<unsigned int> RGBASet;
	RGBASet found;
	bool was_alpha = false;
	for(unsigned int* pix = (unsigned int*)data+(X*Y)-1; pix >= data; --pix)
	{
		if ((*pix)&0xFF000000)
		{
			found.insert(*pix);
			if (((*pix)&0xFF000000) != 0xFF000000)
				was_alpha = true;
		}
		else
		{
			found.insert(0); // full transparent
			was_alpha = true;
		}
	}
	int numColors = found.size();

	png_colorp palette = NULL;
	*alpha = NULL;
	if(numColors <= PNG_MAX_PALETTE_LENGTH)
	{
		palette = (png_colorp)png_malloc(png_ptr, numColors * sizeof(png_color));
		if(palette)
		{
			if (was_alpha)
				*alpha = (png_bytep)png_malloc(png_ptr, numColors);

			if (*alpha)
			{
				int i = 0;
				for(RGBASet::iterator iter = found.begin(); iter != found.end(); ++iter)
				{
					png_color color = {
						((*iter) >> 16) & 0xFF,
						((*iter) >>  8) & 0xFF,
						((*iter) >>  0) & 0xFF
						};

					(*alpha)[i] = ((*iter) >> 24) & 0xFF;

					palette[i++] = color;
				}
			}
			else
			{
				int i = 0;
				for(RGBASet::iterator iter = found.begin(); iter != found.end(); ++iter)
				{
					png_color color = {
						((*iter) >> 16) & 0xFF,
						((*iter) >>  8) & 0xFF,
						((*iter) >>  0) & 0xFF
						};
					palette[i++] = color;
				}
			}
		}
	}
	else if (was_alpha)
		*alpha = (png_bytep)alpha;

	if(numColorsOut)
		*numColorsOut = numColors;

	return palette;
}

// write a png file (PNG8 if possible to do so losslessly, PNG24 otherwise)
// input data is assumed to be 32-bit color BGRA
bool write_png(void* data, int X, int Y, FILE* fp, png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn)
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

	if (write_data_fn)
		png_set_write_fn(png_ptr, fp, write_data_fn, output_flush_fn);
	else
		png_init_io(png_ptr, fp);

	int numColors = 0;
	png_bytep alpha = NULL;
	png_colorp palette = MakePalette(data, X, Y, &numColors, png_ptr, &alpha);

	int type = PNG_COLOR_TYPE_RGB_ALPHA;
	if (palette)
		type = PNG_COLOR_TYPE_PALETTE;
	else if (!alpha) // if RGB and without alpha then plain RGB
		type = PNG_COLOR_TYPE_RGB;
	else // if RGBA, then remove fake alpha pointer
		alpha = NULL;

	png_set_IHDR(png_ptr, info_ptr, X, Y, 8, type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	if(palette)
		png_set_PLTE(png_ptr, info_ptr, palette, numColors);

	if (alpha)
	{
		// works only before png_write_info
		png_set_tRNS(png_ptr, info_ptr, (png_bytep)alpha, numColors, NULL);
	}

	png_write_info(png_ptr, info_ptr);

	if (type == PNG_COLOR_TYPE_RGB)
	{
		assert(X <= 320);
		png_byte row [320][3];
		png_set_bgr(png_ptr);
		for(int y = 0; y < Y; y++)
		{
			png_bytep src = (png_bytep)data + ((Y-y)-1) * X * 4;
			for(int x = 0; x < X; x++)
				for (int i=0; i<3; ++i)
					row[x][i] = src[x*4 + i];
			png_write_row(png_ptr, row[0]);
		}
	}
	else if (type == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		assert(Y <= 240);
		png_bytep row_pointers [240];
		for(int y = 0; y < Y; y++)
			row_pointers[y] = (png_bytep)data + ((Y-y)-1) * X * 4;
		//png_set_swap_alpha(png_ptr);
		png_set_bgr(png_ptr);
		png_write_image(png_ptr, row_pointers);
	}
	else
	{
		assert(X <= 320);
		png_byte row [320];
		for(int y = 0; y < Y; y++)
		{
			png_bytep src = (png_bytep)data + ((Y-y)-1) * X * 4;
			for(int x = 0; x < X; x++)
			{
				png_byte b = *src++;
				png_byte g = *src++;
				png_byte r = *src++;
				png_byte a = *src++;
				int i;
				if (!a) // full transparent
				{
					for(i = 0; i < numColors; ++i)
						if(!alpha[i])
							break;
				}
				else if (alpha) // alpha channel
				{
					for(i = 0; i < numColors; ++i)
						if(alpha[i] == a && palette[i].red == r && palette[i].green == g && palette[i].blue == b)
							break;
				}
				else // full opacity
					for(i = 0; i < numColors; ++i)
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
	png_free(png_ptr, alpha);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}

bool write_png(void* data, int X, int Y, FILE* fp)
{
	return write_png(data, X, Y, fp, NULL, NULL);
}

struct memory_buffer
{
	unsigned char *buffer;
	size_t size, pos;
	bool fail;
};

void png_write_into_memory(png_structp png_ptr, png_bytep data, png_size_t size)
{
	memory_buffer* b=(struct memory_buffer*)png_get_io_ptr(png_ptr);
	if (b->pos + size > b->size)
	{
		png_error(png_ptr, "Buffer overflow");
		b->fail = true;
		return;
	}
	memcpy(b->buffer + b->pos, data, size);
	b->pos += size;
}

void png_flush_into_memory(png_structp png_ptr)
{
}

// Write png into buffer, and return its size
int write_png(void* data, int X, int Y, void* buffer, int size)
{
	memory_buffer mem;
	mem.pos = 0;
	mem.size = size;
	mem.buffer = (unsigned char*)buffer;
	mem.fail = false;

	if (!write_png(data, X, Y, (FILE*)&mem, png_write_into_memory, png_flush_into_memory))
		return 0;

	if (!mem.fail)
		return mem.pos;
	return 0;
}


int CopyToClipboard(int Type, unsigned char *Buffer, size_t buflen, bool clear = true) // feos added this
{
	HGLOBAL hResult;
	if (!OpenClipboard(NULL)) return 0;
	if (clear)
	{
		if (!EmptyClipboard()) return 0;
	}

	hResult = GlobalAlloc(GMEM_MOVEABLE, buflen);
	if (hResult == NULL) return 0;

	memcpy(GlobalLock(hResult), Buffer, buflen);
	GlobalUnlock(hResult);

	if (SetClipboardData(Type, hResult) == NULL)
	{
		CloseClipboard();
		GlobalFree(hResult);
		return 0;
	}

	CloseClipboard();
	GlobalFree(hResult);
	return 1;
}

void MakeBitmapHeader(unsigned char *Dest, int BmpSize)
{
	Dest[ 0] = 'B';
	Dest[ 1] = 'M';
	Dest[ 2] = (unsigned char) ((BmpSize >>  0) & 0xFF); 
	Dest[ 3] = (unsigned char) ((BmpSize >>  8) & 0xFF);
	Dest[ 4] = (unsigned char) ((BmpSize >> 16) & 0xFF);
	Dest[ 5] = (unsigned char) ((BmpSize >> 24) & 0xFF);

	// Reserved
	Dest[ 6] = Dest[7] = Dest[8] = Dest[9] = 0;	

	// OffBits
	Dest[10] = 54;
	Dest[11] = Dest[12] = Dest[13] = 0;
}

void MakeBitmapInfo(unsigned char *Dest, int Width, int Height, int bpp = 24)
{
	Dest[ 0] = 40;
	Dest[ 1] = Dest[2] = Dest[3] = 0;
	Dest[ 4] = (unsigned char) ((Width  >>  0) & 0xFF);
	Dest[ 5] = (unsigned char) ((Width  >>  8) & 0xFF);
	Dest[ 6] = (unsigned char) ((Width  >> 16) & 0xFF);
	Dest[ 7] = (unsigned char) ((Width  >> 24) & 0xFF);
	Dest[ 8] = (unsigned char) ((Height >>  0) & 0xFF);
	Dest[ 9] = (unsigned char) ((Height >>  8) & 0xFF);
	Dest[10] = (unsigned char) ((Height >> 16) & 0xFF);
	Dest[11] = (unsigned char) ((Height >> 24) & 0xFF);
	Dest[12] = 1;
	Dest[13] = 0;
	Dest[14] = bpp;
	Dest[15] = 0;
	Dest[16] = Dest[17] = Dest[18] = Dest[19] = 0;
	int i = Width * Height * (bpp / 8);
	Dest[20] = (unsigned char) ((i >>  0) & 0xFF);
	Dest[21] = (unsigned char) ((i >>  8) & 0xFF);
	Dest[22] = (unsigned char) ((i >> 16) & 0xFF);
	Dest[23] = (unsigned char) ((i >> 24) & 0xFF);
	Dest[24] = Dest[28] = 0xC4;
	Dest[25] = Dest[29] = 0x0E;
	Dest[26] = Dest[30] = Dest[27] = Dest[31] = 0;
	Dest[32] = Dest[33] = Dest[34] = Dest[35] = 0;
	Dest[36] = Dest[37] = Dest[38] = Dest[39] = 0;
}

// DIBV5 BitmapInfo, working, but...
// who knows how to export with alpha channel correctly.
// DIBV5 with transparency does not working almost everywhere.
void MakeBitmapInfoV5(unsigned char *Dest, int Width, int Height)
{
	MakeBitmapInfo(Dest, Width, Height);

	// zero memset v5 addition
	memset(Dest + 40, 0, (5 + 7)*4 + sizeof(CIEXYZTRIPLE));

	// rewrite Size
	Dest[ 0] = sizeof(BITMAPV5HEADER);

	// rewrite Size of image
	int i = Width * Height * 4;
	Dest[20] = (unsigned char) ((i >>  0) & 0xFF);
	Dest[21] = (unsigned char) ((i >>  8) & 0xFF);
	Dest[22] = (unsigned char) ((i >> 16) & 0xFF);
	Dest[23] = (unsigned char) ((i >> 24) & 0xFF);

	// rewrite BitCount
	Dest[14] = 32;

	// rewrite Compression
	Dest[16] = BI_BITFIELDS;

	// AlphaMask
	Dest[55] = 0xFF;
	
	// CS_Type
	Dest[56] = 'B';
	Dest[57] = 'G';
	Dest[58] = 'R';
	Dest[59] = 's';
}

void WriteFrame(void* Screen, unsigned char *Dest, int mode, int Hmode, int Vmode, int X, int Y)
{
	int i, j, tmp, offs;
	unsigned char *Src = (unsigned char *)(Screen);

	Src += ((336 * (Vmode ? 239 : 223) + 8) * ((mode&2) ? 4 : 2));

	if(mode & 4)
	{
		// 32-bit BGRA output
#define WRITE_PIXEL(i) Dest[offs + (4 * (i)) + 3] = ((tmp >> 24) & 0xFF); \
                       Dest[offs + (4 * (i)) + 2] = ((tmp >> 16) & 0xFF); \
                       Dest[offs + (4 * (i)) + 1] = ((tmp >>  8) & 0xFF); \
                       Dest[offs + (4 * (i))    ] = ( tmp        & 0xFF);
		if(mode & 2) // 32-bit:
		{
			#define READ_PIXEL(i) (*(pix32*)&(Src[4 * (i)]))
			WRITE_FRAME_TO_SRC(32,4);
			#undef READ_PIXEL
		}
		else if(!(mode & 1)) // 16-bit 565:
		{
			#define READ_PIXEL(i) (DrawUtil::Pix16To32((pix16)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8))) | 0xFF000000)
			WRITE_FRAME_TO_SRC(16,4);
			#undef READ_PIXEL
		}
		else // 16-bit 555:
		{
			#define READ_PIXEL(i) (DrawUtil::Pix15To32((pix15)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8))) | 0xFF000000)
			WRITE_FRAME_TO_SRC(15,4);
			#undef READ_PIXEL
		}
		#undef WRITE_PIXEL
	}
	else
	{
		// 24-bit BGR output
#define WRITE_PIXEL(i) Dest[offs + (3 * (i)) + 2] = ((tmp >> 16) & 0xFF); \
                       Dest[offs + (3 * (i)) + 1] = ((tmp >>  8) & 0xFF); \
                       Dest[offs + (3 * (i))    ] = ( tmp        & 0xFF);
		if(mode & 2) // 32-bit:
		{
			#define READ_PIXEL(i) *(pix32*)&(Src[4 * (i)]);
			WRITE_FRAME_TO_SRC(32,3);
			#undef READ_PIXEL
		}
		else if(!(mode & 1)) // 16-bit 565:
		{
			#define READ_PIXEL(i) DrawUtil::Pix16To32((pix16)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
			WRITE_FRAME_TO_SRC(16,3);
			#undef READ_PIXEL
		}
		else // 16-bit 555:
		{
			#define READ_PIXEL(i) DrawUtil::Pix15To32((pix15)(Src[2 * (i)] + (Src[2 * (i) + 1] << 8)))
			WRITE_FRAME_TO_SRC(15,3);
			#undef READ_PIXEL
		}
		#undef WRITE_PIXEL
	}
}

int Save_Shot_Clipboard(void* Screen, int mode, int Hmode, int Vmode) // feos added this
{
	unsigned char *Src = NULL, *Dest = NULL;

	if (!Game) return(0);
	if (CleanAvi) Do_VDP_Refresh(); // clean screenshots

	int X = (Hmode || Correct_256_Aspect_Ratio) ? 320 : 256;
	int Y = Vmode ? 240 : 224;
	int i = (X * Y * 3) + 40;
	if (PinkBG) // transparency on
		i = (X * Y * 4) + 40;
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	memset(Dest, 0, i);

	MakeBitmapInfo(Dest, X, Y, PinkBG ? 32 : 24);

	if (PinkBG) // 32 bit, some editors can read ABGR bitmap
		WriteFrame(Screen, Dest + 40, mode | 4, Hmode, Vmode, X, Y);
	else
		WriteFrame(Screen, Dest + 40, mode, Hmode, Vmode, X, Y);

	CopyToClipboard(CF_DIB, Dest, i);

	if (PinkBG)
	{
		// Export PNG
		unsigned char *png = (unsigned char*)malloc(i);
		if (png)
		{
			int png_size = write_png(Dest + 40, X, Y, png, i);
			if (png_size)
				CopyToClipboard(RegisterClipboardFormat("PNG"), png, png_size, false);

			free(png);
		}
	}

	free(Dest);
	Put_Info("Screen shot saved to clipboard");

	return(1);
}

int Save_Shot(void* Screen,int mode, int Hmode, int Vmode)
{
	HANDLE Temp_ScrShot_File_Handle;
	FILE* ScrShot_File;
	unsigned char *Src = NULL, *Dest = NULL;
	char Name[1024], Message[1024], ext[16];

	SetCurrentDirectory(Gens_Path);

	if (!Game) return(0);
	if (CleanAvi) Do_VDP_Refresh(); // clean screenshots

	int num = -1;
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

		Temp_ScrShot_File_Handle = CreateFile(
			Name, GENERIC_WRITE, NULL, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL
		);
	} while(Temp_ScrShot_File_Handle == INVALID_HANDLE_VALUE);

	CloseHandle(Temp_ScrShot_File_Handle);
	ScrShot_File = fopen(Name, "wb");
	if(!ScrShot_File) return 0;

	int X = (Hmode || Correct_256_Aspect_Ratio) ? 320 : 256;
	int Y = Vmode ? 240 : 224;
	int i = (X * Y * 3) + 54;
	if (ShotPNGFormat)
		i = (X * Y * 4) + 54;
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	memset(Dest, 0, i);

	MakeBitmapHeader(Dest, i);
	MakeBitmapInfo(Dest + 14, X, Y);

	WriteFrame(Screen, Dest + 54, mode | (ShotPNGFormat?4:0), Hmode, Vmode, X, Y);

	if(!ShotPNGFormat)
		fwrite(Dest, (X * Y * 3) + 54, 1, ScrShot_File); // save BMP
	else
		write_png(Dest + 54, X, Y, ScrShot_File); // save PNG

	fclose(ScrShot_File);
	free(Dest);

	wsprintf(Message, "Screen shot %d saved (%s)", num, Name);
	Put_Info(Message);

	return(1);
}

int Save_Shot_AVI(void* VideoBuf, int mode, int Hmode, int Vmode, HWND hWnd)
{
	unsigned char *Src = NULL, *Dest = NULL;
	int i;

	SetCurrentDirectory(Gens_Path);
	
	if (!Game) return(0);

	int X = (Hmode || Correct_256_Aspect_Ratio) ? 320 : 256;
	int Y = AVIRecorder ? AVICurrentY : ((Vmode || !AVIHeight224IfNotPAL) ? 240 : 224);
	
	if(AVIRecorder)
	{
		if((AVIRecorder->GetSize() >> 10) > ((unsigned int)AVISplit << 10) && AVISplit > 0
			|| AVICurrentX != X || AVICurrentY != Y)
		{
			int PrevAVIBreakMovie = AVIBreakMovie;
			Close_AVI();
			AVIBreakMovie = PrevAVIBreakMovie + 1;
		}
	}

	AVICurrentY = Y;
	AVICurrentX = X;

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
		if (CleanAvi && AVIBreakMovie==0)
			return Update_WAV_Dump_AVI();
    }

	if(VideoBuf == NULL)
		return 1;

	i = (X * Y * 3);
	if ((Dest = (unsigned char *) malloc(i)) == NULL) return(0);
	memset(Dest, 0, i);

	WriteFrame(VideoBuf, Dest, mode, Hmode, Vmode, X, Y);

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