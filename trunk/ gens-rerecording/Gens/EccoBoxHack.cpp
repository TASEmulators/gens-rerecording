#include <stdio.h>
#include <windows.h>
#include "guidraw.h"
#include "EccoBoxHack.h"
#include "HacksCommon.h"
#include "misc.h"
#include "g_main.h"
#include "io.h"
#include "movie.h"
#ifdef ECCOBOXHACK
void EccoDraw3D()
{
	short ScreenX = (CheatRead<int>(0xFFD5E0) >> 0xC);
	short ScreenY = (CheatRead<int>(0xFFD5E8) >> 0xC);
	short ScreenZ = (CheatRead<int>(0xFFD5E4) >> 0xB);
	unsigned int CardBoard = CheatRead<int>(0xFFD4C0);
	while (CardBoard)
	{
		short Xpos = (CheatRead<int>(CardBoard + 0x6) >> 0xC);
		short Ypos = (CheatRead<int>(CardBoard + 0xE) >> 0xC);
		short Zpos = (CheatRead<int>(CardBoard + 0xA) >> 0xB);
		short Y = 224 - (Zpos - ScreenZ);
		short X = (Xpos - ScreenX) + 0xA0;
		unsigned int type = CheatRead<unsigned int> (CardBoard + 0x5A);
		short height,width;
		int display = 0;
		if ((type == 0xD817E) || (type == 0xD4AB8))
		{
			Y = 113 - (Ypos - ScreenY);
			height = 0x10;
			if (CheatRead<unsigned int>(0xFFB166) < 0x1800) height = 0x8;
			short radius = 31;
			if (type == 0xD4AB8) radius = 7, height = 0x20;
			width = radius;
			DrawEccoOct(X,Y,radius,0x00FF00,0x07E0,0,-1);
			display = 1;
		}
		else 
		{
			width = height = 0;
			if (CardBoard == 0xFFFFB134) display = 3;
		}
		if (display & 1)
		{
			Y = 224 - (Zpos - ScreenZ);
			DrawBoxMWH(X,Y,width,height,0x0000FF,0x001F,0,-1);
			DrawBoxMWH(X,Y,width+1,height+1,0,0,0,-1);
			DrawBoxMWH(X,Y,width-1,height-1,0,0,0,-1);
		}
		if (display & 2)
		{
			Y = 113 - (Ypos - ScreenY);
			DrawBoxMWH(X,Y,width,height,0x00FF00,0x07E0,0,-1);
			DrawBoxMWH(X,Y,width+1,height+1,0x00FF00,0x07E0,0,-1);
			DrawBoxMWH(X,Y,width-1,height-1,0x00FF00,0x07E0,0,-1);
		}
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
}
#endif
#if (defined ECCOBOXHACK) || (defined ECCO1BOXHACK)
void EccoDrawBoxes()
{
//	CamX-=8;
	unsigned short Width2,Height2;
	//Ecco HP and Air
	int i = 0;
	short HP = CheatRead<short>(0xFFAA16) << 2;
	short air = CheatRead<short>(0xFFAA18);
	int color32 = (air << 16) | (air << 8) | air;
	short color16 = ((air << 8) & 0xF800) | ((air << 3) & 0x7E0) | (air >> 3);
	for (int j = 0; j < air; j++)
	{
		int off = i * 224;
		if (j - off == 224) i++, off+=224;
		color32 = j >> 2;
		color16 = ((color32 << 8) & 0xF800) | ((color32 << 3) & 0x7E0) | (color32 >> 3);
		color32 = (color32 << 16) | (color32 << 8) | color32;
		DrawLine(0,j - off,8,j - off,color32,color16,0);
	}
	for (i = 8; i < 16; i++)
		for (int j = 0; j < min(HP,224); j++)
		{
			color32 = (j & 0xF0);
			color16 = (j & 0xF0) >> 3;
			Pixel(i,j,color32,color16,0);
		}

	//Asterite
	unsigned int CardBoard = CheatRead<unsigned int>(0xFFCFC8);
	short Xpos,Xpos2,Ypos,Ypos2,Xmid,Ymid,X,X2,Y,Y2;
	while (CardBoard)
	{
		if (CheatRead<unsigned int>(CardBoard + 8))
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x3C);
			Xpos2= CheatRead<unsigned short>(CardBoard + 0x24);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x40);
			Ypos2= CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX, Xpos2 -= CamX;
			Ypos -= CamY, Ypos2 -= CamY;
			Xmid = (Xpos + Xpos2) >> 1;
			Ymid = (Ypos + Ypos2) >> 1;
			if (CheatRead <unsigned char>(CardBoard + 0x71))
			{
				DrawEccoOct(Xpos,Ypos,40,0xFFC000,0xFE00,0,-1);
				DrawEccoOct(Xpos2,Ypos2,40,0xFFC000,0xFE00,0,-1);
			}
		}
		else 
		{
			Xmid = CheatRead<unsigned short>(CardBoard + 0x24) - CamX;
			Ymid = CheatRead<unsigned short>(CardBoard + 0x28) - CamY;
		}
		sprintf(Str_Tmp,"%08X",CardBoard);
		Print_Text(Str_Tmp,8,min(max(0,Xmid-16),285),min(max(0,Ymid-5),216),VERT);
		Print_Text(Str_Tmp,8,min(max(2,Xmid-14),287),min(max(2,Ymid-3),218),VERT);
		Print_Text(Str_Tmp,8,min(max(0,Xmid-16),285),min(max(2,Ymid-3),218),VERT);
		Print_Text(Str_Tmp,8,min(max(2,Xmid-14),287),min(max(0,Ymid-5),216),VERT);
		Print_Text(Str_Tmp,8,min(max(1,Xmid-15),286),min(max(1,Ymid-4),217),BLEU);
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	unsigned char curlev = CheatRead<unsigned char>(0xFFA7E0);
	if ((CheatRead<unsigned char>(0xFFA7D0) == 30))
	{
		CardBoard = CheatRead<unsigned int>(0xFFD424);
		if ((CardBoard) && (CheatRead<unsigned int>(CardBoard + 8)))
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x1C) - CamX;
			Ypos = CheatRead<unsigned short>(CardBoard + 0x20) - CamY;
			DrawEccoOct(Xpos,Ypos,20,0xFFC000,0xFE00,0,-1);
		}
	}
	//aqua tubes
	CardBoard = CheatRead<unsigned int>(0xFFCFC4);
	while (CardBoard)
	{
		Xpos = CheatRead<unsigned short>(CardBoard + 0x2C);
		Xpos2= CheatRead<unsigned short>(CardBoard + 0x34);
		Ypos = CheatRead<unsigned short>(CardBoard + 0x30);
		Ypos2= CheatRead<unsigned short>(CardBoard + 0x38);
		Xpos -= CamX, Xpos2 -= CamX;
		Ypos -= CamY, Ypos2 -= CamY;
		Xmid = (Xpos + Xpos2) >> 1;
		Ymid = (Ypos + Ypos2) >> 1;
//		displayed = false;
		unsigned char type = CheatRead<unsigned char>(CardBoard + 0x7E);
		int yoff = 0;
		switch (type)
		{
/*			case 0x11:
				Xpos2 = Xmid;
				Xmid = (Xpos + Xpos2) >> 1;
				break;
			case 0x12:
				Xpos = Xmid;
				Xmid = (Xpos + Xpos2) >> 1;
				break;
			case 0x13:
				Ypos = Ymid;
				Ymid = (Ypos + Ypos2) >> 1;
				break;
			case 0x14:
				Ypos2 = Ymid;
				Ymid = (Ypos + Ypos2) >> 1;
				break;*/
			case 0x15:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++,yoff++)
					Pixel(Xpos2 - JXQ,Ymid + yoff,0x7F00FF,0x381F,0);
				for (short JXQ = min(max(0,Xmid),320); JXQ <= min(max(8,Xpos2),327); JXQ++)
					Pixel(JXQ,Ymid,0x7F00FF,0x381F,0);
				for (unsigned short JXQ = min(max(0,Ymid),223); JXQ <= min(max(0,Ypos2),223); JXQ++)
					Pixel(Xmid,JXQ,0x7F00FF,0x381F,0);
				break;
			case 0x18:
			case 0x19:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xmid + yoff,Ypos2-JXQ,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x1A:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++,yoff++)
					Pixel(Xpos + JXQ,Ymid + yoff,0x7F00FF,0x381F,0);
				for (short JXQ = min(max(0,Xpos),320); JXQ <= min(max(8,Xmid),327); JXQ++)
					Pixel(JXQ,Ymid,0x7F00FF,0x381F,0);
				for (unsigned short JXQ = min(max(0,Ymid),223); JXQ <= min(max(0,Ypos2),223); JXQ++)
					Pixel(Xmid,JXQ,0x7F00FF,0x381F,0);
				break;
			case 0x1D:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xmid - yoff,Ypos2 - JXQ,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x1F:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++,yoff++)
					Pixel(Xpos + JXQ,Ymid - yoff,0x7F00FF,0x381F,0);
				for (short JXQ = min(max(0,Xpos),320); JXQ <= min(max(8,Xmid),327); JXQ++)
					Pixel(JXQ,Ymid,0x7F00FF,0x381F,0);
				for (unsigned short JXQ = min(max(0,Ypos),223); JXQ <= min(max(0,Ymid),223); JXQ++)
					Pixel(Xmid,JXQ,0x7F00FF,0x381F,0);
				break;
			case 0x20:
			case 0x21:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++)
				{
					Pixel(Xpos + JXQ, Ymid - yoff,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x22:
			case 0x23:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xmid - yoff,Ypos + JXQ,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x24:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++,yoff++)
					Pixel(Xpos2 - JXQ, Ymid - yoff,0x7F00FF,0x381F,0);
				break;
			case 0x25:
			case 0x26:
				for (short JXQ = 0; JXQ <= Xmid-Xpos; JXQ++)
				{
					Pixel(Xpos2 - JXQ, Ymid - yoff,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x27:
			case 0x28:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xmid + yoff, Ypos + JXQ,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				break;
			case 0x2B:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xpos + yoff,Ymid - JXQ,0x7F00FF,0x381F,0);
					Pixel(Xpos2 - yoff,Ymid - JXQ,0x7F00FF,0x381F,0);
					if (JXQ & 1) yoff++;
				}
				yoff = Xmid - (Xpos + yoff);
				for (short JXQ = 0; JXQ <= yoff; JXQ++)
				{
					Pixel(Xmid + JXQ,Ypos,0x7F00FF,0x381F,0);
					Pixel(Xmid - JXQ,Ypos,0x7F00FF,0x381F,0);
				}
				break;
			default:
				DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,0x7F00FF,0x381F,0,-1);
				break;
		}
//		if (!displayed)
		if (type != 0x10)
		{
			sprintf(Str_Tmp,"%02X",CheatRead<unsigned char>(CardBoard + 0x7E));
			Print_Text(Str_Tmp,2,min(max(0,Xmid-3),308),min(max(0,Ymid-5),216),ROUGE);
			Print_Text(Str_Tmp,2,min(max(2,Xmid-1),310),min(max(2,Ymid-3),218),ROUGE);
			Print_Text(Str_Tmp,2,min(max(0,Xmid-3),308),min(max(2,Ymid-3),218),ROUGE);
			Print_Text(Str_Tmp,2,min(max(2,Xmid-1),310),min(max(0,Ymid-5),216),ROUGE);
			Print_Text(Str_Tmp,2,min(max(1,Xmid-2),309),min(max(1,Ymid-4),217),BLEU);
		}
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	//walls
	bool displayed;
	CardBoard = CheatRead<unsigned int>(0xFFCFC0);
	while (CardBoard)
	{

		Xpos = CheatRead<unsigned short>(CardBoard + 0x2C);
		Xpos2= CheatRead<unsigned short>(CardBoard + 0x34);
		Ypos = CheatRead<unsigned short>(CardBoard + 0x30);
		Ypos2= CheatRead<unsigned short>(CardBoard + 0x38);
		Xpos -= CamX, Xpos2 -= CamX;
		Ypos -= CamY, Ypos2 -= CamY;
		Xmid = (Xpos + Xpos2) >> 1;
		Ymid = (Ypos + Ypos2) >> 1;
		displayed = false;
		unsigned char type = CheatRead<unsigned char>(CardBoard + 0x7E);
		int yoff = 0;
		switch (type)
		{
			case 0x11:
				Xpos2 = Xmid;
				Xmid = (Xpos + Xpos2) >> 1;
				break;
			case 0x12:
				Xpos = Xmid;
				Xmid = (Xpos + Xpos2) >> 1;
				break;
			case 0x13:
				Ypos = Ymid;
				Ymid = (Ypos + Ypos2) >> 1;
				break;
			case 0x14:
				Ypos2 = Ymid;
				Ymid = (Ypos + Ypos2) >> 1;
				break;
			case 0x15:
			case 0x16:
			case 0x17:
			case 0x18:
			case 0x19:
				DrawLine(Xmid,Ypos2,Xpos2,Ymid,-1,-1,0);
				displayed = true;
				break;
			case 0x1A:
			case 0x1B:
			case 0x1C:
			case 0x1D:
			case 0x1E:
				DrawLine(Xpos,Ymid,Xmid,Ypos2,-1,-1,0);
				displayed = true;
				break;
			case 0x1F:
			case 0x20:
			case 0x21:
			case 0x22:
			case 0x23:
				DrawLine(Xpos,Ymid,Xmid,Ypos,-1,-1,0);
				displayed = true;
				break;
			case 0x24:
			case 0x25:
			case 0x26:
			case 0x27:
			case 0x28:
				DrawLine(Xmid,Ypos,Xpos2,Ymid,-1,-1,0);
				displayed = true;
				break;
			case 0x2B:
				for (short JXQ = 0; JXQ <= Ymid-Ypos; JXQ++)
				{
					Pixel(Xpos + yoff,Ymid - JXQ,-1,-1,0);
					Pixel(Xpos2 - yoff,Ymid - JXQ,-1,-1,0);
					if (JXQ & 1) yoff++;
				}
				yoff = Xmid - (Xpos + yoff);
				for (short JXQ = 0; JXQ <= yoff; JXQ++)
				{
					Pixel(Xmid + JXQ,Ypos,-1,-1,0);
					Pixel(Xmid - JXQ,Ypos,-1,-1,0);
				}
				displayed = true;
				break;
			default:
				if (type != 0x10)
				{
					sprintf(Str_Tmp,"%02X",CheatRead<unsigned char>(CardBoard + 0x7E));
					Print_Text(Str_Tmp,2,min(max(0,Xmid-3),308),min(max(0,Ymid-5),216),ROUGE);
					Print_Text(Str_Tmp,2,min(max(2,Xmid-1),310),min(max(2,Ymid-3),218),ROUGE);
					Print_Text(Str_Tmp,2,min(max(0,Xmid-3),308),min(max(2,Ymid-3),218),ROUGE);
					Print_Text(Str_Tmp,2,min(max(2,Xmid-1),310),min(max(0,Ymid-5),216),ROUGE);
					Print_Text(Str_Tmp,2,min(max(1,Xmid-2),309),min(max(1,Ymid-4),217),VERT);
				}
				break;
		}
		if (!displayed) DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,-1,-1,0,-1);
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	//inanimate objects
	CardBoard = CheatRead<unsigned int>(0xFFCFBC);
	while (CardBoard)
	{
		if (CheatRead<unsigned char>(CardBoard + 0x7E) > 0xF);
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x2C);
			Xpos2= CheatRead<unsigned short>(CardBoard + 0x34);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x30);
			Ypos2= CheatRead<unsigned short>(CardBoard + 0x38);
			Xpos -= CamX, Xpos2 -= CamX;
			Ypos -= CamY, Ypos2 -= CamY;
			DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,0x0000FF,0x001F,1,-1);
		}
		Xpos += Xpos2;
		Ypos += Ypos2;
		Xpos >>= 1;
		Ypos >>= 1;
		sprintf(Str_Tmp,"%08X",CardBoard);
		Print_Text(Str_Tmp,8,min(max(0,Xpos-16),285),min(max(0,Ypos-5),216),VERT);
		Print_Text(Str_Tmp,8,min(max(2,Xpos-14),287),min(max(2,Ypos-3),218),VERT);
		Print_Text(Str_Tmp,8,min(max(0,Xpos-16),285),min(max(2,Ypos-3),218),VERT);
		Print_Text(Str_Tmp,8,min(max(2,Xpos-14),287),min(max(0,Ypos-5),216),VERT);
		Print_Text(Str_Tmp,8,min(max(1,Xpos-15),286),min(max(1,Ypos-4),217),BLEU);
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	//animate objects
#ifdef ECCOBOXHACK
	CardBoard = CheatRead<unsigned int>(0xFFCFB8);
#else
	CardBoard = CheatRead<unsigned int>(0xFFD828); 
#endif
	while (CardBoard)
	{
#ifdef ECCOBOXHACK
		unsigned short flags = CheatRead<unsigned short>(CardBoard + 0x10);
		//if ((flags & 0x2000) || !(flags & 2));
		unsigned int type = CheatRead<unsigned long>(CardBoard + 0xC);
		if ((type == 0xBA52E) || (type == 0xBA66E))
		{
			int Adelikat = CardBoard;
			while (Adelikat)
			{
				Xpos = CheatRead<unsigned short>(Adelikat + 0x24);
				Ypos = CheatRead<unsigned short>(Adelikat + 0x28);
				Xpos -= CamX,Ypos -= CamY;
				DrawEccoOct(Xpos,Ypos,CheatRead<unsigned short>(Adelikat + 0x44),0x00FF00,0x07E0,1,-1);
				Adelikat = CheatRead<unsigned int>(Adelikat + 4);
			}
			Xpos = CheatRead<unsigned short>(CardBoard + 0x24);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX,Ypos -= CamY;
		}
		else if (type == 0xE47EE)
		{
			int Adelikat = CardBoard;
			while (Adelikat)
			{
				Xpos = CheatRead<unsigned short>(Adelikat + 0x1C);
				Ypos = CheatRead<unsigned short>(Adelikat + 0x20);
				Xpos -= CamX,Ypos -= CamY;
				DrawEccoOct(Xpos,Ypos,(CheatRead<unsigned short>(Adelikat + 0x2C) >> 1) + 16,0x00FF00,0x07E0,1,-1);
				Adelikat = CheatRead<unsigned int>(Adelikat + 4);
			}
			Xpos = CheatRead<unsigned short>(CardBoard + 0x24);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX,Ypos -= CamY;
		}
		else if ((type == 0x9F5B0) || (type == 0xA3B18))
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x24);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX,Ypos -= CamY;
			DrawEccoOct(Xpos,Ypos,CheatRead<unsigned short>(CardBoard + 0x44),0x00FF00,0x07E0,1,-1);
		}
		else if (type == 0xDCEE0)
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x24);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX,Ypos -= CamY;
			DrawEccoOct(Xpos,Ypos,0x5C,0x00FF00,0x07E0,1,-1);
		}
		else
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x2C);
			Xpos2= CheatRead<unsigned short>(CardBoard + 0x34);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x30);
			Ypos2= CheatRead<unsigned short>(CardBoard + 0x38);
			Xmid = CheatRead<unsigned short>(CardBoard + 0x24);
			Ymid = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX, Xpos2 -= CamX;
			Ypos -= CamY, Ypos2 -= CamY;
			Xmid -= CamX, Ymid -= CamY;
			if (type == 0xA6C4A) DrawEccoOct(Xmid,Ymid,70,0x00FF00,0x07E0,1,-1);
#else
			unsigned int type = CheatRead<unsigned long>(CardBoard + 0x6);
			Xpos = CheatRead<unsigned short>(CardBoard + 0x17);
			Xpos2= CheatRead<unsigned short>(CardBoard + 0x1F);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x1B);
			Ypos2= CheatRead<unsigned short>(CardBoard + 0x23);
			Xmid = CheatRead<unsigned short>(CardBoard + 0x0F);
			Ymid = CheatRead<unsigned short>(CardBoard + 0x13);
			Xpos >>= 2, Xpos2 >>= 2;
			Ypos >>= 2, Ypos2 >>= 2;
			Xmid >>= 2, Ymid >>= 2;
			Xpos -= CamX, Xpos2 -= CamX;
			Ypos -= CamY, Ypos2 -= CamY;
			Xmid -= CamX, Ymid -= CamY;
#endif
			DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,0x00FF00,0x07E0,1,-1);
//			Xpos += Xpos2;
//			Ypos += Ypos2;
//			Xpos >>= 1;
//			Ypos >>= 1;
#ifdef ECCOBOXHACK
		}
#endif
		sprintf(Str_Tmp,"%08X",type);
		Print_Text(Str_Tmp,8,min(max(0,Xmid-16),285),min(max(0,Ymid-5),216),BLEU);
		Print_Text(Str_Tmp,8,min(max(2,Xmid-14),287),min(max(2,Ymid-3),218),BLEU);
		Print_Text(Str_Tmp,8,min(max(0,Xmid-16),285),min(max(2,Ymid-3),218),BLEU);
		Print_Text(Str_Tmp,8,min(max(2,Xmid-14),287),min(max(0,Ymid-5),216),BLEU);
		Print_Text(Str_Tmp,8,min(max(1,Xmid-15),286),min(max(1,Ymid-4),217),ROUGE);
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	//events
	CardBoard = CheatRead<unsigned int>(0xFFCFB4);
	while (CardBoard)
	{
		unsigned int type = CheatRead<unsigned int>(CardBoard + 0xC);
		if ((type == 0xA44EE) || (type == 0xD120C))
		{
			Xmid = CheatRead<unsigned short>(CardBoard + 0x1C) - CamX;
			Ymid = CheatRead<unsigned short>(CardBoard + 0x20) - CamY;
			DrawEccoOct(Xmid,Ymid,0x20,0x00FFFF,0x07FF,0,-1);
		}
		else if (type == 0xDEF94)
		{
			Xmid = CheatRead<unsigned short>(CardBoard + 0x24) - CamX;
			Ymid = CheatRead<unsigned short>(CardBoard + 0x28) - CamY;
			DrawEccoOct(Xmid,Ymid,0x18,0x00FFFF,0x07FF,1,-1);
		}
		else 
		{
			Xpos = CheatRead<unsigned short>(CardBoard + 0x2C);
			Xpos2= CheatRead<unsigned short>(CardBoard + 0x34);
			Ypos = CheatRead<unsigned short>(CardBoard + 0x30);
			Ypos2= CheatRead<unsigned short>(CardBoard + 0x38);
			Xmid = CheatRead<unsigned short>(CardBoard + 0x24);
			Ymid = CheatRead<unsigned short>(CardBoard + 0x28);
			Xpos -= CamX, Xpos2 -= CamX;
			Ypos -= CamY, Ypos2 -= CamY;
			Xmid -= CamX, Ymid -= CamY;
			DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,0x00FFFF,0x07FF,0,-1);
		}
		sprintf(Str_Tmp,"%08X",type);
		PutText(Str_Tmp,Xmid,Ymid-4,1,1,0,0,BLANC,BLEU);
		sprintf(Str_Tmp,"%08X",CardBoard);
		PutText(Str_Tmp,Xmid,Ymid+4,1,9,0,0,BLANC,BLEU);
		CardBoard = CheatRead<unsigned int>(CardBoard);
	}
	//Ecco body
	Xpos = CheatRead<unsigned short>(0xFFAA22);
	Ypos = CheatRead<unsigned short>(0xFFAA26);
	Xpos2 = CheatRead<unsigned short>(0xFFAA2A);
	Ypos2 = CheatRead<unsigned short>(0xFFAA2E);
	Xmid = CheatRead<unsigned short>(0xFFAA1A);
	Ymid = CheatRead<unsigned short>(0xFFAA1E);
	Xpos -= CamX, Xpos2 -= CamX;
	Ypos -= CamY, Ypos2 -= CamY;
	Xmid -= CamX, Ymid -= CamY;
	X = Xpos;
	X2 = Xpos2;
	Y = Ypos;
	Y2 = Ypos2;
	short X3 = (Xmid + (unsigned short) Xpos) >> 1;
	short X4 = (Xmid + (unsigned short) Xpos2) >> 1;
	short Y3 = (Ymid + (unsigned short) Ypos) >> 1;
	short Y4 = (Ymid + (unsigned short) Ypos2) >> 1;
	DrawBoxMWH(Xmid,Ymid,1,1,0xFF00FF,0xF81F,1,-1);
	DrawBoxMWH(X,Y,1,1,0xFF00FF,0xF81F,1,-1);
	DrawBoxMWH(X2,Y2,1,1,0xFF00FF,0xF81F,1,-1);
	DrawBoxMWH(X3,Y3,1,1,0xFF00FF,0xF81F,1,-1);
	DrawBoxMWH(X4,Y4,1,1,0xFF00FF,0xF81F,1,-1);
	Pixel(Xmid,Ymid,0x00FF00,0x07E0,1);
	Pixel(X,Y,0x00FF00,0x07E0,1);
	Pixel(X2,Y2,0x00FF00,0x07E0,1);
	Pixel(X3,Y3,0x00FF00,0x07E0,1);
	Pixel(X4,Y4,0x00FF00,0x07E0,1);
	DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,0x00FF00,0x07E0,1,-1);
	//Ecco head
	Xpos = CheatRead<unsigned short>(0xFFA8F8);
	Xpos2 = CheatRead<unsigned short>(0xFFA900);
	Ypos = CheatRead<unsigned short>(0xFFA8FC);
	Ypos2 = CheatRead<unsigned short>(0xFFA904);
	Xpos -= CamX, Xpos2 -= CamX;
	Ypos -= CamY, Ypos2 -= CamY;
	DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,-1,-1,-1);
	//Ecco tail
	Xpos = CheatRead<unsigned short>(0xFFA978);
	Xpos2 = CheatRead<unsigned short>(0xFFA980);
	Ypos = CheatRead<unsigned short>(0xFFA97C);
	Ypos2 = CheatRead<unsigned short>(0xFFA984);
	Xpos -= CamX, Xpos2 -= CamX;
	Ypos -= CamY, Ypos2 -= CamY;
	DrawBoxPP(Xpos,Ypos,Xpos2,Ypos2,-1,-1,-1);
	// sonar
	if (CheatRead<unsigned char>(0xFFAB77))
	{
		Xmid = CheatRead<unsigned short>(0xFFA9EC);
		Width2 = CheatRead<unsigned short>(0xFFA9FC);
		Xmid -= CamX;
		Ymid = CheatRead<unsigned short>(0xFFA9F0);
		Ymid -= CamY;
		Height2 = CheatRead<unsigned short>(0xFFAA00);
		color32 = (CheatRead<unsigned char>(0xFFAA0C) ? 0xFF007F : 0x0000FF);
		color16 = (CheatRead<unsigned char>(0xFFAA0C) ? 0xF80F : 0x001F);
		DrawBoxMWH(Xmid,Ymid,Width2,Height2,color32,color16,1,-1);
		DrawBoxMWH(Xmid,Ymid,1,1,0x00FF00,0x07E0,1,-1);
		Pixel(Xmid,Ymid,color32,color16,1);
	}
	//Pulsar
	CardBoard = CheatRead<unsigned int>(0xFFCFD0);
	if (CardBoard)
	{
//		char Blah = CheatRead<unsigned char>(CardBoard + 0x15);
		CardBoard += 0x26;
//		if (!(Blah & 1))
//			CardBoard += 0x14;
		for (int i = 0; i < 4; i ++)
		{
			if (CheatRead<unsigned short>(CardBoard + 0x12))
			{
				Xmid = CheatRead<unsigned short>(CardBoard);
				Ymid = CheatRead<unsigned short>(CardBoard + 4);
				Xmid -= CamX, Ymid -= CamY;
				DrawBoxMWH(Xmid,Ymid,0x30,0x30,0xFF0000,0xF800,1,-1);
				DrawBoxMWH(Xmid,Ymid,1,1,0x0000FF,0x001F,1,-1);
				Pixel(Xmid,Ymid,0xFF0000,0xF800,1);
			}
			CardBoard += 0x14;
		}
	}
}
void EccoAutofire ()
{
	//Modif N - ECCO HACK - make caps lock (weirdly) autofire player 1's C key
	static int lastCap = 0, temp = 0, nowCap;
	static unsigned char mode = 0, prevCharge = 0;
	unsigned char charge;
	nowCap = GetKeyState(VK_CAPITAL) != 0;
	if(lastCap != nowCap)
	{
		temp = FrameCount - LagCount - 1;
	}
	if (Ram_68k[0xA554] != mode)
	{
		temp = FrameCount - LagCount;
		mode = Ram_68k[0xA554];
	}
	switch (mode)
	{
		case 0xE6:
			if ((* (unsigned int *) &(Ram_68k[0xD5E8])) == 0x00000002)
				Controller_1_B = Controller_1_C = 0;
			else
				Controller_1_B = Controller_1_C = 1;
			break;
		case 0xF6:
			charge = Ram_68k[0xB19A];
			if (nowCap)
			{
				if ((charge == 1) || (prevCharge == 1) || !(lastCap || Ram_68k[0xB19A]))
					Controller_1_B = 0;
				else 
					Controller_1_B = 1;
				if (((*(unsigned short *) &(Ram_68k[0xB168])) == 0x3800) && (((FrameCount - LagCount) - temp) % 2))
					LagCount++;
			}
			prevCharge = charge;
			if (((FrameCount - LagCount) - temp) % 2)
				Controller_1_C = 1;
			else 
				Controller_1_C = 0;
			break;
		case 0x20:
		case 0x28:
		case 0xAC:
			if(nowCap && ((FrameCount - LagCount) > temp))
			{
				if(Controller_1_C && (((FrameCount - LagCount) - temp) % 12))
					Controller_1_C = 0;
				else if(!Controller_1_C && ((FrameCount - LagCount) % 2))
					Controller_1_C = 1;
			}
			break;
		default:
			break;
	}
	lastCap = nowCap;
}
#endif