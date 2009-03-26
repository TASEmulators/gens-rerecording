#ifndef VDP_REND_H
#define VDP_REND_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int MD_Screen32[336 * 240];
extern unsigned short MD_Screen[336 * 240];
extern unsigned int MD_Palette32[256];
extern unsigned short MD_Palette[256];
extern unsigned int Palette32[0x1000];
extern unsigned short Palette[0x1000];
extern unsigned long TAB336[336];
extern unsigned char Bits32;

extern struct
{
	int Pos_X;
	int Pos_Y;
	unsigned int Size_X;
	unsigned int Size_Y;
	int Pos_X_Max;
	int Pos_Y_Max;
	unsigned int Num_Tile;
	int dirt;
} Sprite_Struct[256];

extern unsigned char _32X_Rend_Mode;
extern int Mode_555;
extern int Sprite_Over;
extern char VScrollAl; //Nitsuja added this
extern char VScrollAh; //Nitsuja added this 
extern char VScrollBl; //Nitsuja added this
extern char VScrollBh; //Nitsuja added this
extern char VSpritel; //Upthorn added this
extern char VSpriteh; //Upthorn added this
extern char ScrollAOn;
extern char ScrollBOn;
extern char SpriteOn;
extern char Sprite_Always_Top;
extern char Swap_Scroll_PriorityA;
extern char Swap_Scroll_PriorityB;
extern char Swap_Sprite_Priority;
extern char PalLock;


void Render_Line();
void Post_Line();
void Render_Line_32X();

#ifdef __cplusplus
};
#endif

#endif