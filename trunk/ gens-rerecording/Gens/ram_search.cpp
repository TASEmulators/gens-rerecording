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
#include "g_main.h"
#include <assert.h>
#include <commctrl.h>
#include "G_dsound.h"
#include "ramwatch.h"

extern char rs_c, rs_o, rs_t;
extern int rs_param, rs_val;
bool AutoSearch=false;

LRESULT CALLBACK PromptWatchNameProc(HWND, UINT, WPARAM, LPARAM);

AddressInfo rsaddrs [MAX_RAM_SIZE];
AddrIndex rsresults [MAX_RAM_SIZE];
AddressWatcher rswatches[256];
int ResultCount=0;
int WatchCount=0;
char rs_type_size = 'b', rs_last_type_size = 'b';
bool noMisalign = true;
bool littleEndian = false;
bool preserveChanges = true;
int tempCount;
char Watch_Dir[1024]="";
/*
template <typename T>
T GetRamValue(unsigned int i)
{
	if (Genesis_Started)
        if (i < GENESIS_RAM_SIZE) 
		{
			if(i < _68K_RAM_SIZE)
				return (T) Ram_68k[i];
			i -= _68K_RAM_SIZE;

			return (T) Ram_Z80[i];
		}

	i -= GENESIS_RAM_SIZE;
    
    if (_32X_Started)
        if (i < _32X_RAM_SIZE)
            return (T) _32X_Ram[i];

	i -= _32X_RAM_SIZE;

	if (SegaCD_Started)
        if (i < SEGACD_RAM_SIZE)
		{
			if(i < SEGACD_RAM_PRG_SIZE)
				return (T) Ram_Prg[i];
			i -= SEGACD_RAM_PRG_SIZE;

			if(i < SEGACD_1M_RAM_SIZE)
				return (T) Ram_Word_1M[i];
			i -= SEGACD_1M_RAM_SIZE;

			return (T) Ram_Word_2M[i];
		}

	return (T) -1;
}
unsigned int GetRamValue(unsigned int i,char c)
{
	switch (c)
	{
		case 'b': return (unsigned int) (GetRamValue<char>(i));
		case 'w': return (unsigned int) (GetRamValue<short>(i));
		case 'd': return (unsigned int) (GetRamValue<long>(i));
		default: return (unsigned int) -1;
	}
}
*/
unsigned int sizeConv(unsigned int index, char size, char *prevSize, bool usePrev)
{
	int typeSizeDest = (size == 'd') ? 4 : ((size == 'w') ? 2 : 1);
//	int typeSizeSrc = 1;//(*prevSize == 'd') ? 4 : ((*prevSize == 'w') ? 2 : 1);
//	if (typeSizeDest == typeSizeSrc)
//		return (usePrev ? rsaddrs[index].prev : rsaddrs[index].cur);
//	else if (typeSizeDest < typeSizeSrc)
//	{
//		unsigned int mask = 0x0;
//		if (typeSizeDest == 1) Byte_Swap((usePrev ? &(rsaddrs[index].prev) : &(rsaddrs[index].cur)),typeSizeSrc);
//		for (int i = 0; i < typeSizeDest; i++)
//		{
//			mask <<=8;
//			mask |= 0xFF;
//		}
//		if (littleEndian)
//			Switch_Endian((usePrev ? &(rsaddrs[index].prev) : &(rsaddrs[index].cur)),typeSizeSrc);
//		unsigned int val = ((usePrev ? rsaddrs[index].prev : rsaddrs[index].cur) & mask);
//		if (typeSizeDest == 1) Byte_Swap((usePrev ? &(rsaddrs[index].prev) : &(rsaddrs[index].cur)),typeSizeSrc);
//		return val;
//	}
//	else // (typeSizeDest > typeSizeSrc)
//	{
		unsigned int val = 0;
		for (int i = 0; i < typeSizeDest; i++)
		{
			val <<= 8;
			val |= (usePrev ? rsaddrs[index + i].prev : rsaddrs[index + i].cur);
		}
		return val;
//	}
}
void RamSet(unsigned int *d,void *s,char Size)
{
	switch (Size)
	{
		case 'b': *d = (unsigned int) (*((char *) &s)); break;
		case 'w': *d = (unsigned int) (*((short *) &s)); break;
		case 'd': *d = (unsigned int) (*((long *) &s)); break;
		default: break;
	}
}
//Function to determine the partitions
// partitions the Array and returns the middle index (subscript)
int partition(AddrIndex *Array, int top, int bottom)
{
	unsigned int x = Array[top].Address;
	int i = top - 1;
	int j = bottom + 1;
	do {
		do {
			j--;
		} while (x <Array[j].Address);

		do {
			i++;
		} while (x >Array[i].Address);

		if (i < j)
		{ 
			AddrIndex temp = Array[i];    // switch elements at positions i and j
			Array[i] = Array[j];
			Array[j] = temp;
		}
	} while (i < j);    
	return j;		// returns middle index
}
void quicksort(AddrIndex *Array, int top, int bottom)
{
	// top = subscript of beginning of vector being considered
	// bottom = subscript of end of vector being considered
	// this process uses recursion - the process of calling itself
	int middle;
	if (top < bottom)
	{
		middle = partition(Array, top, bottom);
		quicksort(Array, top, middle);   // sort top partition
		quicksort(Array, middle+1, bottom);    // sort bottom partition
	}
	return;
}
unsigned int getHardwareAddress(unsigned int index)
{
	if (SegaCD_Started)
	{
		if (index < SEGACD_RAM_PRG_SIZE) return index + 0x020000;
		index -= SEGACD_RAM_PRG_SIZE;
		if (index < SEGACD_2M_RAM_SIZE) return index + 0x200000;
		index -= SEGACD_2M_RAM_SIZE;
	}
	if (index < Z80_RAM_SIZE) return index + 0xA00000;
	index-= Z80_RAM_SIZE;
	if (index < _68K_RAM_SIZE) return index + 0xFF0000;
	index-= _68K_RAM_SIZE;
	if (index < _32X_RAM_SIZE) return index + 0x06000000;
	index-= _32X_RAM_SIZE;
	return 0xFFFFFFFF;
}
int getSearchIndex(unsigned int HA,int bot, int top)
{
	if (top<bot)	
		return -1;
	int mid = (top + bot) / 2;
	if (HA > getHardwareAddress(mid))
		return getSearchIndex(HA,mid+1,top);
	else if (HA < getHardwareAddress(mid))
		return getSearchIndex(HA,bot,mid-1);
	return mid;
}
bool sameWatch(AddressWatcher l,AddressWatcher r)
{
	return ((l.Index == r.Index) && (l.Size == r.Size) && (l.Type == r.Type)/* && (l.WrongEndian == r.WrongEndian)*/);
}
void InsertWatch(AddressWatcher Watch, char *Comment)
{
//	MessageBox(NULL,"Inserting Watch",NULL,MB_OK);
	if (rsaddrs[Watch.Index].comment) free(rsaddrs[Watch.Index].comment);
	rsaddrs[Watch.Index].comment = (char *) malloc(strlen(Comment)+2);
	rsaddrs[Watch.Index].flags |= RS_FLAG_WATCHED;
	strcpy(rsaddrs[Watch.Index].comment,Comment);
/*	for (int j = 0; j < WatchCount; j++)
	{
		if (sameWatch(rswatches[j],Watch))
		{
			return;
		}
	}*/
	int i = WatchCount;
	{
		rswatches[i].Address = Watch.Address;
		rswatches[i].Index = Watch.Index;
		rswatches[i].Size = Watch.Size;
		rswatches[i].Type = Watch.Type;
		rswatches[i].WrongEndian = Watch.WrongEndian = 0;
		rsaddrs[Watch.Index].flags &= RS_FLAG_WATCHED;
		CompactAddrs();
		WatchCount++;
		RWfileChanged=true;
		return;
	}
//	MessageBox(NULL,"I am returning",NULL,MB_OK);
}
void CompactAddrs()
{
	const int typeSizeSize = ((rs_type_size=='b') ? 1 : (rs_type_size=='w') ? 2 : 4);
	for (int i,j = i = 0; i < CUR_RAM_MAX; i++)
	{
		if ((rsaddrs[i].flags & RS_FLAG_TEMP_ELIMINATED) && !((i % min(typeSizeSize,2))&&noMisalign))
		{
			rsaddrs[i].flags &= ~RS_FLAG_TEMP_ELIMINATED;
			rsresults[j].Index = i;
			rsresults[j].Address = getHardwareAddress(i);
			ResultCount++;
			j++;
		}
		else if (!((rsaddrs[i].flags & RS_FLAG_ELIMINATED) || (rsaddrs[i].flags & RS_FLAG_TEMP_ELIMINATED)))
		{
			if ((i % min(typeSizeSize,2)) && noMisalign)
			{
				rsaddrs[i].flags |= RS_FLAG_TEMP_ELIMINATED;
				ResultCount--;
			}
			else
			{
				rsresults[j].Index = i;
				rsresults[j].Address = getHardwareAddress(i);
				j++;
			}
		}
	}
	for (int i = 0; i < ResultCount; i++)
	{
		rsresults[i].cur = sizeConv(rsresults[i].Index,rs_type_size,"b");
		rsresults[i].prev = sizeConv(rsresults[i].Index,rs_type_size,"b",true);
	}
}
void ResetRamValues (unsigned int & rangeStart, unsigned int rangeSize, void* RamBuffer, bool firstTime)
{
	if (RamBuffer == (void *) _32X_Ram) Byte_Swap(RamBuffer,rangeSize);
	const unsigned char* RamBuf = (const unsigned char*) RamBuffer;
	const int typeSizeSize = (rs_type_size=='b') ? 1 : (rs_type_size=='w') ? 2 : 4;
	const unsigned int rangeEnd = rangeStart+rangeSize;
	for (unsigned int i = rangeStart; i < (rangeEnd - (typeSizeSize - 1)); i++)
	{
		unsigned char val = RamBuf[(i - rangeStart) ^ 1];

		if(firstTime)
		{
			rsaddrs[i].flags = 0;
			rsaddrs[i].comment = NULL;
		}
		rsaddrs[i].cur = rsaddrs[i].prev = val;
		rsaddrs[i].changes = 0;
		rsaddrs[i].flags &= ~(RS_FLAG_ELIMINATED | RS_FLAG_TEMP_ELIMINATED);
		ResultCount++;

		if (!(rsaddrs[i].flags & RS_FLAG_WATCHED))
		{
			if(rsaddrs[i].comment && !firstTime)
				delete[] rsaddrs[i].comment;
			rsaddrs[i].comment = NULL;
		}
	}
	rangeStart += rangeSize;
	if (RamBuffer == (void *) _32X_Ram) Byte_Swap(RamBuffer,rangeSize);
}
// caution: this function assumes it does not need to decrement ResultCount
void EliminateRamValues (unsigned int & rangeStart, unsigned int rangeSize, bool firstTime)
{
	const unsigned int rangeEnd = rangeStart+rangeSize;
	for (unsigned int i = rangeStart; i < rangeEnd; i++)
	{
		if(firstTime)
		{
			rsaddrs[i].flags = 0;
			rsaddrs[i].comment = NULL;
		}

		rsaddrs[i].flags |= RS_FLAG_ELIMINATED;
	}
	rangeStart += rangeSize;
}
void reset_address_info ()
{
	static bool firstTime = true;

	ResultCount = 0;

	unsigned int rangeStart = 0;
	if (Genesis_Started || _32X_Started || SegaCD_Started)
	{
		if (SegaCD_Started)
		{
			ResetRamValues(rangeStart, SEGACD_RAM_PRG_SIZE, Ram_Prg, firstTime);
			ResetRamValues(rangeStart, SEGACD_2M_RAM_SIZE, (Ram_Word_State & 0x2) ? Ram_Word_1M : Ram_Word_2M, firstTime);
		}
		ResetRamValues(rangeStart, Z80_RAM_SIZE, Ram_Z80, firstTime);
		ResetRamValues(rangeStart, _68K_RAM_SIZE, Ram_68k, firstTime);
		if (_32X_Started)
			ResetRamValues(rangeStart, _32X_RAM_SIZE, _32X_Ram, firstTime);
		//else EliminateRamValues(rangeStart, _32X_RAM_SIZE, firstTime);
	}
	EliminateRamValues(rangeStart, MAX_RAM_SIZE - rangeStart, firstTime);
	CompactAddrs();
	firstTime = false;
}
unsigned int HexStrToInt(char *s)
{
	unsigned int v=0;
	sscanf(s, "%8X", &v);
	return v;
}

void UpdateRamValues (unsigned int & rangeStart, unsigned int rangeSize, void* RamBuffer)
{
	const int typeSizeSize = (rs_type_size=='b') ? 1 : (rs_type_size=='w') ? 2 : 4;
	const unsigned int rangeEnd = rangeStart+rangeSize;
	if (RamBuffer == (void *) _32X_Ram) Byte_Swap(RamBuffer,rangeSize);
	const unsigned char* RamBuf = (const unsigned char*)RamBuffer;
	for (unsigned int i = rangeStart; i < rangeEnd; i++)
	{
		unsigned char val = RamBuf[(i - rangeStart) ^ 1];
		if (rsaddrs[i].cur != val)
		{
			rsaddrs[i].cur = val;
			for (int j = 0; j < typeSizeSize; j++)
				rsaddrs[i - j].changes++;			
		}
	}
	rangeStart = rangeEnd;
	if (RamBuffer == (void *) _32X_Ram) Byte_Swap(RamBuffer,rangeSize);
}
void signal_new_frame ()
{
	unsigned int rangeStart = 0;
	int ResultIndex,WatchIndex=ResultIndex=0;
	if (Genesis_Started || _32X_Started || SegaCD_Started)
	{
		if (SegaCD_Started)
		{
			UpdateRamValues(rangeStart, SEGACD_RAM_PRG_SIZE, Ram_Prg);
			UpdateRamValues(rangeStart, (Ram_Word_State & 0x2) ? SEGACD_1M_RAM_SIZE : SEGACD_2M_RAM_SIZE , (Ram_Word_State & 0x2) ? Ram_Word_1M : Ram_Word_2M);
		}
		UpdateRamValues(rangeStart, Z80_RAM_SIZE, Ram_Z80);
		UpdateRamValues(rangeStart, _68K_RAM_SIZE, Ram_68k);
		if (_32X_Started)
			UpdateRamValues(rangeStart, _32X_RAM_SIZE, _32X_Ram);
//		else rangeStart += _32X_RAM_SIZE;
	}
	CompactAddrs();
}

void Switch_Endian (void *ptr, int size)
{
	char *bef = (char *)ptr;
	char *aft = (char *) malloc(size);
	for (int i = 1; i <= size; i++)
		aft[i-1] = bef[size - i];
	for (int i = 0; i < size; i++)
		bef[i] = aft[i];
	free(aft);
}
// happens after a size change
//void SetCurRamValues (unsigned int & rangeStart, unsigned int rangeSize, void* RamBuffer)
//{
//}
void signal_new_size ()
{
	CompactAddrs();
	rs_last_type_size = rs_type_size;
}

// basic comparison functions:
template <typename T> inline bool LessCmp (T x, T y, T i)        { return x < y; }
template <typename T> inline bool MoreCmp (T x, T y, T i)        { return x > y; }
template <typename T> inline bool LessEqualCmp (T x, T y, T i)   { return x <= y; }
template <typename T> inline bool MoreEqualCmp (T x, T y, T i)   { return x >= y; }
template <typename T> inline bool EqualCmp (T x, T y, T i)       { return x == y; }
template <typename T> inline bool UnequalCmp (T x, T y, T i)     { return x != y; }
template <typename T> inline bool DiffByCmp (T x, T y, T p)      { return x - y == p || y - x == p; }

// compare-to type functions:
template <typename T>
void SearchRelative (bool(*cmpFun)(T,T,T), T ignored, T param)
{
	for(int i = 0; i < ResultCount; i++)
	{
		if(!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_ELIMINATED) && !cmpFun((T)rsresults[i].cur, (T)rsresults[i].prev, param))
		{
			if (!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_TEMP_ELIMINATED)) tempCount--;
			rsaddrs[rsresults[i].Index].flags &= ~RS_FLAG_TEMP_ELIMINATED;
			rsaddrs[rsresults[i].Index].flags |= RS_FLAG_ELIMINATED;
		}
	}
}
template <typename T>
void SearchSpecific (bool(*cmpFun)(T,T,T), T value, T param)
{
	for(int i = 0; i < ResultCount; i++)
	{
		if(!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_ELIMINATED) && !cmpFun((T)rsresults[i].cur, value, param))
		{
			if (!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_TEMP_ELIMINATED)) tempCount--;
			rsaddrs[rsresults[i].Index].flags &= ~RS_FLAG_TEMP_ELIMINATED;
			rsaddrs[rsresults[i].Index].flags |= RS_FLAG_ELIMINATED;
		}
	}
}
template <typename T>
void SearchAddress (bool(*cmpFun)(T,T,T), unsigned int address, T param)
{
	for(int i = 0; i < ResultCount; i++)
	{
		if(!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_ELIMINATED) && !cmpFun(getHardwareAddress(rsresults[i].Index), address, param))
		{
			if (!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_TEMP_ELIMINATED)) tempCount--;
			rsaddrs[rsresults[i].Index].flags &= ~RS_FLAG_TEMP_ELIMINATED;
			rsaddrs[rsresults[i].Index].flags |= RS_FLAG_ELIMINATED;
		}
	}
}
template <typename T>
void SearchChanges (bool(*cmpFun)(T,T,T), unsigned int changes, T param)
{
	for(int i = 0; i < ResultCount; i++)
	{
		if(!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_ELIMINATED) && !cmpFun(rsaddrs[rsresults[i].Index].changes, changes, param))
		{
			if (!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_TEMP_ELIMINATED)) tempCount--;
			rsaddrs[rsresults[i].Index].flags &= ~RS_FLAG_TEMP_ELIMINATED;
			rsaddrs[rsresults[i].Index].flags |= RS_FLAG_ELIMINATED;
		}
	}
}

void prune(char c,char o,char t,int v,int p)
{
	// repetition-reducing macros
	tempCount = ResultCount;
	#define DO_SEARCH(sf) \
	switch (o) \
	{ \
		case '<': DO_SEARCH_2(LessCmp,sf); break; \
		case '>': DO_SEARCH_2(MoreCmp,sf); break; \
		case '=': DO_SEARCH_2(EqualCmp,sf); break; \
		case '!': DO_SEARCH_2(UnequalCmp,sf); break; \
		case 'l': DO_SEARCH_2(LessEqualCmp,sf); break; \
		case 'm': DO_SEARCH_2(MoreEqualCmp,sf); break; \
		case 'd': DO_SEARCH_2(DiffByCmp,sf); break; \
		default: assert(!"Invalid operator for this search type."); break; \
	}

	// perform the search, eliminating nonmatching values
	switch (c)
	{
		#define DO_SEARCH_2(CmpFun,sf) \
		switch(rs_type_size) \
		{ \
			case 'b': t == 's' ? sf<signed char>(CmpFun,v,p)  : sf<unsigned char>(CmpFun,v,p);  break; \
			case 'w': t == 's' ? sf<signed short>(CmpFun,v,p) : sf<unsigned short>(CmpFun,v,p); break; \
			case 'd': t == 's' ? sf<signed long>(CmpFun,v,p)  : sf<unsigned long>(CmpFun,v,p);  break; \
		}
		case 'r': DO_SEARCH(SearchRelative); break;
		case 's': DO_SEARCH(SearchSpecific); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) sf<unsigned long>(CmpFun,v,p);
		case 'a': DO_SEARCH(SearchAddress); break;
		case 'n': DO_SEARCH(SearchChanges); break;

		default: assert(!"Invalid search comparison type."); break;
	}

	ResultCount = tempCount;
	CompactAddrs();
	// update address info after search
	for (int i = 0; i < ResultCount; i++)
	{
		if(!(rsaddrs[rsresults[i].Index].flags & RS_FLAG_ELIMINATED))
		{
			rsaddrs[rsresults[i].Index].prev = rsaddrs[rsresults[i].Index].cur;
			rsresults[i].prev = rsresults[i].cur;
			if (!preserveChanges)
				rsaddrs[i].changes = 0;
		}
	}

}

void Update_RAM_Search() //keeps RAM values up to date in the search and watch windows
{
	static DWORD previousTime = timeGetTime();
	if ((timeGetTime() - previousTime) < 0x40) return;
	int watchChanges[128];
	int changes[128];
	if(RamWatchHWnd)
	{
		// prepare to detect changes to displayed listview items
		HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv);
		int i;
		for(i = top; i <= top+count; i++)
			watchChanges[i-top] = (int)sizeConv(rswatches[i].Index,rswatches[i].Size);
	}
	if(RamSearchHWnd)
	{
		// prepare to detect changes to displayed listview items
		HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv);
		int i;
		for(i = top; i <= top+count; i++)
			changes[i-top] = rsaddrs[rsresults[i].Index].changes;
	}
	if (RamSearchHWnd || RamWatchHWnd)
	{
		// update all RAM values
		signal_new_frame();
	}
	if (AutoSearch)
	{
		int SetPaused = (!(Paused == 1));
		if (SetPaused) Paused = 1;
		//Clear_Sound_Buffer();
		prune(rs_c,rs_o,rs_t,rs_val,rs_param);

		if(!ResultCount)
		{
			reset_address_info();
			prune(rs_c,rs_o,rs_t,rs_val,rs_param);
			if(ResultCount && rs_c != 'a')
				MessageBox(HWnd,"Performing search on all addresses.","Out of results.",MB_OK|MB_ICONINFORMATION);
		}
		if (RamSearchHWnd) ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
		if (SetPaused) Paused = 0;
	}
	if (RamSearchHWnd)
	{
		HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv)+1;
		int i;
		// refresh any visible parts of the listview box that changed
		int start = -1;
		bool changed = false;
		for(i = top; i <= top+count; i++)
		{
			if(start == -1)
			{
				if((i != top+count) && (changes[i-top] != rsaddrs[rsresults[i].Index].changes))
				{
					start = i;
					if (!changed)
						changed = true;
				}
			}
			else
			{
				if((i == top+count) || (changes[i-top] == rsaddrs[rsresults[i].Index].changes))
				{
					ListView_RedrawItems(lv, start, i-1);
					start = -1;
				}
			}
		}
		if (changed)
			UpdateWindow(lv);
	}
	if (RamWatchHWnd)
	{
		HWND lv = GetDlgItem(RamWatchHWnd,IDC_WATCHLIST);
		int top = ListView_GetTopIndex(lv);
		int count = ListView_GetCountPerPage(lv)+1;
		int i;
		// refresh any visible parts of the listview box that changed
		int start = -1;
		bool changed = false;
		for(i = top; i <= top+count; i++)
		{
			if(start == -1)
			{
				if((i != top+count) && (watchChanges[i-top] != (int)sizeConv(rswatches[i].Index,rswatches[i].Size)))
				{
					start = i;
					if (!changed)
						changed = true;
				}
			}
			else
			{
				if((i == top+count) || (watchChanges[i-top] == (int)sizeConv(rswatches[i].Index,rswatches[i].Size)))
				{
					ListView_RedrawItems(lv, start, i-1);
					start = -1;
				}
			}
		}
		UpdateWindow(lv);
	}
	previousTime = timeGetTime();
}

LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_INITDIALOG: {
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

			// push it away from the main window if we can
			const int width = (r.right-r.left); 
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

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			switch(rs_o)
			{
				case '<':
					SendDlgItemMessage(hDlg, IDC_LESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '>':
					SendDlgItemMessage(hDlg, IDC_MORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'l':
					SendDlgItemMessage(hDlg, IDC_NOMORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'm':
					SendDlgItemMessage(hDlg, IDC_NOLESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '=': 
					SendDlgItemMessage(hDlg, IDC_EQUALTO, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '!':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTFROM, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTBY, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					break;
			}
			switch (rs_c)
			{
				case 'r':
					SendDlgItemMessage(hDlg, IDC_PREVIOUSVALUE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 's':
					SendDlgItemMessage(hDlg, IDC_SPECIFICVALUE, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					break;
				case 'a':
					SendDlgItemMessage(hDlg, IDC_SPECIFICADDRESS, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					break;
				case 'n':
					SendDlgItemMessage(hDlg, IDC_NUMBEROFCHANGES, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					break;
			}
			switch (rs_t)
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
			}
			switch (rs_type_size)
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
			}
			SendDlgItemMessage(hDlg,IDC_C_AUTOSEARCH,BM_SETCHECK,AutoSearch?BST_CHECKED:BST_UNCHECKED,0);
			char names[5][11] = {"Address","Value","Previous","Changes","Notes"};
			int widths[5] = {62,64,64,51,53};
			if (!ResultCount)
				reset_address_info();
			else
				signal_new_frame();
			init_list_box(GetDlgItem(hDlg,IDC_RAMLIST),names,5,widths);
			ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
			if (!noMisalign) SendDlgItemMessage(hDlg, IDC_MISALIGN, BM_SETCHECK, BST_CHECKED, 0);
			if (littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);
			return true;
		}	break;

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
							sprintf(num,"%08X",rsresults[iNum].Address);
							Item->item.pszText = num;
							return true;
						case 1: {
							int i;
							int size;
							switch (rs_type_size)
							{

								case 'w':
									size = 2;
									break;
								case 'd':
									size = 4;
									break;
								case 'b':
								default:
									size = 1;
									break;									
							}
							i = rsresults[Item->item.iItem].cur;
//							for (int j = 1; j < size; j++)
//								i |= rsaddrs[rsresults[Item->item.iItem].Index + j].cur << (8 * j);
//							Byte_Swap(&i,size);
							sprintf(num,
								((rs_t == 's')?
									"%d":
									(rs_t == 'u')?
										"%u":
										"%X"),
								(rs_t != 's')?
									i:
									((rs_type_size == 'b')?
										(char)(i&0xff):
										(rs_type_size == 'w')?
											(short)(i&0xffff):
											(long)(i&0xffffffff)));
							Item->item.pszText = num;
						}	return true;
						case 2: {
							int i;
							int size;
							switch (rs_type_size)
							{

								case 'w':
									size = 2;
									break;
								case 'd':
									size = 4;
									break;
								case 'b':
								default:
									size = 1;
									break;									
							}
							i = rsresults[Item->item.iItem].prev;
//							for (int j = 1; j < size; j++)
//								i |= rsaddrs[rsresults[Item->item.iItem].Index + j].prev << (8 * j);
//							Byte_Swap(&i,size);
							sprintf(num,
								((rs_t == 's')?
									"%d":
									(rs_t == 'u')?
										"%u":
										"%X"),
								(rs_t != 's')?
									i:
									((rs_type_size == 'b')?
										(char)(i&0xff):
										(rs_type_size == 'w')?
											(short)(i&0xffff):
											(long)(i&0xffffffff)));
							Item->item.pszText = num;
						}	return true;
						case 3:
							sprintf(num,"%d",rsaddrs[rsresults[iNum].Index].changes);
							Item->item.pszText = num;
							return true;
						case 4:
							Item->item.pszText = rsaddrs[rsresults[iNum].Index].comment ? rsaddrs[rsresults[iNum].Index].comment : "";
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
				case IDC_SIGNED:
					rs_t='s';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_UNSIGNED:
					rs_t='u';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_HEX:
					rs_t='h';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_1_BYTE:
					rs_type_size = 'b';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_2_BYTES:
					rs_type_size = 'w';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_4_BYTES:
					rs_type_size = 'd';
					signal_new_size();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_MISALIGN:
					noMisalign = !noMisalign;
					CompactAddrs();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_ENDIAN:
//					littleEndian = !littleEndian;
//					signal_new_size();
//					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;				
				case IDC_LESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '<';
					return true;
				case IDC_MORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '>';
					return true;
				case IDC_NOMORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = 'l';
					return true;
				case IDC_NOLESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = 'm';
					return true;
				case IDC_EQUALTO:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '=';
					return true;
				case IDC_DIFFERENTFROM:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					rs_o = '!';
					return true;
				case IDC_DIFFERENTBY:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					rs_o = 'd';
					return true;
				case IDC_PREVIOUSVALUE:
					rs_c='r';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_SPECIFICVALUE:
					rs_c='s';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_SPECIFICADDRESS:
					rs_c='a';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					return true;
				case IDC_NUMBEROFCHANGES:
					rs_c='n';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					return true;
				case IDC_C_ADDCHEAT:
				{
//					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
//					Liste_GG[CheatCount].restore = Liste_GG[CheatCount].data = rsresults[watchIndex].cur;
//					Liste_GG[CheatCount].addr = rsresults[watchIndex].Address;
//					Liste_GG[CheatCount].size = rs_type_size;
//					Liste_GG[CheatCount].Type = rs_t;
//					Liste_GG[CheatCount].oper = '=';
//					Liste_GG[CheatCount].mode = 0;
//					DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_EDITCHEAT), hDlg, (DLGPROC) EditCheatProc,(LPARAM) 0);
				}
				case IDC_C_SAVE:
					// NYI
					return true;
				case IDC_C_LOAD:
					// NYI
					//strncpy(Str_Tmp,Rom_Name,512);
					////strcat(Str_Tmp,".gmv");
					//if(Change_File_S(Str_Tmp, Movhie_Dir, "Save Movie", "GENs Movie\0*.gmv*\0All Files\0*.*\0\0", "gmv"))
					//{
					//		SendDlgItemMessage(hDlg,IDC_EDIT_MOVIE_NAME,WM_SETTEXT,0,(LPARAM)Str_Tmp);
					//}
					return true;
				case IDC_C_RESET:
					reset_address_info();
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				case IDC_C_AUTOSEARCH:
					AutoSearch = !AutoSearch;
					if (!AutoSearch) return true;
				case IDC_C_SEARCH:
				{
					rs_param = ((rs_o == 'd') ? GetDlgItemInt(hDlg,IDC_EDIT_DIFFBY,NULL,false) : 0);

					switch(rs_c)
					{
						case 'r':
						default:
							rs_val = 0;
							break;
						case 's':
							if(rs_t == 'h')
							{
								if(!GetDlgItemText(hDlg,IDC_EDIT_COMPAREVALUE,Str_Tmp,9))
									goto invalid_field;
								rs_val = HexStrToInt(Str_Tmp);
							}
							else
							{
								BOOL success;
								rs_val = GetDlgItemInt(hDlg,IDC_EDIT_COMPAREVALUE,&success,(rs_t == 's'));
								if(!success)
									goto invalid_field;
							}
							if((rs_type_size == 'b' && rs_t == 's' && (rs_val < -128 || rs_val > 127)) ||
							   (rs_type_size == 'b' && rs_t != 's' && (rs_val < 0 || rs_val > 255)) ||
							   (rs_type_size == 'w' && rs_t == 's' && (rs_val < -32768 || rs_val > 32767)) ||
							   (rs_type_size == 'w' && rs_t != 's' && (rs_val < 0 || rs_val > 65535)))
							   goto invalid_field;
							break;
						case 'a':
							if(!GetDlgItemText(hDlg,IDC_EDIT_COMPAREADDRESS,Str_Tmp,9))
								goto invalid_field;
							rs_val = HexStrToInt(Str_Tmp);
							if(rs_val < 0 || rs_val > 0x0603FFFF)
								goto invalid_field;
							break;
						case 'n': {
							BOOL success;
							rs_val = GetDlgItemInt(hDlg,IDC_EDIT_COMPARECHANGES,&success,false);
							if(!success || rs_val < 0)
								goto invalid_field;
						}	break;
					}

					int SetPaused = (!(Paused == 1));
					if (SetPaused) Paused = 1;
					Clear_Sound_Buffer();
					prune(rs_c,rs_o,rs_t,rs_val,rs_param);

					if(!ResultCount)
					{
						reset_address_info();
						prune(rs_c,rs_o,rs_t,rs_val,rs_param);
						if(ResultCount && rs_c != 'a')
							MessageBox(HWnd,"Performing search on all addresses.","Out of results.",MB_OK|MB_ICONINFORMATION);
					}

					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					if (SetPaused) Paused = 0;
					return true;

invalid_field:
					MessageBox(HWnd,"Invalid or out-of-bound entered value.","Error",MB_OK|MB_ICONSTOP);
					return true;
				}
				case IDC_C_WATCH:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
					rswatches[WatchCount].Index = rsresults[watchIndex].Index;
					rswatches[WatchCount].Address = rsresults[watchIndex].Address;
					rswatches[WatchCount].Size = rs_type_size;
					rswatches[WatchCount].Type = rs_t;
					rswatches[WatchCount].WrongEndian = 0; //Replace when I get little endian working
					DialogBox(ghInstance, MAKEINTRESOURCE(IDD_PROMPT), hDlg, (DLGPROC) PromptWatchNameProc);
					return true;
				}
				case IDC_C_WATCH_EDIT:
				{
					watchIndex = ListView_GetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST));
					rsaddrs[rsresults[watchIndex].Index].flags |= RS_FLAG_ELIMINATED;
					ResultCount--;
					signal_new_size();
//					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),0);
					ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					return true;
				}
				case IDOK:
				case IDCANCEL:
					if (Full_Screen)
					{
						while (ShowCursor(true) < 0);
						while (ShowCursor(false) >= 0);
					}
					DialogsOpen--;
					RamSearchHWnd = NULL;
					EndDialog(hDlg, true);
					return true;
			}
			break;

		case WM_CLOSE:
			if (Full_Screen)
			{
				while (ShowCursor(true) < 0);
				while (ShowCursor(false) >= 0);
			}
			DialogsOpen--;
			RamSearchHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}
LRESULT CALLBACK PromptWatchNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //Gets the description of a watched address
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;

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
					AddressWatcher Temp;
					Temp = rswatches[WatchCount];
					InsertWatch(Temp,Str_Tmp);
					if(RamWatchHWnd)
					{
						ListView_SetItemCount(GetDlgItem(RamWatchHWnd,IDC_WATCHLIST),WatchCount);
					}
					DialogsOpen--;
					EndDialog(hDlg, true);
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