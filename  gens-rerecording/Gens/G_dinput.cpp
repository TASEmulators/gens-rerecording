#include <stdio.h>
#include "resource.h"
#include "G_Input.h"
#include "io.h"
#include "G_main.h"
#include "movie.h"
#include "save.h" //Modif
#include "hackdefs.h"

#define KEYDOWN(key) (Keys[key] & 0x80) 
#define MAX_JOYS 8

LPDIRECTINPUT lpDI;
LPDIRECTINPUTDEVICE lpDIDKeyboard;
LPDIRECTINPUTDEVICE lpDIDMouse;
char Phrase[1024];
int Nb_Joys = 0;
static IDirectInputDevice2 *Joy_ID[MAX_JOYS] = {NULL};
static DIJOYSTATE Joy_State[MAX_JOYS] = {{0}};
long MouseX, MouseY;
unsigned char Keys[256];
unsigned char Kaillera_Keys[16];
int Cur_Player; //Upth-Add - For the new key-redefinition dialogs
unsigned int DelayFactor = 5;


struct K_Def Keys_Def[8] = {
	{DIK_RETURN, DIK_RSHIFT,
	DIK_A, DIK_S, DIK_D,
	DIK_Z, DIK_X, DIK_C,
	DIK_UP, DIK_DOWN, DIK_LEFT, DIK_RIGHT},
	{DIK_U, DIK_T,
	DIK_K, DIK_L, DIK_M,
	DIK_I, DIK_O, DIK_P,
	DIK_Y, DIK_H, DIK_G, DIK_J}
};
K_Def Temp_Keys; //Upth-Add - Stores the key definitions so that the user can cancel redefinition without changing them
static bool autoAlternator = false; //Nitsuja did this part, it's part of his AutoHold and AutoFire mod
#define MAKE_AUTO_KEY_VAR(x) unsigned int x##_Last = 1; bool x##_Just = 0; bool x##_Autofire = 0; bool x##_Autofire2 = 0; bool x##_Autohold = 0;
MAKE_AUTO_KEY_VAR(Controller_1_Up);
MAKE_AUTO_KEY_VAR(Controller_1_Down);
MAKE_AUTO_KEY_VAR(Controller_1_Left);
MAKE_AUTO_KEY_VAR(Controller_1_Right);
MAKE_AUTO_KEY_VAR(Controller_1_Start);
MAKE_AUTO_KEY_VAR(Controller_1_Mode);
MAKE_AUTO_KEY_VAR(Controller_1_A);
MAKE_AUTO_KEY_VAR(Controller_1_B);
MAKE_AUTO_KEY_VAR(Controller_1_C);
MAKE_AUTO_KEY_VAR(Controller_1_X);
MAKE_AUTO_KEY_VAR(Controller_1_Y);
MAKE_AUTO_KEY_VAR(Controller_1_Z);
MAKE_AUTO_KEY_VAR(Controller_2_Up);
MAKE_AUTO_KEY_VAR(Controller_2_Down);
MAKE_AUTO_KEY_VAR(Controller_2_Left);
MAKE_AUTO_KEY_VAR(Controller_2_Right);
MAKE_AUTO_KEY_VAR(Controller_2_Start);
MAKE_AUTO_KEY_VAR(Controller_2_Mode);
MAKE_AUTO_KEY_VAR(Controller_2_A);
MAKE_AUTO_KEY_VAR(Controller_2_B);
MAKE_AUTO_KEY_VAR(Controller_2_C);
MAKE_AUTO_KEY_VAR(Controller_2_X);
MAKE_AUTO_KEY_VAR(Controller_2_Y);
MAKE_AUTO_KEY_VAR(Controller_2_Z);


int String_Size(char *Chaine)
{
	int i = 0;

	while (*(Chaine + i++));

	return(i - 1);	
}


void End_Input()
{
	int i;
	
	if (lpDI)
	{
		if(lpDIDMouse)
		{
			lpDIDMouse->Release();
			lpDIDMouse = NULL;
		}

		if(lpDIDKeyboard)
		{
			lpDIDKeyboard->Release();
			lpDIDKeyboard = NULL;
		}

		for(i = 0; i < MAX_JOYS; i++)
		{
			if (Joy_ID[i])
			{
				Joy_ID[i]->Unacquire();
				Joy_ID[i]->Release();
			}
		}

		Nb_Joys = 0;
		lpDI->Release();
		lpDI = NULL;
	}
}


BOOL CALLBACK InitJoystick(LPCDIDEVICEINSTANCE lpDIIJoy, LPVOID pvRef)
{
	HRESULT rval;
	LPDIRECTINPUTDEVICE	lpDIJoy;
	DIPROPRANGE diprg;
	int i;
 
	if (Nb_Joys >= MAX_JOYS) return(DIENUM_STOP);
		
	Joy_ID[Nb_Joys] = NULL;

	rval = lpDI->CreateDevice(lpDIIJoy->guidInstance, &lpDIJoy, NULL);
	if (rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInput::CreateDevice FAILED", "erreur joystick", MB_OK);
		return(DIENUM_CONTINUE);
	}

	rval = lpDIJoy->QueryInterface(IID_IDirectInputDevice2, (void **)&Joy_ID[Nb_Joys]);
	lpDIJoy->Release();
	if (rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInputDevice2::QueryInterface FAILED", "erreur joystick", MB_OK);
	    Joy_ID[Nb_Joys] = NULL;
	    return(DIENUM_CONTINUE);
	}

	rval = Joy_ID[Nb_Joys]->SetDataFormat(&c_dfDIJoystick);
	if (rval != DI_OK)
	{
		MessageBox(HWnd, "IDirectInputDevice::SetDataFormat FAILED", "erreur joystick", MB_OK);
		Joy_ID[Nb_Joys]->Release();
		Joy_ID[Nb_Joys] = NULL;
		return(DIENUM_CONTINUE);
	}

	rval = Joy_ID[Nb_Joys]->SetCooperativeLevel((HWND)pvRef, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

	if (rval != DI_OK)
	{ 
		MessageBox(HWnd, "IDirectInputDevice::SetCooperativeLevel FAILED", "erreur joystick", MB_OK);
		Joy_ID[Nb_Joys]->Release();
		Joy_ID[Nb_Joys] = NULL;
		return(DIENUM_CONTINUE);
	}
 
	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_X;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = Joy_ID[Nb_Joys]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT)) 
		MessageBox(HWnd, "IDirectInputDevice::SetProperty() (X-Axis) FAILED", "erreur joystick", MB_OK);

	diprg.diph.dwSize = sizeof(diprg); 
	diprg.diph.dwHeaderSize = sizeof(diprg.diph); 
	diprg.diph.dwObj = DIJOFS_Y;
	diprg.diph.dwHow = DIPH_BYOFFSET;
	diprg.lMin = -1000; 
	diprg.lMax = +1000;
 
	rval = Joy_ID[Nb_Joys]->SetProperty(DIPROP_RANGE, &diprg.diph);
	if ((rval != DI_OK) && (rval != DI_PROPNOEFFECT)) 
		MessageBox(HWnd, "IDirectInputDevice::SetProperty() (Y-Axis) FAILED", "erreur joystick", MB_OK);

	for(i = 0; i < 10; i++)
	{
		rval = Joy_ID[Nb_Joys]->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	Nb_Joys++;

	return(DIENUM_CONTINUE);
}


int Init_Input(HINSTANCE hInst, HWND hWnd)
{
	int i;
	HRESULT rval;

	End_Input();
	
	rval = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpDI, NULL);
	if (rval != DI_OK)
	{
		MessageBox(hWnd, "DirectInput failed ...You must have DirectX 5", "Error", MB_OK);
		return 0;
	}
	
	Nb_Joys = 0;

	for(i = 0; i < MAX_JOYS; i++) Joy_ID[i] = NULL;

	rval = lpDI->EnumDevices(DIDEVTYPE_JOYSTICK, &InitJoystick, hWnd, DIEDFL_ATTACHEDONLY);
	if (rval != DI_OK) return 0;

//	rval = lpDI->CreateDevice(GUID_SysMouse, &lpDIDMouse, NULL);
	rval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDKeyboard, NULL);
	if (rval != DI_OK) return 0;

//	rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	rval = lpDIDKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (rval != DI_OK) return 0;

//	rval = lpDIDMouse->SetDataFormat(&c_dfDIMouse);
	rval = lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if (rval != DI_OK) return 0;

//	rval = lpDIDMouse->Acquire();
	for(i = 0; i < 10; i++)
	{
		rval = lpDIDKeyboard->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	return 1;
}


void Restore_Input()
{
//	lpDIDMouse->Acquire();
	lpDIDKeyboard->Acquire();
}


void Update_Input()
{
//	DIMOUSESTATE MouseState;
	HRESULT rval;
	int i;

	rval = lpDIDKeyboard->GetDeviceState(256, &Keys);

	// HACK because DirectInput is totally wacky about recognizing the PAUSE/BREAK key
	// still not perfect with this, but at least it goes above a 25% success rate
	if(GetAsyncKeyState(VK_PAUSE)) // normally this should have & 0x8000, but apparently this key is too special for that to work
		Keys[0xC5] |= 0x80;

	if ((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
		Restore_Input();

	for (i = 0; i < Nb_Joys; i++)
	{
		if (Joy_ID[i])
		{
			Joy_ID[i]->Poll();
			rval = Joy_ID[i]->GetDeviceState(sizeof(Joy_State[i]), &Joy_State[i]);
			if (rval != DI_OK) Joy_ID[i]->Acquire();
		}
	}

//	rval = lpDIDMouse->GetDeviceState(sizeof(MouseState), &MouseState);
	
//	if ((rval == DIERR_INPUTLOST) | (rval == DIERR_NOTACQUIRED))
//		Restore_Input();

//  MouseX = MouseState.lX;
//  MouseY = MouseState.lY;
}


int Check_Key_Pressed(unsigned int key)
{
	int Num_Joy;

	if (key < 0x100)
	{
		if KEYDOWN(key) return(1);
	}
	else
	{
		Num_Joy = ((key >> 8) & 0xF);

		if (Joy_ID[Num_Joy])
		{
			if (key & 0x80)			// Test POV Joys
			{
				int value = Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3];
				if (value == -1) return (0);
				switch(key & 0xF)
				{
					case 1:
						if ((value >= 29250) || (value <=  6750)) return(1); break;
					case 2:
						if ((value >=  2250) && (value <= 15750)) return(1); break;
					case 3:
						if ((value >= 11250) && (value <= 24750)) return(1); break;
					case 4:
						if ((value >= 20250) && (value <= 33750)) return(1); break;
				}

			}
			else if (key & 0x70)		// Test Button Joys
			{
				if (Joy_State[Num_Joy].rgbButtons[(key & 0xFF) - 0x10]) return(1);
			}
			else
			{
				switch(key & 0xF)
				{
					case 1:
						if (Joy_State[Num_Joy].lY < -500) return(1); break;

					case 2:
						if (Joy_State[Num_Joy].lY > +500) return(1); break;

					case 3:
						if (Joy_State[Num_Joy].lX < -500) return(1); break;

					case 4:
						if (Joy_State[Num_Joy].lX > +500) return(1); break;
				}
			}
		}
	}

	return 0;
}


unsigned int Get_Key(void)
{
	int i, j;

	while(1)
	{
		Update_Input();

		for(i = 1; i < 256; i++)
			if KEYDOWN(i) return i;

		for(i = 0; i < Nb_Joys; i++)
		{
			if (Joy_ID[i])
			{
				if (Joy_State[i].lY < -500)
					return(0x1000 + (0x100 * i) + 0x1);

				if (Joy_State[i].lY > +500)
					return(0x1000 + (0x100 * i) + 0x2);

				if (Joy_State[i].lX < -500)
					return(0x1000 + (0x100 * i) + 0x3);

				if (Joy_State[i].lX > +500)
					return(0x1000 + (0x100 * i) + 0x4);

				for (j = 0; j < 4; j++)
					if (Joy_State[i].rgdwPOV[j] == 0)
						return(0x1080 + (0x100 * i) + (0x10 * j) + 0x1);

				for (j = 0; j < 4; j++)
					if (Joy_State[i].rgdwPOV[j] == 9000)
						return(0x1080 + (0x100 * i) + (0x10 * j) + 0x2);

				for (j = 0; j < 4; j++)
					if (Joy_State[i].rgdwPOV[j] == 18000)
						return(0x1080 + (0x100 * i) + (0x10 * j) + 0x3);

				for (j = 0; j < 4; j++)
					if (Joy_State[i].rgdwPOV[j] == 27000)
						return(0x1080 + (0x100 * i) + (0x10 * j) + 0x4);

				for (j = 0; j < 32; j++)
					if (Joy_State[i].rgbButtons[j])
						return(0x1010 + (0x100 * i) + j);
			}
		}
	}
}

#ifdef ECCOBOXHACK
#include "EccoBoxHack.h"
#endif
void Update_Controllers()
{

	Update_Input();

	if (Check_Key_Pressed(Keys_Def[0].Up))
	{
		Controller_1_Up = 0;
		if(LeftRightEnabled==0) Controller_1_Down = 1;
		else {if(Check_Key_Pressed(Keys_Def[0].Down)) Controller_1_Down = 0;
		else Controller_1_Down = 1;}
	}
	else
	{
		Controller_1_Up = 1;
		if (Check_Key_Pressed(Keys_Def[0].Down)) Controller_1_Down = 0;
		else Controller_1_Down = 1;
	}
	
	if (Check_Key_Pressed(Keys_Def[0].Left))
	{
		Controller_1_Left = 0;
		if(LeftRightEnabled==0) Controller_1_Right = 1;
		else {if(Check_Key_Pressed(Keys_Def[0].Right)) Controller_1_Right = 0;
		else Controller_1_Right = 1;}
	}
	else
	{
		Controller_1_Left = 1;
		if (Check_Key_Pressed(Keys_Def[0].Right)) Controller_1_Right = 0;
		else Controller_1_Right = 1;
	}

	if (Check_Key_Pressed(Keys_Def[0].Start)) Controller_1_Start = 0;
	else Controller_1_Start = 1;

	if (Check_Key_Pressed(Keys_Def[0].A)) Controller_1_A = 0;
	else Controller_1_A = 1;

	if (Check_Key_Pressed(Keys_Def[0].B)) Controller_1_B = 0;
	else Controller_1_B = 1;

	if (Check_Key_Pressed(Keys_Def[0].C)) Controller_1_C = 0;
	else Controller_1_C = 1;

	if (Controller_1_Type & 1)
	{
		if (Check_Key_Pressed(Keys_Def[0].Mode)) Controller_1_Mode = 0;
		else Controller_1_Mode = 1;

		if (Check_Key_Pressed(Keys_Def[0].X)) Controller_1_X = 0;
		else Controller_1_X = 1;

		if (Check_Key_Pressed(Keys_Def[0].Y)) Controller_1_Y = 0;
		else Controller_1_Y = 1;

		if (Check_Key_Pressed(Keys_Def[0].Z)) Controller_1_Z = 0;
		else Controller_1_Z = 1;
	}

	if (Check_Key_Pressed(Keys_Def[1].Up))
	{
		Controller_2_Up = 0;
		if(LeftRightEnabled==0) Controller_2_Down = 1;
		else {if(Check_Key_Pressed(Keys_Def[1].Down)) Controller_2_Down = 0;
		else Controller_1_Down = 1;}
	}
	else
	{
		Controller_2_Up = 1;
		if (Check_Key_Pressed(Keys_Def[1].Down)) Controller_2_Down = 0;
		else Controller_2_Down = 1;
	}

	
	if (Check_Key_Pressed(Keys_Def[1].Left))
	{
		Controller_2_Left = 0;
		if(LeftRightEnabled==0) Controller_2_Right = 1;
		else {if(Check_Key_Pressed(Keys_Def[1].Right)) Controller_2_Right = 0;
		else Controller_2_Right = 1;}
	}
	else
	{
		Controller_2_Left = 1;
		if (Check_Key_Pressed(Keys_Def[1].Right)) Controller_2_Right = 0;
		else Controller_2_Right = 1;
	}

	if (Check_Key_Pressed(Keys_Def[1].Start)) Controller_2_Start = 0;
	else Controller_2_Start = 1;

	if (Check_Key_Pressed(Keys_Def[1].A)) Controller_2_A = 0;
	else Controller_2_A = 1;

	if (Check_Key_Pressed(Keys_Def[1].B)) Controller_2_B = 0;
	else Controller_2_B = 1;

	if (Check_Key_Pressed(Keys_Def[1].C)) Controller_2_C = 0;
	else Controller_2_C = 1;

	if (Controller_2_Type & 1)
	{
		if (Check_Key_Pressed(Keys_Def[1].Mode)) Controller_2_Mode = 0;
		else Controller_2_Mode = 1;

		if (Check_Key_Pressed(Keys_Def[1].X)) Controller_2_X = 0;
		else Controller_2_X = 1;

		if (Check_Key_Pressed(Keys_Def[1].Y)) Controller_2_Y = 0;
		else Controller_2_Y = 1;

		if (Check_Key_Pressed(Keys_Def[1].Z)) Controller_2_Z = 0;
		else Controller_2_Z = 1;
	}

	if (Controller_1_Type & 0x10)			// TEAMPLAYER PORT 1
	{
		if (Check_Key_Pressed(Keys_Def[2].Up))
		{
			Controller_1B_Up = 0;
			if(LeftRightEnabled==0) Controller_1B_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[2].Down)) Controller_1B_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_1B_Up = 1;
			if (Check_Key_Pressed(Keys_Def[2].Down)) Controller_1B_Down = 0;
			else Controller_1B_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[2].Left))
		{
			Controller_1B_Left = 0;
			if(LeftRightEnabled==0) Controller_1B_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[2].Right)) Controller_1B_Right = 0;
			else Controller_1B_Right = 1;}
		}
		else
		{
			Controller_1B_Left = 1;
			if (Check_Key_Pressed(Keys_Def[2].Right)) Controller_1B_Right = 0;
			else Controller_1B_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[2].Start)) Controller_1B_Start = 0;
		else Controller_1B_Start = 1;

		if (Check_Key_Pressed(Keys_Def[2].A)) Controller_1B_A = 0;
		else Controller_1B_A = 1;

		if (Check_Key_Pressed(Keys_Def[2].B)) Controller_1B_B = 0;
		else Controller_1B_B = 1;

		if (Check_Key_Pressed(Keys_Def[2].C)) Controller_1B_C = 0;
		else Controller_1B_C = 1;

		if (Controller_1B_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[2].Mode)) Controller_1B_Mode = 0;
			else Controller_1B_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[2].X)) Controller_1B_X = 0;
			else Controller_1B_X = 1;

			if (Check_Key_Pressed(Keys_Def[2].Y)) Controller_1B_Y = 0;
			else Controller_1B_Y = 1;

			if (Check_Key_Pressed(Keys_Def[2].Z)) Controller_1B_Z = 0;
			else Controller_1B_Z = 1;
		}

		if (Check_Key_Pressed(Keys_Def[3].Up))
		{
			Controller_1C_Up = 0;
			if(LeftRightEnabled==0) Controller_1C_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[3].Down)) Controller_1C_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_1C_Up = 1;
			if (Check_Key_Pressed(Keys_Def[3].Down)) Controller_1C_Down = 0;
			else Controller_1C_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[3].Left))
		{
			Controller_1C_Left = 0;
			if(LeftRightEnabled==0) Controller_1C_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[3].Right)) Controller_1C_Right = 0;
			else Controller_1C_Right = 1;}
		}
		else
		{
			Controller_1C_Left = 1;
			if (Check_Key_Pressed(Keys_Def[3].Right)) Controller_1C_Right = 0;
			else Controller_1C_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[3].Start)) Controller_1C_Start = 0;
		else Controller_1C_Start = 1;

		if (Check_Key_Pressed(Keys_Def[3].A)) Controller_1C_A = 0;
		else Controller_1C_A = 1;

		if (Check_Key_Pressed(Keys_Def[3].B)) Controller_1C_B = 0;
		else Controller_1C_B = 1;

		if (Check_Key_Pressed(Keys_Def[3].C)) Controller_1C_C = 0;
		else Controller_1C_C = 1;

		if (Controller_1C_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[3].Mode)) Controller_1C_Mode = 0;
			else Controller_1C_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[3].X)) Controller_1C_X = 0;
			else Controller_1C_X = 1;

			if (Check_Key_Pressed(Keys_Def[3].Y)) Controller_1C_Y = 0;
			else Controller_1C_Y = 1;

			if (Check_Key_Pressed(Keys_Def[3].Z)) Controller_1C_Z = 0;
			else Controller_1C_Z = 1;
		}

		if (Check_Key_Pressed(Keys_Def[4].Up))
		{
			Controller_1D_Up = 0;
			if(LeftRightEnabled==0) Controller_1D_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[4].Down)) Controller_1D_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_1D_Up = 1;
			if (Check_Key_Pressed(Keys_Def[4].Down)) Controller_1D_Down = 0;
			else Controller_1D_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[4].Left))
		{
			Controller_1D_Left = 0;
			if(LeftRightEnabled==0) Controller_1D_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[4].Right)) Controller_1D_Right = 0;
			else Controller_1D_Right = 1;}
		}
		else
		{
			Controller_1D_Left = 1;
			if (Check_Key_Pressed(Keys_Def[4].Right)) Controller_1D_Right = 0;
			else Controller_1D_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[4].Start)) Controller_1D_Start = 0;
		else Controller_1D_Start = 1;

		if (Check_Key_Pressed(Keys_Def[4].A)) Controller_1D_A = 0;
		else Controller_1D_A = 1;

		if (Check_Key_Pressed(Keys_Def[4].B)) Controller_1D_B = 0;
		else Controller_1D_B = 1;

		if (Check_Key_Pressed(Keys_Def[4].C)) Controller_1D_C = 0;
		else Controller_1D_C = 1;

		if (Controller_1D_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[4].Mode)) Controller_1D_Mode = 0;
			else Controller_1D_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[4].X)) Controller_1D_X = 0;
			else Controller_1D_X = 1;

			if (Check_Key_Pressed(Keys_Def[4].Y)) Controller_1D_Y = 0;
			else Controller_1D_Y = 1;

			if (Check_Key_Pressed(Keys_Def[4].Z)) Controller_1D_Z = 0;
			else Controller_1D_Z = 1;
		}
	}

	if (Controller_2_Type & 0x10)			// TEAMPLAYER PORT 2
	{
		if (Check_Key_Pressed(Keys_Def[5].Up))
		{
			Controller_2B_Up = 0;
			if(LeftRightEnabled==0) Controller_2B_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[5].Down)) Controller_2B_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_2B_Up = 1;
			if (Check_Key_Pressed(Keys_Def[5].Down)) Controller_2B_Down = 0;
			else Controller_2B_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[5].Left))
		{
			Controller_2B_Left = 0;
			if(LeftRightEnabled==0) Controller_2B_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[5].Right)) Controller_2B_Right = 0;
			else Controller_2B_Right = 1;}
		}
		else
		{
			Controller_2B_Left = 1;
			if (Check_Key_Pressed(Keys_Def[5].Right)) Controller_2B_Right = 0;
			else Controller_2B_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[5].Start)) Controller_2B_Start = 0;
		else Controller_2B_Start = 1;

		if (Check_Key_Pressed(Keys_Def[5].A)) Controller_2B_A = 0;
		else Controller_2B_A = 1;

		if (Check_Key_Pressed(Keys_Def[5].B)) Controller_2B_B = 0;
		else Controller_2B_B = 1;

		if (Check_Key_Pressed(Keys_Def[5].C)) Controller_2B_C = 0;
		else Controller_2B_C = 1;

		if (Controller_2B_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[5].Mode)) Controller_2B_Mode = 0;
			else Controller_2B_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[5].X)) Controller_2B_X = 0;
			else Controller_2B_X = 1;

			if (Check_Key_Pressed(Keys_Def[5].Y)) Controller_2B_Y = 0;
			else Controller_2B_Y = 1;

			if (Check_Key_Pressed(Keys_Def[5].Z)) Controller_2B_Z = 0;
			else Controller_2B_Z = 1;
		}

		if (Check_Key_Pressed(Keys_Def[6].Up))
		{
			Controller_2C_Up = 0;
			if(LeftRightEnabled==0) Controller_2C_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[6].Down)) Controller_2C_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_2C_Up = 1;
			if (Check_Key_Pressed(Keys_Def[6].Down)) Controller_2C_Down = 0;
			else Controller_2C_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[6].Left))
		{
			Controller_2C_Left = 0;
			if(LeftRightEnabled==0) Controller_2C_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[6].Right)) Controller_2C_Right = 0;
			else Controller_2C_Right = 1;}
		}
		else
		{
			Controller_2C_Left = 1;
			if (Check_Key_Pressed(Keys_Def[6].Right)) Controller_2C_Right = 0;
			else Controller_2C_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[6].Start)) Controller_2C_Start = 0;
		else Controller_2C_Start = 1;

		if (Check_Key_Pressed(Keys_Def[6].A)) Controller_2C_A = 0;
		else Controller_2C_A = 1;

		if (Check_Key_Pressed(Keys_Def[6].B)) Controller_2C_B = 0;
		else Controller_2C_B = 1;

		if (Check_Key_Pressed(Keys_Def[6].C)) Controller_2C_C = 0;
		else Controller_2C_C = 1;

		if (Controller_2C_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[6].Mode)) Controller_2C_Mode = 0;
			else Controller_2C_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[6].X)) Controller_2C_X = 0;
			else Controller_2C_X = 1;

			if (Check_Key_Pressed(Keys_Def[6].Y)) Controller_2C_Y = 0;
			else Controller_2C_Y = 1;

			if (Check_Key_Pressed(Keys_Def[6].Z)) Controller_2C_Z = 0;
			else Controller_2C_Z = 1;
		}

		if (Check_Key_Pressed(Keys_Def[7].Up))
		{
			Controller_2D_Up = 0;
			if(LeftRightEnabled==0) Controller_2D_Down = 1;
			else {if(Check_Key_Pressed(Keys_Def[7].Down)) Controller_2D_Down = 0;
			else Controller_1_Down = 1;}
		}
		else
		{
			Controller_2D_Up = 1;
			if (Check_Key_Pressed(Keys_Def[7].Down)) Controller_2D_Down = 0;
			else Controller_2D_Down = 1;
		}
	
		if (Check_Key_Pressed(Keys_Def[7].Left))
		{
			Controller_2D_Left = 0;
			if(LeftRightEnabled==0) Controller_2D_Right = 1;
			else {if(Check_Key_Pressed(Keys_Def[7].Right)) Controller_2D_Right = 0;
			else Controller_2D_Right = 1;}
		}
		else
		{
			Controller_2D_Left = 1;
			if (Check_Key_Pressed(Keys_Def[7].Right)) Controller_2D_Right = 0;
			else Controller_2D_Right = 1;
		}

		if (Check_Key_Pressed(Keys_Def[7].Start)) Controller_2D_Start = 0;
		else Controller_2D_Start = 1;

		if (Check_Key_Pressed(Keys_Def[7].A)) Controller_2D_A = 0;
		else Controller_2D_A = 1;

		if (Check_Key_Pressed(Keys_Def[7].B)) Controller_2D_B = 0;
		else Controller_2D_B = 1;

		if (Check_Key_Pressed(Keys_Def[7].C)) Controller_2D_C = 0;
		else Controller_2D_C = 1;

		if (Controller_2D_Type & 1)
		{
			if (Check_Key_Pressed(Keys_Def[7].Mode)) Controller_2D_Mode = 0;
			else Controller_2D_Mode = 1;

			if (Check_Key_Pressed(Keys_Def[7].X)) Controller_2D_X = 0;
			else Controller_2D_X = 1;

			if (Check_Key_Pressed(Keys_Def[7].Y)) Controller_2D_Y = 0;
			else Controller_2D_Y = 1;

			if (Check_Key_Pressed(Keys_Def[7].Z)) Controller_2D_Z = 0;
			else Controller_2D_Z = 1;
		}
	}


	// autofire / autohold
	{
		extern long unsigned int FrameCount;
		autoAlternator = (FrameCount % 2) == 0;

		#define APPLY_AUTOS(x) APPLY_AUTOS_I(Controller_1_##x); APPLY_AUTOS_I(Controller_2_##x); 
		#define APPLY_AUTOS_I(x)\
		{\
			const bool autoFireFrame = autoAlternator ? x##_Autofire : x##_Autofire2;\
			if(autoFireFrame != x##_Autohold)\
				x = !x;\
		}

		APPLY_AUTOS(Up);
		APPLY_AUTOS(Down);
		APPLY_AUTOS(Left);
		APPLY_AUTOS(Right);
		APPLY_AUTOS(Start);
		APPLY_AUTOS(Mode);
		APPLY_AUTOS(A);
		APPLY_AUTOS(B);
		APPLY_AUTOS(C);
		APPLY_AUTOS(X);
		APPLY_AUTOS(Y);
		APPLY_AUTOS(Z);
	}

	#ifdef ECCOBOXHACK
		EccoAutofire();
	#endif
}

//Modif N. - moved some existing code into this function to reduce redundancy 
//Upth-Modif - No longer crashes after subdialog closure; no longer has unneeded sleep calls
int setupKey (char* message, unsigned int & keyVar, HWND hset)
{
	if (!Init_Input(ghInstance, hset)) MessageBox(NULL,"I failed to initialize the input.","Notice",MB_OK);
	for (int i = 0; i < 256; i++)
		Keys[i] &= ~0x80;
	MSG m;

	keyVar = Get_Key();

	while (PeekMessage(&m, hset, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));

	return 1;
}

int Setting_Skip_Key(HWND hset)
{
	return setupKey("INPUT KEY FOR ADVANCE FRAME", SkipKey, hset);
}
int Setting_Slow_Key(HWND hset)
{
	return setupKey("INPUT KEY FOR TOGGLE SLOW MODE", SlowDownKey, hset);
}
int Setting_Quickpause_Key(HWND hset)
{
	return setupKey("INPUT KEY FOR PAUSE", QuickPauseKey, hset);
}
int Setting_Quickload_Key(HWND hset)	//Modif
{
	return setupKey("INPUT KEY FOR QUICKLOAD", QuickLoadKey, hset);
}

int Setting_Quicksave_Key(HWND hset)		//Modif
{
	return setupKey("INPUT KEY FOR QUICKSAVE", QuickSaveKey, hset);
}

int Setting_Autofire_Key(HWND hset)		//Modif N.
{
	return setupKey("INPUT KEY FOR AUTO-FIRE", AutoFireKey, hset);
}

int Setting_Autohold_Key(HWND hset)		//Modif N.
{
	return setupKey("INPUT KEY FOR AUTO-HOLD", AutoHoldKey, hset);
}

int Setting_Autoclear_Key(HWND hset)		//Modif N.
{
	return setupKey("INPUT KEY FOR CLEARING AUTOS", AutoClearKey, hset);
}

/*int Setting_Keys(HWND hset, int Player, int TypeP) //Upth-Modif - totally redid the controller key redefines. commented out the old version
{
	HWND Txt1, Txt2;
	MSG m;
	Cur_Player = Player;

	Sleep(250);
	Txt1 = GetDlgItem(hset, IDC_STATIC_TEXT1);
	Txt2 = GetDlgItem(hset, IDC_STATIC_TEXT2);
	if (Txt1 == NULL) return 0;
	if (Txt2 == NULL) return 0;

	SetWindowText(Txt1, "INPUT KEY FOR UP");
	Keys_Def[Player].Up = Get_Key();
	Sleep(250);

	SetWindowText(Txt1, "INPUT KEY FOR DOWN");
	Keys_Def[Player].Down = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR LEFT");
	Keys_Def[Player].Left = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR RIGHT");
	Keys_Def[Player].Right = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR START");
	Keys_Def[Player].Start = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR A");
	Keys_Def[Player].A = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR B");
	Keys_Def[Player].B = Get_Key();
	Sleep(250);
	
	SetWindowText(Txt1, "INPUT KEY FOR C");
	Keys_Def[Player].C = Get_Key();
	Sleep(250);

	if (TypeP)
	{
		SetWindowText(Txt1, "INPUT KEY FOR MODE");
		Keys_Def[Player].Mode = Get_Key();
		Sleep(250);

		SetWindowText(Txt1, "INPUT KEY FOR X");
		Keys_Def[Player].X = Get_Key();
		Sleep(250);

		SetWindowText(Txt1, "INPUT KEY FOR Y");
		Keys_Def[Player].Y = Get_Key();
		Sleep(250);

		SetWindowText(Txt1, "INPUT KEY FOR Z");
		Keys_Def[Player].Z = Get_Key();
		Sleep(250);
	}

	SetWindowText(Txt1, "CONFIGURATION SUCCESSFULL");
	SetWindowText(Txt2, "PRESS A KEY TO CONTINUE ...");
	Get_Key();
	Sleep(500);

	while (PeekMessage(&m, hset, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));

	SetWindowText(Txt1, "");
	SetWindowText(Txt2, "");

	return 1;
}*/


LRESULT CALLBACK KeyProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Upth-Add - This is the new controller key redefinition dialog box
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	int Player = Cur_Player;
	MSG m;

	static HWND Tex0 = NULL;
	static HWND Tex1 = NULL;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}
			GetWindowRect(HWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			SetWindowPos(hDlg, NULL, r.left + (dx1 - dx2), r.top + (dy1 - dy2), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			Tex0 = GetDlgItem(hDlg, IDC_STATIC_TEXT1);
			Tex1 = GetDlgItem(hDlg, IDC_STATIC_TEXT2);

			switch (Cur_Player) //Upth-Add - This part shows what player we're redefining keys for.
			{
				case 0:
					sprintf(Str_Tmp,"Player 1");
					break;
				case 1:
					sprintf(Str_Tmp,"Player 2");
					break;
				case 2:
					sprintf(Str_Tmp,"Player 1-B");
					break;
				case 3:
					sprintf(Str_Tmp,"Player 1-C");
					break;
				case 4:
					sprintf(Str_Tmp,"Player 1-D");
					break;
				case 5:
					sprintf(Str_Tmp,"Player 2-B");
					break;
				case 6:
					sprintf(Str_Tmp,"Player 2-C");
					break;
				case 7:
					sprintf(Str_Tmp,"Player 2-D");
					break;
				default:
					sprintf(Str_Tmp,"Invalid Player");
					break;
			}

			SetWindowText(Tex0, Str_Tmp);

			if (!Init_Input(ghInstance, hDlg)) return false;

			return true;
			break;

		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					Keys_Def[Cur_Player]=Temp_Keys;
					SetWindowText(Tex0, "");
					End_Input();
					DialogsOpen--;
					EndDialog(hDlg, false);
					return true;
					break;
				case ID_CANCEL:
					SetWindowText(Tex0, "");
					End_Input();
					DialogsOpen--;
					EndDialog(hDlg, false);
					return true;
					break;
				case IDC_BUTTON_REDEFINE_UP_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR UP");
					Temp_Keys.Up = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_DOWN_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR DOWN");
					Temp_Keys.Down = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_LEFT_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR LEFT");
					Temp_Keys.Left = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_RIGHT_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR RIGHT");
					Temp_Keys.Right = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_A_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR A");
					Temp_Keys.A = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_B_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR B");
					Temp_Keys.B = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_C_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR C");
					Temp_Keys.C = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_START_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR START");
					Temp_Keys.Start = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_X_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR X");
					Temp_Keys.X = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_Y_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR Y");
					Temp_Keys.Y = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_Z_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR Z");
					Temp_Keys.Z = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
				case IDC_BUTTON_REDEFINE_MODE_KEY:
					SetWindowText(Tex1, "INPUT KEY FOR MODE");
					Temp_Keys.Mode = Get_Key();
					while (PeekMessage(&m, hDlg, WM_KEYDOWN, WM_KEYDOWN, PM_REMOVE));
					SetWindowText(Tex1, "");
					return true;
					break;
			}
			break;

		case WM_CLOSE:
			SetWindowText(Tex0, "");
			End_Input();
			DialogsOpen--;
			EndDialog(hDlg, false);
			return true;
			break;
	}

	return false;
}

int Setting_Keys(HWND hset, int Player, int TypeP) //Upth-Add - Opens the new dialog box
{
//	HWND Tex1, Txt2;
	Cur_Player = Player; //Upth-Add - Tells it what player we're redefining keys for
	Temp_Keys = Keys_Def[Player]; //Upth-Add - So that any keys that aren't redefined don't get unset when the user hits "OK"

	DialogsOpen++;
	DialogBox(ghInstance, MAKEINTRESOURCE(TypeP ? IDD_SETJOYKEYS_6 : IDD_SETJOYKEYS_3), hset, (DLGPROC) KeyProc); //Upth-Add - Opens the dialog

	return 1;
}


void Scan_Player_Net(int Player)
{
	if (!Player) return;

	Update_Input();

	if (Check_Key_Pressed(Keys_Def[0].Up))
	{
		Kaillera_Keys[0] &= ~0x08;
		Kaillera_Keys[0] |= 0x04;
	}
	else
	{
		Kaillera_Keys[0] |= 0x08;
		if (Check_Key_Pressed(Keys_Def[0].Down)) Kaillera_Keys[0] &= ~0x04;
		else Kaillera_Keys[0] |= 0x04;
	}
	
	if (Check_Key_Pressed(Keys_Def[0].Left))
	{
		Kaillera_Keys[0] &= ~0x02;
		Kaillera_Keys[0] |= 0x01;
	}
	else
	{
		Kaillera_Keys[0] |= 0x02;
		if (Check_Key_Pressed(Keys_Def[0].Right)) Kaillera_Keys[0] &= ~0x01;
		else Kaillera_Keys[0] |= 0x01;
	}

	if (Check_Key_Pressed(Keys_Def[0].Start)) Kaillera_Keys[0] &= ~0x80;
	else Kaillera_Keys[0] |= 0x80;

	if (Check_Key_Pressed(Keys_Def[0].A)) Kaillera_Keys[0] &= ~0x40;
	else Kaillera_Keys[0] |= 0x40;

	if (Check_Key_Pressed(Keys_Def[0].B)) Kaillera_Keys[0] &= ~0x20;
	else Kaillera_Keys[0] |= 0x20;

	if (Check_Key_Pressed(Keys_Def[0].C)) Kaillera_Keys[0] &= ~0x10;
	else Kaillera_Keys[0] |= 0x10;

	if (Controller_1_Type & 1)
	{
		if (Check_Key_Pressed(Keys_Def[0].Mode)) Kaillera_Keys[1] &= ~0x08;
		else Kaillera_Keys[1] |= 0x08;

		if (Check_Key_Pressed(Keys_Def[0].X)) Kaillera_Keys[1] &= ~0x04;
		else Kaillera_Keys[1] |= 0x04;

		if (Check_Key_Pressed(Keys_Def[0].Y)) Kaillera_Keys[1] &= ~0x02;
		else Kaillera_Keys[1] |= 0x02;

		if (Check_Key_Pressed(Keys_Def[0].Z)) Kaillera_Keys[1] &= ~0x01;
		else Kaillera_Keys[1] |= 0x01;
	}
}


void Update_Controllers_Net(int num_player)
{
	Controller_1_Up = (Kaillera_Keys[0] & 0x08) >> 3;
	Controller_1_Down = (Kaillera_Keys[0] & 0x04) >> 2;
	Controller_1_Left = (Kaillera_Keys[0] & 0x02) >> 1;
	Controller_1_Right = (Kaillera_Keys[0] & 0x01);
	Controller_1_Start = (Kaillera_Keys[0] & 0x80) >> 7;
	Controller_1_A = (Kaillera_Keys[0] & 0x40) >> 6;
	Controller_1_B = (Kaillera_Keys[0] & 0x20) >> 5;
	Controller_1_C = (Kaillera_Keys[0] & 0x10) >> 4;

	if (Controller_1_Type & 1)
	{
		Controller_1_Mode = (Kaillera_Keys[0 + 1] & 0x08) >> 3;
		Controller_1_X = (Kaillera_Keys[0 + 1] & 0x04) >> 2;
		Controller_1_Y = (Kaillera_Keys[0 + 1] & 0x02) >> 1;
		Controller_1_Z = (Kaillera_Keys[0 + 1] & 0x01);
	}

	if (num_player > 2)			// TEAMPLAYER
	{
		Controller_1B_Up = (Kaillera_Keys[2] & 0x08) >> 3;
		Controller_1B_Down = (Kaillera_Keys[2] & 0x04) >> 2;
		Controller_1B_Left = (Kaillera_Keys[2] & 0x02) >> 1;
		Controller_1B_Right = (Kaillera_Keys[2] & 0x01);
		Controller_1B_Start = (Kaillera_Keys[2] & 0x80) >> 7;
		Controller_1B_A = (Kaillera_Keys[2] & 0x40) >> 6;
		Controller_1B_B = (Kaillera_Keys[2] & 0x20) >> 5;
		Controller_1B_C = (Kaillera_Keys[2] & 0x10) >> 4;

		if (Controller_1B_Type & 1)
		{
			Controller_1B_Mode = (Kaillera_Keys[2 + 1] & 0x08) >> 3;
			Controller_1B_X = (Kaillera_Keys[2 + 1] & 0x04) >> 2;
			Controller_1B_Y = (Kaillera_Keys[2 + 1] & 0x02) >> 1;
			Controller_1B_Z = (Kaillera_Keys[2 + 1] & 0x01);
		}

		Controller_1C_Up = (Kaillera_Keys[4] & 0x08) >> 3;
		Controller_1C_Down = (Kaillera_Keys[4] & 0x04) >> 2;
		Controller_1C_Left = (Kaillera_Keys[4] & 0x02) >> 1;
		Controller_1C_Right = (Kaillera_Keys[4] & 0x01);
		Controller_1C_Start = (Kaillera_Keys[4] & 0x80) >> 7;
		Controller_1C_A = (Kaillera_Keys[4] & 0x40) >> 6;
		Controller_1C_B = (Kaillera_Keys[4] & 0x20) >> 5;
		Controller_1C_C = (Kaillera_Keys[4] & 0x10) >> 4;

		if (Controller_1C_Type & 1)
		{
			Controller_1C_Mode = (Kaillera_Keys[4 + 1] & 0x08) >> 3;
			Controller_1C_X = (Kaillera_Keys[4 + 1] & 0x04) >> 2;
			Controller_1C_Y = (Kaillera_Keys[4 + 1] & 0x02) >> 1;
			Controller_1C_Z = (Kaillera_Keys[4 + 1] & 0x01);
		}

		Controller_1D_Up = (Kaillera_Keys[6] & 0x08) >> 3;
		Controller_1D_Down = (Kaillera_Keys[6] & 0x04) >> 2;
		Controller_1D_Left = (Kaillera_Keys[6] & 0x02) >> 1;
		Controller_1D_Right = (Kaillera_Keys[6] & 0x01);
		Controller_1D_Start = (Kaillera_Keys[6] & 0x80) >> 7;
		Controller_1D_A = (Kaillera_Keys[6] & 0x40) >> 6;
		Controller_1D_B = (Kaillera_Keys[6] & 0x20) >> 5;
		Controller_1D_C = (Kaillera_Keys[6] & 0x10) >> 4;

		if (Controller_1D_Type & 1)
		{
			Controller_1D_Mode = (Kaillera_Keys[6 + 1] & 0x08) >> 3;
			Controller_1D_X = (Kaillera_Keys[6 + 1] & 0x04) >> 2;
			Controller_1D_Y = (Kaillera_Keys[6 + 1] & 0x02) >> 1;
			Controller_1D_Z = (Kaillera_Keys[6 + 1] & 0x01);
		}
	}
	else
	{
		Controller_2_Up = (Kaillera_Keys[2] & 0x08) >> 3;
		Controller_2_Down = (Kaillera_Keys[2] & 0x04) >> 2;
		Controller_2_Left = (Kaillera_Keys[2] & 0x02) >> 1;
		Controller_2_Right = (Kaillera_Keys[2] & 0x01);
		Controller_2_Start = (Kaillera_Keys[2] & 0x80) >> 7;
		Controller_2_A = (Kaillera_Keys[2] & 0x40) >> 6;
		Controller_2_B = (Kaillera_Keys[2] & 0x20) >> 5;
		Controller_2_C = (Kaillera_Keys[2] & 0x10) >> 4;

		if (Controller_2_Type & 1)
		{
			Controller_2_Mode = (Kaillera_Keys[2 + 1] & 0x08) >> 3;
			Controller_2_X = (Kaillera_Keys[2 + 1] & 0x04) >> 2;
			Controller_2_Y = (Kaillera_Keys[2 + 1] & 0x02) >> 1;
			Controller_2_Z = (Kaillera_Keys[2 + 1] & 0x01);
		}
	}
}

int Check_Pause_Key()
{
	Update_Input();
	if(Check_Key_Pressed(QuickPauseKey)==1 && QuickPauseKeyIsPressed==0) //Modif N - allow frame advance key to also pause
	{
		QuickPauseKeyIsPressed=1;
		return 1;
	}
	if(Check_Key_Pressed(QuickPauseKey)==0 && QuickPauseKeyIsPressed==1) //Modif N - allow frame advance key to also pause
		QuickPauseKeyIsPressed=0;
	return 0;
}

//Modif N - changed to make frame advance key continuous, after a delay:
int Check_Skip_Key()
{
	Update_Input();

	static time_t lastSkipTime = 0;
	const int skipPressedNew = Check_Key_Pressed(SkipKey);

	static int checks = 0;
	if(skipPressedNew && timeGetTime()-lastSkipTime >= 5)
	{
		checks++;
		if(checks > 8000 + 60)
			checks -= 8000;
		lastSkipTime = timeGetTime();
	}

	if(skipPressedNew && (!SkipKeyIsPressed || ((checks > 60) && ((checks % DelayFactor) == 0))))
	{
		SkipKeyIsPressed=1;
		return 1;
	}
	else {
		if(!skipPressedNew && SkipKeyIsPressed)
		{
			SkipKeyIsPressed=0;
			checks=0;
		}
		return 0;
	}
}

void Check_Misc_Key()
{
	Update_Input();

	if(SlowDownKey)
	{
		if(Check_Key_Pressed(SlowDownKey)==1 && SlowDownKeyIsPressed==0)
		{
			SlowDownKeyIsPressed=1;
			if(SlowDownMode)
			{
				SlowDownMode=0;
			}
			else
			{
				SlowDownMode=1;
			}
			MustUpdateMenu=1;
		}
		if(Check_Key_Pressed(SlowDownKey)==0 && SlowDownKeyIsPressed==1)
			SlowDownKeyIsPressed=0;
	}

	if (QuickLoadKey!=0)
	{
		if(Check_Key_Pressed(QuickLoadKey)==1 && QuickLoadKeyIsPressed==0 && Check_Key_Pressed(QuickSaveKey)==0)
		{
			Str_Tmp[0] = 0;
			Get_State_File_Name(Str_Tmp);
			Load_State(Str_Tmp);
			QuickLoadKeyIsPressed=1;
		}
		if(Check_Key_Pressed(QuickLoadKey)==0 && QuickLoadKeyIsPressed==1)
			QuickLoadKeyIsPressed=0;
	}
	if (QuickSaveKey!=0) 
	{
		if(Check_Key_Pressed(QuickSaveKey)==1 && QuickSaveKeyIsPressed==0 && Check_Key_Pressed(QuickLoadKey)==0)
		{
			Str_Tmp[0] = 0;
			Get_State_File_Name(Str_Tmp);
			Save_State(Str_Tmp);
			QuickSaveKeyIsPressed=1;
		}
		if(Check_Key_Pressed(QuickSaveKey)==0 && QuickSaveKeyIsPressed==1)
			QuickSaveKeyIsPressed=0;
	}

	// N - checks for enabling/disabling the autofire and autohold toggles
	if(AutoFireKey || AutoHoldKey || AutoClearKey)
	{
		#define TRANSFER_PRESSED(x) {const int now = !Check_Key_Pressed(Keys_Def[0].##x); Controller_1_##x##_Just = Controller_1_##x##_Last && !now; Controller_1_##x##_Last = now;} {const int now = !Check_Key_Pressed(Keys_Def[1].##x); Controller_2_##x##_Just = Controller_2_##x##_Last && !now; Controller_2_##x##_Last = now;}
		TRANSFER_PRESSED(Up);
		TRANSFER_PRESSED(Down);
		TRANSFER_PRESSED(Left);
		TRANSFER_PRESSED(Right);
		TRANSFER_PRESSED(Start);
		TRANSFER_PRESSED(Mode);
		TRANSFER_PRESSED(A);
		TRANSFER_PRESSED(B);
		TRANSFER_PRESSED(C);
		TRANSFER_PRESSED(X);
		TRANSFER_PRESSED(Y);
		TRANSFER_PRESSED(Z);

		const bool autoFireHeld = AutoFireKey && Check_Key_Pressed(AutoFireKey)==1;
		const bool autoHoldHeld = AutoHoldKey && Check_Key_Pressed(AutoHoldKey)==1;
		const bool autoClearHeld = AutoClearKey && Check_Key_Pressed(AutoClearKey)==1;
		if(autoFireHeld || autoHoldHeld)
		{
			extern long unsigned int FrameCount;
			autoAlternator = (FrameCount % 2) == 0;

			#define CHECK_TOGGLE_AUTO(x) CHECK_TOGGLE_AUTO_I(Controller_1_##x); CHECK_TOGGLE_AUTO_I(Controller_2_##x); 
			#define CHECK_TOGGLE_AUTO_I(x)\
			if(x##_Just)\
			{\
				if(autoHoldHeld)\
				{\
					x##_Autohold = !x##_Autohold;\
					x##_Autofire = x##_Autofire2 = false;\
				}\
				if(autoFireHeld)\
				{\
					const bool autoFired = x##_Autofire || x##_Autofire2;\
					x##_Autohold = x##_Autofire = x##_Autofire2 = false;\
					if(autoAlternator)\
						x##_Autofire = !autoFired;\
					else\
						x##_Autofire2 = !autoFired;\
				}\
			}

			CHECK_TOGGLE_AUTO(Up);
			CHECK_TOGGLE_AUTO(Down);
			CHECK_TOGGLE_AUTO(Left);
			CHECK_TOGGLE_AUTO(Right);
			CHECK_TOGGLE_AUTO(Start);
			CHECK_TOGGLE_AUTO(Mode);
			CHECK_TOGGLE_AUTO(A);
			CHECK_TOGGLE_AUTO(B);
			CHECK_TOGGLE_AUTO(C);
			CHECK_TOGGLE_AUTO(X);
			CHECK_TOGGLE_AUTO(Y);
			CHECK_TOGGLE_AUTO(Z);
		}
		if(autoClearHeld)
		{
			#define CLEAR_AUTO(x) CLEAR_AUTO_I(Controller_1_##x); CLEAR_AUTO_I(Controller_2_##x); 
			#define CLEAR_AUTO_I(x) x##_Autofire = 0; x##_Autofire2 = 0; x##_Autohold = 0;

			CLEAR_AUTO(Up);
			CLEAR_AUTO(Down);
			CLEAR_AUTO(Left);
			CLEAR_AUTO(Right);
			CLEAR_AUTO(Start);
			CLEAR_AUTO(Mode);
			CLEAR_AUTO(A);
			CLEAR_AUTO(B);
			CLEAR_AUTO(C);
			CLEAR_AUTO(X);
			CLEAR_AUTO(Y);
			CLEAR_AUTO(Z);
		}
	}

}

