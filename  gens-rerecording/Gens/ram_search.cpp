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
