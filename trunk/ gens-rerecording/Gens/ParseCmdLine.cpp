#include <windows.h>
#include "rom.h"

char Str_Tmpy[1024];

void ParseCmdLine(LPSTR lpCmdLine, HWND HWnd)
{

	if (lpCmdLine[0])
	{
		int src;

#ifdef CC_SUPPORT
//		src = CC_Connect("CCGEN://Stef:gens@emu.consoleclassix.com/sonicthehedgehog2.gen", (char *) Rom_Data, CC_End_Callback);
		src = CC_Connect(lpCmdLine, (char *) Rom_Data, CC_End_Callback);

		if (src == 0)
		{
			Load_Rom_CC(CCRom.RName, CCRom.RSize);
			Build_Main_Menu();
		}
		else if (src == 1)
		{
			MessageBox(HWnd, "Error during connection", NULL, MB_OK);
		}
		else if (src == 2)
		{
#endif
		src = 0;
		
		if (lpCmdLine[src] == '"')
		{
			src++;
			
			while ((lpCmdLine[src] != '"') && (lpCmdLine[src] != 0))
			{
				Str_Tmpy[src - 1] = lpCmdLine[src];
				src++;
			}

			Str_Tmpy[src - 1] = 0;
		}
		else
		{
			while (lpCmdLine[src] != 0)
			{
				Str_Tmpy[src] = lpCmdLine[src];
				src++;
			}

			Str_Tmpy[src] = 0;
		}

		Pre_Load_Rom(HWnd, Str_Tmpy);

#ifdef CC_SUPPORT
		}
#endif
	}
}