#include "resource.h"
#include "gens.h"
#include "mem_m68k.h"
#include "mem_s68k.h"
#include "mem_sh2.h"
#include "misc.h"
#include "mem_z80.h"
#include "vdp_io.h"
#include "save.h"
#include "ram_search.h"
#include "ramwatch.h"
#include "g_main.h"
#include <assert.h>
#include <windows.h>
#include <commctrl.h>
#include <string>

static HMENU ramwatchmenu;
static HMENU rwrecentmenu;
char rw_recent_files[5][1024];
char Watch_Dir[1024]="";
const unsigned int RW_MENU_FIRST_RECENT_FILE = 600;
const unsigned int RW_MAX_NUMBER_OF_RECENT_FILES = sizeof(rw_recent_files)/sizeof(*rw_recent_files);
bool RWfileChanged = false; //Keeps track of whether the current watch file has been changed, if so, ramwatch will prompt to save changes
bool AutoRWLoad = false;    //Keeps track of whether Auto-load is checked
char currentWatch[1024];
int ramw_x, ramw_y; //Used to store ramwatch dialog window positions
AddressWatcher rswatches[256];
int WatchCount=0;

void QuickSaveWatches();
void ResetWatches();
extern "C" int Clear_Sound_Buffer(void);

unsigned int GetCurrentValue(AddressWatcher& watch)
{
	return ReadValueAtHardwareAddress(watch.Address, watch.Size == 'd' ? 4 : watch.Size == 'w' ? 2 : 1);
}

bool IsSameWatch(const AddressWatcher& l, const AddressWatcher& r)
{
	return ((l.Address == r.Address) && (l.Size == r.Size) && (l.Type == r.Type)/* && (l.WrongEndian == r.WrongEndian)*/);
}

bool VerifyWatchNotAlreadyAdded(const AddressWatcher& watch)
{
	for (int j = 0; j < WatchCount; j++)
	{
		if (IsSameWatch(rswatches[j], watch))
		{
			if(RamWatchHWnd)
				SetForegroundWindow(RamWatchHWnd);
			return false;
		}
	}
	return true;
}


bool InsertWatch(const AddressWatcher& Watch, char *Comment)
{
	if(!VerifyWatchNotAlreadyAdded(Watch))
		return false;

	if(WatchCount >= MAX_WATCH_COUNT)
		return false;

	int i = WatchCount++;
	AddressWatcher& NewWatch = rswatches[i];
	NewWatch = Watch;
	//if (NewWatch.comment) free(NewWatch.comment);
	NewWatch.comment = (char *) malloc(strlen(Comment)+2);
	strcpy(NewWatch.comment, Comment);
	ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged=true;

	return true;
}

LRESULT CALLBACK PromptWatchNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets the description of a watched address
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			Clear_Sound_Buffer();

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

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			strcpy(Str_Tmp,"Enter a name for this RAM address.");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			strcpy(Str_Tmp,"");
			SendDlgItemMessage(hDlg,IDC_PROMPT_TEXT2,WM_SETTEXT,0,(LPARAM)Str_Tmp);
			return true;
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,80);
					InsertWatch(rswatches[WatchCount],Str_Tmp);
					DialogsOpen--;
					EndDialog(hDlg, true);
					return true;
					break;
				}
				case ID_CANCEL:
				case IDCANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}

					DialogsOpen--;
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}

bool InsertWatch(const AddressWatcher& Watch, HWND parent)
{
	if(!VerifyWatchNotAlreadyAdded(Watch))
		return false;

	if(!parent)
		parent = RamWatchHWnd;
	if(!parent)
		parent = HWnd;

	int prevWatchCount = WatchCount;

	rswatches[WatchCount] = Watch;
	DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PROMPT), parent, (DLGPROC) PromptWatchNameProc);

	return WatchCount > prevWatchCount;
}

void Update_RAM_Watch()
{
	// update cached values and detect changes to displayed listview items
	BOOL watchChanged[128] = {0};
	HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
	int top = ListView_GetTopIndex(lv);
	int count = ListView_GetCountPerPage(lv);
	int i;
	for(i = top; i <= top+count; i++)
	{
		unsigned int prevCurValue = rswatches[i].CurValue;
		unsigned int newCurValue = GetCurrentValue(rswatches[i]);
		if(prevCurValue != newCurValue)
		{
			rswatches[i].CurValue = newCurValue;
			watchChanged[i-top] = TRUE;
		}
	}

	// refresh any visible parts of the listview box that changed
	int start = -1;
	for(i = top; i <= top+count; i++)
	{
		if(start == -1)
		{
			if(i != top+count && watchChanged[i-top])
			{
				start = i;
				//somethingChanged = true;
			}
		}
		else
		{
			if(i == top+count || !watchChanged[i-top])
			{
				ListView_RedrawItems(lv, start, i-1);
				start = -1;
			}
		}
	}
}

bool AskSave()
{
	//This function simply asks to save changes if the watch file contents have changed
	//returns true if the user decided to save changes, and false if they did not (this is currently not used, but could be valuable information in future feature enhancements
	if (RWfileChanged)
	{
		if(MessageBox(RamWatchHWnd, "Save Changes?", "Ram Watch Settings", MB_YESNO)==IDYES)
			{
				QuickSaveWatches();
				return true;
			}
	}
	
	return false;
}


void UpdateRW_RMenu(HMENU menu, unsigned int mitem, unsigned int baseid)
{
	MENUITEMINFO moo;
	int x;

	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;

	GetMenuItemInfo(GetSubMenu(ramwatchmenu, 0), mitem, FALSE, &moo);
	moo.hSubMenu = menu;
	moo.fState = strlen(rw_recent_files[0]) ? MFS_ENABLED : MFS_GRAYED;

	SetMenuItemInfo(GetSubMenu(ramwatchmenu, 0), mitem, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < RW_MAX_NUMBER_OF_RECENT_FILES; x++)
	{
		RemoveMenu(menu, baseid + x, MF_BYCOMMAND);
	}

	// Recreate the menus
	for(x = RW_MAX_NUMBER_OF_RECENT_FILES - 1; x >= 0; x--)
	{  
		char tmp[128 + 5];

		// Skip empty strings
		if(!strlen(rw_recent_files[x]))
		{
			continue;
		}

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;

		// Fill in the menu text.
		if(strlen(rw_recent_files[x]) < 128)
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, rw_recent_files[x]);
		}
		else
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, rw_recent_files[x] + strlen( rw_recent_files[x] ) - 127);
		}

		// Insert the menu item
		moo.cch = strlen(tmp);
		moo.fType = 0;
		moo.wID = baseid + x;
		moo.dwTypeData = tmp;
		InsertMenuItem(menu, 0, 1, &moo);
	}
}

void UpdateRWRecentArray(const char* addString, unsigned int arrayLen, HMENU menu, unsigned int menuItem, unsigned int baseId)
{
	// Try to find out if the filename is already in the recent files list.
	for(unsigned int x = 0; x < arrayLen; x++)
	{
		if(strlen(rw_recent_files[x]))
		{
			if(!strcmp(rw_recent_files[x], addString))    // Item is already in list.
			{
				// If the filename is in the file list don't add it again.
				// Move it up in the list instead.

				int y;
				char tmp[1024];

				// Save pointer.
				strcpy(tmp,rw_recent_files[x]);
				
				for(y = x; y; y--)
				{
					// Move items down.
					strcpy(rw_recent_files[y],rw_recent_files[y - 1]);
				}

				// Put item on top.
				strcpy(rw_recent_files[0],tmp);

				// Update the recent files menu
				UpdateRW_RMenu(menu, menuItem, baseId);

				return;
			}
		}
	}

	// The filename wasn't found in the list. That means we need to add it.

	// Move the other items down.
	for(unsigned int x = arrayLen - 1; x; x--)
	{
		strcpy(rw_recent_files[x],rw_recent_files[x - 1]);
	}

	// Add the new item.
	strcpy(rw_recent_files[0], addString);

	// Update the recent files menu
	UpdateRW_RMenu(menu, menuItem, baseId);
}

/**
* Add a filename to the recent files list.
*
* @param filename Name of the file to add.
**/
void RWAddRecentFile(const char *filename)
{
	UpdateRWRecentArray(filename, RW_MAX_NUMBER_OF_RECENT_FILES, rwrecentmenu, RAMMENU_FILE_RECENT, RW_MENU_FIRST_RECENT_FILE);
}

void OpenRWRecentFile(int memwRFileNumber)
{
	AskSave();
	ResetWatches();
	int rnum=memwRFileNumber;
		if (rnum > RW_MAX_NUMBER_OF_RECENT_FILES) return; //just in case
		
	char* x = rw_recent_files[rnum];
	if (strlen(x)==0) return; //If no recent files exist just return.  Useful for Load last file on startup (or if something goes screwy)
	//char watchfcontents[2048];
	
	if (rnum != 0) //Change order of recent files if not most recent
		RWAddRecentFile(x);
	strcpy(currentWatch,x);
	strcpy(Str_Tmp,currentWatch);
	//loadwatches here
	FILE *WatchFile = fopen(Str_Tmp,"rb");
		if (!WatchFile)
		{
			MessageBox(RamWatchHWnd,"Error opening file.","ERROR",MB_OK);
			return;
		}
		const char DELIM = '\t';
		AddressWatcher Temp;
		char mode;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%c%*s",&mode);
		if ((mode == '1' && !(SegaCD_Started)) || (mode == '2' && !(_32X_Started)))
		{
			char Device[8];
			strcpy(Device,(mode > '1')?"32X":"SegaCD");
			sprintf(Str_Tmp,"Warning: %s not started. \nWatches for %s addresses will be ignored.",Device,Device);
			MessageBox(RamWatchHWnd,Str_Tmp,"Device Mismatch",MB_OK);
		}
		int WatchAdd;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%d%*s",&WatchAdd);
		WatchAdd+=WatchCount;
		for (int i = WatchCount; i < WatchAdd; i++)
		{
			while (i < 0)
				i++;
			do {
				fgets(Str_Tmp,1024,WatchFile);
			} while (Str_Tmp[0] == '\n');
			sscanf(Str_Tmp,"%*05X%*c%08X%*c%c%*c%c%*c%d",&(Temp.Address),&(Temp.Size),&(Temp.Type),&(Temp.WrongEndian));
			Temp.WrongEndian = 0;
			char *Comment = strrchr(Str_Tmp,DELIM) + 1;
			*strrchr(Comment,'\n') = '\0';
			InsertWatch(Temp,Comment);
		}
		
		fclose(WatchFile);
		if (RamWatchHWnd)
			ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
		RWfileChanged=false;
		return;
}

bool Save_Watches()
{
	strncpy(Str_Tmp,Rom_Name,512);
	strcat(Str_Tmp,".wch");
	if(Change_File_S(Str_Tmp, Gens_Path, "Save Watches", "GENs Watchlist\0*.wch\0All Files\0*.*\0\0", "wch", RamWatchHWnd))
	{
		FILE *WatchFile = fopen(Str_Tmp,"r+b");
		if (!WatchFile) WatchFile = fopen(Str_Tmp,"w+b");
		fputc(SegaCD_Started?'1':(_32X_Started?'2':'0'),WatchFile);
		fputc('\n',WatchFile);
		strcpy(currentWatch,Str_Tmp);
		RWAddRecentFile(currentWatch);
		sprintf(Str_Tmp,"%d\n",WatchCount);
		fputs(Str_Tmp,WatchFile);
		const char DELIM = '\t';
		for (int i = 0; i < WatchCount; i++)
		{
			sprintf(Str_Tmp,"%05X%c%08X%c%c%c%c%c%d%c%s\n",i,DELIM,rswatches[i].Address,DELIM,rswatches[i].Size,DELIM,rswatches[i].Type,DELIM,rswatches[i].WrongEndian,DELIM,rswatches[i].comment);
			fputs(Str_Tmp,WatchFile);
		}
		
		fclose(WatchFile);
		RWfileChanged=false;
		//TODO: Add to recent list function call here
		return true;
	}
	return false;
}

void QuickSaveWatches()
{
if (RWfileChanged==false) return; //If file has not changed, no need to save changes
if (currentWatch[0] == NULL) //If there is no currently loaded file, run to Save as and then return
	{
		Save_Watches();
		return;
	}
		
		strcpy(Str_Tmp,currentWatch);
		FILE *WatchFile = fopen(Str_Tmp,"r+b");
		if (!WatchFile) WatchFile = fopen(Str_Tmp,"w+b");
		fputc(SegaCD_Started?'1':(_32X_Started?'2':'0'),WatchFile);
		fputc('\n',WatchFile);
		sprintf(Str_Tmp,"%d\n",WatchCount);
		fputs(Str_Tmp,WatchFile);
		const char DELIM = '\t';
		for (int i = 0; i < WatchCount; i++)
		{
			sprintf(Str_Tmp,"%05X%c%08X%c%c%c%c%c%d%c%s\n",i,DELIM,rswatches[i].Address,DELIM,rswatches[i].Size,DELIM,rswatches[i].Type,DELIM,rswatches[i].WrongEndian,DELIM,rswatches[i].comment);
			fputs(Str_Tmp,WatchFile);
		}
		fclose(WatchFile);
		RWfileChanged=false;
		return;
}



bool Load_Watches()
{
	AskSave();
	strncpy(Str_Tmp,Rom_Name,512);
	strcat(Str_Tmp,".wch");
	const char DELIM = '\t';
	if(Change_File_L(Str_Tmp, Watch_Dir, "Load Watches", "GENs Watchlist\0*.wch\0All Files\0*.*\0\0", "wch", RamWatchHWnd))
	{
		
		FILE *WatchFile = fopen(Str_Tmp,"rb");
		if (!WatchFile)
		{
			MessageBox(RamWatchHWnd,"Error opening file.","ERROR",MB_OK);
			return false;
		}
		strcpy(currentWatch,Str_Tmp);
		RWAddRecentFile(currentWatch);
		AddressWatcher Temp;
		char mode;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%c%*s",&mode);
		if ((mode == '1' && !(SegaCD_Started)) || (mode == '2' && !(_32X_Started)))
		{
			char Device[8];
			strcpy(Device,(mode > '1')?"32X":"SegaCD");
			sprintf(Str_Tmp,"Warning: %s not started. \nWatches for %s addresses will be ignored.",Device,Device);
			MessageBox(RamWatchHWnd,Str_Tmp,"Device Mismatch",MB_OK);
		}
		int WatchAdd;
		fgets(Str_Tmp,1024,WatchFile);
		sscanf(Str_Tmp,"%d%*s",&WatchAdd);
		WatchAdd+=WatchCount;
		for (int i = WatchCount; i < WatchAdd; i++)
		{
			while (i < 0)
				i++;
			do {
				fgets(Str_Tmp,1024,WatchFile);
			} while (Str_Tmp[0] == '\n');
			sscanf(Str_Tmp,"%*05X%*c%08X%*c%c%*c%c%*c%d",&(Temp.Address),&(Temp.Size),&(Temp.Type),&(Temp.WrongEndian));
			Temp.WrongEndian = 0;
			char *Comment = strrchr(Str_Tmp,DELIM) + 1;
			*strrchr(Comment,'\n') = '\0';
			InsertWatch(Temp,Comment);
		}
		
		fclose(WatchFile);
		if (RamWatchHWnd)
			ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
		RWfileChanged=false;
		return true;
	}
	return false;
}

void ResetWatches()
{
	AskSave();
	for (;WatchCount>=0;WatchCount--)
	{
		free(rswatches[WatchCount].comment);
		rswatches[WatchCount].comment = NULL;
	}
	WatchCount++;
	if (RamWatchHWnd)
		ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
	RWfileChanged = false;
	currentWatch[0] = NULL;
}

void RemoveWatch(int watchIndex)
{
	free(rswatches[watchIndex].comment);
	rswatches[watchIndex].comment = NULL;
	for (int i = watchIndex; i <= WatchCount; i++)
		rswatches[i] = rswatches[i+1];
	WatchCount--;
}

bool Open_Watches()
{
//Closes existing watch and loads a new one.
ResetWatches();
if (Load_Watches()) return true;
else return false;
}

LRESULT CALLBACK EditWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets info for a RAM Watch, and then inserts it into the Watch List
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int index;
	static char s,t = s = 0;

	switch(uMsg)
	{
		case WM_INITDIALOG:
			Clear_Sound_Buffer();
			
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

			//SetWindowPos(hDlg, NULL, max(0, r.left + (dx1 - dx2)), max(0, r.top + (dy1 - dy2)), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			index = (int)lParam;
			sprintf(Str_Tmp,"%08X",rswatches[index].Address);
			SetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp);
			if (rswatches[index].comment != NULL)
				SetDlgItemText(hDlg,IDC_PROMPT_EDIT,rswatches[index].comment);
			s = rswatches[index].Size;
			t = rswatches[index].Type;
			switch (s)
			{
				case 'b':
					SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'w':
					SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					s = 0;
					break;
			}
			switch (t)
			{
				case 's':
					SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'u':
					SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'h':
					SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
					break;
				default:
					t = 0;
					break;
			}
			return true;
			break;
		
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SIGNED:
					t='s';
					return true;
				case IDC_UNSIGNED:
					t='u';
					return true;
				case IDC_HEX:
					t='h';
					return true;
				case IDC_1_BYTE:
					s = 'b';
					return true;
				case IDC_2_BYTES:
					s = 'w';
					return true;
				case IDC_4_BYTES:
					s = 'd';
					return true;
				case IDOK:
				{
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					if (s && t)
					{
						AddressWatcher Temp;
						Temp.Size = s;
						Temp.Type = t;
						Temp.WrongEndian = false; //replace this when I get little endian working properly
						GetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp,1024);
						char *addrstr = Str_Tmp;
						if (strlen(Str_Tmp) > 8) addrstr = &(Str_Tmp[strlen(Str_Tmp) - 9]);
						sscanf(addrstr,"%08X",&(Temp.Address));

						if(IsHardwareAddressValid(Temp.Address))
						{
							GetDlgItemText(hDlg,IDC_PROMPT_EDIT,Str_Tmp,80);
							if (index < WatchCount) RemoveWatch(index);
							InsertWatch(Temp,Str_Tmp);
							if(RamWatchHWnd)
							{
								ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
							}
							DialogsOpen--;
							EndDialog(hDlg, true);
						}
						else
						{
							MessageBox(hDlg,"Invalid Address","ERROR",MB_OK);
						}
					}
					else
					{
						strcpy(Str_Tmp,"Error:");
						if (!s)
							strcat(Str_Tmp," Size must be specified.");
						if (!t)
							strcat(Str_Tmp," Type must be specified.");
						MessageBox(hDlg,Str_Tmp,"ERROR",MB_OK);
					}
					RWfileChanged=true;
					return true;
					break;
				}
				case ID_CANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					EndDialog(hDlg, false);
					return false;
					break;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			EndDialog(hDlg, false);
			return false;
			break;
	}

	return false;
}




LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_MOVE: {
			RECT wrect;
			GetWindowRect(hDlg,&wrect);
			ramw_x = wrect.left;
			ramw_y = wrect.top;
			break;
			};
			
		case WM_INITDIALOG: {
			if (Full_Screen)
			{
				while (ShowCursor(false) >= 0);
				while (ShowCursor(true) < 0);
			}

			GetWindowRect(HWnd, &r);  //Ramwatch window
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2); // Gens window
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			
			// push it away from the main window if we can
			const int width = (r.right-r.left);
			const int height = (r.bottom - r.top);
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}

			//If saved window pos exists and meets certain requirements, use it.
			if (ramw_x > (0 - width)) //If ramw x appears completely off the left of the screen, use default instead
			{
				if (!(ramw_x > (r2.left-width) && ramw_x < r2.right && ramw_y > (r2.top - height + 5) && ramw_y < r2.bottom - 5)) //Check if it osbcres gens window
					r.left = ramw_x;
			}
			if (ramw_y > 0 - height && ramw_y < height + GetSystemMetrics(SM_CYSCREEN)) //If ramw y appears completely off screen use default instead
			{
				r.top = ramw_y;
			}
			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			
			ramwatchmenu=GetMenu(hDlg);
			rwrecentmenu=CreateMenu();
			UpdateRW_RMenu(rwrecentmenu, RAMMENU_FILE_RECENT, RW_MENU_FIRST_RECENT_FILE);
			
			const char* names[3] = {"Address","Value","Notes"};
			int widths[3] = {62,64,64+51+53};
			init_list_box(GetDlgItem(hDlg,IDC_WATCHLIST),names,3,widths);
			if (!ResultCount)
				reset_address_info();
			else
				signal_new_frame();
			ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
			if (!noMisalign) SendDlgItemMessage(hDlg, IDC_MISALIGN, BM_SETCHECK, BST_CHECKED, 0);
			if (littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);
			return true;
			break;
		}
		
		
		case WM_INITMENU:
		CheckMenuItem(ramwatchmenu, RAMMENU_FILE_AUTOLOAD, AutoRWLoad ? MF_CHECKED : MF_UNCHECKED);
		break;

		case WM_NOTIFY:
		{
			LPNMHDR lP = (LPNMHDR) lParam;
			switch (lP->code)
			{
				case LVN_GETDISPINFO:
				{
					LV_DISPINFO *Item = (LV_DISPINFO *)lParam;
					Item->item.mask = LVIF_TEXT;
					Item->item.state = 0;
					Item->item.iImage = 0;
					const unsigned int iNum = Item->item.iItem;
					static char num[11];
					switch (Item->item.iSubItem)
					{
						case 0:
							sprintf(num,"%08X",rswatches[iNum].Address);
							Item->item.pszText = num;
							return true;
						case 1: {
							int i = GetCurrentValue(rswatches[iNum]); // could use rswatches[iNum].CurValue here but this seems fast enough and less likely to show out-of-date values
							int t = rswatches[iNum].Type;
							int size = rswatches[iNum].Size;
							const char* formatString = ((t=='s') ? "%d" : (t=='u') ? "%u" : (size=='d' ? "%08X" : size=='w' ? "%04X" : "%02X"));
							switch (size)
							{
								case 'b':
								default: sprintf(num, formatString, t=='s' ? (char)(i&0xff) : (unsigned char)(i&0xff)); break;
								case 'w': sprintf(num, formatString, t=='s' ? (short)(i&0xffff) : (unsigned short)(i&0xffff)); break;
								case 'd': sprintf(num, formatString, t=='s' ? (long)(i&0xffffffff) : (unsigned long)(i&0xffffffff)); break;
							}

							Item->item.pszText = num;
						}	return true;
						case 2:
							Item->item.pszText = rswatches[iNum].comment ? rswatches[iNum].comment : "";
							return true;

						default:
							return false;
					}
				}
				//case LVN_ODCACHEHINT: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{
				//	LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)lParam;
				//	return 0;
				//}
				//case LVN_ODFINDITEM: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{	
				//	LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
				//	return 0;
				//}
			}
		}

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ACCEL_CTRL_S:
				case RAMMENU_FILE_SAVE:
					QuickSaveWatches();
					break;

				case ACCEL_CTRL_SHIFT_S:
				case RAMMENU_FILE_SAVEAS:	
				//case IDC_C_SAVE:
					return Save_Watches();
				case ACCEL_CTRL_O:
				case RAMMENU_FILE_OPEN:
					return Open_Watches();
				case RAMMENU_FILE_APPEND:
				//case IDC_C_LOAD:
					return Load_Watches();
				case ACCEL_CTRL_N:
				case RAMMENU_FILE_NEW:
				//case IDC_C_RESET:
					ResetWatches();
					return true;
				case RAMMENU_WATCHES_REMOVEWATCH:
				case IDC_C_SEARCH:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					RemoveWatch(watchIndex);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);	
					RWfileChanged=true;
					return true;
				case RAMMENU_WATCHES_EDITWATCH:
				case IDC_C_WATCH_EDIT:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) watchIndex);
					return true;
				case RAMMENU_WATCHES_NEWWATCH:
				case IDC_C_WATCH:
					rswatches[WatchCount].Address = rswatches[WatchCount].WrongEndian = 0;
					rswatches[WatchCount].Size = 'b';
					rswatches[WatchCount].Type = 's';
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					return true;
				case RAMMENU_WATCHES_DUPLICATEWATCH:
				case IDC_C_WATCH2:
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					rswatches[WatchCount].Address = rswatches[watchIndex].Address;
					rswatches[WatchCount].WrongEndian = rswatches[watchIndex].WrongEndian;
					rswatches[WatchCount].Size = rswatches[watchIndex].Size;
					rswatches[WatchCount].Type = rswatches[watchIndex].Type;
					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITWATCH), hDlg, (DLGPROC) EditWatchProc,(LPARAM) WatchCount);
					return true;
				case RAMMENU_WATCHES_MOVEUP:
				case IDC_C_WATCH_UP:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if (watchIndex == 0)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex - 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex - 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex-1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					RWfileChanged=true;
					return true;
				}
				case RAMMENU_WATCHES_MOVEDOWN:
				case IDC_C_WATCH_DOWN:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST));
					if (watchIndex >= WatchCount - 1)
						return true;
					void *tmp = malloc(sizeof(AddressWatcher));
					memcpy(tmp,&(rswatches[watchIndex]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex]),&(rswatches[watchIndex + 1]),sizeof(AddressWatcher));
					memcpy(&(rswatches[watchIndex + 1]),tmp,sizeof(AddressWatcher));
					free(tmp);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1);
					ListView_SetItemState(GetDlgItem(hDlg,IDC_WATCHLIST),watchIndex+1,LVIS_FOCUSED|LVIS_SELECTED,LVIS_FOCUSED|LVIS_SELECTED);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_WATCHLIST),WatchCount);
					RWfileChanged=true;
					return true;
				}
				case RAMMENU_FILE_AUTOLOAD:
				{
					AutoRWLoad ^= 1;
					CheckMenuItem(ramwatchmenu, RAMMENU_FILE_AUTOLOAD, AutoRWLoad ? MF_CHECKED : MF_UNCHECKED);
					break;
				}
				case IDC_C_ADDCHEAT:
				{
//					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_WATCHLIST)) | (1 << 24);
//					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITCHEAT), hDlg, (DLGPROC) EditCheatProc,(LPARAM) searchIndex);
				}
				case IDOK:
				case ACCEL_CTRL_W:
				case RAMMENU_FILE_CLOSE:
				//case IDCANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					RamWatchHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
				default:
					if (LOWORD(wParam) >= RW_MENU_FIRST_RECENT_FILE && LOWORD(wParam) < RW_MENU_FIRST_RECENT_FILE+RW_MAX_NUMBER_OF_RECENT_FILES)
					OpenRWRecentFile(LOWORD(wParam) - RW_MENU_FIRST_RECENT_FILE);
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			RamWatchHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}

