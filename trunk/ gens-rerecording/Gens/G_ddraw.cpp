#include <stdio.h>
#include <math.h>
#include "G_ddraw.h"
#include "G_dsound.h"
#include "G_Input.h"
#include "G_main.h"
#include "resource.h"
#include "gens.h"
#include "mem_M68K.h"
#include "vdp_io.h"
#include "vdp_rend.h"
#include "misc.h"
#include "blit.h"
#include "scrshot.h"
#include "net.h"
#include "save.h"

#include "cdda_mp3.h"

#include "movie.h"
#include "moviegfx.h"
#include "io.h"
#include "hackscommon.h"
#include "drawutil.h"
#include "luascript.h"


LPDIRECTDRAW lpDD_Init;
LPDIRECTDRAW4 lpDD;
LPDIRECTDRAWSURFACE4 lpDDS_Primary;
LPDIRECTDRAWSURFACE4 lpDDS_Flip;
LPDIRECTDRAWSURFACE4 lpDDS_Back;
LPDIRECTDRAWSURFACE4 lpDDS_Blit;
LPDIRECTDRAWCLIPPER lpDDC_Clipper;

clock_t Last_Time = 0, New_Time = 0;
clock_t Used_Time = 0;

int Flag_Clr_Scr = 0;
int Sleep_Time = 1;
int FS_VSync;
int W_VSync;
int Res_X = 320 << (int) (Render_FS > 0); //Upth-Add - For the new configurable
int Res_Y = 240 << (int) (Render_FS > 0); //Upth-Add - fullscreen resolution (defaults 320 x 240)
bool FS_No_Res_Change = false; //Upth-Add - For the new fullscreen at same resolution
int Stretch; 
int Blit_Soft;
int Effect_Color = 7;
int FPS_Style = EMU_MODE | BLANC;
int Message_Style = EMU_MODE | BLANC | SIZE_X2;
int Kaillera_Error = 0;
unsigned char CleanAvi = 1;
extern "C" int disableSound, disableSound2, disableRamSearchUpdate;

long int MovieSize;//Modif
int SlowFrame=0; //Modif

static char Info_String[1024] = "";
static int Message_Showed = 0;
static unsigned int Info_Time = 0;

void (*Blit_FS)(unsigned char *Dest, int pitch, int x, int y, int offset);
void (*Blit_W)(unsigned char *Dest, int pitch, int x, int y, int offset);
int (*Update_Frame)();
int (*Update_Frame_Fast)();

int Correct_256_Aspect_Ratio = 1;
#define ALT_X_RATIO_RES (Correct_256_Aspect_Ratio ? 320 : 256)

#define IS_FULL_X_RESOLUTION ((VDP_Reg.Set4 & 0x1) || Debug || !Game || !FrameCount)
#define IS_FULL_Y_RESOLUTION ((VDP_Reg.Set2 & 0x8) && !(Debug || !Game || !FrameCount))



// if you have to debug something in fullscreen mode
// but the fullscreen lock prevents you from seeing the debugger
// when a breakpoint/assertion/crash/whatever happens,
// then it might help to define this.
// absolutely don't leave it enabled by default though, not even in _DEBUG
//#define DISABLE_EXCLUSIVE_FULLSCREEN_LOCK


//#define MAPHACK
#if ((!defined SONICMAPHACK) && (!defined SONICROUTEHACK)) 
#define Update_RAM_Cheats(); 
#else
#include "SonicHackSuite.h"
#endif

#define DUMPAVICLEAN \
	if (CleanAvi)\
	{\
		if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))\
		{\
			if (Message_Showed)\
			{\
				if (GetTickCount() > Info_Time)\
				{\
					Message_Showed = 0;\
					strcpy(Info_String, "");\
				}\
				else \
				{\
					if(!(Message_Style & TRANS))\
					{\
						int backColor = (((Message_Style & (BLEU|VERT|ROUGE)) == BLEU) ? ROUGE : BLEU) | (Message_Style & SIZE_X2) | TRANS;\
						const static int xOffset [] = {-1,-1,-1,0,1,1,1,0};\
						const static int yOffset [] = {-1,0,1,1,1,0,-1,-1};\
						for(int i = 0 ; i < 8 ; i++)\
						{\
							Print_Text(Info_String, strlen(Info_String), 10+xOffset[i], 210+yOffset[i], backColor);\
							Print_Text(Info_String, strlen(Info_String), 10+xOffset[i], 210+yOffset[i], backColor);\
						}\
					}\
					Print_Text(Info_String, strlen(Info_String), 10, 210, Message_Style);\
					Print_Text(Info_String, strlen(Info_String), 10, 210, Message_Style);\
				}\
			}\
			Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);\
		}\
	}
#ifdef SONICCAMHACK
#include "SonicHackSuite.h"
#elif defined RKABOXHACK
#include "RKABoxHack.h"
#elif (defined ECCOBOXHACK) || (defined ECCO1BOXHACK)
#include "EccoBoxHack.h"
#endif

int Update_Frame_Adjusted()
{
	if(disableVideoLatencyCompensationCount)
		disableVideoLatencyCompensationCount--;

	if(!IsVideoLatencyCompensationOn())
	{
		// normal update
		return Update_Frame();
	}
	else
	{
		// update, and render the result that's some number of frames in the (emulated) future
		// typically the video takes 2 frames to catch up with where the game really is,
		// so setting VideoLatencyCompensation to 2 can make the input more responsive
		//
		// in a way this should actually make the emulation more accurate, because
		// the delay from your computer hardware stacks with the delay from the emulated hardware,
		// so eliminating some of that delay should make it feel closer to the real system

		disableSound2 = true;
		int retval = Update_Frame_Fast();
		Update_RAM_Search();
		disableRamSearchUpdate = true;
		Save_State_To_Buffer(State_Buffer);
		for(int i = 0; i < VideoLatencyCompensation-1; i++)
			Update_Frame_Fast();
		disableSound2 = false;
		Update_Frame();
		disableRamSearchUpdate = false;
		Load_State_From_Buffer(State_Buffer);
		return retval;
	}
}

int Update_Frame_Hook()
{
	// note: we don't handle LUACALL_BEFOREEMULATION here
	// because it needs to run immediately before SetCurrentInputCondensed() might get called
	// in order for joypad.get() and joypad.set() to work as expected

	#ifdef SONICCAMHACK
	int retval = SonicCamHack();
	#else
		int retval = Update_Frame_Adjusted();

		#ifdef RKABOXHACK
			RKADrawBoxes();
			CamX = CheatRead<short>(0xFFB158);
			CamY = CheatRead<short>(0xFFB1D6);
		#elif (defined ECCOBOXHACK) || (defined ECCO1BOXHACK) 
			unsigned char curlev = CheatRead<unsigned char>(0xFFA7E0);
			static int PrevX = 0,PrevY = 0;
			int xpos,ypos;
//			switch (CheatRead<unsigned char>(0xFFA555))
//			{
//				case 0x20:
//				case 0x28:
//				case 0xAC:
					EccoDrawBoxes();
					xpos = CheatRead<int>(0xFFA8F0);
					ypos = CheatRead<int>(0xFFA8F4);
//					break;
//				case 0xF6:
//					EccoDraw3D();
//					xpos = ypos = CheatRead<int>(0xFFB13E);
//					break;
//				default:
//					xpos = PrevX + 1;
//					ypos = PrevY + 1;
//			}
//			if ((xpos == PrevX) && (ypos == PrevY) && CheatRead<int>(0xFFAA32)) Lag_Frame = 1;
//			else	PrevX = xpos,PrevY = ypos;
			CamX = CheatRead<short>(CAMXPOS);
			CamY = CheatRead<short>(CAMYPOS);
		#endif
	#endif

	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);

	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATIONGUI);

	return retval;
}
int Update_Frame_Fast_Hook()
{
	// note: we don't handle LUACALL_BEFOREEMULATION here
	// because it needs to run immediately before SetCurrentInputCondensed() might get called
	// in order for joypad.get() and joypad.set() to work as expected

	int retval = Update_Frame_Fast();

	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);
	Update_RAM_Search();
	return retval;
}

void Put_Info_NonImmediate(char *Message, int Duration)
{
	if (Show_Message)
	{
		strcpy(Info_String, Message);
		Info_Time = GetTickCount() + Duration;
		Message_Showed = 1;
	}
}

void Put_Info(char *Message, int Duration)
{
	if (Show_Message)
	{
		Put_Info_NonImmediate(Message, Duration);

		// Modif N. - in case game is paused or at extremely low speed, update screen on new message
		extern bool g_anyScriptsHighSpeed;
		if(!Setting_Render && !g_anyScriptsHighSpeed)
		{
			extern HWND HWnd;

			if(FrameCount == 0) // if frame count is 0 it's OK to clear the screen first so we can see the message better
			{
				memset(MD_Screen, 0, sizeof(MD_Screen));
				memset(MD_Screen32, 0, sizeof(MD_Screen32));
			}

			Flip(HWnd);
		}
	}
}


int Init_Fail(HWND hwnd, char *err)
{
	End_DDraw();
	MessageBox(hwnd, err, "Oups ...", MB_OK);

	return 0;
}


int Init_DDraw(HWND hWnd)
{
	int Rend;
	HRESULT rval;
	DDSURFACEDESC2 ddsd;

	int oldBits32 = Bits32;

	End_DDraw();
	
	if (Full_Screen) Rend = Render_FS;
	else Rend = Render_W;

	if (FAILED(DirectDrawCreate(NULL, &lpDD_Init, NULL)))
		return Init_Fail(hWnd, "Error with DirectDrawCreate !");

	if (FAILED(lpDD_Init->QueryInterface(IID_IDirectDraw4, (LPVOID *) &lpDD)))
		return Init_Fail(hWnd, "Error with QueryInterface !\nUpgrade your DirectX version.");

	lpDD_Init->Release();
	lpDD_Init = NULL;

	if (!(Mode_555 & 2))
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		lpDD->GetDisplayMode(&ddsd);

		if (ddsd.ddpfPixelFormat.dwGBitMask == 0x03E0) Mode_555 = 1;
		else Mode_555 = 0;

		Recalculate_Palettes();
	}

#ifdef DISABLE_EXCLUSIVE_FULLSCREEN_LOCK
	FS_VSync = 0;
	rval = lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
#else
	if (Full_Screen)
		rval = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
	else
		rval = lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
#endif

	if (FAILED(rval))
		return Init_Fail(hWnd, "Error with lpDD->SetCooperativeLevel !");
	
	if (Res_X < (320 << (int) (Render_FS > 0))) Res_X = 320 << (int) (Render_FS > 0); //Upth-Add - Set a floor for the resolution
    if (Res_Y < (240 << (int) (Render_FS > 0))) Res_Y = 240 << (int) (Render_FS > 0); //Upth-Add - 320x240 for single, 640x480 for double and other

	//if (Full_Screen && Render_FS >= 2) //Upth-Modif - Since software blits don't stretch right, we'll use 640x480 for those render modes
	// Modif N. removed the forced 640x480 case because it caused "windowed fullscreen mode" to be ignored and because I fixed the fullscreen software blit stretching

	if (Full_Screen && !(FS_No_Res_Change))
	{
		if (FAILED(lpDD->SetDisplayMode(Res_X, Res_Y, 16, 0, 0)))
			return Init_Fail(hWnd, "Error with lpDD->SetDisplayMode !");
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if ((Full_Screen) && (FS_VSync))
	{
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		ddsd.dwBackBufferCount = 2;
	}
	else
	{
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	}

	if (FAILED(lpDD->CreateSurface(&ddsd, &lpDDS_Primary, NULL )))
		return Init_Fail(hWnd, "Error with lpDD->CreateSurface !");

	if (Full_Screen)
	{
	    if (FS_VSync)
		{
			ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

			if (FAILED(lpDDS_Primary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDS_Flip)))
				return Init_Fail(hWnd, "Error with lpDDPrimary->GetAttachedSurface !");

			lpDDS_Blit = lpDDS_Flip;
		}
		else lpDDS_Blit = lpDDS_Primary;
	}
	else
	{
		if (FAILED(lpDD->CreateClipper(0, &lpDDC_Clipper, NULL )))
			return Init_Fail(hWnd, "Error with lpDD->CreateClipper !");

		if (FAILED(lpDDC_Clipper->SetHWnd(0, hWnd)))
			return Init_Fail(hWnd, "Error with lpDDC_Clipper->SetHWnd !");

		if (FAILED(lpDDS_Primary->SetClipper(lpDDC_Clipper)))
			return Init_Fail(hWnd, "Error with lpDDS_Primary->SetClipper !");
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;

	if (Rend < 2)
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwWidth = 336;
		ddsd.dwHeight = 240;
	}
	else
	{
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		ddsd.dwWidth = 672; //Upth-Modif - was 640, but for single mode the value was 336, not 320.
		ddsd.dwHeight = 480;
	}

	if (FAILED(lpDD->CreateSurface(&ddsd, &lpDDS_Back, NULL)))
		return Init_Fail(hWnd, "Error with lpDD->CreateSurface !");

	if (!Full_Screen || (Rend >= 2 && (FS_No_Res_Change || Res_X != 640 || Res_Y != 480)))
		lpDDS_Blit = lpDDS_Back;

	if (Rend < 2)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (FAILED(lpDDS_Back->GetSurfaceDesc(&ddsd)))
			return Init_Fail(hWnd, "Error with lpDD_Back->GetSurfaceDesc !");

		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE | DDSD_PIXELFORMAT;
		ddsd.dwWidth = 336;
		ddsd.dwHeight = 240;
		if (ddsd.ddpfPixelFormat.dwRGBBitCount > 16) 
		{
			ddsd.lpSurface = &MD_Screen32[0];
			ddsd.lPitch = 336 * 4;
		}
		else 
		{
			ddsd.lpSurface = &MD_Screen[0];
			ddsd.lPitch = 336 * 2;
		}

		if (FAILED(lpDDS_Back->SetSurfaceDesc(&ddsd, 0)))
			return Init_Fail(hWnd, "Error with lpDD_Back->SetSurfaceDesc !");
	}

	// make sure Bits32 is correct (which it could easily not be at this point)
	{
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);

		if (FAILED(lpDDS_Blit->GetSurfaceDesc(&ddsd)))
			return Init_Fail(hWnd, "Error with lpDDS_Blit->GetSurfaceDesc !");

		Bits32 = (ddsd.ddpfPixelFormat.dwRGBBitCount > 16) ? 1 : 0;

		// also prevent the colors from sometimes being messed up for 1 frame if we changed color depth

		if(Bits32 && !oldBits32)
			for(int i = 0 ; i < 336 * 240 ; i++)
				MD_Screen32[i] = DrawUtil::Pix16To32(MD_Screen[i]);

		if(!Bits32 && oldBits32)
			for(int i = 0 ; i < 336 * 240 ; i++)
				MD_Screen[i] = DrawUtil::Pix32To16(MD_Screen32[i]);
	}

	// make sure the render mode is still valid (changing options in a certain order can make it invalid at this point)
	Set_Render(hWnd, Full_Screen, -1, false);

	// make sure the menu reflects the current mode (which it generally won't yet if we changed to/from 32-bit mode in this function)
	Build_Main_Menu();

	return 1;
}

void End_DDraw()
{
	if (lpDDC_Clipper)
	{
		lpDDC_Clipper->Release();
		lpDDC_Clipper = NULL;
	}

	if (lpDDS_Back)
	{
		lpDDS_Back->Release();
		lpDDS_Back = NULL;
	}

	if (lpDDS_Flip)
	{
		lpDDS_Flip->Release();
		lpDDS_Flip = NULL;
	}

	if (lpDDS_Primary)
	{
		lpDDS_Primary->Release();
		lpDDS_Primary = NULL;
	}

	if (lpDD)
	{
		lpDD->SetCooperativeLevel(HWnd, DDSCL_NORMAL);
		lpDD->Release();
		lpDD = NULL;
	}

	lpDDS_Blit = NULL;
}


HRESULT RestoreGraphics(HWND hWnd)
{
	HRESULT rval1 = lpDDS_Primary->Restore();
	HRESULT rval2 = lpDDS_Back->Restore();

	// Modif N. -- fixes lost surface handling when the color depth has changed
	if (rval1 == DDERR_WRONGMODE || rval2 == DDERR_WRONGMODE)
		return Init_DDraw(hWnd) ? DD_OK : DDERR_GENERIC;

	return SUCCEEDED(rval2) ? rval1 : rval2;
}


int Clear_Primary_Screen(HWND hWnd)
{
	if(!lpDD)
		return 0; // bail if directdraw hasn't been initialized yet or if we're still in the middle of initializing it

	DDSURFACEDESC2 ddsd;
	DDBLTFX ddbltfx;
	RECT RD;
	POINT p;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	memset(&ddbltfx, 0, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = 0;

	if (Full_Screen)
	{
		if (FS_VSync)
		{
			lpDDS_Flip->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
			lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);

			lpDDS_Flip->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
			lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);

			lpDDS_Flip->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
			lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
		}
		else lpDDS_Primary->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
	}
	else
	{
		p.x = p.y = 0;
		GetClientRect(hWnd, &RD);
		ClientToScreen(hWnd, &p);

		RD.left = p.x;
		RD.top = p.y;
		RD.right += p.x;
		RD.bottom += p.y;

		if (RD.top < RD.bottom)
			lpDDS_Primary->Blt(&RD, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
	}

	return 1;
}


int Clear_Back_Screen(HWND hWnd)
{
	if(!lpDD)
		return 0; // bail if directdraw hasn't been initialized yet or if we're still in the middle of initializing it

	DDSURFACEDESC2 ddsd;
	DDBLTFX ddbltfx;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	memset(&ddbltfx, 0, sizeof(ddbltfx));
	ddbltfx.dwSize = sizeof(ddbltfx);
	ddbltfx.dwFillColor = 0;

	lpDDS_Back->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);

	return 1;
}


void Restore_Primary(void)
{
	if (lpDD && Full_Screen && FS_VSync)
	{
		while (lpDDS_Primary->GetFlipStatus(DDGFS_ISFLIPDONE) == DDERR_SURFACEBUSY);
		lpDD->FlipToGDISurface();
	}
}

// Render_Mode is input, RectDest is input and output, everything else is output only
void CalculateDrawArea(int Render_Mode, RECT& RectDest, RECT& RectSrc, float& Ratio_X, float& Ratio_Y, int& Dep)
{
	Ratio_X = (float) RectDest.right / 320.0f;  //Upth-Modif - why use two lines of code
	Ratio_Y = (float) RectDest.bottom / 240.0f; //Upth-Modif - when you can do this?
	Ratio_X = Ratio_Y = (Ratio_X < Ratio_Y) ? Ratio_X : Ratio_Y; //Upth-Add - and here we floor the value

	POINT q; //Upth-Add - For determining the correct ratio
	q.x = RectDest.right; //Upth-Add - we need to get
	q.y = RectDest.bottom; //Upth-Add - the bottom-right corner

	if (Render_Mode < 2)
	{
		RectSrc.top = 0;
		RectSrc.bottom = VDP_Num_Vis_Lines;

		if ((VDP_Num_Vis_Lines == 224) && (Stretch == 0))
		{
			RectDest.top = (int) ((q.y - (224 * Ratio_Y))/2); //Upth-Modif - Centering the screen properly
			RectDest.bottom = (int) (224 * Ratio_Y) + RectDest.top; //Upth-Modif - along the y axis
		}
	}
	else
	{
		if (VDP_Num_Vis_Lines == 224)
		{
			RectSrc.top = 8 * 2;
			RectSrc.bottom = (224 + 8) * 2;

			if (Stretch == 0)
			{
				RectDest.top = (int) ((q.y - (224 * Ratio_Y))/2); //Upth-Modif - Centering the screen properly
				RectDest.bottom = (int) (224 * Ratio_Y) + RectDest.top; //Upth-Modif - along the y axis again
			}
		}
		else
		{
			RectSrc.top = 0; //Upth-Modif - Was "0 * 2"
			RectSrc.bottom = (240 * 2);
		}
	}

	if (IS_FULL_X_RESOLUTION)
	{
		Dep = 0;

		if (Render_Mode < 2)
		{
			RectSrc.left = 8 + 0 ;
			RectSrc.right = 8 + 320;
		}
		else
		{
			RectSrc.left = 0; //Upth-Modif - Was "0 * 2"
			RectSrc.right = 320 * 2;
		}
		RectDest.left = (int) ((q.x - (320 * Ratio_X))/2); //Upth-Add - center the picture
		RectDest.right = (int) (320 * Ratio_X) + RectDest.left; //Upth-Add - along the x axis
	}
	else // less-wide X resolution:
	{
		Dep = 64;

		if (Stretch == 0)
		{
			RectDest.left = (int) ((q.x - (ALT_X_RATIO_RES * Ratio_X))/2); //Upth-Modif - center the picture properly
			RectDest.right = (int) (ALT_X_RATIO_RES * Ratio_X) + RectDest.left; //Upth-Modif - along the x axis
		}

		if (Render_Mode < 2)
		{
			RectSrc.left = 8 + 0;
			RectSrc.right = 8 + 256;
		}
		else
		{
			RectSrc.left = 32 * 2;
			RectSrc.right = (256 * 2) + (32 * 2);
		}
	}
}

// text, recording status, etc.
void DrawInformationOnTheScreen()
{
	int i;
	int n,j,m,l[3]; //UpthModif - Added l[3] for use when displaying input
	char FCTemp[256],pos;
	static float FPS = 0.0f, frames[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	static unsigned int old_time = 0, view_fps = 0, index_fps = 0, freq_cpu[2] = {0, 0};
	unsigned int new_time[2];

	if (MainMovie.Status == MOVIE_RECORDING)
	{
		static unsigned int CircleFrameCount =0;
		if((CircleFrameCount++)%64 < 32)
		{
			m=0;
			n=70560+309;
			for(j=0;j<9;j++,n+=327)
			{
				for(i=0;i<9;i++,m++,n++)
				{
					if (RCircle[m]>0) MD_Screen[n]=RCircle[m];
					if (RCircle32[m]>0) MD_Screen32[n]=RCircle32[m];
				}
			}
		}
	}
#ifdef SONICSPEEDHACK 
	// SONIC THE HEDGEHOG SPEEDOMETER HACK
    {
		unsigned short SineTable[] = 
		{
			0x0000, 0x0006, 0x000C, 0x0012, 0x0019, 0x001F, 0x0025, 0x002B, 0x0031, 0x0038, 0x003E, 0x0044, 0x004A, 0x0050, 0x0056, 0x005C, 
			0x0061, 0x0067, 0x006D, 0x0073, 0x0078, 0x007E, 0x0083, 0x0088, 0x008E, 0x0093, 0x0098, 0x009D, 0x00A2, 0x00A7, 0x00AB, 0x00B0, 
			0x00B5, 0x00B9, 0x00BD, 0x00C1, 0x00C5, 0x00C9, 0x00CD, 0x00D1, 0x00D4, 0x00D8, 0x00DB, 0x00DE, 0x00E1, 0x00E4, 0x00E7, 0x00EA, 
			0x00EC, 0x00EE, 0x00F1, 0x00F3, 0x00F4, 0x00F6, 0x00F8, 0x00F9, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FE, 0x00FF, 0x00FF, 0x00FF, 
			0x0100, 0x00FF, 0x00FF, 0x00FF, 0x00FE, 0x00FE, 0x00FD, 0x00FC, 0x00FB, 0x00F9, 0x00F8, 0x00F6, 0x00F4, 0x00F3, 0x00F1, 0x00EE, 
			0x00EC, 0x00EA, 0x00E7, 0x00E4, 0x00E1, 0x00DE, 0x00DB, 0x00D8, 0x00D4, 0x00D1, 0x00CD, 0x00C9, 0x00C5, 0x00C1, 0x00BD, 0x00B9, 
			0x00B5, 0x00B0, 0x00AB, 0x00A7, 0x00A2, 0x009D, 0x0098, 0x0093, 0x008E, 0x0088, 0x0083, 0x007E, 0x0078, 0x0073, 0x006D, 0x0067, 
			0x0061, 0x005C, 0x0056, 0x0050, 0x004A, 0x0044, 0x003E, 0x0038, 0x0031, 0x002B, 0x0025, 0x001F, 0x0019, 0x0012, 0x000C, 0x0006, 
			0x0000, 0xFFFA, 0xFFF4, 0xFFEE, 0xFFE7, 0xFFE1, 0xFFDB, 0xFFD5, 0xFFCF, 0xFFC8, 0xFFC2, 0xFFBC, 0xFFB6, 0xFFB0, 0xFFAA, 0xFFA4, 
			0xFF9F, 0xFF99, 0xFF93, 0xFF8B, 0xFF88, 0xFF82, 0xFF7D, 0xFF78, 0xFF72, 0xFF6D, 0xFF68, 0xFF63, 0xFF5E, 0xFF59, 0xFF55, 0xFF50, 
			0xFF4B, 0xFF47, 0xFF43, 0xFF3F, 0xFF3B, 0xFF37, 0xFF33, 0xFF2F, 0xFF2C, 0xFF28, 0xFF25, 0xFF22, 0xFF1F, 0xFF1C, 0xFF19, 0xFF16, 
			0xFF14, 0xFF12, 0xFF0F, 0xFF0D, 0xFF0C, 0xFF0A, 0xFF08, 0xFF07, 0xFF05, 0xFF04, 0xFF03, 0xFF02, 0xFF02, 0xFF01, 0xFF01, 0xFF01, 
			0xFF00, 0xFF01, 0xFF01, 0xFF01, 0xFF02, 0xFF02, 0xFF03, 0xFF04, 0xFF05, 0xFF07, 0xFF08, 0xFF0A, 0xFF0C, 0xFF0D, 0xFF0F, 0xFF12, 
			0xFF14, 0xFF16, 0xFF19, 0xFF1C, 0xFF1F, 0xFF22, 0xFF25, 0xFF28, 0xFF2C, 0xFF2F, 0xFF33, 0xFF37, 0xFF3B, 0xFF3F, 0xFF43, 0xFF47, 
			0xFF4B, 0xFF50, 0xFF55, 0xFF59, 0xFF5E, 0xFF63, 0xFF68, 0xFF6D, 0xFF72, 0xFF78, 0xFF7D, 0xFF82, 0xFF88, 0xFF8B, 0xFF93, 0xFF99, 
			0xFF9F, 0xFFA4, 0xFFAA, 0xFFB0, 0xFFB6, 0xFFBC, 0xFFC2, 0xFFC8, 0xFFCF, 0xFFD5, 0xFFDB, 0xFFE1, 0xFFE7, 0xFFEE, 0xFFF4, 0xFFFA, 
			0x0000, 0x0006, 0x000C, 0x0012, 0x0019, 0x001F, 0x0025, 0x002B, 0x0031, 0x0038, 0x003E, 0x0044, 0x004A, 0x0050, 0x0056, 0x005C, 
			0x0061, 0x0067, 0x006D, 0x0073, 0x0078, 0x007E, 0x0083, 0x0088, 0x008E, 0x0093, 0x0098, 0x009D, 0x00A2, 0x00A7, 0x00AB, 0x00B0, 
			0x00B5, 0x00B9, 0x00BD, 0x00C1, 0x00C5, 0x00C9, 0x00CD, 0x00D1, 0x00D4, 0x00D8, 0x00DB, 0x00DE, 0x00E1, 0x00E4, 0x00E7, 0x00EA, 
			0x00EC, 0x00EE, 0x00F1, 0x00F3, 0x00F4, 0x00F6, 0x00F8, 0x00F9, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FE, 0x00FF, 0x00FF, 0x00FF
		};
		char temp[64]; //increased to prevent stack corruption
		extern unsigned char Ram_68k[64 * 1024];
		#ifdef SK //Sonic and Knuckles
		const unsigned char angle = (unsigned char)(Ram_68k[0xB027]);
		const unsigned char Char = (unsigned char) (Ram_68k[0xFF08]);
		bool Knuckles = Game ? (Char == 3) : 0;
		const unsigned int P1OFFSET = 0xB010;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned int SinTableOffset = 0x1D64; // sonic 1: 29F2 S1K: 2980 ... sonic 2: 33CE ... sonic 3: 1D64 //Where in the ROM is the sine table?
		bool Super = (Ram_68k[0xFE18])?true:false;
		const unsigned char XVo = 0x8;
		const unsigned char YVo = 0xA;
		const unsigned char GVo = 0xC;
		const unsigned char XPo = 0x0;
		const unsigned char YPo = 0x4;
		#elif defined S2 //Sonic 2
		bool Knuckles = Game ? (Game->Rom_Name[6] == '&') : 0; //Check's the ROM title to see if it's "SONIC & KNUCKLES"
		const unsigned char angle = (unsigned char)(Ram_68k[0xB027]);
		bool Super = (Ram_68k[0xFE18])?true:false;
		const unsigned int P1OFFSET = 0xB008;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned int SinTableOffset = Knuckles ? 0x1D64 : 0x33CE; // sonic 1: 29F2 S1K: 2980 ... sonic 2: 33CE ... sonic 3: 1D64 //Where in the ROM is the sine table?
		const unsigned char XVo = 0x8;
		const unsigned char YVo = 0xA;
		const unsigned char GVo = 0xC;
		const unsigned char XPo = 0x0;
		const unsigned char YPo = 0x4;
		#elif (defined S1) || (defined GAME_SCD) //Sonic 1
		bool Knuckles = Game ? !(strncmp("KNUCKLES",Game->Rom_Name,strlen("KNUCKLES"))) : 0; //Checks the Rom title to see if it's "KNUCKLES THE ECHIDNA IN - SONIC THE HEDGEHOG"
		const unsigned int P1OFFSET = 0xD008;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned int SinTableOffset = ((Knuckles) ? 0x2980: 0x29F2); // sonic 1: 29F2 S1K: 2980 ... sonic 2: 33CE ... sonic 3: 1D64 //Where in the ROM is the sine table?
		const unsigned char angle = (unsigned char)(Ram_68k[0xD027]);
		const bool Super = false;
		const unsigned char XVo = 0x8;
		const unsigned char YVo = 0xA;
		const unsigned char GVo = 0xC;
		const unsigned char XPo = 0x0;
		const unsigned char YPo = 0x4;
		#elif defined S1MM //Sonic 1
		const bool Knuckles = false; //Checks the Rom title to see if it's "KNUCKLES THE ECHIDNA IN - SONIC THE HEDGEHOG"
		bool Super = (Ram_68k[0xFE18])?true:false;
		const unsigned int P1OFFSET = 0xD008;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned int SinTableOffset = 0x2F4A	; // sonic 1: 29F2 S1K: 2980 ... sonic 2: 33CE ... sonic 3: 1D64 //Where in the ROM is the sine table?
		const unsigned char angle = (unsigned char)(Ram_68k[0xD027]);
		const unsigned char XVo = 0x8;
		const unsigned char YVo = 0xA;
		const unsigned char GVo = 0xC;
		const unsigned char XPo = 0x0;
		const unsigned char YPo = 0x4;
		#elif defined RISTAR
		const bool Knuckles = false;
		const unsigned int P1OFFSET = 0xC01A;// sonic 1: D008 ... sonic 2: B008 ... sonic 3: B010 // Where in RAM are these values stored?
		const unsigned int SinTableOffset = (0x663C); // sonic 1: 29F2 S1K: 2980 ... sonic 2: 33CE ... sonic 3: 1D64 //Where in the ROM is the sine table?
		const unsigned char angle = (unsigned char)(Ram_68k[0xC02F]);
		const unsigned char XVo = 0x2;
		const unsigned char YVo = 0x4;
		const unsigned char GVo = 0x0;
		const unsigned char XPo = 0x6;
		const unsigned char YPo = 0xA;
		#endif
		short xvel = (short)(Ram_68k[P1OFFSET + XVo] | (Ram_68k[P1OFFSET + XVo + 1] << 8)); // "*(short *) Ram_68k[P1OFFSET + 0x8]" would be just as effective, and more efficient
		short yvel = (short)(Ram_68k[P1OFFSET + YVo] | (Ram_68k[P1OFFSET + YVo + 1] << 8)); // but it works as is, and changing it involves modifications
		unsigned short xpos = (unsigned short)(Ram_68k[P1OFFSET + XPo] | (Ram_68k[P1OFFSET + XPo + 1] << 8)); // to many many lines which were this way already when we discovered
		unsigned short ypos = (unsigned short)(Ram_68k[P1OFFSET + YPo] | (Ram_68k[P1OFFSET  + YPo + 1] << 8)); // that Ram_68k is little endian to begin with
		unsigned char xspos = Ram_68k[P1OFFSET + XPo + 3]; // subpixel position
		unsigned char yspos = Ram_68k[P1OFFSET  + YPo + 3]; // subpixel position
//		short CamX[0] = *(short *) &Ram_68k[0xF700];
//		short CamY[0] = *(short *) &Ram_68k[0xF704];
		#ifndef RISTAR
		const unsigned char status = (unsigned char)(Ram_68k[P1OFFSET + 0x1B]); //live, dead, rolling, running, falling, jumping?
		const bool inair = (status & 0x02) ? true : false;
		const bool water = (status & 0x40) ? true : false;
		const short jumpspeed = ((Knuckles) ? (water ? 768 : 1536) : ((Super)?(water ? 1280 : 2048):(water ? 896 : 1664))); // this gets multiplied by a sine table value for jump prediction
		unsigned short SinOffset = (unsigned char) (angle - 0x40); // vertical sine ratio
		unsigned short CosOffset = (unsigned char) (angle); // horizontal sine ratio
		short xjumpvel = SineTable[CosOffset];
		short yjumpvel = SineTable[SinOffset];
		if (inair)
		{
			xjumpvel = xvel;
			for (yjumpvel = max(yvel,-1024); yjumpvel < 0; yjumpvel +=56) 
			{
				xjumpvel -= (xjumpvel / 32);
				if (!(status & 0x10) && !Controller_1_Left) xjumpvel = max(-1536,xjumpvel-24);
				if (!(status & 0x10) && !Controller_1_Right) xjumpvel = min(1536,xjumpvel+24);
			}
		}
		else
		{
			xjumpvel = ((xjumpvel * jumpspeed) >> 8) + xvel;
			yjumpvel = ((yjumpvel * jumpspeed) >> 8) + yvel;
		}
		#else
		const unsigned char status = (unsigned char)(Ram_68k[P1OFFSET + 0x17]); //live, dead, rolling, running, falling, jumping?
		unsigned short SinOffset,CosOffset;
		unsigned short Sin,Cos;
		short xjumpvel,yjumpvel;
		if (Ram_68k[0xC059] == 0x04)
		{
			SinOffset = (angle & 0xFF) * 2;
			CosOffset = SinOffset + 0x80;
			Sin = Rom_Data[SinOffset + SinTableOffset] | (Rom_Data[SinOffset + SinTableOffset + 1] << 8);
			Cos = Rom_Data[CosOffset + SinTableOffset] | (Rom_Data[CosOffset + SinTableOffset + 1] << 8);
			xjumpvel = (Sin * 0xF);
			yjumpvel = (Cos * 0xF);
		}
		else
		{
			SinOffset = (angle + (Ram_68k[0xE537] > 0)? 0x80 :0x40);
			SinOffset = (SinOffset & 0xFF) * 2;
			CosOffset = SinOffset + 0x80;
			Sin = Rom_Data[SinOffset + SinTableOffset] | (Rom_Data[SinOffset + SinTableOffset + 1] << 8);
			Cos = Rom_Data[CosOffset + SinTableOffset] | (Rom_Data[CosOffset + SinTableOffset + 1] << 8);
			xjumpvel = (Sin * 0x2A * Ram_68k[0xE537] * -1) >> 5;
			yjumpvel = (Cos * 0x2A * Ram_68k[0xE537] * -1) >> 5;
		}
		#endif
		if (!GetKeyState(VK_SCROLL))
		{
			sprintf(temp, "%d:%d",xvel,xjumpvel); // here we output all the values, framecounter style, in the top-right corner
			n=FRAME_COUNTER_TOP_RIGHT+4704;
			int prevn = n;
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n -= 336*7-6;
			}
			strcpy(temp,"");
			sprintf(temp,"%u:%03u",xpos,xspos);
			n=prevn+2352;
			prevn=n;
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}	
				}	
				n -= 336*7-6;
			}
			n=prevn+2352;
			prevn=n;
			strcpy(temp,"");
			sprintf(temp,"%d:%d",yvel,yjumpvel);
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n -= 336*7-6;
			}
			n=prevn+2352;
			prevn=n;
			strcpy(temp,"");
			sprintf(temp,"%u:%03u",ypos,yspos);
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n -= 336*7-6;
			}
			n=prevn+2352;
			prevn=n;
			strcpy(temp,"");
			sprintf(temp,"%d:%d:%05.2f"/**/,*((short *) (&Ram_68k[P1OFFSET + GVo])),(signed char)angle,((signed char)angle)*(90.0/64.0));
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n -= 336*7-6;
			}
	#ifdef S1
			n=prevn+2352;
			prevn=n;
			strcpy(temp,"");
	//		sprintf(temp,"%d",CalcFuckFrames());
			if (strlen(temp) > 8) n-=(6*(strlen(temp)-8));
			if (!IS_FULL_X_RESOLUTION)
				n-=64;	
			for(pos=0;pos<(int)strlen(temp);pos++)
			{
				m=(temp[pos]-'-')*30;
				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++];
							else MD_Screen[n]=FCDigit[m++];
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n -= 336*7-6;
			}
	#endif
		}
	}
#endif
	//Framecounter
	if(FrameCounterEnabled)
	{
		if(FrameCounterFrames) //Nitsuja added this option, raw integer frame number display
		{
			if(!MainMovie.File || MainMovie.Status == MOVIE_RECORDING)
				sprintf(FCTemp, "%d", FrameCount);
			else
				sprintf(FCTemp, "%d/%d", FrameCount, MainMovie.LastFrame);
		}
		else
		{
			const int fps = CPU_Mode ? 50 : 60;
			if(!MainMovie.File || MainMovie.Status == MOVIE_RECORDING)
				sprintf(FCTemp,"%d:%02d:%02d",(FrameCount/(fps*60)),(FrameCount/fps)%60,FrameCount%fps); //Nitsuja modified this to use the FPS value instead of 60 | Upth-Modif - Nitsuja forgot that even in PAL mode, it's 60 seconds per minute
			else
				sprintf(FCTemp,"%d:%02d:%02d/%d:%02d:%02d",(FrameCount/(fps*60)),(FrameCount/fps)%60,FrameCount%fps, (MainMovie.LastFrame/(fps*60)),(MainMovie.LastFrame/fps)%60,MainMovie.LastFrame%fps);
		}

		unsigned int slashPos = ~0;
		const char* slash = strchr(FCTemp, '/');
		if(slash)
			slashPos = slash - FCTemp;

		if(strlen(FCTemp)>0)
		{
			n=FrameCounterPosition;
			if (FrameCounterPosition==FRAME_COUNTER_TOP_RIGHT || FrameCounterPosition==FRAME_COUNTER_BOTTOM_RIGHT)
			{
				if (!IS_FULL_X_RESOLUTION)
					n-=64;
				int off = strlen(FCTemp) - 8;
				if (off > 0) n-=off*6;
			}
			BOOL drawRed = Lag_Frame;
			for(pos=0;(unsigned int)pos<strlen(FCTemp);pos++)
			{
				if((unsigned int)pos == slashPos)
					drawRed = (MainMovie.Status != MOVIE_PLAYING);

				m=(FCTemp[pos]-'-')*30;

				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++] & (drawRed?0xFF0000:0xFFFFFF);
							else MD_Screen[n]=FCDigit[m++] & (drawRed?0xF800:0xFFFF);
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n-=336*7-6;
			}
		}
	}
	//Lagcounter
	if(LagCounterEnabled)
	{
		if(LagCounterFrames) //Nitsuja added this option, raw integer Lag number display
		{
			sprintf(FCTemp,"%d",LagCount);
		}
		else
		{
			const int fps = CPU_Mode ? 50 : 60;
			sprintf(FCTemp,"%d:%02d:%02d",(LagCount/(fps*60)),(LagCount/fps)%60,LagCount%fps); //Nitsuja modified this to use the FPS value instead of 60 | Upth-Modif - Nitsuja forgot that even in PAL mode, it's 60 seconds per minute
		}

		if(strlen(FCTemp)>0)
		{
			n=FrameCounterPosition+2352;
			if (!IS_FULL_X_RESOLUTION && (FrameCounterPosition==FRAME_COUNTER_TOP_RIGHT || FrameCounterPosition==FRAME_COUNTER_BOTTOM_RIGHT))
				n-=64;
			for(pos=0;(unsigned int)pos<strlen(FCTemp);pos++)
			{
				m=(FCTemp[pos]-'-')*30;

				for(j=0;j<7;j++,n+=330)
				{
					for(i=0;i<6;i++,n++)
					{
						if(j>0 && j<6)
						{
							if (Bits32) MD_Screen32[n]=FCDigit32[m++] & 0xFF0000;
							else MD_Screen[n]=FCDigit[m++] & 0xF800;
						}
						else
						{
							if (Bits32) MD_Screen32[n]=0x0000;
							else MD_Screen[n]=0x0000;
						}
					}
				}
				n-=336*7-6;
			}
		}
	}
	//Show Input
	if(ShowInputEnabled)
	{

		strcpy(FCTemp,"[\\]^ABCSXYZM[\\]^ABCSXYZM[\\]^ABCSXYZM"); //upthmodif

		if(Controller_1_Up) FCTemp[0]='@';
		if(Controller_1_Down) FCTemp[1]='@';
		if(Controller_1_Left) FCTemp[2]='@';
		if(Controller_1_Right) FCTemp[3]='@';
		if(Controller_1_A) FCTemp[4]='@';
		if(Controller_1_B) FCTemp[5]='@';
		if(Controller_1_C) FCTemp[6]='@';
		if(Controller_1_Start) FCTemp[7]='@';
		if(Controller_1_X) FCTemp[8]='@';
		if(Controller_1_Y) FCTemp[9]='@';
		if(Controller_1_Z) FCTemp[10]='@';
		if(Controller_1_Mode) FCTemp[11]='@';
		//Upth - added 3 controller display support
		if (Controller_1_Type & 0x10) {
			if(Controller_1B_Up) FCTemp[12+0]='@';
			if(Controller_1B_Down) FCTemp[12+1]='@';
			if(Controller_1B_Left) FCTemp[12+2]='@';
			if(Controller_1B_Right) FCTemp[12+3]='@';
			if(Controller_1B_A) FCTemp[12+4]='@';
			if(Controller_1B_B) FCTemp[12+5]='@';
			if(Controller_1B_C) FCTemp[12+6]='@';
			if(Controller_1B_Start) FCTemp[12+7]='@';
			if(Controller_1B_X) FCTemp[12+8]='@';
			if(Controller_1B_Y) FCTemp[12+9]='@';
			if(Controller_1B_Z) FCTemp[12+10]='@';
			if(Controller_1B_Mode) FCTemp[12+11]='@';
		}
		else {
			if(Controller_2_Up) FCTemp[12+0]='@';
			if(Controller_2_Down) FCTemp[12+1]='@';
			if(Controller_2_Left) FCTemp[12+2]='@';
			if(Controller_2_Right) FCTemp[12+3]='@';
			if(Controller_2_A) FCTemp[12+4]='@';
			if(Controller_2_B) FCTemp[12+5]='@';
			if(Controller_2_C) FCTemp[12+6]='@';
			if(Controller_2_Start) FCTemp[12+7]='@';
			if(Controller_2_X) FCTemp[12+8]='@';
			if(Controller_2_Y) FCTemp[12+9]='@';
			if(Controller_2_Z) FCTemp[12+10]='@';
			if(Controller_2_Mode) FCTemp[12+11]='@';
		}
		if(Controller_1C_Up) FCTemp[12+12+0]='@';
		if(Controller_1C_Down) FCTemp[12+12+1]='@';
		if(Controller_1C_Left) FCTemp[12+12+2]='@';
		if(Controller_1C_Right) FCTemp[12+12+3]='@';
		if(Controller_1C_A) FCTemp[12+12+4]='@';
		if(Controller_1C_B) FCTemp[12+12+5]='@';
		if(Controller_1C_C) FCTemp[12+12+6]='@';
		if(Controller_1C_Start) FCTemp[12+12+7]='@';
		if(Controller_1C_X) FCTemp[12+12+8]='@';
		if(Controller_1C_Y) FCTemp[12+12+9]='@';
		if(Controller_1C_Z) FCTemp[12+12+10]='@';
		if(Controller_1C_Mode) FCTemp[12+12+11]='@';
		n=FrameCounterPosition-2352;
		if (!IS_FULL_X_RESOLUTION && (FrameCounterPosition==FRAME_COUNTER_TOP_RIGHT || FrameCounterPosition==FRAME_COUNTER_BOTTOM_RIGHT))
			n-=64;
		for(pos=0;pos<12;pos++) //upthmodif
		{
			l[0]=(FCTemp[pos]-'@')*20+420;
			l[1]=(FCTemp[pos+12]-'@')*20+420;
			l[2]=(FCTemp[pos+12+12]-'@')*20+420;

			for(j=0;j<7;j++,n+=332)
			{
				for(i=0;i<4;i++,n++)
				{
					if(j>0 && j<6) {
						if (Controller_1_Type & 0x10)
						{
							if (Bits32) MD_Screen32[n]=(FCDigit32[l[0]++] & FCColor32[0]) | (FCDigit32[l[1]++] & FCColor32[1]) | (FCDigit32[l[2]++] & FCColor32[2]); //Upthmodif - three player display support - three player mode
							else MD_Screen[n]=(FCDigit[l[0]++] & FCColor[Mode_555 & 1][0]) | (FCDigit[l[1]++] & FCColor[Mode_555 & 1][1]) | (FCDigit[l[2]++] & FCColor[Mode_555 & 1][2]); //Upthmodif - three player display support - three player mode
						}
						else 
						{
							if (Bits32) MD_Screen32[n]=(FCDigit32[l[0]++] & FCColor32[3]) | (FCDigit32[l[1]++] & FCColor32[4]); //Upthmodif - three player display support - three player mode
							else MD_Screen[n]=(FCDigit[l[0]++] & FCColor[Mode_555 & 1][3]) | (FCDigit[l[1]++] & FCColor[Mode_555 & 1][4]); //Upthmodif - three player display support - two player mode
						}
					}
					else
					{
						if (Bits32) MD_Screen32[n]=0x0000;
						else MD_Screen[n]=0x0000;
					}
				}
			}
			n-=336*7-4;
		}
	
	}

	//// original SONIC THE HEDGEHOG 3 SPEEDOMETER HACK
	//{
	//	char temp [128];
	//	extern unsigned char Ram_68k[64 * 1024];
	//	// sonic 1: D010 ... sonic 2: B010 ... sonic 3: B018
	//	short xvel = (short)(Ram_68k[0xB018] | (Ram_68k[0xB018 + 1] << 8));
	//	short yvel = (short)(Ram_68k[0xB01A] | (Ram_68k[0xB01A + 1] << 8));
	//	short xvel2 = (short)(Ram_68k[0xB062] | (Ram_68k[0xB062 + 1] << 8)); // tails
	//	short yvel2 = (short)(Ram_68k[0xB064] | (Ram_68k[0xB064 + 1] << 8));
	//	sprintf(temp, "(%d, %d) [%d, %d]", xvel,yvel, xvel2,yvel2);
	//	Message_Showed = 1;
	//	if(GetTickCount() > Info_Time)
	//	{
	//		Info_Time = 0x7fffffff;
	//		strcpy(Info_String, "");
	//	}
	//	char temp2 [1024];
	//	char* paren = strchr(Info_String, ']');
	//	strcpy(temp2, paren ? (paren+2) : Info_String);
	//	sprintf(Info_String, "%s %s", temp, temp2);
	//}

	if (Message_Showed)
	{
		if (GetTickCount() > Info_Time)
		{
			Message_Showed = 0;
			strcpy(Info_String, "");
		}
		else //Modif N - outlined message text, so it isn't unreadable on any background color:
		{
			if(!(Message_Style & TRANS))
			{
				int backColor = (((Message_Style & (BLEU|VERT|ROUGE)) == BLEU) ? ROUGE : BLEU) | (Message_Style & SIZE_X2) | TRANS;
				const static int xOffset [] = {-1,-1,-1,0,1,1,1,0};
				const static int yOffset [] = {-1,0,1,1,1,0,-1,-1};
				for(int i = 0 ; i < 8 ; i++)
					Print_Text(Info_String, strlen(Info_String), 10+xOffset[i], 210+yOffset[i], backColor);
			}
			Print_Text(Info_String, strlen(Info_String), 10, 210, Message_Style);
		}

	}
	else if (Show_FPS)
	{	
		if (freq_cpu[0] > 1)				// accurate timer ok
		{
			if (++view_fps >= 16)
			{
				QueryPerformanceCounter((union _LARGE_INTEGER *) new_time);

				if (new_time[0] != old_time)
				{					
					FPS = (float) (freq_cpu[0]) * 16.0f / (float) (new_time[0] - old_time);
					sprintf(Info_String, "%.1f", FPS);
				}
				else
				{
					sprintf(Info_String, "too much...");
				}

				old_time = new_time[0];
				view_fps = 0;
			}
		}
		else if (freq_cpu[0] == 1)			// accurate timer not supported
		{
			if (++view_fps >= 10)
			{
				new_time[0] = GetTickCount();
		
				if (new_time[0] != old_time) frames[index_fps] = 10000 / (float)(new_time[0] - old_time);
				else frames[index_fps] = 2000;

				index_fps++;
				index_fps &= 7;
				FPS = 0.0f;

				for(i = 0; i < 8; i++) FPS += frames[i];

				FPS /= 8.0f;
				old_time = new_time[0];
				view_fps = 0;
			}

			sprintf(Info_String, "%.1f", FPS);
		}
		else
		{
			QueryPerformanceFrequency((union _LARGE_INTEGER *) freq_cpu);
			if (freq_cpu[0] == 0) freq_cpu[0] = 1;

			sprintf(Info_String, "", FPS);
		}
		Print_Text(Info_String, strlen(Info_String), 10, 210, FPS_Style);
	}
}

int Flip(HWND hWnd)
{
	if(!lpDD)
		return 0; // bail if directdraw hasn't been initialized yet or if we're still in the middle of initializing it

	HRESULT rval = DD_OK;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(ddsd);
	RECT RectDest, RectSrc;
	POINT p;
	float Ratio_X, Ratio_Y;
	int Dep = 0;
	int bpp = Bits32 ? 4 : 2; // Modif N. -- added: bytes per pixel

	DrawInformationOnTheScreen(); // Modif N. -- moved all this stuff out to its own function

	if (Fast_Blur) Half_Blur();

	if (Full_Screen)
	{
		int FS_X,FS_Y; //Upth-Add - So we can set the fullscreen resolution to the current res without changing the value that gets saved to the config
		if (Res_X < (320 << (int) (Render_FS > 0))) Res_X = 320 << (int) (Render_FS > 0); //Upth-Add - Flooring the resolution to 320x240
	    if (Res_Y < (240 << (int) (Render_FS > 0))) Res_Y = 240 << (int) (Render_FS > 0); //Upth-Add - or 640x480, as appropriate
		if (FS_No_Res_Change) { //Upth-Add - If we didn't change resolution when we went Full Screen
			DEVMODE temp;
			EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&temp); //Upth-Add - Gets the current screen resolution
			FS_X = temp.dmPelsWidth;
			FS_Y = temp.dmPelsHeight;
		}
		else { //Upth-Add - Otherwise use the configured resolution values
			FS_X = Res_X; 
			FS_Y = Res_Y;
		}
		Ratio_X = (float) FS_X / 320.0f; //Upth-Add - Find the current size-ratio on the x-axis
		Ratio_Y = (float) FS_Y / 240.0f; //Upth-Add - Find the current size-ratio on the y-axis
		Ratio_X = Ratio_Y = (Ratio_X < Ratio_Y) ? Ratio_X : Ratio_Y; //Upth-Add - Floor them to the smaller value for correct ratio display
		
		if (IS_FULL_X_RESOLUTION)
		{
			if (Flag_Clr_Scr != 40)
			{
				Clear_Primary_Screen(hWnd);
				Clear_Back_Screen(hWnd);
				Flag_Clr_Scr = 40;
			}

			Dep = 0;
			RectSrc.left = 0 + 8;
			RectSrc.right = 320 + 8;
			RectDest.left = (int) ((FS_X - (320 * Ratio_X))/2); //Upth-Modif - Offset the left edge of the picture to the center of the screen
			RectDest.right = (int) (320 * Ratio_X) + RectDest.left; //Upth-Modif - Stretch the picture and move the right edge the same amount
		}
		else
		{
			if (Flag_Clr_Scr != 32)
			{
				Clear_Primary_Screen(hWnd);
				Clear_Back_Screen(hWnd);
				Flag_Clr_Scr = 32;
			}

			Dep = 64;
			RectSrc.left = 0 + 8;
			RectSrc.right = 256 + 8;

			if (Stretch)
			{
				RectDest.left = 0;
				RectDest.right = FS_X; //Upth-Modif - use the user configured value
				RectDest.top = 0;      //Upth-Add - also, if we have stretch enabled
				RectDest.bottom = FS_Y;//Upth-Add - we don't correct the screen ratio
			}
			else
			{
				RectDest.left = (int) ((FS_X - (ALT_X_RATIO_RES * Ratio_X))/2); //Upth-Modif - Centering the screen left-right
				RectDest.right = (int) (ALT_X_RATIO_RES * Ratio_X + RectDest.left); //Upth-modif - again
			}
			RectDest.top = (int) ((FS_Y - (240 * Ratio_Y))/2); //Upth-Add - Centers the screen top-bottom, in case Ratio_X was the floor.
		}

		if (Render_FS == 1) //Upth-Modif - If we're using the render "Double" we apply the stretching
		{
			if (Blit_Soft == 1)
			{
				rval = lpDDS_Blit->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

				if (FAILED(rval)) goto cleanup_flip;

				if (Render_FS == 0)
					Blit_FS((unsigned char *) ddsd.lpSurface + ((ddsd.lPitch * (240 - VDP_Num_Vis_Lines) >> 1) + Dep), ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, 32 + Dep * 2);
				else
					Blit_FS((unsigned char *) ddsd.lpSurface + ((ddsd.lPitch * ((240 - VDP_Num_Vis_Lines) >> 1) + Dep) << 1), ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, 32 + Dep * 2);

				lpDDS_Blit->Unlock(NULL);

				if (FS_VSync)
				{
					lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
				}
			}
			else
			{
				RectSrc.top = 0;
				RectSrc.bottom = VDP_Num_Vis_Lines;

				if ((VDP_Num_Vis_Lines == 224) && (Stretch == 0))
				{
					RectDest.top = (int) ((FS_Y - (224 * Ratio_Y))/2); //Upth-Modif - centering top-bottom
					RectDest.bottom = (int) (224 * Ratio_Y) + RectDest.top; //Upth-Modif - with the method I already described for left-right
				}
				else
				{
					RectDest.top = (int) ((FS_Y - (240 * Ratio_Y))/2); //Upth-Modif - centering top-bottom under other circumstances
					RectDest.bottom = (int) (240 * Ratio_Y) + RectDest.top; //Upth-Modif - using the same method
				}
				RectDest.left = (int) ((FS_X - (320 * Ratio_X))/2); //Upth-Add - Centering left-right
				RectDest.right = (int) (320 * Ratio_X) + RectDest.left; //Upth-Add - I wonder why I had to change the center-stuff three times...

				if (FS_VSync)
				{
					lpDDS_Flip->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
					lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
				}
				else
				{
					lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
//					lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, NULL, NULL);
				}
			}
		}
		else if (Render_FS == 0) //Upth-Add - If the render is "single" we don't stretch it
		{
			if (Blit_Soft == 1)
			{
				rval = lpDDS_Blit->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

				if (FAILED(rval)) goto cleanup_flip;

				if (Render_FS == 0)
					Blit_FS((unsigned char *) ddsd.lpSurface + ((ddsd.lPitch * (240 - VDP_Num_Vis_Lines) >> 1) + Dep), ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, 32 + Dep * 2);
				else
					Blit_FS((unsigned char *) ddsd.lpSurface + ((ddsd.lPitch * ((240 - VDP_Num_Vis_Lines) >> 1) + Dep) << 1), ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, 32 + Dep * 2);

				lpDDS_Blit->Unlock(NULL);

				if (FS_VSync)
				{
					lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
				}
			}
			else
			{
				RectSrc.top = 0;
				RectSrc.bottom = VDP_Num_Vis_Lines;

				if ((VDP_Num_Vis_Lines == 224) && (Stretch == 0))
				{
					RectDest.top = (int) ((FS_Y - 224)/2); //Upth-Add - But we still
					RectDest.bottom = 224 + RectDest.top;  //Upth-Add - center the screen
				}
				else
				{
					RectDest.top = (int) ((FS_Y - 240)/2); //Upth-Add - for both of the
					RectDest.bottom = 240 + RectDest.top;  //Upth-Add - predefined conditions
				}
				RectDest.left = (int) ((FS_X - 320)/2); //Upth-Add - and along the
				RectDest.right = 320 + RectDest.left;   //Upth-Add - x axis, also

				if (FS_VSync)
				{
					lpDDS_Flip->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
					lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
				}
				else
				{
					lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
//					lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, NULL, NULL);
				}
			}
		}
		else
		{
			LPDIRECTDRAWSURFACE4 curBlit = lpDDS_Blit;
			if(Correct_256_Aspect_Ratio)
				if(!IS_FULL_X_RESOLUTION)
					curBlit = lpDDS_Back; // have to use it or the aspect ratio will be way off

			rval = curBlit->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

			if (FAILED(rval)) goto cleanup_flip;

			Blit_FS((unsigned char *) ddsd.lpSurface + ddsd.lPitch * (240 - VDP_Num_Vis_Lines) + Dep * bpp, ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, (16 + Dep) * bpp);

			curBlit->Unlock(NULL);

			if (curBlit == lpDDS_Back) // note: this can happen in windowed fullscreen, or if Correct_256_Aspect_Ratio is defined and the current display mode is 256 pixels across
			{
				RectDest.left = 0;
				RectDest.top = 0;
				RectDest.right = GetSystemMetrics(SM_CXSCREEN); // not SM_XVIRTUALSCREEN since we only want the primary monitor if there's more than one
				RectDest.bottom = GetSystemMetrics(SM_CYSCREEN);

				CalculateDrawArea(Render_FS, RectDest, RectSrc, Ratio_X, Ratio_Y, Dep);

				if (FS_VSync)
				{
					int vb;
					lpDD->GetVerticalBlankStatus(&vb);
					if (!vb) lpDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
				}

				lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
			}
			else
			{
				if (FS_VSync)
				{
					lpDDS_Primary->Flip(NULL, DDFLIP_WAIT);
				}
			}
		}
	}
	else
	{
		GetClientRect(hWnd, &RectDest);
		CalculateDrawArea(Render_W, RectDest, RectSrc, Ratio_X, Ratio_Y, Dep);

		int Clr_Cmp_Val = IS_FULL_X_RESOLUTION ? 40 : 32;
		if (Flag_Clr_Scr != Clr_Cmp_Val)
		{
			Clear_Primary_Screen(hWnd);
			Clear_Back_Screen(hWnd);
			Flag_Clr_Scr = Clr_Cmp_Val;
		}

		if (Render_W >= 2)
		{
			rval = lpDDS_Blit->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

			if (FAILED(rval)) goto cleanup_flip;

			Blit_W((unsigned char *) ddsd.lpSurface + ddsd.lPitch * (240 - VDP_Num_Vis_Lines) + Dep * bpp, ddsd.lPitch, 320 - Dep, VDP_Num_Vis_Lines, (16 + Dep) * bpp);

			lpDDS_Blit->Unlock(NULL);
		}

		p.x = p.y = 0;
		ClientToScreen(hWnd, &p);

		RectDest.top += p.y; //Upth-Modif - this part moves the picture into the window
		RectDest.bottom += p.y; //Upth-Modif - I had to move it after all of the centering
		RectDest.left += p.x;   //Upth-Modif - because it modifies the values
		RectDest.right += p.x;  //Upth-Modif - that I use to find the center

		if (RectDest.top < RectDest.bottom)
		{
			if (W_VSync)
			{
				int vb;
				lpDD->GetVerticalBlankStatus(&vb);
				if (!vb) lpDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
			}

			rval = lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, DDBLT_WAIT | DDBLT_ASYNC, NULL);
//			rval = lpDDS_Primary->Blt(&RectDest, lpDDS_Back, &RectSrc, NULL, NULL);
		}
	}

cleanup_flip:
	if (rval == DDERR_SURFACELOST)
		rval = RestoreGraphics(hWnd);

	return 1;
}


int Update_Gens_Logo(HWND hWnd)
{
	int i, j, m, n;
	static short tab[64000], Init = 0;
	static float renv = 0, ang = 0, zoom_x = 0, zoom_y = 0, pas;
	unsigned short c;

	if (!Init)
	{
		HBITMAP Logo;

		Logo = LoadBitmap(ghInstance,  MAKEINTRESOURCE(IDB_LOGO_BIG));
		GetBitmapBits(Logo, 64000 * 2, tab);
		pas = 0.05f;
		Init = 1;
	}

	renv += pas;
	zoom_x = sin(renv);
	if (zoom_x == 0.0f) zoom_x = 0.0000001f;
	zoom_x = (1 / zoom_x) * 1;
	zoom_y = 1;

	if (IS_FULL_X_RESOLUTION)
	{
		for(j = 0; j < 240; j++)
		{
			for(i = 0; i < 320; i++)
			{
				m = (int)((float)(i - 160) * zoom_x);
				n = (int)((float)(j - 120) * zoom_y);

				if ((m < 130) && (m >= -130) && (n < 90) && (n >= -90))
				{
					c = tab[m + 130 + (n + 90) * 260];
					if ((c > 31) || (c < 5)) MD_Screen[TAB336[j] + i + 8] = c;
				}
			}
		}
	}
	else
	{
		for(j = 0; j < 240; j++)
		{
			for(i = 0; i < 256; i++)
			{
				m = (int)((float)(i - 128) * zoom_x);
				n = (int)((float)(j - 120) * zoom_y);

				if ((m < 130) && (m >= -130) && (n < 90) && (n >= -90))
				{
					c = tab[m + 130 + (n + 90) * 260];
					if ((c > 31) || (c < 5)) MD_Screen[TAB336[j] + i + 8] = c;
				}
			}
		}
	}

	Half_Blur();
	Flip(hWnd);

	return 1;
}


int Update_Crazy_Effect(HWND hWnd)
{
	int i, j, offset;
	int r = 0, v = 0, b = 0, prev_l, prev_p;
	int RB, G;

 	for(offset = 336 * 240, j = 0; j < 240; j++)
	{
		for(i = 0; i < 336; i++, offset--)
		{
			prev_l = MD_Screen32[offset - 336];
			prev_p = MD_Screen32[offset - 1];

			RB = ((prev_l & 0xFF00FF) + (prev_p & 0xFF00FF)) >> 1;
			G = ((prev_l & 0x00FF00) + (prev_p & 0x00FF00)) >> 1;

			if (Effect_Color & 0x4)
			{
				r = RB & 0xFF0000;
				if (rand() > 0x1600) r += 0x010000;
				else r -= 0x010000;
				if (r > 0xFF0000) r = 0xFF0000;
				else if (r < 0x00FFFF) r = 0;
			}

			if (Effect_Color & 0x2)
			{
				v = G & 0x00FF00;
				if (rand() > 0x1600) v += 0x000100;
				else v -= 0x000100;
				if (v > 0xFF00) v = 0xFF00;
				else if (v < 0x00FF) v = 0;
			}

			if (Effect_Color & 0x1)
			{
				b = RB & 0xFF;
				if (rand() > 0x1600) b++;
				else b--;
				if (b > 0xFF) b = 0xFF;
				else if (b < 0) b = 0;
			}
			MD_Screen32[offset] = r + v + b;
			prev_l = MD_Screen[offset - 336];
			prev_p = MD_Screen[offset - 1];

			if (Mode_555 & 1)
			{
				RB = ((prev_l & 0x7C1F) + (prev_p & 0x7C1F)) >> 1;
				G = ((prev_l & 0x03E0) + (prev_p & 0x03E0)) >> 1;

				if (Effect_Color & 0x4)
				{
					r = RB & 0x7C00;
					if (rand() > 0x2C00) r += 0x0400;
					else r -= 0x0400;
					if (r > 0x7C00) r = 0x7C00;
					else if (r < 0x0400) r = 0;
				}

				if (Effect_Color & 0x2)
				{
					v = G & 0x03E0;
					if (rand() > 0x2C00) v += 0x0020;
					else v -= 0x0020;
					if (v > 0x03E0) v = 0x03E0;
					else if (v < 0x0020) v = 0;
				}

				if (Effect_Color & 0x1)
				{
					b = RB & 0x001F;
					if (rand() > 0x2C00) b++;
					else b--;
					if (b > 0x1F) b = 0x1F;
					else if (b < 0) b = 0;
				}
			}
			else
			{
				RB = ((prev_l & 0xF81F) + (prev_p & 0xF81F)) >> 1;
				G = ((prev_l & 0x07C0) + (prev_p & 0x07C0)) >> 1;

				if (Effect_Color & 0x4)
				{
					r = RB & 0xF800;
					if (rand() > 0x2C00) r += 0x0800;
					else r -= 0x0800;
					if (r > 0xF800) r = 0xF800;
					else if (r < 0x0800) r = 0;
				}

				if (Effect_Color & 0x2)
				{
					v = G & 0x07C0;
					if (rand() > 0x2C00) v += 0x0040;
					else v -= 0x0040;
					if (v > 0x07C0) v = 0x07C0;
					else if (v < 0x0040) v = 0;
				}

				if (Effect_Color & 0x1)
				{
					b = RB & 0x001F;
					if (rand() > 0x2C00) b++;
					else b--;
					if (b > 0x1F) b = 0x1F;
					else if (b < 0) b = 0;
				}
			}

			MD_Screen[offset] = r + v + b;
		}
	}

	Flip(hWnd);

	return 1;
}

void UpdateInput()
{
	if(MainMovie.Status==MOVIE_PLAYING)
		MoviePlayingStuff();
	else
		Update_Controllers();

	//// XXX - probably most people won't need to use this?
	////Modif N - playback one player while recording another player
	//if(MainMovie.Status==MOVIE_RECORDING)
	//{
	//	if(GetKeyState(VK_CAPITAL) != 0)
	//	{
	//		extern void MoviePlayPlayer2();
	//		MoviePlayPlayer2();
	//	}
	//	if(GetKeyState(VK_NUMLOCK) != 0)
	//	{
	//		extern void MoviePlayPlayer1();
	//		MoviePlayPlayer1();
	//	}
	//}
}

void UpdateLagCount()
{
	if (Lag_Frame)
	{
		LagCount++;
		LagCountPersistent++;
	}

	// catch-all to fix problem with sound stuttered when paused during frame skipping
	// looks out of place but maybe this function should be renamed
	extern bool soundCleared;
	soundCleared = false;
}


int Dont_Skip_Next_Frame = 0;
void Prevent_Next_Frame_Skipping()
{
	Dont_Skip_Next_Frame = 8;
}


int Update_Emulation(HWND hWnd)
{
	int prevFrameCount = FrameCount;

	static int Over_Time = 0;
	int current_div;

	int Temp_Frame_Skip = Frame_Skip; //Modif N - part of a quick hack to make Tab the fast-forward key
	if(FastForwardKeyDown && (GetActiveWindow()==hWnd || BackgroundInput))
		Temp_Frame_Skip = 8;
	if (SeekFrame) // If we've set a frame to seek to
	{
		if(FrameCount < (SeekFrame - 5000))      // we fast forward
			Temp_Frame_Skip = 10;
		else if (FrameCount < (SeekFrame - 100)) // at a variable rate
			Temp_Frame_Skip = 0;
		else 						     // based on distance to target
			Temp_Frame_Skip = -1;
		if(((SeekFrame - FrameCount) == 1))
		{
			Paused = 1; //then pause when we arrive
			SeekFrame = 0; //and clear the seek target
			Put_Info("Seek complete. Paused",1500);
			Clear_Sound_Buffer();
			MustUpdateMenu = 1;
		}
	}
	if (Temp_Frame_Skip != -1)
	{
		if (AVIRecording && CleanAvi && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
			Temp_Frame_Skip = 0;
		if (Sound_Enable)
		{
			WP = (WP + 1) & (Sound_Segs - 1);

			if (WP == Get_Current_Seg())
				WP = (WP + Sound_Segs - 1) & (Sound_Segs - 1);

			Write_Sound_Buffer(NULL);
		}
	    if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
		{
			if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
		}
		UpdateInput(); //Modif N
		if(MainMovie.Status==MOVIE_RECORDING)	//Modif
			MovieRecordingStuff();
		FrameCount++; //Modif

		if (Frame_Number++ < Temp_Frame_Skip) //Modif N - part of a quick hack to make Tab the fast-forward key
		{
			Lag_Frame = 1;
			Update_Frame_Fast_Hook();
			Update_RAM_Cheats();
			UpdateLagCount();
		
		}
		else
		{
			Frame_Number = 0;
			Lag_Frame = 1;
			Update_Frame_Hook();
			Update_RAM_Cheats();
			UpdateLagCount();
			DUMPAVICLEAN
			Flip(hWnd);
		}
	}
	else
	{
		if (Sound_Enable)
		{
			while (WP == Get_Current_Seg()) Sleep(Sleep_Time);
			RP = Get_Current_Seg();
			while (WP != RP)
			{
				Write_Sound_Buffer(NULL);
				WP = (WP + 1) & (Sound_Segs - 1);
				if(SlowDownMode)
				{
					if(SlowFrame)
					{
						SlowFrame--;
					}
					else
					{
						SlowFrame=SlowDownSpeed;
						UpdateInput(); //Modif N
						if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
						{
							if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
						}
						if(MainMovie.Status==MOVIE_RECORDING)	//Modif
							MovieRecordingStuff();
						FrameCount++; //Modif

						// note: we check for RamSearchHWnd because if it's open then it's likely causing most of any slowdown we get, in which case skipping renders will only make the slowdown appear worse
						if (WP != RP && AVIRecording==0 && Never_Skip_Frame==0 && !Dont_Skip_Next_Frame && !(RamSearchHWnd || RamWatchHWnd))
						{
							Lag_Frame = 1;
							Update_Frame_Fast_Hook();
							Update_RAM_Cheats();
							UpdateLagCount();
						}
						else
						{
							Lag_Frame = 1;
							Update_Frame_Hook();
							Update_RAM_Cheats();
							UpdateLagCount();
							DUMPAVICLEAN
							Flip(hWnd);
						}
					}
				}
				else
				{
					if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
					{
						if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
					}
					UpdateInput(); //Modif N
					if(MainMovie.Status==MOVIE_RECORDING)	//Modif
						MovieRecordingStuff();
					FrameCount++; //Modif

					if (WP != RP && AVIRecording==0 && Never_Skip_Frame==0 && !Dont_Skip_Next_Frame && !(RamSearchHWnd || RamWatchHWnd))
					{
						Lag_Frame = 1;
						Update_Frame_Fast_Hook();
						Update_RAM_Cheats();
						UpdateLagCount();					
					}
					else
					{
						Lag_Frame = 1;
						Update_Frame_Hook();
						Update_RAM_Cheats();
						UpdateLagCount();
						DUMPAVICLEAN
						Flip(hWnd);
					}
				}
				if (Never_Skip_Frame || Dont_Skip_Next_Frame)
					WP=RP;
			}
		}
		else
		{
			if (CPU_Mode) current_div = 20;
			else current_div = 16 + (Over_Time ^= 1);

			if(SlowDownMode)
				current_div*=(SlowDownSpeed+1);

			New_Time = GetTickCount();
			Used_Time += (New_Time - Last_Time);
			Frame_Number = Used_Time / current_div;
			Used_Time %= current_div;
			Last_Time = New_Time;
			
			if (Frame_Number > 8) Frame_Number = 8;
			if((Never_Skip_Frame!=0 || Dont_Skip_Next_Frame) && Frame_Number>0)
				Frame_Number=1;
			/*if(SlowDownMode)
			{
				if(SlowFrame)
				{
					Frame_Number>>=1; //Modif
					SlowFrame--;
				}
				else
					SlowFrame=SlowDownSpeed;
			}*/

			for (; Frame_Number > 1; Frame_Number--)
			{
				if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
					if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
				UpdateInput(); //Modif N
				if(MainMovie.Status==MOVIE_RECORDING)	//Modif
					MovieRecordingStuff();
				FrameCount++; //Modif

				if(AVIRecording==0 && Never_Skip_Frame==0 && !Dont_Skip_Next_Frame)
				{
					Lag_Frame = 1;
					Update_Frame_Fast_Hook();
					Update_RAM_Cheats();
					UpdateLagCount();
				
				}
				else
				{
					Lag_Frame = 1;
					Update_Frame_Hook();
					Update_RAM_Cheats();
					UpdateLagCount();
					DUMPAVICLEAN
					Flip(hWnd);
				}
			}

			if (Frame_Number)
			{
				if(AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
					if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
				UpdateInput(); //Modif N
				if(MainMovie.Status==MOVIE_RECORDING)	//Modif
					MovieRecordingStuff();
				FrameCount++; //Modif
				Lag_Frame = 1;
				Update_Frame_Hook();
				Update_RAM_Cheats();
				UpdateLagCount();
				DUMPAVICLEAN
				Flip(hWnd);
			}
			else Sleep(Sleep_Time);
		}
	}

	if(Dont_Skip_Next_Frame)
		Dont_Skip_Next_Frame--;

	return prevFrameCount != FrameCount;
}

void Update_Emulation_One_Before(HWND hWnd)
{
	if (Sound_Enable)
	{
		WP = (WP + 1) & (Sound_Segs - 1);

		if (WP == Get_Current_Seg())
			WP = (WP + Sound_Segs - 1) & (Sound_Segs - 1);

		Write_Sound_Buffer(NULL);
	}
	UpdateInput(); //Modif N
	if(MainMovie.Status==MOVIE_RECORDING)	//Modif
		MovieRecordingStuff();
	if (AVIRecording!=0 && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
	{
		if (!CleanAvi) Save_Shot_AVI(Bits32?(unsigned char *) MD_Screen32:(unsigned char *) MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION,hWnd);
	}
	FrameCount++; //Modif
	Lag_Frame = 1;
}

void Update_Emulation_One_Before_Minimal()
{
	UpdateInput();
	if(MainMovie.Status==MOVIE_RECORDING)
		MovieRecordingStuff();
	FrameCount++;
	Lag_Frame = 1;
}

void Update_Emulation_One_After(HWND hWnd)
{
	Update_RAM_Cheats();
	UpdateLagCount();
	DUMPAVICLEAN
	Flip(hWnd);
}

void Update_Emulation_After_Fast(HWND hWnd)
{
	Update_RAM_Cheats();
	UpdateLagCount();

	int Temp_Frame_Skip = Frame_Skip;
	if(FastForwardKeyDown && (GetActiveWindow()==hWnd || BackgroundInput))
		Temp_Frame_Skip = 8;
	if (AVIRecording && CleanAvi && (AVIWaitMovie == 0 || MainMovie.Status==MOVIE_PLAYING || MainMovie.Status==MOVIE_FINISHED))
		Temp_Frame_Skip = 0;
	if (Frame_Number > 8) Frame_Number = 8;
	if (Frame_Number++ < Temp_Frame_Skip)
		return;
	Frame_Number = 0;

	DUMPAVICLEAN
	Flip(hWnd);
}

void Update_Emulation_After_Controlled(HWND hWnd, bool flip)
{
	Update_RAM_Cheats();
	UpdateLagCount();

	if(flip)
	{
		DUMPAVICLEAN
		Flip(hWnd);
	}
}


int Update_Emulation_One(HWND hWnd)
{
	Update_Emulation_One_Before(hWnd);
	Update_Frame_Hook();
	Update_Emulation_One_After(hWnd);
	return 1;
}



int Update_Emulation_Netplay(HWND hWnd, int player, int num_player)
{
	static int Over_Time = 0;
	int current_div;

	if (CPU_Mode) current_div = 20;
	else current_div = 16 + (Over_Time ^= 1);

	New_Time = GetTickCount();
	Used_Time += (New_Time - Last_Time);
	Frame_Number = Used_Time / current_div;
	Used_Time %= current_div;
	Last_Time = New_Time;

	if (Frame_Number > 6) Frame_Number = 6;

	for (; Frame_Number > 1; Frame_Number--)
	{
		if (Sound_Enable)
		{
			if (WP == Get_Current_Seg()) WP = (WP - 1) & (Sound_Segs - 1);
			Write_Sound_Buffer(NULL);
			WP = (WP + 1) & (Sound_Segs - 1);
		}

		Scan_Player_Net(player);
		if (Kaillera_Error != -1) Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		//Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		if(MainMovie.Status==MOVIE_PLAYING)
			MoviePlayingStuff();
		else
			Update_Controllers_Net(num_player);
		if(MainMovie.Status==MOVIE_RECORDING)	//Modif
			MovieRecordingStuff();
		FrameCount++;
		Lag_Frame = 1;
		Update_Frame_Fast_Hook();
		Update_RAM_Cheats();
		UpdateLagCount();
	
	}

	if (Frame_Number)
	{
		if (Sound_Enable)
		{
			if (WP == Get_Current_Seg()) WP = (WP - 1) & (Sound_Segs - 1);
			Write_Sound_Buffer(NULL);
			WP = (WP + 1) & (Sound_Segs - 1);
		}

		Scan_Player_Net(player);
		if (Kaillera_Error != -1) Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		//Kaillera_Error = Kaillera_Modify_Play_Values((void *) (Kaillera_Keys), 2);
		if(MainMovie.Status==MOVIE_PLAYING)
			MoviePlayingStuff();
		else
			Update_Controllers_Net(num_player);
		if(MainMovie.Status==MOVIE_RECORDING)	//Modif
			MovieRecordingStuff();
		FrameCount++;
		Lag_Frame = 1;
		Update_Frame_Hook();
		Update_RAM_Cheats();
		UpdateLagCount();
	
		Flip(hWnd);
	}

	return 1;
}


int Eff_Screen(void)
{
	int i;

	for(i = 0; i < 336 * 240; i++)
	{
		MD_Screen[i] = 0;
		MD_Screen32[i] = 0;
	}

	return 1;
}


int Pause_Screen(void)
{
	int i, j, offset;
	int r, v, b, nr, nv, nb;

	if(Disable_Blue_Screen) return 1;

	r = v = b = nr = nv = nb = 0;

	for(offset = j = 0; j < 240; j++)
	{
		for(i = 0; i < 336; i++, offset++)
		{
			r = (MD_Screen32[offset] & 0xFF0000) >> 16;
			v = (MD_Screen32[offset] & 0x00FF00) >> 8;
			b = (MD_Screen32[offset] & 0x0000FF);

			nr = nv = nb = (r + v + b) / 3;
			
			if ((nb <<= 1) > 255) nb = 255;

			nr &= 0xFE;
			nv &= 0xFE;
			nb &= 0xFE;

			MD_Screen32[offset] = (nr << 16) + (nv << 8) + nb;

			if (Mode_555 & 1)
			{
				r = (MD_Screen[offset] & 0x7C00) >> 10;
				v = (MD_Screen[offset] & 0x03E0) >> 5;
				b = (MD_Screen[offset] & 0x001F);
			}
			else
			{
				r = (MD_Screen[offset] & 0xF800) >> 11;
				v = (MD_Screen[offset] & 0x07C0) >> 6;
				b = (MD_Screen[offset] & 0x001F);
			}

			nr = nv = nb = (r + v + b) / 3;
			
			if ((nb <<= 1) > 31) nb = 31;

			nr &= 0x1E;
			nv &= 0x1E;
			nb &= 0x1E;

			if (Mode_555 & 1)
				MD_Screen[offset] = (nr << 10) + (nv << 5) + nb;
			else
				MD_Screen[offset] = (nr << 11) + (nv << 6) + nb;
		}
	}

	return 1;
}

int Show_Genesis_Screen(HWND hWnd)
{
	Do_VDP_Only();
	Flip(hWnd);

	return 1;
}
int Show_Genesis_Screen()
{
	return Show_Genesis_Screen(HWnd);
}

int Take_Shot()
{
	return Save_Shot(Bits32?(unsigned char*)MD_Screen32:(unsigned char*)MD_Screen,(Mode_555 & 1) | (Bits32?2:0),IS_FULL_X_RESOLUTION,IS_FULL_Y_RESOLUTION);
}







/*
void MP3_init_test()
{
	FILE *f;

	f = fopen("\\vc\\gens\\release\\test.mp3", "rb");
	
	if (f == NULL) return;
	
	MP3_Test(f);

	Play_Sound();
}


void MP3_update_test()
{
	int *buf[2];
		
	buf[0] = Seg_L;
	buf[1] = Seg_R;

	while (WP == Get_Current_Seg());
			
	RP = Get_Current_Seg();

	while (WP != RP)
	{
		Write_Sound_Buffer(NULL);
		WP = (WP + 1) & (Sound_Segs - 1);

		memset(Seg_L, 0, (Seg_Length << 2));
		memset(Seg_R, 0, (Seg_Length << 2));
		MP3_Update(buf, Seg_Length);
	}
}
*/
