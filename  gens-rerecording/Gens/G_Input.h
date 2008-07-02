#ifndef G_INPUT_H
#define G_INPUT_H

#define DIRECTINPUT_VERSION 0x0500  // for joystick support

#include <dinput.h>
//#include <mmsystem.h>

struct K_Def {
	unsigned int Start, Mode;
	unsigned int A, B, C;
	unsigned int X, Y, Z;
	unsigned int Up, Down, Left, Right;
	};

extern struct K_Def Keys_Def[8];
extern unsigned char Kaillera_Keys[16];
extern unsigned int DelayFactor; // determines the timing of "continuous frame advance"

int String_Size(char *Chaine);
unsigned int Get_Key(void);
int Check_Key_Pressed(unsigned int key);
int Setting_Quickload_Key(HWND hset);	//Modif
int Setting_Quicksave_Key(HWND hset);	//Modif
int Setting_Quickpause_Key(HWND hset);	//Modif
int Setting_Autofire_Key(HWND hset);	//Modif
int Setting_Autohold_Key(HWND hset);	//Modif
int Setting_Autoclear_Key(HWND hset);	//Modif
int Setting_Slow_Key(HWND hset);	//Modif
int Check_Skip_Key(); // Modif
int Setting_Skip_Key(HWND hset);	//Modif
int Setting_Keys(HWND hset, int Player, int Type);
int Init_Input(HINSTANCE hInst, HWND hWnd);
void End_Input(void);
void Update_Input(void);
void Update_Controllers(void);
int Check_Pause_Key(void);	//Modif
void Check_Misc_Key(); //Modif
void Scan_Player_Net(int Player);
void Update_Controllers_Net(int num_player);


#endif