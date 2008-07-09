#include <stdio.h>
#include <windows.h>
#include "guidraw.h"
#include "RKABoxHack.h"
#include "HacksCommon.h"
#include "misc.h"
#include "g_main.h"
#ifdef RKABOXHACK
void RKADrawBoxes()
{
	for (unsigned short CardBoard = POSOFFSET; CardBoard < POSOFFSET + SSTLEN; CardBoard += SPRITESIZE)
	{
		if (!(CheatRead<unsigned short>(0xFF0000 | CardBoard)))
			continue;
		short Xpos = CheatRead<short>(0xFF0000 | CardBoard + XPo); // too many many lines which were this way already when we discovered
		short Ypos = CheatRead<short>(0xFF0000 | CardBoard + YPo); // that Ram_68k is little endian to begin with
		unsigned short Height2 = CheatRead<short>(0xFF0000 | CardBoard + Ho);
		unsigned short Width2 = CheatRead<short>(0xFF0000 | CardBoard + Wo);
		if (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x09)
		{
			Height2 = 0x20;
			Width2 = 0x10;
		}
		else if ((CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x33) || (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x64) 
			  || (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x66) || (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x68))
		{
			Height2 = 0x10;
			Width2 = 0x8;
		}

//		Xpos += 8;	
//		Ypos -= CheatRead<short>(0xFF0000 | CardBoard + YVo);
//		Xpos -= CheatRead<short>(0xFF0000 | CardBoard + XVo);
		if (!GetKeyState(VK_SCROLL))
		{
			Xpos -= CamX;
			Ypos -= CamY;
		}
//		Height2 /= 2;
//		Ypos -= Height2;
//		Height2 = min(Height2,0xFE);
//		Width2 = min(Width2,0xFE);
		DrawBoxCWH(Xpos - Width2,Ypos - Height2,Width2 * 2, Height2, -1, -1, -1,-1,1);
/*		for (unsigned char JXQ = 0; JXQ <= Width2; JXQ++)
		{
			int x1 = Xpos - JXQ, x2 = Xpos + JXQ;
			int y1 = Ypos - Height2, y2 = Ypos + Height2;
			while (x1 < 8) x1 += 320;
			while (x1 > 327) x1 -= 320;
			while (x2 < 8) x2 += 320;
			while (x2 > 327) x2 -= 320;
			while (y1 < 0) y1 += 224;
			while (y1 > 223) y1 -= 224;
			while (y2 < 0) y2 += 224;
			while (y2 > 223) y2 -= 224;
			MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFFFF;
		}
		for (unsigned char JXQ = 0; JXQ <= Height2; JXQ++)
		{
			int x1 = Xpos - Width2, x2 = Xpos + Width2;
			int y1 = Ypos - JXQ, y2 = Ypos + JXQ;
			while (x1 < 8) x1 += 320;
			while (x1 > 327) x1 -= 320;
			while (x2 < 8) x2 += 320;
			while (x2 > 327) x2 -= 320;
			while (y1 < 0) y1 += 224;
			while (y1 > 223) y1 -= 224;
			while (y2 < 0) y2 += 224;
			while (y2 > 223) y2 -= 224;
			MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFFFF;
			MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFFFFFF;
			MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFFFF;
		}*/
//		Ypos += Height2;
		DrawBoxMWH(Xpos,Ypos,2,2,0x00FF00,0x07E0,0);
		DrawBoxMWH(Xpos,Ypos,1,1,0x00FF00,0x07E0,0);
		DrawLine(Xpos-1,Ypos,Xpos+1,Ypos,0,0,0);
		DrawLine(Xpos,Ypos-1,Xpos,Ypos+1,0,0,0);
/*		for (char jxq = 0; jxq <=2; jxq++)
		{
			for (char Jxq = 0; Jxq <=2; Jxq++)
			{
				MD_Screen32[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x00FF00;
				MD_Screen32[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x00FF00;
				MD_Screen32[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x00FF00;
				MD_Screen32[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x00FF00;
				MD_Screen[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x07E0;
				MD_Screen[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos - Jxq)))] =0x07E0;
				MD_Screen[max(8,min(327,(Xpos - jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x07E0;
				MD_Screen[max(8,min(327,(Xpos + jxq))) + 336 * max(0,min(223,(Ypos + Jxq)))] =0x07E0;
			}
		}
		MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen32[max(8,min(327,(Xpos - 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen32[max(8,min(327,(Xpos + 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
		MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos + 1)))] = 0;
		MD_Screen32[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
		MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen[max(8,min(327,(Xpos - 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen[max(8,min(327,(Xpos + 1))) + 336 * max(0,min(223,(Ypos)))] = 0;
		MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;
		MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos + 1)))] = 0;
		MD_Screen[max(8,min(327,(Xpos))) + 336 * max(0,min(223,(Ypos - 1)))] = 0;*/
		int HP = (int) CheatRead<short>(0xFF0000 | CardBoard + 0x40);
		if (HP + 1 == 0x8000) HP = 0;
		sprintf(Str_Tmp,"%d", HP);
		if (HP)
		{
			PutText(Str_Tmp,Xpos,Ypos - (Height2 >> 1),0,0,319,223,BLANC,BLEU);
/*			short xpos = Xpos - 8 - ((5 * (strlen(Str_Tmp) - 1))/2);
			xpos = min(max(xpos,1),318 - 4 * strlen(Str_Tmp));
			short ypos = min(max(Ypos - Height2 - 3,1),217);
			const static int xOffset [] = {-1,-1,-1,0,1,1,1,0};
			const static int yOffset [] = {-1,0,1,1,1,0,-1,-1};
			for(int i = 0 ; i < 8 ; i++)
				Print_Text(Str_Tmp,strlen(Str_Tmp),xpos + xOffset[i],ypos + yOffset[i],0);
			Print_Text(Str_Tmp,strlen(Str_Tmp),xpos,ypos,2);*/
		}	
		if ((CheatRead<short>(0xFF0000 | CardBoard) == 0x01) || (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x09))
		{
			bool attack = false;
			bool EU = (CPU_Mode || (!Game_Mode));
			if ((CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x04) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x17) 
			 || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x13) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x18) 
			 || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x20) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x21) 
			 || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x06) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x07)
			 || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x08) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x09)
			 || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x0A) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x0B))	
			{
				attack = true;
				unsigned int Blah = *(unsigned short *) &(Rom_Data[0xFAB6 + (EU * 0xC0) + (CheatRead<short>(0xFF0000 | CardBoard + 0x60) & 7) * 4]);
				if (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x06)
					Blah = 0xFB34 + (EU * 0xC0);
				else if (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x07)
					Blah = 0xFB3C + (EU * 0xC0);
				else if ((CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x08) || (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x0B))
					Blah = 0xFB44 + (EU * 0xC0);
				else if (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x09)
					Blah = 0xFB4C + (EU * 0xC0);
				else if (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x0A)
					Blah = 0xFB54 + (EU * 0xC0);
				Width2 = * (short *) &(Rom_Data[Blah + 4]);
				Width2 >>= 1;
				Height2 = * (short *) &(Rom_Data[Blah + 6]);
				Width2 = min(0xFE,Width2);
				Height2 = min(0xFE,Height2);
				if (CheatRead<short>(0xFF0000 | CardBoard + 2) == 0x18)
				{
					Ypos -= Height2;
					Ypos -= * (short *) &(Rom_Data[Blah + 2]);
				}
				else
				{
					Ypos += Height2;
					Ypos += * (short *) &(Rom_Data[Blah + 2]);
				}
				if (CheatRead<short>(0xFF0000 | CardBoard + 0x1A))
				{
					Xpos -= * (short *) &(Rom_Data[Blah]);
					Xpos -= Width2;
				}
				else
				{
					Xpos += * (short *) &(Rom_Data[Blah]);
					Xpos += Width2;
				}
			}
			else if (CheatRead<unsigned short>(0xFF0000 | CardBoard) == 0x09)
			{
				Height2 = 0x20;
				Width2 = 0x10;
				attack = true;
			}
			if (attack)
			{
				DrawBoxCWH(Xpos - Width2,Ypos - Height2,Width2 * 2, Height2, 0xFF0000, 0xF800, -1,-1,1);
/*				Height2 >>= 1;
				Ypos -= Height2;
				Width2 = min(Width2,0xFE);
				for (unsigned char JXQ = 0; JXQ <= Width2; JXQ++)
				{
					int x1 = Xpos - JXQ, x2 = Xpos + JXQ;
					int y1 = Ypos - Height2, y2 = Ypos + Height2;
					while (x1 < 8) x1 += 320;
					while (x1 > 327) x1 -= 320;
					while (x2 < 8) x2 += 320;
					while (x2 > 327) x2 -= 320;
					while (y1 < 0) y1 += 224;
					while (y1 > 223) y1 -= 224;
					while (y2 < 0) y2 += 224;
					while (y2 > 223) y2 -= 224;
					MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFF0000;
					MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xF800;
					MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xF800;
					MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xF800;
					MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xF800;
				}
				for (unsigned char JXQ = 0; JXQ <= Height2; JXQ++)
				{
					int x1 = Xpos - Width2, x2 = Xpos + Width2;
					int y1 = Ypos - JXQ, y2 = Ypos + JXQ;
					while (x1 < 8) x1 += 320;
					while (x1 > 327) x1 -= 320;
					while (x2 < 8) x2 += 320;
					while (x2 > 327) x2 -= 320;
					while (y1 < 0) y1 += 224;
					while (y1 > 223) y1 -= 224;
					while (y2 < 0) y2 += 224;
					while (y2 > 223) y2 -= 224;
					MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xFF0000;
					MD_Screen32[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xFF0000;
					MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y1)))] = 0xF800;
					MD_Screen[max(8,min(327,x1)) + (336 * max(0,min(223,y2)))] = 0xF800;
					MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y1)))] = 0xF800;
					MD_Screen[max(8,min(327,x2)) + (336 * max(0,min(223,y2)))] = 0xF800;
				}*/
			}
		}
	}
}
#endif