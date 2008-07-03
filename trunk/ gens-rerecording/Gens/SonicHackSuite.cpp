#include <stdio.h>
#include <windows.h>
#include "guidraw.h"
#include "SonicHackSuite.h"
#include "HacksCommon.h"
#include "misc.h"
#include "g_main.h"
#include "g_ddraw.h"
#include "save.h"
#include "ym2612.h"

#ifdef SONICCAMHACK
unsigned char genesisbuf[GENESIS_STATE_LENGTH + GENESIS_LENGTH_EX];
#ifdef SCD
#include "mem_s68k.h"
unsigned char scdbuf[SEGACD_LENGTH_EX];
#endif
void FindObjectDims (unsigned int index, unsigned char &X, unsigned char &Y)
{
	unsigned char Num = CheatRead<unsigned char>(index);
	switch (Num)
	{
//#ifdef S1
		case 0x0C:
		case 0x44:
			X = 0x8;
			Y = 0x20;
			return;
		case 0x10:
			if (CheatRead<unsigned char>(index + 0x28) & 4)
				X = min(0xFE,CheatRead<unsigned short>(index + 0x32)),Y = 0x10;
			else
				Y = min(0xFE,CheatRead<unsigned short>(index + 0x32)),X = 0x10;
			return;
		case 0x11:
			X = CheatRead<unsigned char>(index + 0x28);
			X <<= 3;
			X += 8;
			Y = 8;
			return;
		case 0x12:
		case 0x46:
		case 0x47:
		case 0x51:
		case 0x76:
		case 0x7B:
			X = Y = 0x10;
			return;
		case 0x15:
		case 0x56:
		case 0x61:
		case 0x6B:
		case 0x70:
		case 0x71:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = CheatRead<unsigned char>(index + 0x16);
			return;
		case 0x18:
		case 0x52:
		case 0x59:
		case 0x5A:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = 9;
			return;
		case 0x1A:
		case 0x5E:
			X = 0x30;
			Y = CheatRead<unsigned char>(index + 0x16);
			return;
		case 0x2A:
			X = 6;
			Y = 0x20;
		case 0x2F:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = (CheatRead<unsigned char>(index + 0x1A) == 2)?0x30:0x20;
			return;
		case 0x30:
			X = 0x20;
			Y = (CheatRead<unsigned char>(index + 0x24) < 3)?0x48:0x38;
			return;
		case 0x31:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = 0xC;
			return;
		case 0x32:
			X = 0x10;
			Y = 5;
			return;
		case 0x33:
		case 0x5B:
		case 0x83:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = 0x10;
			return;
		case 0x36:
			X = ((CheatRead<unsigned char>(index + 0x1A) == 5) || (CheatRead<unsigned char>(index + 0x1A) == 1))?0x10:CheatRead<unsigned char>(index + 0x19);
			Y = (CheatRead<unsigned char>(index + 0x1A) == 5)?4:((CheatRead<unsigned char>(index + 0x1A) == 1)?0x14:0x10);
			return;
		case 0x3B:
			X = Y = 0x10;
			return;
		case 0x3C:
			X = 0x10;
			Y = 0x20;
			return;
		case 0x3E:
			X = CheatRead<unsigned char>(index + 0x17);
			Y = 0x18;
			if (CheatRead<unsigned char>(index + 0x24) == 0x04)
				Y = 0x08;
			return;
		case 0x41:
			X = ((CheatRead<unsigned char>(index + 0x24) == 8) || (CheatRead<unsigned char>(index + 0x24) == 0xA) || (CheatRead<unsigned char>(index + 0x24) == 0xC))?8:0x10;
			Y = ((CheatRead<unsigned char>(index + 0x24) == 8) || (CheatRead<unsigned char>(index + 0x24) == 0xA) || (CheatRead<unsigned char>(index + 0x24) == 0xC))?0xE:0x8;
			return;
//		case 0x44:
//			X = 8;
//			Y = 0x20;
//			return;
		case 0x45:
			X = 0xC;
			Y = 0x20;
			return;
		case 0x4E:
			X = 0x20;
			Y = 0x18;
			return;
		case 0x53:
			X = 0x20;
			Y = 9;
			return;
		case 0x64:
			X = Y = (CheatRead<unsigned char>(index + 0x2E)) ? 0x10 : 8;
			return;
		case 0x66:
			X = 0x25;
			Y = 0x30;
			return;
		case 0x69:
			X = (CheatRead<unsigned char>(index + 0x24) == 2)?0x40:0x10;
			Y = (CheatRead<unsigned char>(index + 0x24) == 2)?0xC:0x7;
			return;
		case 0x6F:
			X=0x10;
			Y=0x7;
			return;
		case 0x79:
			X=0x8;
			Y=(CheatRead<unsigned char>(index + 0x24) == 2)?0x20:0x8;
			return;
		case 0x84:
			X = 0x20;
			Y = 0x60;
			return;
		case 0x85:
			X = (CheatRead<unsigned char>(index + 0x24) > 0xA)?0x10:0x20;
			Y = (CheatRead<unsigned char>(index + 0x24) > 0xA)?0x70:0x14;
			return;
		case 0x86:
			X = Y = 8;
			return;
//#endif
		default:
			X = Y = 3;
	}
}
#define ENEMY 0
#define ROUT 1
#define HURT 2
#define SPEC 3
int ColorTable32[4] = {0x8000FF,0x00FFFF,0xFF0000,0x00FF00};
unsigned short ColorTable16[4] = {0x4010,0x07FF,0xF800,0x07E0};
	// Sonic camera hack
	#ifdef SK
		const unsigned int P1OFFSET = 0xFFB000;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned char XPo = 0x10;
		const unsigned char YPo = 0x14;
		const unsigned char XVo = 0x18;
		const unsigned char YVo = 0x1A;
		const unsigned char To = 0x28;
		const unsigned char Wo = 0x1F;
		const unsigned char Ho = 0x1E;
		const unsigned char Fo = 0x4;
		const unsigned int CAMOFFSET1 = 0xFFEE78;
		const unsigned int CAMOFFSET2 = 0xFFEE80;
		const unsigned int INLEVELFLAG = 0xFFB004;
		const unsigned int POSOFFSET = 0xB000;
		const unsigned int SSTLEN = 0x1FCC;
		const unsigned int SPRITESIZE = 0x4A;
		unsigned int LEVELHEIGHT = CheatRead<unsigned short>(0xFFEEAA);
		const unsigned char XSCROLLRATE = 24;
		const unsigned char YSCROLLRATE = 32;
	#elif defined S2
		const unsigned int P1OFFSET = 0xFFB000;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned char XPo = 0x8;
		const unsigned char YPo = 0xC;
		const unsigned char XVo = 0x10;
		const unsigned char YVo = 0x12;
		const unsigned char To = 0x20;
		const unsigned char Wo = 0x17;
		const unsigned char Ho = 0x16;
		const unsigned char Fo = 1;
		const unsigned int POSOFFSET = 0xB000;
		const unsigned int SSTLEN = 0x2600;
		const unsigned int SPRITESIZE = 0x40;
		const unsigned int CAMOFFSET1 = 0xFFEE00;
		const unsigned int INLEVELFLAG = 0xFFB001;
		const unsigned int LEVELHEIGHT = 2047;
		const unsigned char XSCROLLRATE = 16;
		const unsigned char YSCROLLRATE = 16;
	#elif defined SCD
		const unsigned int P1OFFSET = 0xFFD000;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned char XPo = 0x8;
		const unsigned char YPo = 0xC;
		const unsigned char XVo = 0x10;
		const unsigned char YVo = 0x12;
		const unsigned char To = 0x20;
		const unsigned char Wo = 0x17;
		const unsigned char Ho = 0x16;
		const unsigned char Fo = 1;
		const unsigned int CAMOFFSET1 = 0xFFF700;
		const unsigned int CAMOFFSET2 = 0xFF1926;
		const unsigned int CAMOFFSET3 = 0xFFFCA4;
		const unsigned int INLEVELFLAG = 0xFFD001;
		const unsigned int POSOFFSET = 0xD000;
		const unsigned int SSTLEN = 0x2000;
		const unsigned int SPRITESIZE = 0x40;
		const unsigned int LEVELHEIGHT = 2047;
		const unsigned char XSCROLLRATE = 16;
		const unsigned char YSCROLLRATE = 16;
	#elif defined S1
		const unsigned int P1OFFSET = 0xFFD000;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned char XPo = 0x8;
		const unsigned char YPo = 0xC;
		const unsigned char XVo = 0x10;
		const unsigned char YVo = 0x12;
		const unsigned char To = 0x20;
		const unsigned char Wo = 0x17;
		const unsigned char Ho = 0x16;
		const unsigned char Fo = 1;
		const unsigned int CAMOFFSET1 = 0xFFF700;
		const unsigned int CAMOFFSET2 = 0xFFF710;
		const unsigned int CAMOFFSET3 = 0xFFFDB8;
		const unsigned int CAMOFFSET4 = 0xFFF616;
		const unsigned int INLEVELFLAG = 0xFFD04B;
		const unsigned int POSOFFSET = 0xD000;
		const unsigned int SSTLEN = 0x2000;
		const unsigned int SPRITESIZE = 0x40;
		const unsigned int LEVELHEIGHT = 2047;
		const unsigned char XSCROLLRATE = 16;
		const unsigned char YSCROLLRATE = 16;
	#endif
		const unsigned char SizeTableX[] = {0x04, 0x14, 0x0C, 0x14, 0x04, 0x0C, 0x10, 0x06, 0x18, 0x0C, 0x10, 0x08, 0x14,
											0x14, 0x0E, 0x18, 0x28, 0x10, 0x08, 0x20, 0x40, 0x80, 0x20, 0x08, 0x04, 0x20, 
											0x0C, 0x08, 0x18, 0x28, 0x04, 0x04, 0x04, 0x04, 0x18, 0x0C, 0x48, 0x18, 0x10, 
											0x20, 0x04, 0x18, 0x20, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x02, 0x20};
		const unsigned char SizeTableY[] = {0x04, 0x14, 0x14, 0x0C, 0x10, 0x12, 0x10, 0x06, 0x0C, 0x10, 0x0C, 0x08, 0x10, 
											0x08, 0x0E, 0x18, 0x10, 0x18, 0x10, 0x70, 0x20, 0x20, 0x20, 0x08, 0x04, 0x08, 
											0x0C, 0x04, 0x04, 0x04, 0x08, 0x18, 0x28, 0x20, 0x18, 0x18, 0x08, 0x28, 0x04, 
											0x02, 0x40, 0x80, 0x10, 0x20, 0x30, 0x40, 0x50, 0x02, 0x01, 0x08, 0x1C};
//unsigned short CamX = 0;
//short CamY = 0;
unsigned short PX = 0;
unsigned short PY = 0;
bool offscreen = false;
unsigned short (*GETBLOCK)(int X,int Y);
unsigned short GetBlockS1(int X,int Y) 
{
	return (X >> 8) + ((Y & 0x700) >> 1);
}
unsigned short GetBlockSCD(int X,int Y) 
{
	return (X >> 8) + ((Y & 0xF00) >> 1);
}
unsigned short GetBlockS2(int X,int Y)
{
	return ((X >> 7) & 0x7F) + ((Y << 1) & 0xF00);
}
unsigned short GetBlockSK(int X,int Y)
{
	return CheatRead<unsigned short>(0xFF8008 + ((Y >> 5) & CheatRead<unsigned short>(0xFFEEAE))) + (X >> 7);
}
void DisplaySolid ()
{
		unsigned int COLARR,COLARR2,BLOCKSTART,CAMMASK,ANGARR;
		unsigned short BLOCKSIZE,TILEMASK;
		unsigned char BLOCKSHIFT,SOLIDSHIFT,DRAWSHIFT;
		bool S3;
	#ifdef S1
		COLARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0x3F81)? 0x055236 : 0x062A00);
		COLARR2 = ((* (unsigned short *) &Rom_Data[0x18E] == 0x3F81)? 0x056236 : 0x063A00);
		ANGARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0x3F81)? 0x055136 : 0x062900);
		CAMMASK = 0xFF00;	
		BLOCKSIZE = 0x100;
		GETBLOCK = GetBlockS1;//(X >> 8) + ((Y & 0x700) >> 1)
		BLOCKSTART = 0xFFA400;
		BLOCKSHIFT = 9;
		SOLIDSHIFT = 0xD;
		DRAWSHIFT = 0xB;
		TILEMASK = 0x7FF;
		S3 = 0;
	#elif defined SCD
		static unsigned int NITSUJA = 0x2011E8;
		static unsigned int STEALTH = 0;
		ANGARR = CheatRead<unsigned int>(NITSUJA);
		COLARR = ANGARR + 0x100;
		COLARR2 = COLARR + 0x1000;
		CAMMASK = 0xFF00;	
		BLOCKSIZE = 0x100;
		GETBLOCK = GetBlockSCD;//(X >> 8) + ((Y & 0x700) >> 1)
		BLOCKSTART = 0xFFA000;
		BLOCKSHIFT = 9;
		SOLIDSHIFT = 0xD;
		DRAWSHIFT = 0xB;
		TILEMASK = 0x7FF;
		S3 = 0;
	#elif defined S2
		COLARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)? 0x242E50 : 0x042E50);
		COLARR2 = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)? 0x243E50 : 0x043E50);
		ANGARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)? 0x242D50 : 0x42D50);
		CAMMASK = 0xFF80;
		BLOCKSIZE = 0x80;
		GETBLOCK = GetBlockS2;//((X >> 7) & 0x7F) + ((Y << 1) & 0xF00);
		BLOCKSTART = 0xFF8000;
		BLOCKSHIFT = 7;
		SOLIDSHIFT = CheatRead<unsigned char>(0xFFB03E);
		DRAWSHIFT = 0xA;
		TILEMASK = 0x3FF;
		S3 = 0;
	#elif defined SK
		COLARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)?0x096100:0x706A0);
		COLARR2 = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)?0x097100:0x736A0);
		ANGARR = ((* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3)?0x096000:0x704A0);
		CAMMASK = 0xFF80;
		BLOCKSIZE = 0x80;
		GETBLOCK = &GetBlockSK;//CheatRead<unsigned short>(0xFF8008 + ((Y >> 5) & CheatRead<unsigned short>(0xFFEEAE)));
		BLOCKSTART = 0;//(X >> 7);
		BLOCKSHIFT = 7;
		SOLIDSHIFT = CheatRead<unsigned char>(0xFFB046);
		DRAWSHIFT = 0xA;
		TILEMASK = 0x3FF;
		S3 = !(* (unsigned short *) &Rom_Data[0x18E] == 0xDFB3);
	#endif
	if (CheatRead<unsigned int>(NITSUJA) != STEALTH) 
	{
		int addr = 0x200000;
		bool found = false;
		while ((addr <= 0x23FFFC) && !found)
		{
			addr += 2;
			if (CheatRead<unsigned int>(addr) == 0x6100FE16)
			{
				if (CheatRead<unsigned int>(addr + 4) == 0x0C810021)
				{
					found = true;
				}
			}
		}
		if (found) 
		{
			NITSUJA = addr + 0x38;
			STEALTH = CheatRead<unsigned int>(NITSUJA);
			ANGARR = STEALTH;
			COLARR = ANGARR + 0x100;
			COLARR2 = COLARR + 0x1000;
		}
		else return;
	}
#ifdef SCD
	if (CheatRead<char>(INLEVELFLAG))
#else
	if (((CheatRead<char>(0xFFF600) & 0x7F) == 0x18)||((CheatRead<char>(0xFFF600) & 0x7F) == 8)||((CheatRead<char>(0xFFF600) & 0x7F) == 12))
#endif
	{
		unsigned long ColMapPt;
		int X,TempX,Y,TempY;
		unsigned short ColInd,BlockNum,Tile;
//		short ;
		unsigned char TileNum,Block,SolidType,DrawType,Angle;
		unsigned char SolidMap[336 * 240];
		memset(SolidMap,0,336*240);

		Y = CamY & CAMMASK; //round y down to 128 pixel boundary
		if (Y & 0x8000) Y |= 0xFFFF0000;
		while (Y < CamY + 224)
		{
			X = CamX & CAMMASK;	//round x down to 128 pixel boundary
			if (X & 0x8000) X |= 0xFFFF0000;
			while (X < CamX + 320)
			{
				BlockNum = GETBLOCK(X,Y);
				#ifdef SK
					unsigned int ADDR = BLOCKSTART + BlockNum;
					if (ADDR & 0x8000) ADDR |= 0xFF0000;
					Block = CheatRead<unsigned char>(ADDR);
				#else
					Block = CheatRead<unsigned char>(BLOCKSTART + BlockNum);
				#endif
				#if defined S1 || defined SCD
					if (Block)
					{
						if (Block & 0x80) 
						{
							Block &= 0x7F;
							if (CheatRead<unsigned char>(0xFFD001) & 0x40)
							{
								Block ++;
								if (Block == 0x29) Block = 0x51;
							}
						}
						Block--;
						if ((GetKeyState(VK_CAPITAL)))
						{
							sprintf(Str_Tmp,"%02X",Block);
							Print_Text(Str_Tmp,2,min(max((X-CamX)-1,0),310),min(max(Y-CamY,1),215),BLANC);
							Print_Text(Str_Tmp,2,min(max((X-CamX)+1,2),312),min(max(Y-CamY,1),215),BLANC);
							Print_Text(Str_Tmp,2,min(max((X-CamX),1),311),min(max((Y-CamY)-1,0),214),BLANC);
							Print_Text(Str_Tmp,2,min(max((X-CamX),1),311),min(max((Y-CamY)+1,2),216),BLANC);
							Print_Text(Str_Tmp,2,max(X-CamX,1),min(max(Y-CamY,1),215),BLEU);
						}
					#endif
					TempY = Y;
					while ((TempY < Y + BLOCKSIZE) && (TempY < CamY + 224))
					{
						while ((TempY + 0x10) < CamY ) TempY+=0x10;
						if (TempY < Y + BLOCKSIZE)
						{
							TempX = X;
							while ((TempX < X + BLOCKSIZE) && (TempX < CamX + 320))
							{
								while ((TempX + 0x10) < CamX) TempX += 0x10;
								if (TempX < X + BLOCKSIZE)
								{
									#if defined S1
										TileNum = ((TempX >> 4) & 0xF) + (TempY & 0xF0);
										Tile = CheatRead<short>(0xFF0000 | (((unsigned short)Block << BLOCKSHIFT) + (TileNum << 1)));
									#elif defined SCD
										TileNum = ((TempX >> 4) & 0xF) + (TempY & 0xF0);
										Tile = CheatRead<short>(0x210000 | (((unsigned short)Block << BLOCKSHIFT) + (TileNum << 1)));
									#else
										TileNum = ((TempX >> 3) & 0xE) + (TempY & 0x70);
										Tile = CheatRead<short>(0xFF0000 | (((unsigned short)Block << BLOCKSHIFT) + TileNum));
										SolidType = (Tile >> (SOLIDSHIFT ^ 2)) & 3;
										DrawType = (Tile >> DRAWSHIFT) & 3;
										Tile &= TILEMASK;
										if (!Tile) {TempX +=0x10; continue;}
										ColMapPt = (SOLIDSHIFT & 2);
										ColMapPt ^= 2;
										#ifdef SK
											ColMapPt <<= 1;
											ColMapPt += 0xF7B4;
											ColMapPt = CheatRead<unsigned long>(0xFF0000 | ColMapPt);
											if (S3) ColMapPt++;
											short ColInd = (CheatRead<unsigned char>(ColMapPt + (Tile << 1)) & 0xFF) << 4;
										#else
											ColMapPt = ((ColMapPt & 2)?0xFFFFD900:0xFFFFD600);
											ColInd = (CheatRead<unsigned char>(ColMapPt + Tile) & 0xFF) << 4;
										#endif
										if (SolidType & 1)
										{
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 4;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 4;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										if (SolidType & 2)
										{
											unsigned char width;
											int blahy = 0;
											while (blahy < 0x10) 
											{
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												width = CheatRead<unsigned char>(COLARR2 + ColInd + blahy);
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												if (DrawType & 1) width = 0 - width;
												if (width <= 0x10)
												{
													for (int drawX = (TempX + 0xF); (drawX + width) >= (TempX + 0x10); drawX--)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 8;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x20);
														g = max(0,g-0x40);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												else if (width >= 0xF0)
												{
													width = 0x100 - width;
													for (int drawX = TempX; (drawX - width) < TempX; drawX++)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 8;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x20);
														g = max(0,g-0x40);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												blahy++;
											}
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 8;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 8;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										Tile = CheatRead<short>(0xFF0000 |(((unsigned short)Block << BLOCKSHIFT) + TileNum));
									#endif
									SolidType = (Tile >> SOLIDSHIFT) & 3;
									DrawType = (Tile >> DRAWSHIFT) & 3;
									Tile &= TILEMASK;
									if (!Tile) {TempX +=0x10; continue;}
									if (!SolidType) {TempX +=0x10; continue;}
									#if !(defined S1 || defined SCD)
										ColMapPt = (SOLIDSHIFT & 2);
										#ifdef SK
											ColMapPt <<= 1;
											ColMapPt += 0xF7B4;
											ColMapPt = CheatRead<unsigned long>(0xFF0000 | ColMapPt);
											if (S3) ColMapPt++;
										#elif defined S2
											ColMapPt = ((ColMapPt & 2)?0xFFFFD900:0xFFFFD600);
										#else
											ColMapPt = CheatRead<unsigned long>(0xFFF796);
										#endif
									#else
										ColMapPt = CheatRead<unsigned long>(0xFFF796);
									#endif

									#ifdef SK
										ColInd = ((CheatRead<unsigned char>(ColMapPt + (Tile << 1))) & 0xFF) << 4;
									#else
										ColInd = (CheatRead<unsigned char>(ColMapPt + Tile) & 0xFF) << 4;
									#endif
									Angle = CheatRead<unsigned char>(ANGARR + (ColInd >> 4));
									if (!(Angle & 1))
									{
										if (DrawType & 1) Angle = (0 - Angle);
										if (DrawType & 2) Angle = (0 - (Angle + 0x40)) - 0x40;
									}
									if (SolidType & 1)
									{
										unsigned char height;
										int blahx = 0;
										while (blahx < 0x10) 
										{
											if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
											height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
											if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
											if (DrawType & 2) height = 0 - height;
											if (height <= 0x10)
											{
												for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
												{
													if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 1;
/*													int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = max(0,r-0x80);
													g = min(0xFF,g+0x40);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
												}
											}
											else if (height >= 0xF0)
											{
												height = 0x100 - height;
												for (int drawY = TempY; (drawY - height) < TempY; drawY++)
												{
													if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 1;
/*													int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = max(0,r-0x80);
													g = min(0xFF,g+0x40);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
												}
											}
											blahx++;
										}
									}
									if (SolidType & 2)
									{
										unsigned char width;
										int blahy = 0;
										while (blahy < 0x10) 
										{
											if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
											width = CheatRead<unsigned char>(COLARR2 + ColInd + blahy);
											if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
											if (DrawType & 1) width = 0 - width;
											if (width <= 0x10)
											{
												for (int drawX = (TempX + 0xF); (drawX + width) >= (TempX + 0x10); drawX--)
												{
													if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 2;
/*													int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = min(0xFF,r+0x40);
													g = max(0,g-0x80);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
												}
											}
											else if (width >= 0xF0)
											{
												width = 0x100 - width;
												for (int drawX = TempX; (drawX - width) < TempX; drawX++)
												{
													if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 2;
/*													int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = min(0xFF,r+0x40);
													g = max(0,g-0x80);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
												}
											}
											blahy++;
										}
										unsigned char height;
										int blahx = 0;
										while (blahx < 0x10) 
										{
											if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
											height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
											if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
											if (DrawType & 2) height = 0 - height;
											if (height <= 0x10)
											{
												for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
												{
													if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 2;
/*													int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = max(0,r-0x80);
													g = min(0xFF,g+0x40);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
												}
											}
											else if (height >= 0xF0)
											{
												height = 0x100 - height;
												for (int drawY = TempY; (drawY - height) < TempY; drawY++)
												{
													if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 2;
/*													int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
													short r= pix >> 16 & 0xFF;
													short g = pix >> 8 & 0xFF;
													short b = pix & 0xFF;
													r = max(0,r-0x80);
													g = min(0xFF,g+0x40);
													b = min(0xFF,b+0x40);
													pix = (r << 16) | (g << 8) | b;
													MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
												}
											}
											blahx++;
										}
									}
									TempX += 0x10;
									if (GetKeyState(VK_NUMLOCK))
									{
										sprintf(Str_Tmp,"%02X",Angle);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC) -CamX)-1,0),max(TempY-CamY,1),ROUGE);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX)+1,2),max(TempY-CamY,1),ROUGE);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)-1,0),ROUGE);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)+1,2),ROUGE);
										Print_Text(Str_Tmp,2,max((TempX - 0xC)-CamX,1),max(TempY-CamY,1),BLEU);
										TempY += 8;
										sprintf(Str_Tmp,"%02X",ColInd >> 4);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC) -CamX)-1,0),max(TempY-CamY,1),VERT);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX)+1,2),max(TempY-CamY,1),VERT);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)-1,0),VERT);
										Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)+1,2),VERT);
										Print_Text(Str_Tmp,2,max((TempX - 0xC)-CamX,1),max(TempY-CamY,1),BLEU);
										TempY -= 8;
									}
								}
							}
						}
						TempY += 0x10;
				#if defined S1 || defined SCD
					}
				#endif
				}
				X += BLOCKSIZE;
			}
			Y+=BLOCKSIZE;
		}
		#ifdef SK
			if (CheatRead<unsigned char>(0xFFF664))
			{
				unsigned short X;
				short Y,Xoff,Yoff;
				Xoff = CheatRead<signed short>(0xFFEE3E);
				Yoff = CheatRead<signed short>(0xFFEE40);
				Y = (CamY - Yoff) & CAMMASK; //round y down to 128 pixel boundary
				while ((Y + Yoff) < CamY + 224)
				{
					X = (CamX - Xoff) & CAMMASK;	//round x down to 128 pixel boundary
					while ((X + Xoff) < CamX + 320)
					{
						BlockNum = CheatRead<unsigned short>(0xFF800A + ((Y >> 5) & CheatRead<unsigned short>(0xFFEEAE))) + (X >> 7);
						unsigned int ADDR = BLOCKSTART + BlockNum;
						if (ADDR & 0x8000) ADDR |= 0xFF0000;
						Block = M68K_RB(ADDR);
						if ((GetKeyState(VK_NUMLOCK)))
						{
							sprintf(Str_Tmp,"%02X",Block);
							Print_Text(Str_Tmp,2,max((X+Xoff-CamX)-1,0),max(Y+Yoff-CamY,1),BLANC);
							Print_Text(Str_Tmp,2,max((X+Xoff-CamX)+1,2),max(Y+Yoff-CamY,1),BLANC);
							Print_Text(Str_Tmp,2,max((X+Xoff-CamX),1),max((Y+Yoff-CamY)-1,0),BLANC);
							Print_Text(Str_Tmp,2,max((X+Xoff-CamX),1),max((Y+Yoff-CamY)+1,2),BLANC);
							Print_Text(Str_Tmp,2,max(X+Xoff-CamX,1),max(Y+Yoff-CamY,1),BLEU);
						}
						TempY = Y+Yoff;
						while ((TempY < Y+Yoff + BLOCKSIZE) && (TempY < CamY + 224))
						{
							while ((TempY + 0x10) < CamY ) TempY+=0x10;
							if (TempY < Y+Yoff + BLOCKSIZE)
							{
								TempX = X+Xoff;
								while ((TempX < X+Xoff + BLOCKSIZE) && (TempX < CamX + 320))
								{
									while ((TempX + 0x10) < CamX) TempX += 0x10;
									if (TempX < X+Xoff + BLOCKSIZE)
									{
										TileNum = (((TempX - Xoff) >> 3) & 0xE) + ((TempY - Yoff) & 0x70);
										Tile = CheatRead<short>(0xFF0000 | (((unsigned short)Block << BLOCKSHIFT) + TileNum));
										unsigned char SolidType = (Tile >> (SOLIDSHIFT ^ 2)) & 3;
										unsigned char DrawType = (Tile >> DRAWSHIFT) & 3;
										Tile &= TILEMASK;
										if (!Tile) {TempX +=0x10; continue;}
//										if (!SolidType) {TempX +=0x10; continue;}
										unsigned long ColMapPt = CheatRead<unsigned long>(0xFFF796);
										if (S3) ColMapPt++;
										short ColInd = (CheatRead<unsigned char>(ColMapPt + (Tile << 1)) & 0xFF) << 4;
										if (SolidType & 1)
										{
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x40;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x40;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										if (SolidType & 2)
										{
											unsigned char width;
											int blahy = 0;
											while (blahy < 0x10) 
											{
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												width = CheatRead<unsigned char>(COLARR2 + ColInd + blahy);
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												if (DrawType & 1) width = 0 - width;
												if (width <= 0x10)
												{
													for (int drawX = (TempX + 0xF); (drawX + width) >= (TempX + 0x10); drawX--)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 0x80;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x20);
														g = max(0,g-0x40);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												else if (width >= 0xF0)
												{
													width = 0x100 - width;
													for (int drawX = TempX; (drawX - width) < TempX; drawX++)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 0x80;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x20);
														g = max(0,g-0x40);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												blahy++;
											}
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x80;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x80;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x40);
														g = min(0xFF,g+0x20);
														b = min(0xFF,b+0x20);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										Tile = CheatRead<short>(0xFF0000 | (((unsigned short)Block << BLOCKSHIFT) + TileNum));
										SolidType = (Tile >> SOLIDSHIFT) & 3;
										Tile &= TILEMASK;
										if (!Tile) {TempX +=0x10; continue;}
										if (!SolidType) {TempX +=0x10; continue;}
										ColMapPt = CheatRead<unsigned long>(0xFFF796);
										ColInd = (CheatRead<unsigned char>(ColMapPt + (Tile << 1)) & 0xFF) << 4;
										if (SolidType & 1)
										{
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x10;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x80);
														g = min(0xFF,g+0x40);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x10;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x80);
														g = min(0xFF,g+0x40);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										if (SolidType & 2)
										{
											unsigned char width;
											int blahy = 0;
											while (blahy < 0x10) 
											{
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												width = CheatRead<unsigned char>(COLARR2 + ColInd + blahy);
												if (DrawType & 2) blahy = ~blahy, blahy &= 0xF;
												if (DrawType & 1) width = 0 - width;
												if (width <= 0x10)
												{
													for (int drawX = (TempX + 0xF); (drawX + width) >= (TempX + 0x10); drawX--)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 0x20;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x40);
														g = max(0,g-0x80);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												else if (width >= 0xF0)
												{
													width = 0x100 - width;
													for (int drawX = TempX; (drawX - width) < TempX; drawX++)
													{
														if ((((TempY + blahy - CamY) >= 0) && ((TempY + blahy - CamY) < 224)) && ((((drawX - CamX) + 8) >= 8) && (((drawX - CamX) + 8) < 328))) SolidMap[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] |= 0x20;
/*														int pix = MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = min(0xFF,r+0x40);
														g = max(0,g-0x80);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((TempY + blahy - CamY) * 336) + (drawX - CamX) + 8] = pix;*/
													}
												}
												blahy++;
											}
											unsigned char height;
											int blahx = 0;
											while (blahx < 0x10) 
											{
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												height = CheatRead<unsigned char>(COLARR + ColInd + blahx);
												if (DrawType & 1) blahx = ~blahx, blahx &= 0xF;
												if (DrawType & 2) height = 0 - height;
												if (height <= 0x10)
												{
													for (int drawY = (TempY + 0xF); (drawY + height) >= (TempY + 0x10); drawY--)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x20;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x80);
														g = min(0xFF,g+0x40);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												else if (height >= 0xF0)
												{
													height = 0x100 - height;
													for (int drawY = TempY; (drawY - height) < TempY; drawY++)
													{
														if ((((drawY - CamY) >= 0) && ((drawY - CamY) < 224)) && ((((TempX + blahx - CamX) + 8) >= 8) && (((TempX + blahx - CamX) + 8) < 328))) SolidMap[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] |= 0x20;
/*														int pix = MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8];
														short r= pix >> 16 & 0xFF;
														short g = pix >> 8 & 0xFF;
														short b = pix & 0xFF;
														r = max(0,r-0x80);
														g = min(0xFF,g+0x40);
														b = min(0xFF,b+0x40);
														pix = (r << 16) | (g << 8) | b;
														MD_Screen32[((drawY - CamY) * 336) + (TempX + blahx - CamX) + 8] = pix;*/
													}
												}
												blahx++;
											}
										}
										TempX += 0x10;
										if (GetKeyState(VK_NUMLOCK))
										{
											sprintf(Str_Tmp,"%02X",ColInd >> 4);
											TempY += 8;
											Print_Text(Str_Tmp,2,max(((TempX - 0xC) -CamX)-1,0),max(TempY-CamY,1),ROUGE);
											Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX)+1,2),max(TempY-CamY,1),ROUGE);
											Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)-1,0),ROUGE);
											Print_Text(Str_Tmp,2,max(((TempX - 0xC)-CamX),1),max((TempY-CamY)+1,2),ROUGE);
											Print_Text(Str_Tmp,2,max((TempX - 0xC)-CamX,1),max(TempY-CamY,1),BLEU);
											TempY -= 8;
										}
									}
								}
							}
							TempY += 0x10;
						}
						X += BLOCKSIZE;
					}
					Y+=BLOCKSIZE;
				}	
			}
		#endif
		for (short y = 0; y < 224; y++)
		{
			for (short x = 8; x<328;x++)
			{
				if (!SolidMap[(y*336)+x]) continue;
				unsigned int pix = MD_Screen32[(y*336)+x];
				short r,g,b;
				r = (pix >> 16) & 0xFF;
				g = (pix >> 8) & 0xFF;
				b = pix & 0xFF;
				if ((SolidMap[(y*336)+x] & 0x3) == 1)
				{
					r -= 0x30;
					g += 0x60;
					b -= 0x30;
				}
				if ((SolidMap[(y*336)+x] & 0x3) == 2)
				{
					r += 0x60;
					g -= 0x30;
					b -= 0x30;
				}
				if ((SolidMap[(y*336)+x] & 0x3) == 3)
				{
					r -= 0x30;
					g -= 0x30;
					b += 0x60;
				}
				if ((SolidMap[(y*336)+x] & 0xC) == 4)
				{
					r -= 0x18;
					g += 0x30;
					b -= 0x18;
				}
				if ((SolidMap[(y*336)+x] & 0xC) == 8)
				{
					r += 0x30;
					g -= 0x18;
					b -= 0x18;
				}
				if ((SolidMap[(y*336)+x] & 0xC) == 0xC)
				{
					r -= 0x18;
					g -= 0x18;
					b += 0x30;
				}
				if ((SolidMap[(y*336)+x] & 0x30) == 0x10)
				{
					r -= 0x30;
					g += 0x60;
					b -= 0x30;
				}
				if ((SolidMap[(y*336)+x] & 0x30) == 0x20)
				{
					r += 0x60;
					g -= 0x30;
					b -= 0x30;
				}
				if ((SolidMap[(y*336)+x] & 0x30) == 0x30)
				{
					r -= 0x30;
					g -= 0x30;
					b += 0x60;
				}
				if ((SolidMap[(y*336)+x] & 0xC0) == 0x40)
				{
					r -= 0x18;
					g += 0x30;
					b -= 0x18;
				}
				if ((SolidMap[(y*336)+x] & 0xC0) == 0x80)
				{
					r += 0x30;
					g -= 0x18;
					b -= 0x18;
				}
				if ((SolidMap[(y*336)+x] & 0xC0) == 0xC0)
				{
					r -= 0x18;
					g -= 0x18;
					b += 0x30;
				}
				if (r < 0) r = 0;
				if (r > 0xFF) r = 0xFF;
				if (g < 0) g = 0;
				if (g > 0xFF) g = 0xFF;
				if (b < 0) b = 0;
				if (b > 0xFF) b = 0xFF;
				pix = (r << 16) | (g << 8) | b;
				MD_Screen32[(y*336)+x]=pix;
			}
		}
	}
}
void DrawBoxes()
{
	if (!GetKeyState(VK_SCROLL))
	{
		PX = CheatRead<unsigned short>(P1OFFSET + XPo);
		PY = CheatRead<unsigned short>(P1OFFSET + YPo);
		short Xpos,Ypos;
		unsigned char Height,Width,Height2,Width2,Type;
		bool Touchable;
		unsigned int DrawColor32;
		unsigned short DrawColor16;
		for (unsigned int CardBoard = P1OFFSET; CardBoard < P1OFFSET + SSTLEN; CardBoard += SPRITESIZE)
		{
			if (!CheatRead<unsigned long>(0xFF0000 + CardBoard)) continue;
			Xpos = CheatRead<short>(CardBoard + XPo);
			Ypos = CheatRead<short>(CardBoard + YPo);
			if ((CheatRead<unsigned char>(CardBoard + Fo) & 4) && !(Xpos | Ypos)) continue;
			Height = SizeTableY[(CheatRead<unsigned char>(CardBoard + To) & 0x3F)];
			Width = SizeTableX[(CheatRead<unsigned char>(CardBoard + To) & 0x3F)];
			Touchable = (CheatRead<unsigned char>(CardBoard + To)?true:false);
			if (Touchable) Type = (CheatRead<unsigned char>(CardBoard + To) & 0xC0) >> 6, DrawColor32=ColorTable32[Type],DrawColor16=ColorTable16[Type];
			else DrawColor32 = 0xFFFFFF, DrawColor16 = 0xFFFF;
			Height2 = CheatRead<unsigned char>(CardBoard + Ho);
			Width2 = CheatRead<unsigned char>(CardBoard + Wo);
			#ifdef SK
				bool ducking = (CheatRead<unsigned char>(CardBoard + 0x22) == 0x39);
			#else
				bool ducking = (CheatRead<unsigned char>(CardBoard + 0x1A) == 0x39);
			#endif
			if (CardBoard == P1OFFSET)
			{
	//			bool instashield = ((Ram_68k[0xFFBA] == 01) && !(Ram_68k[0xFE2C]) && !(Ram_68k[0xFE2D]) && (Ram_68k[0xFED1] == 2));
				Width = 10;
				Height = (ducking) ? 0xA : CheatRead<unsigned char>(CardBoard + Ho);
	//			if (instashield)
	//			{
	//				Width = 0x18;
	//				Height = 0x18;
	//			}
			}
			unsigned char angle = CheatRead<unsigned char>(CardBoard + 0x26);
			angle += 0x20;
			if (angle & 0x40) Width2 ^= Height2, Height2 ^= Width2, Width2 ^= Height2;
			Xpos += 8;	
			if ((CheatRead<unsigned char>(CardBoard + Fo) & 0x4) || (CheatRead<unsigned char>(CardBoard) == 0x7D)) {
				Xpos -= CamX;
				Ypos -= CamY;
			}
			else {
				Xpos -= 0x80;
				Ypos = (CheatRead<short>(CardBoard + 2 + XPo)) - 0x80;
			}
	#ifdef S2
			if (!(Ram_68k[CardBoard + Fo] & 0x40))
			{
	#endif
				Width2 = min(Width2,0xFE);
				Height2 = min(Height2,0xFE);
				for (unsigned char JXQ = 0; JXQ <= Width2; JXQ++)
				{
					if (Bits32) 
					{
						MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xFF00FF;
					}
					else
					{
						MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xF81F;
					}
				}
				for (unsigned char JXQ = 0; JXQ <= Height2; JXQ++)
				{
					if (Bits32) 
					{
						MD_Screen32[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xFF00FF;
						MD_Screen32[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xFF00FF;
					}
					else
					{
						MD_Screen[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xF81F;
						MD_Screen[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xF81F;
					}
				}
	#ifdef S2
			}
	#endif
			if (ducking && (CardBoard == P1OFFSET)) Ypos+=CheatRead<unsigned char>(CardBoard + Ho) - 0xA;
			for (char jxq = 0; jxq <=2; jxq++)
			{
				for (char Jxq = 0; Jxq <=2; Jxq++)
				{
					if (Bits32)
					{
						MD_Screen32[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x00FF00;
						MD_Screen32[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x00FF00;
						MD_Screen32[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x00FF00;
						MD_Screen32[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x00FF00;
					}
					else
					{
						MD_Screen[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x07E0;
						MD_Screen[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x07E0;
						MD_Screen[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x07E0;
						MD_Screen[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x07E0;
					}
				}
			}
			if(Bits32)
			{
				MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen32[max(8,min(327,(Xpos - 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen32[max(8,min(327,(Xpos + 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
				MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos + 1)))] = 0;
				MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
			}
			else
			{
				MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen[max(8,min(327,(Xpos - 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen[max(8,min(327,(Xpos + 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
				MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
				MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos + 1)))] = 0;
				MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
			}
	//		if (!(Ram_68k[CardBoard + Fo] & 0x04))
	//			continue;
			if  (!(CheatRead<unsigned char>(CardBoard + To)) && (CardBoard > P1OFFSET))
			{
				FindObjectDims(CardBoard,Width,Height);
	//				Ypos -= 0x8;
			}
			if (CheatRead<unsigned char>(CardBoard) == 0x7D) Width = Height = 0x10;
			Height = min(Height,0xFE);
			Width = min(Width,0xFE);
			for (unsigned char JXQ = 0; JXQ <= Width; JXQ++)
			{
				if(Bits32)
				{
					MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height))))] = DrawColor32;
				}
				else
				{
					MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height))))] = DrawColor16;
				}
			}
			for (unsigned char JXQ = 0; JXQ <= Height; JXQ++)
			{
				if(Bits32)
				{
					MD_Screen32[max(8,min(327,(Xpos - Width))) + (336 * max(0,min(223,(Ypos - JXQ))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos - Width))) + (336 * max(0,min(223,(Ypos + JXQ))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos + Width))) + (336 * max(0,min(223,(Ypos - JXQ))))] = DrawColor32;
					MD_Screen32[max(8,min(327,(Xpos + Width))) + (336 * max(0,min(223,(Ypos + JXQ))))] = DrawColor32;
				}
				else
				{
					MD_Screen[max(8,min(327,(Xpos - Width))) + (336 * max(0,min(223,(Ypos - JXQ))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos - Width))) + (336 * max(0,min(223,(Ypos + JXQ))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos + Width))) + (336 * max(0,min(223,(Ypos - JXQ))))] = DrawColor16;
					MD_Screen[max(8,min(327,(Xpos + Width))) + (336 * max(0,min(223,(Ypos + JXQ))))] = DrawColor16;
				}
			}
			sprintf(Str_Tmp,"%04X",CardBoard & 0xFFFF);
			Print_Text(Str_Tmp,4,max(0,min(303,Xpos-17)),max(0,min(216,Ypos-4)),BLEU);
			Print_Text(Str_Tmp,4,max(2,min(305,Xpos-14)),max(2,min(216,Ypos-4)),BLEU);
			Print_Text(Str_Tmp,4,max(1,min(304,Xpos-16)),max(1,min(215,Ypos-5)),BLEU);
			Print_Text(Str_Tmp,4,max(1,min(304,Xpos-16)),max(1,min(217,Ypos-3)),BLEU);
			Print_Text(Str_Tmp,4,max(1,min(304,Xpos-16)),max(1,min(216,Ypos-4)),VERT);
		}
	#ifdef S2
		for (unsigned short CardBoard = 0xE806; CardBoard < 0xEE00; CardBoard += 6)
		{
			if (((* (short *) &(Ram_68k[CardBoard + 2])) == -1) || !(CheatRead<unsigned char>(INLEVELFLAG))) break;
			else if ((* (short *) &(Ram_68k[CardBoard + 2])) && !(*(short *) &(Ram_68k[CardBoard])))
			{
				Width2 = Height2 = 6;
				Xpos = *(short *) &(Ram_68k[CardBoard + 2]) - CamX;
				Ypos = *(short *) &(Ram_68k[CardBoard + 4]) - CamY;
				Xpos += 8;
				if ((Xpos <= -6) || (Xpos >= 326) || (Ypos <= -6) || (Ypos >= 230)) continue;
				MD_Screen32[max(8,min(327,Xpos)) + (336 * max(0,min(223,Ypos)))] = 0xFF;
				MD_Screen[max(8,min(327,Xpos)) + (336 * max(0,min(223,Ypos)))] = 0x1F;
				for (unsigned char JXQ = 0; JXQ <= Width2; JXQ++)
				{
					if (Bits32) 
					{
						MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0xFF;
					}
					else
					{
						MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos - JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos - Height2))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos + JXQ))) + (336 * max(0,min(223,(Ypos + Height2))))] = 0x1F;
					}
				}
				for (unsigned char JXQ = 0; JXQ <= Height2; JXQ++)
				{
					if (Bits32) 
					{
						MD_Screen32[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0xFF;
						MD_Screen32[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0xFF;
					}
					else
					{
						MD_Screen[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos - Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos - JXQ))))] = 0x1F;
						MD_Screen[max(8,min(327,(Xpos + Width2))) + (336 * max(0,min(223,(Ypos + JXQ))))] = 0x1F;
					}
				}
			}
		}
	#endif
	}
}
int SonicCamHack()
{
#ifdef SK
	LEVELHEIGHT = CheatRead<unsigned short>(0xFFEEAA);
#endif
	static short off = 0;
	bool up = (GetAsyncKeyState(VK_PRIOR))?1:0;
	bool down = (GetAsyncKeyState(VK_NEXT))?1:0;
	unsigned int OBJ;
	unsigned char flags;
	static bool upprev = up;
	static bool downprev = upprev = 0;
	strcpy(Str_Tmp,"");
	unsigned short x;
	signed short y;

	if (up && !upprev && CheatRead<unsigned char>(INLEVELFLAG)) 
	{
		do {
			off -= SPRITESIZE;
			if (off < 0) off += SSTLEN;
			#ifdef SK
				OBJ = CheatRead<unsigned long>(P1OFFSET + off);
			#else
				OBJ = CheatRead<unsigned char>(P1OFFSET + off);
			#endif
				flags = CheatRead<unsigned char>(P1OFFSET + off + Fo);
				x = CheatRead<signed short>(P1OFFSET + off + XPo);
				y = CheatRead<signed short>(P1OFFSET + off + YPo);
		} while (!(OBJ && (flags & 4) && (x | y)));
	}
	else if (down && !downprev && CheatRead<unsigned char>(INLEVELFLAG)) 
	{
		do {
			off += SPRITESIZE;
			if (off >= SSTLEN) off -= SSTLEN;
			#ifdef SK
				OBJ = CheatRead<unsigned long>(P1OFFSET + off);
			#else
				OBJ = CheatRead<unsigned char>(P1OFFSET + off);
			#endif
				flags = CheatRead<unsigned char>(P1OFFSET + off + Fo);
				x = CheatRead<signed short>(P1OFFSET + off + XPo);
				y = CheatRead<signed short>(P1OFFSET + off + YPo);
		} while (!(OBJ && (flags & 4) && (x | y)));
	}
	else {
/*		#ifdef SK
			OBJ = CheatRead<unsigned long>(P1OFFSET + off);
		#else
			OBJ = CheatRead<unsigned char>(P1OFFSET + off);
		#endif*/
		flags = CheatRead<unsigned char>(P1OFFSET + off + Fo);
		x = CheatRead<signed short>(P1OFFSET + off + XPo);
		y = CheatRead<signed short>(P1OFFSET + off + YPo);
	}
	upprev=up;
	downprev=down;
	short origx = (int) CheatRead<signed short>(CAMOFFSET1); //all experiments done with Sonic alone, Tails alone, and Knuckles show that camera positions are
	short origy = (int) CheatRead<signed short>(CAMOFFSET1+4); //signed words with a floor of -255 (which is only reached when travelling upward through level wraps)
	short xx = max(0,CheatRead<signed short>(P1OFFSET + off + XPo) - 160); 
	short yy = CheatRead<signed short>(P1OFFSET + off + YPo) - 112;
#ifndef SCD
	if((GetKeyState(VK_SCROLL)) || (flags & 0x80) || CheatRead<unsigned char>(0xFFF7CD) ||
	  (((unsigned short) (x - origx) <= 320) &&
	  (((unsigned short) (y - origy) <= 240) ||
	  (((unsigned short) (y + LEVELHEIGHT - origy) <= 224) && (origy >= LEVELHEIGHT - 224)) || //so we don't trigger camhack when going downward through level-wraps
	  ((origy < 0) && ((y >= LEVELHEIGHT + origy) || (y <= origy + 224)))) || //so we don't trigger camhack when going upward through level-wraps
	  (CheatRead<unsigned char>(INLEVELFLAG) == 0)))
#endif
	{
		CamX = CheatRead<signed short>(CAMOFFSET1);
		CamY = CheatRead<signed short>(CAMOFFSET1+4);
		int retval = Update_Frame(); // no need for cam hack now
		offscreen = false;
//		DrawBoxes();
		DisplaySolid();
		x = CheatRead<unsigned short>(P1OFFSET + off + XPo);
		y = CheatRead<short>(P1OFFSET + off + YPo);
		if (!GetKeyState(VK_SCROLL))
		{
			sprintf(Str_Tmp,"%04X",(P1OFFSET + off) & 0xFFFF);
			Print_Text(Str_Tmp,4,max(0,min(304,(x-CamX)-8)),max(0,min(216,(y-CamY)-4)),ROUGE);
		}
		return retval;
	}
	offscreen = true;
	xx = (CheatRead<signed short>(P1OFFSET + off + XPo) - 160);
	yy = (CheatRead<signed short>(P1OFFSET + off + YPo) - 112); 
	Export_Genesis(genesisbuf);
	#ifdef SCD
	Export_SegaCD(scdbuf);
	#endif
	origx = (origx > (signed short) xx) ? max(0,xx-320) : max(max(0,xx-320),origx);
	origy = (origy > (signed short) yy) ? yy-240 : max(yy-240,origy);
	int numframesx = (xx - origx)/XSCROLLRATE;
	int numframesy = (yy - origy)/YSCROLLRATE;
	int numframes = max(numframesx,numframesy) + 1;
//	numframes += (int) ((numframes/4.0)+0.5);

	disableSound = true;
	unsigned char posbuf[SSTLEN];
	void *soundbuf;
	soundbuf = malloc(sizeof(YM2612));
	memcpy(soundbuf,&YM2612,sizeof(YM2612));
	memcpy(posbuf,&(Ram_68k[POSOFFSET]),SSTLEN);
	int time = CheatRead<signed int>(0xFFFE22);
	for(int i = 0 ; i < numframes+2 ; i++)
	{
		#ifdef SK
			CheatWrite<unsigned char>(0xFFEE0B, 1); // Freezes world.*/
		#endif
		CheatWrite<signed short>(CAMOFFSET1, origx);
		CheatWrite<signed short>(CAMOFFSET1+4, origy);
		#ifndef S2
			CheatWrite<signed short>(CAMOFFSET2, origx);
			CheatWrite<signed short>(CAMOFFSET2+4, origy);
		#endif
		#ifdef SCD
			CheatWrite<signed short>(CAMOFFSET3, origx);
			CheatWrite<signed short>(CAMOFFSET3+4, origy);
		#endif
		#ifdef S1
			CheatWrite<unsigned long>(CAMOFFSET3, origx);
			CheatWrite<unsigned long>(CAMOFFSET4, origy);
			CheatWrite<unsigned long>(CAMOFFSET4+4, origx);
		#endif
		memcpy(&(Ram_68k[POSOFFSET]),posbuf,SSTLEN); //FREEZE WORLD
		CheatWrite<signed int>(0xFFFE22,time);
		if (yy > origy)
			origy += min(YSCROLLRATE,yy-origy);
		if (xx > origx)
			origx += min(XSCROLLRATE,xx-origx);

		if(i == numframes+1)
		{
			Update_Frame();
//			DrawBoxes();
			DisplaySolid();
			x = CheatRead<unsigned short>(P1OFFSET + off + XPo);
			y = CheatRead<short>(P1OFFSET + off + YPo);
			sprintf(Str_Tmp,"%04X",(P1OFFSET + off) & 0xFFFF);
			Print_Text(Str_Tmp,4,max(0,min(304,(x-CamX)-8)),max(0,min(216,(y-CamY)-4)),ROUGE);
		}
		else
		{
			Update_Frame_Fast();
			if (i == numframes)
			{
				CamX = CheatRead<signed short>(CAMOFFSET1);
				CamY = CheatRead<signed short>(CAMOFFSET1+4);
			}
//			if (Lag_Frame) numframes++;
		}
	}
	disableSound = false;

	Import_Genesis(genesisbuf);
	memcpy(&YM2612,soundbuf,sizeof(YM2612));
	free(soundbuf);

	Update_RAM_Search(); //RAM_Search was updating with the "fake" values, and doesn't seem to update after "skipped" frames.
	return Update_Frame_Fast();
}
#endif