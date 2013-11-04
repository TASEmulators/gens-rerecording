#include "resource.h"
#include "gens.h"
#include "mem_m68k.h"
#include "mem_s68k.h"
#include "mem_sh2.h"
#include "G_main.h"
#include "G_ddraw.h"
#include "G_dsound.h"
#include "ram_search.h"
#include "hexeditor.h"
#include "ramwatch.h"
#include "luascript.h"
#include <assert.h>
#include <commctrl.h>
#include <list>
#include <vector>

#ifdef _WIN32
   #include "BaseTsd.h"
   typedef INT_PTR intptr_t;
#else
   #include "stdint.h"
#endif

HWND HexEditorHWnd;
HDC HexDC;

struct HexParameters
{
	bool FontBold;
	unsigned int
		FontHeight, FontWidth, FontWeight,
		GapFontH, GapFontV, GapHeaderH, GapHeaderV,
		CellHeight, CellWidth,
		DialogPosX, DialogPosY, DialogSizeX, DialogSizeY,
		OffsetVisibleFirst, OffsetVisibleLast, OffsetVisibleTotal,
		AddressSelectedFirst, AddressSelectedLast, AddressSelectedTotal,
		MemoryRegion;
	COLORREF ColorFont, ColorBG, ColorSelection;
}
Hex =
{
	0, 15, Hex.FontHeight / 2, Hex.FontBold ? 600 : 400,					// font
	8, 0, Hex.FontWidth * 8, Hex.FontHeight + Hex.GapFontV,					// gaps
	Hex.FontHeight + Hex.GapFontV, Hex.FontWidth * 2 + Hex.GapFontH,		// cell size
	0, 0, Hex.CellWidth * 17 + Hex.GapHeaderH, (Hex.CellHeight + 2) * 17,	// dialog pos and size
	0, 0, Hex.OffsetVisibleLast - Hex.OffsetVisibleFirst	,					// visible offsets
	0, 0, Hex.AddressSelectedLast - Hex.AddressSelectedFirst,				// selected addresses
	0xff0000,																// memory region
	0x00000000, 0x00ffffff, 0x00ffdc00,										// colors
};

HFONT HexFont = CreateFont(
	Hex.FontHeight,		// height
	Hex.FontWidth,		// width
	0,					// escapement
	0,					// orientation
	Hex.FontWeight,		// weight
	FALSE,				// italic
	FALSE,				// underline
	FALSE,				// strikeout
	ANSI_CHARSET,		// charset
	OUT_DEVICE_PRECIS,	// precision
	CLIP_MASK,			// clipping
	DEFAULT_QUALITY,	// quality
	DEFAULT_PITCH,		// pitch
	"Courier New"		// name
);

void UpdateCaption()
{/*
	static char str[1000];

	if (CursorEndAddy == -1)
	{
		if (EditingMode == MODE_NES_FILE)
		{
			if (CursorStartAddy < 16)
				sprintf(str, "Hex Editor - ROM Header Offset 0x%06x", CursorStartAddy);
			else if (CursorStartAddy - 16 < (int)PRGsize[0])
				sprintf(str, "Hex Editor - (PRG) ROM Offset 0x%06x", CursorStartAddy);
			else if (CursorStartAddy - 16 - PRGsize[0] < (int)CHRsize[0])
				sprintf(str, "Hex Editor - (CHR) ROM Offset 0x%06x", CursorStartAddy);
		} else
		{
			sprintf(str, "Hex Editor - %s Offset 0x%06x", EditString[EditingMode], CursorStartAddy);
		}

		if (EditingMode == MODE_NES_MEMORY && symbDebugEnabled)
		{
			// when watching RAM we may as well see Symbolic Debug names
			Name* node = findNode(getNamesPointerForAddress(CursorStartAddy), CursorStartAddy);
			if (node)
			{
				strcat(str, " - ");
				strcat(str, node->name);
			}
		}
	} else
	{
		sprintf(str, "Hex Editor - %s Offset 0x%06x - 0x%06x, 0x%x bytes selected ",
			EditString[EditingMode], CursorStartAddy, CursorEndAddy, CursorEndAddy - CursorStartAddy + 1);
	}*/
	SetWindowText(HexEditorHWnd, "LOL");
	return;
}

void UpdateHexEditor()
{
	InvalidateRect(HexEditorHWnd, NULL, FALSE);
}

void KillHexEditor()
{
	DialogsOpen--;	
	ReleaseDC(HexEditorHWnd, HexDC);
	DestroyWindow(HexEditorHWnd);
	UnregisterClass("HEXEDITOR", ghInstance);
	HexEditorHWnd = 0;
	return;
}

LRESULT CALLBACK HexEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
//	RECT r2;
	PAINTSTRUCT ps;
	SCROLLINFO si;
//	int dx1, dy1, dx2, dy2;
//	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_CREATE:
		{
			HexDC = GetDC(hDlg);
			SelectObject(HexDC, HexFont);
			SetTextAlign(HexDC, TA_UPDATECP | TA_TOP | TA_LEFT);
			
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);
			Hex.DialogPosX = r.right;
			Hex.DialogPosY = r.top;

			SetWindowPos(
				hDlg,
				NULL,
				Hex.DialogPosX,
				Hex.DialogPosY,
				Hex.DialogSizeX,
				Hex.DialogSizeY,
				SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
			);
			
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof(si);
			si.fMask  = SIF_RANGE | SIF_PAGE;
			si.nMin   = 0;
			si.nMax   = _68K_RAM_SIZE / 16;
			si.nPage  = 16;
			SetScrollInfo(hDlg, SB_VERT, &si, TRUE);
			return 0;
		}
		break;
/*
		case WM_COMMAND:
		{
			switch(wParam)
			{
			case IDC_SAVE:
				{
				char fname[2048];
				strcpy(fname,"dump.bin");
				if(Change_File_S(fname,".","Save Full Dump As...","All Files\0*.*\0\0","*.*",hDlg))
				{
					FILE *out=fopen(fname,"wb+");
					int i;
					for (i=0;i<sizeof(Ram_68k);++i)
					{
						fname[i&2047]=Ram_68k[i^1];
						if ((i&2047)==2047)
							fwrite(fname,1,sizeof(fname),out);
					}
					fwrite(fname,1,i&2047,out);
					fclose(out);
				}
				}
				break;

			case IDC_BUTTON1:
				{
				char fname[2048];
				GetDlgItemText(hDlg,IDC_EDIT1,fname,2047);
				int CurPos;
				if ((strnicmp(fname,"ff",2)==0) && sscanf(fname+2,"%x",&CurPos))
				{
					SetScrollPos(GetDlgItem(hDlg,IDC_SCROLLBAR1),SB_CTL,(CurPos>>4),TRUE);
					Update_RAM_Dump();
				}
				}
				break;
			}
		}	break;
*/
		case WM_VSCROLL:
		{
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.fMask = SIF_ALL;
			si.cbSize = sizeof(SCROLLINFO);
			GetScrollInfo(hDlg,SB_VERT,&si);

			switch(LOWORD(wParam))
			{
				case SB_ENDSCROLL:
				case SB_TOP:
				case SB_BOTTOM:
					break;
				case SB_LINEUP:
					si.nPos--;
					break;
				case SB_LINEDOWN:
					si.nPos++;
					break;
				case SB_PAGEUP:
					si.nPos -= si.nPage;
					break;
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					si.nPos = si.nTrackPos;
					break;
			}
			if (si.nPos < si.nMin)
				si.nPos = si.nMin;
			if ((si.nPos + (int) si.nPage) > si.nMax)
				si.nPos = si.nMax - si.nPage;

			Hex.OffsetVisibleFirst = si.nPos * 16;
			SetScrollInfo(hDlg, SB_VERT, &si, TRUE);
			UpdateHexEditor();
			return 0;
		}
		break;

		case WM_MOUSEWHEEL:
		{
			int WheelDelta = (short) HIWORD(wParam);

			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.fMask  = SIF_ALL;
			si.cbSize = sizeof(SCROLLINFO);
			GetScrollInfo(hDlg, SB_VERT, &si);

			if (WheelDelta < 0)
				si.nPos += si.nPage;
			if (WheelDelta > 0)
				si.nPos -= si.nPage;
			if (si.nPos < si.nMin)
				si.nPos = si.nMin;
			if ((si.nPos + (int) si.nPage) > si.nMax)
				si.nPos = si.nMax - si.nPage;

			Hex.OffsetVisibleFirst = si.nPos*16;
			SetScrollInfo(hDlg,SB_VERT,&si,TRUE);
			UpdateHexEditor();
			return 0;
		}
		break;

		case WM_PAINT:
		{
			BeginPaint(hDlg, &ps);			
			static char buf[10];
			int row = 0, line = 0;

			// TOP HEADER, static.
			for (row = 0; row < 16; row++)
			{
				MoveToEx(HexDC, row * Hex.CellWidth + Hex.GapHeaderH, 0, NULL);
				SetBkColor(HexDC, Hex.ColorBG);
				SetTextColor(HexDC, Hex.ColorFont);
				sprintf(buf, "%2X", row);
				TextOut(HexDC, 0, 0, buf, strlen(buf));					
			}

			// LEFT HEADER, semi-dynamic.
			for (line = 0; line < 16; line++)
			{
				MoveToEx(HexDC, 0, line * Hex.CellHeight + Hex.GapHeaderV, NULL);
				SetBkColor(HexDC, Hex.ColorBG);
				SetTextColor(HexDC, Hex.ColorFont);
				sprintf(buf, "%06X:", Hex.OffsetVisibleFirst + line*16 + Hex.MemoryRegion);
				TextOut(HexDC, 0, 0, buf, strlen(buf));
			}

			// RAM, dynamic.
			for (line = 0; line < 16; line++)
			{
				for (row = 0; row < 16; row++)
				{
					MoveToEx(HexDC, row * Hex.CellWidth + Hex.GapHeaderH, line * Hex.CellHeight + Hex.GapHeaderV, NULL);
					sprintf(buf, "%02X", (int) Ram_68k[Hex.OffsetVisibleFirst + line*16 + row]);
					TextOut(HexDC, 0, 0, buf, strlen(buf));
				}
			}
			EndPaint(hDlg, &ps);
			return 0;
		}
		break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			KillHexEditor();
			return 0;
	}
	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

void DoHexEditor()
{
	WNDCLASSEX wndclass;

	if (!HexEditorHWnd)
	{
		memset(&wndclass,0,sizeof(wndclass));
		wndclass.cbSize=sizeof(WNDCLASSEX);
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = HexEditorProc ;
		wndclass.cbClsExtra    = 0 ;
		wndclass.cbWndExtra    = 0 ;
		wndclass.hInstance     = ghInstance;
		wndclass.hIcon         = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_GENS));
		wndclass.hIconSm       = LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_GENS));
		wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
		wndclass.lpszClassName = "HEXEDITOR";

		if(!RegisterClassEx(&wndclass))
		{
			Put_Info("Error Registering HEXEDITOR Window Class.");
			return;
		}

		HexEditorHWnd = CreateWindowEx(
			0,
			"HEXEDITOR",
			"HexEditor",
			WS_SYSMENU | WS_MINIMIZEBOX | WS_VSCROLL,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			580,
			248,
			NULL,
			NULL,
			ghInstance,
			NULL
		);
		ShowWindow(HexEditorHWnd, SW_SHOW);
		UpdateCaption();
		DialogsOpen++;
	}
	else
	{
		ShowWindow(HexEditorHWnd, SW_SHOWNORMAL);
		SetForegroundWindow(HexEditorHWnd);
		UpdateCaption();
	}
}
