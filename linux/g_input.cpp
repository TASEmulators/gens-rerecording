#include "port.h"
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include "resource.h"
#include "g_input.h"
#include "io.h"
#include "g_main.h"

#ifdef WITH_GTK
#include "glade/support.h"
#include <gdk/gdkkeysyms.h>
#endif

#define KEYDOWN(key) (Keys[key] & 0x80) 
#define KEYDOWNSDL(key) (key[keys] || key_states[keys]) 
#define MAX_JOYS 8
char Phrase[1024];
unsigned char key[1024];
int Nb_Joys = 0;
long MouseX, MouseY;
unsigned char Keys[256];
unsigned char Kaillera_Keys[16];
unsigned char key_states[1024];

struct K_Def Keys_Def[8] = {
	{SDLK_RETURN, SDLK_RSHIFT,
	SDLK_a, SDLK_z, SDLK_e,
	SDLK_q, SDLK_s, SDLK_d,
	SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT},
	{SDLK_u, SDLK_t,
	SDLK_k, SDLK_l, SDLK_m,
	SDLK_i, SDLK_o, SDLK_p,
	SDLK_y, SDLK_h, SDLK_g, SDLK_j}
};

int String_Size(char *Chaine)
{
	int i = 0;

	while (*(Chaine + i++));

	return(i - 1);	
}

void End_Input()
{
#if 0
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
#endif
}

#if 0
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
#endif

int Init_Input(int hInst, int hWnd)
{
	memset(key_states, 0, sizeof(key_states));
#if 0
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
#endif
	return 1;
}


void Restore_Input()
{
//	lpDIDMouse->Acquire();
	//lpDIDKeyboard->Acquire();
}

void Update_Input()
{
	//key_states = SDL_GetKeyState(NULL);
#if 0
//	DIMOUSESTATE MouseState;
	HRESULT rval;
	int i;

	rval = lpDIDKeyboard->GetDeviceState(256, &Keys);

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
#endif
}


int Check_Key_Pressed(unsigned int keys)
{
	//int Num_Joy;
	//if (key < 0x100)
	//{
		if KEYDOWNSDL(key) return(1);
	//}
	
#if 0
	else
	{
		Num_Joy = ((key >> 8) & 0xF);

		if (Joy_ID[Num_Joy])
		{
			if (key & 0x80)			// Test POV Joys
			{
				switch(key & 0xF)
				{
					case 1:
						if (Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3] == 0) return(1); break;

					case 2:
						if (Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3] == 9000) return(1); break;

					case 3:
						if (Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3] == 18000) return(1); break;

					case 4:
						if (Joy_State[Num_Joy].rgdwPOV[(key >> 4) & 3] == 27000) return(1); break;
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
#endif
	return 0;
}

#ifdef WITH_GTK
static int
gdk_to_sdl_keyval(int gdk_key)
{
	switch(gdk_key)
	{
		case GDK_BackSpace : return	SDLK_BACKSPACE;
		case GDK_Tab : return	SDLK_TAB;
		case GDK_Clear : return	SDLK_CLEAR;
		case GDK_Return : return	SDLK_RETURN;
		case GDK_Pause : return	SDLK_PAUSE;
		case GDK_Escape : return	SDLK_ESCAPE;
		case GDK_KP_Space : return	SDLK_SPACE;
		case GDK_exclamdown : return	SDLK_EXCLAIM;
		case GDK_quotedbl : return	SDLK_QUOTEDBL;
		case GDK_numbersign : return	SDLK_HASH;
		case GDK_dollar : return	SDLK_DOLLAR;
		case GDK_ampersand : return	SDLK_AMPERSAND;
		case GDK_quoteright : return	SDLK_QUOTE;
		case GDK_parenleft : return	SDLK_LEFTPAREN;
		case GDK_parenright : return	SDLK_RIGHTPAREN;
		case GDK_asterisk : return	SDLK_ASTERISK;
		case GDK_plus : return	SDLK_PLUS;
		case GDK_comma : return	SDLK_COMMA;
		case GDK_minus : return	SDLK_MINUS;
		case GDK_period : return	SDLK_PERIOD;
		case GDK_slash : return	SDLK_SLASH;
		case GDK_0 : return	SDLK_0	;
		case GDK_1 : return	SDLK_1	;
		case GDK_2 : return	SDLK_2	;
		case GDK_3 : return	SDLK_3	;
		case GDK_4 : return	SDLK_4	;
		case GDK_5 : return	SDLK_5	;
		case GDK_6 : return	SDLK_6	;
		case GDK_7 : return	SDLK_7	;
		case GDK_8 : return	SDLK_8	;
		case GDK_9 : return	SDLK_9	;
		case GDK_colon : return	SDLK_COLON;
		case GDK_semicolon : return	SDLK_SEMICOLON;
		case GDK_less : return	SDLK_LESS;
		case GDK_equal : return	SDLK_EQUALS;
		case GDK_greater : return	SDLK_GREATER;
		case GDK_question : return	SDLK_QUESTION;
		case GDK_at : return	SDLK_AT	;
		case GDK_bracketleft : return	SDLK_LEFTBRACKET	;
		case GDK_backslash : return	SDLK_BACKSLASH;
		case GDK_bracketright : return	SDLK_RIGHTBRACKET	;
		case GDK_asciicircum : return	SDLK_CARET;
		case GDK_underscore : return	SDLK_UNDERSCORE;
		case GDK_quoteleft : return	SDLK_BACKQUOTE;
		case GDK_a : return	SDLK_a	;
		case GDK_b : return	SDLK_b	;
		case GDK_c : return	SDLK_c	;
		case GDK_d : return	SDLK_d	;
		case GDK_e : return	SDLK_e	;
		case GDK_f : return	SDLK_f	;
		case GDK_g : return	SDLK_g	;
		case GDK_h : return	SDLK_h	;
		case GDK_i : return	SDLK_i	;
		case GDK_j : return	SDLK_j	;
		case GDK_k : return	SDLK_k	;
		case GDK_l : return	SDLK_l	;
		case GDK_m : return	SDLK_m	;
		case GDK_n : return	SDLK_n	;
		case GDK_o : return	SDLK_o	;
		case GDK_p : return	SDLK_p	;
		case GDK_q : return	SDLK_q	;
		case GDK_r : return	SDLK_r	;
		case GDK_s : return	SDLK_s	;
		case GDK_t : return	SDLK_t	;
		case GDK_u : return	SDLK_u	;
		case GDK_v : return	SDLK_v	;
		case GDK_w : return	SDLK_w	;
		case GDK_x : return	SDLK_x	;
		case GDK_y : return	SDLK_y	;
		case GDK_z : return	SDLK_z	;
		case GDK_Delete : return	SDLK_DELETE;
		case GDK_KP_0 : return	SDLK_KP0;
		case GDK_KP_1 : return	SDLK_KP1;
		case GDK_KP_2 : return	SDLK_KP2;
		case GDK_KP_3 : return	SDLK_KP3;
		case GDK_KP_4 : return	SDLK_KP4;
		case GDK_KP_5 : return	SDLK_KP5;
		case GDK_KP_6 : return	SDLK_KP6;
		case GDK_KP_7 : return	SDLK_KP7;
		case GDK_KP_8 : return	SDLK_KP8;
		case GDK_KP_9 : return	SDLK_KP9;
		case GDK_KP_Decimal : return	SDLK_KP_PERIOD;
		case GDK_KP_Divide : return	SDLK_KP_DIVIDE;
		case GDK_KP_Multiply : return	SDLK_KP_MULTIPLY	;
		case GDK_KP_Subtract : return	SDLK_KP_MINUS;
		case GDK_KP_Add : return	SDLK_KP_PLUS;
		case GDK_KP_Enter : return	SDLK_KP_ENTER;
		case GDK_KP_Equal : return	SDLK_KP_EQUALS;
		case GDK_Up : return	SDLK_UP	;
		case GDK_Down : return	SDLK_DOWN;
		case GDK_Right : return	SDLK_RIGHT;
		case GDK_Left : return	SDLK_LEFT;
		case GDK_Insert : return	SDLK_INSERT;
		case GDK_Home : return	SDLK_HOME;
		case GDK_End : return	SDLK_END;
		case GDK_Page_Up : return	SDLK_PAGEUP;
		case GDK_Page_Down : return	SDLK_PAGEDOWN;
		case GDK_F1 : return	SDLK_F1	;
		case GDK_F2 : return	SDLK_F2	;
		case GDK_F3 : return	SDLK_F3	;
		case GDK_F4 : return	SDLK_F4	;
		case GDK_F5 : return	SDLK_F5	;
		case GDK_F6 : return	SDLK_F6	;
		case GDK_F7 : return	SDLK_F7	;
		case GDK_F8 : return	SDLK_F8	;
		case GDK_F9 : return	SDLK_F9	;
		case GDK_F10 : return	SDLK_F10;
		case GDK_F11 : return	SDLK_F11;
		case GDK_F12 : return	SDLK_F12;
		case GDK_F13 : return	SDLK_F13;
		case GDK_F14 : return	SDLK_F14;
		case GDK_F15 : return	SDLK_F15;
		case GDK_Num_Lock : return	SDLK_NUMLOCK;
		case GDK_Caps_Lock : return	SDLK_CAPSLOCK;
		case GDK_Scroll_Lock : return	SDLK_SCROLLOCK;
		case GDK_Shift_R : return	SDLK_RSHIFT;
		case GDK_Shift_L : return	SDLK_LSHIFT;
		case GDK_Control_R : return	SDLK_RCTRL;
		case GDK_Control_L : return	SDLK_LCTRL;
		case GDK_Alt_R : return	SDLK_RALT;
		case GDK_Alt_L : return	SDLK_LALT;
		case GDK_Meta_R : return	SDLK_RMETA;
		case GDK_Meta_L : return	SDLK_LMETA;
		case GDK_Super_L : return	SDLK_LSUPER;
		case GDK_Super_R : return	SDLK_RSUPER;
		case GDK_Mode_switch : return	SDLK_MODE;
		//case GDK_ : return	SDLK_COMPOSE;
		case GDK_Help : return	SDLK_HELP;
		case GDK_Print : return	SDLK_PRINT;
		case GDK_Sys_Req : return	SDLK_SYSREQ;
		case GDK_Break : return	SDLK_BREAK;
		case GDK_Menu : return	SDLK_MENU;
		//case GDK_ : return	SDLK_POWER;
		case GDK_EuroSign : return	SDLK_EURO;
		//case GDK_Undo : return	SDLK_UNDO;
		
		default : fprintf(stderr, "unknown gdk key"); return gdk_key;
	}
}
#endif

unsigned int Get_Key(void)
{
#ifdef __PORT__
#ifdef WITH_GTK
	GdkEvent* event;
	while (gtk_events_pending())
		gtk_main_iteration();
	while (1)
	{
		event = gdk_event_get();
		if (event && event->type == GDK_KEY_PRESS)
		{	
			return gdk_to_sdl_keyval(event->key.keyval);
		}
	}
#endif
#else
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
#endif
}

void input_set_key_down(int sym)
{
	key_states[sym] = 1;
}

void input_set_key_up(int sym)
{
	key_states[sym] = 0;
}

void input_set_joy_down(int button, int on)
{
	unsigned long *st = (unsigned long *)&Keys_Def[0];
	if(button < 8) key_states[st[button]] = on;
}

void input_set_joy_motion(int axis, int value)
{
	key_states[Keys_Def[0].Up] = axis & 1;
	key_states[Keys_Def[0].Down] = axis & 2;
	key_states[Keys_Def[0].Left] = axis & 4;
	key_states[Keys_Def[0].Right] = axis & 8;
}

void Update_Controllers()
{
	Update_Input();

	if (Check_Key_Pressed(Keys_Def[0].Up))
	{
		Controller_1_Up = 0;
		Controller_1_Down = 1;
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
		Controller_1_Right = 1;
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
		Controller_2_Down = 1;
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
		Controller_2_Right = 1;
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
			Controller_1B_Down = 1;
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
			Controller_1B_Right = 1;
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
			Controller_1C_Down = 1;
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
			Controller_1C_Right = 1;
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
			Controller_1D_Down = 1;
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
			Controller_1D_Right = 1;
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
			Controller_2B_Down = 1;
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
			Controller_2B_Right = 1;
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
			Controller_2C_Down = 1;
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
			Controller_2C_Right = 1;
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
			Controller_2D_Down = 1;
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
			Controller_2D_Right = 1;
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
}

#ifdef __PORT__
#ifdef WITH_GTK
int Setting_Keys(GtkWidget* control_window, int Player, int TypeP)
{
	GtkWidget* txt;

	txt = lookup_widget(control_window, "keyEcho");
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR UP");
	Keys_Def[Player].Up = Get_Key();
	Sleep(250);

	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR DOWN");
	Keys_Def[Player].Down = Get_Key();
	Sleep(250);
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR LEFT");
	Keys_Def[Player].Left = Get_Key();
	Sleep(250);
		
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR RIGHT");
	Keys_Def[Player].Right = Get_Key();
	Sleep(250);
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR START");
	Keys_Def[Player].Start = Get_Key();
	Sleep(250);
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR A");
	Keys_Def[Player].A = Get_Key();
	Sleep(250);
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR B");
	Keys_Def[Player].B = Get_Key();
	Sleep(250);
	
	gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR C");
	Keys_Def[Player].C = Get_Key();
	Sleep(250);

	if (TypeP)
	{
		gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR MODE");
		Keys_Def[Player].Mode = Get_Key();
		Sleep(250);
	
		gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR X");
		Keys_Def[Player].X = Get_Key();
		Sleep(250);

		gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR Y");
		Keys_Def[Player].Y = Get_Key();
		Sleep(250);

		gtk_label_set_text(GTK_LABEL(txt), "INPUT KEY FOR Z");
		Keys_Def[Player].Z = Get_Key();
		Sleep(250);
	}
	gtk_label_set_text(GTK_LABEL(txt), "CONFIGURATION SUCCESSFULL\nPRESS A KEY TO CONTINUE ...");
	Get_Key();
	Sleep(500);
	gtk_label_set_text(GTK_LABEL(txt), "");
	return 1;
}
#endif

#else
int Setting_Keys(int hset, int Player, int TypeP)
{
	HWND Txt1, Txt2;
	MSG m;

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
}
#endif

void Scan_Player_Net(int Player)
{
#if 0
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
#endif
}


void Update_Controllers_Net(int num_player)
{
#if 0
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
#endif
}


