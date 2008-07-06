#include "blit.h"
#include "vdp_rend.h"
#include "drawutil.h"

template<typename pixel>
static void TBlit_EPX(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel L = *(SrcLine-1);
			pixel C = *(SrcLine);
			pixel R = *(SrcLine+1);
			if(L != R)
			{
				pixel U = *(SrcLine-srcWidth);
				pixel D = *(SrcLine+srcWidth);
				if(U != D)
				{
					*DstLine1++ = (U == L) ? U : C;
					*DstLine1++ = (R == U) ? R : C;
					*DstLine2++ = (L == D) ? L : C;
					*DstLine2++ = (D == R) ? D : C;
					SrcLine++;
					continue;
				}
			}
			*DstLine1++ = C; 
			*DstLine1++ = C; 
			*DstLine2++ = C; 
			*DstLine2++ = C; 
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			*DstLine1++ = C;
			*DstLine1++ = C;
			*DstLine2++ = 0;
			*DstLine2++ = 0;
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline_50(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			*DstLine1++ = C;
			*DstLine1++ = C;
			*DstLine2++ = DrawUtil::Blend(C, 0);
			*DstLine2++ = DrawUtil::Blend(C, 0);
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline_25(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			*DstLine1++ = C;
			*DstLine1++ = C;
			*DstLine2++ = DrawUtil::Blend(C,0, 3,1, 2);
			*DstLine2++ = DrawUtil::Blend(C,0, 3,1, 2);
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_X2_Int(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			pixel R = *(SrcLine+1);
			pixel D = *(SrcLine+srcWidth);
			pixel DR = *(SrcLine+srcWidth+1);
			*DstLine1++ = C;
			*DstLine1++ = DrawUtil::Blend(C, R);
			*DstLine2++ = DrawUtil::Blend(C, D);
			*DstLine2++ = DrawUtil::Blend(C, R, D, DR);
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline_Int(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			pixel R = *(SrcLine+1);
			*DstLine1++ = C;
			*DstLine1++ = DrawUtil::Blend(C, R);
			*DstLine2++ = 0;
			*DstLine2++ = 0;
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline_50_Int(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			pixel R = *(SrcLine+1);
			pixel D = *(SrcLine+srcWidth);
			pixel DR = *(SrcLine+srcWidth+1);
			*DstLine1++ = C;
			*DstLine1++ = DrawUtil::Blend(C, R);
			*DstLine2++ = DrawUtil::Blend(C,D, 1,1, 2);
			*DstLine2++ = DrawUtil::Blend(DrawUtil::Blend(C, R, D, DR), 0);
			SrcLine++;
		}
	}
}

template<typename pixel>
static void TBlit_Scanline_25_Int(pixel* Src, pixel* Dest, int srcWidth, int dstWidth, int x, int y)
{
	for(int j = 0; j < y; j++)
	{
		pixel* SrcLine = Src + srcWidth*j;
		pixel* DstLine1 = Dest + dstWidth*(j*2);
		pixel* DstLine2 = Dest + dstWidth*(j*2+1);
		for(int i = 0; i < x; i++)
		{
			pixel C = *(SrcLine);
			pixel R = *(SrcLine+1);
			pixel D = *(SrcLine+srcWidth);
			pixel DR = *(SrcLine+srcWidth+1);
			*DstLine1++ = C;
			*DstLine1++ = DrawUtil::Blend(C, R);
			*DstLine2++ = DrawUtil::Blend(DrawUtil::Blend(C, D),0, 3,1, 2);
			*DstLine2++ = DrawUtil::Blend(DrawUtil::Blend(C, R, D, DR),0, 3,1, 2);
			SrcLine++;
		}
	}
}


#define MAKE_CBLIT_FUNC(name) \
	void CBlit_##name(unsigned char *Dest, int pitch, int x, int y, int offset) \
	{ \
		if(Bits32) \
			TBlit_##name(MD_Screen32 + 8, (unsigned int*)Dest, 336, pitch>>2, x, y); \
		else \
			TBlit_##name(MD_Screen + 8, (unsigned short*)Dest, 336, pitch>>1, x, y); \
	}

MAKE_CBLIT_FUNC(EPX)
MAKE_CBLIT_FUNC(X2_Int)
MAKE_CBLIT_FUNC(Scanline)
MAKE_CBLIT_FUNC(Scanline_Int)
MAKE_CBLIT_FUNC(Scanline_50)
MAKE_CBLIT_FUNC(Scanline_50_Int)
MAKE_CBLIT_FUNC(Scanline_25)
MAKE_CBLIT_FUNC(Scanline_25_Int)
