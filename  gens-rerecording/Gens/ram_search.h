#ifndef RAM_SEARCH_H
#define RAM_SEARCH_H

#ifdef __cplusplus
extern "C" {
#endif

//64k in Ram_68k[], 8k in Ram_Z80[]   
#define _68K_RAM_SIZE 64*1024
#define Z80_RAM_SIZE 8*1024
/*#define SRAM_SIZE (((SRAM_End - SRAM_Start) > 2) ? SRAM_End - SRAM_Start : 0)
#define BRAM_SIZE ((8 << BRAM_Ex_Size) * 1024)*/
#define GENESIS_RAM_SIZE (_68K_RAM_SIZE + Z80_RAM_SIZE)

//_32X_Ram[]
#define _32X_RAM_SIZE 256*1024

//512k in Ram_Prg, 256k in Ram_Word_1M and Ram_Word_2M
//(docs say 6Mbit of ram, but I'm not sure what's used when)
#define SEGACD_RAM_PRG_SIZE 512*1024
#define SEGACD_1M_RAM_SIZE 256*1024
#define SEGACD_2M_RAM_SIZE 256*1024

#define SEGACD_RAM_SIZE (SEGACD_RAM_PRG_SIZE + SEGACD_2M_RAM_SIZE)
#define MAX_TYPE_SIZE 4


#define RAM_VAL(x) ( \
        (x)<(1024*64) ?  \
        Ram_68k[(x)] :  \
        Ram_Z80[(x)- 1024*64] \
        )
//how many ram values can appear onscreen at one time    
#define RAM_VALS_SHOW 15
#define MAX_RAM_SIZE (0xD2000)
#define CUR_RAM_MAX (SegaCD_Started ? (SEGACD_RAM_SIZE + GENESIS_RAM_SIZE): (_32X_Started ? _32X_RAM_SIZE + GENESIS_RAM_SIZE : (Genesis_Started ? GENESIS_RAM_SIZE : 0)))

#define RS_FLAG_ELIMINATED 0x01      // not in search results
#define RS_FLAG_TEMP_ELIMINATED 0x02 // not in search results but still to be updated
#define RS_FLAG_WATCHED    0x04      // watched, possibly with a comment

struct AddressInfo
{
	unsigned char cur; // current value, may not be kept up to date during fast-forward
	unsigned char prev; // value at last search or reset
	unsigned int changes; // number of times value has changed since the last search
	char *comment; // NULL means no comment, non-NULL means allocated comment
	unsigned char flags; // RS_FLAG_...
};
struct AddressWatcher
{
	unsigned int Address;
	int Index;
	char Size;
	char Type;
	bool WrongEndian;
};
struct AddrIndex
{
	unsigned int Address;
	unsigned int cur;
	unsigned int prev;
	int Index;
};
extern AddressInfo rsaddrs [MAX_RAM_SIZE]; //0xD2000
extern AddrIndex rsresults[MAX_RAM_SIZE];	// address numbers, which are also array indices into rsaddrs 
//UpthModif - when I fixed RamSearch to display hardware accurate addresses, I had the bright idea to add
//an address component to this array. Now that it's working that way I can't easily change it back.
extern AddressWatcher rswatches[256]; //contains an array index to rsaddrs, and some information about the data format. 
extern int ResultCount; // number of valid items in rsresults
extern int WatchCount; // number of valid items in rswatches
extern char rs_type_size;
extern char Watch_Dir[1024];

//how many ram values can appear onscreen at one time    
//#define RAM_VALS_SHOW 15
//#define MAX_RAM_SIZE (GENESIS_RAM_SIZE + _32X_RAM_SIZE + SEGACD_RAM_SIZE)

int getSearchIndex(unsigned int HA,int bot, int top);
void Switch_Endian(void *ptr,int size);
unsigned int sizeConv(unsigned int index,char size, char *prevSize = &rs_type_size, bool usePrev = false);
unsigned int GetRamValue(unsigned int Addr,char Size);
unsigned int HexStrToInt(char *);
void InsertWatch(AddressWatcher Watch, char *Comment);
void prune(char Search, char Operater, char Type, int Value, int OperatorParameter);
void CompactAddrs();
void reset_address_info();
void signal_new_frame();
void signal_new_size();
extern int curr_ram_size;
extern unsigned int good,prev,curr;
extern int show_ram[RAM_VALS_SHOW]; 
extern bool noMisalign;
extern bool littleEndian;

#ifdef __cplusplus
}
#endif

#endif

