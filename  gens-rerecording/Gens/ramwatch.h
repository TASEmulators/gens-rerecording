#ifndef RAMWATCH_H
#define RAMWATCH_H
bool ResetWatches();
void OpenRWRecentFile(int memwRFileNumber);
extern bool AutoRWLoad;
#define MAX_RECENT_WATCHES 5
extern char rw_recent_files[MAX_RECENT_WATCHES][1024];
extern bool AskSave();
extern int ramw_x;
extern int ramw_y;
extern bool RWfileChanged;

// AddressWatcher is self-contained now
struct AddressWatcher
{
	unsigned int Address; // hardware address
	char Size;
	char Type;
	char* comment; // NULL means no comment, non-NULL means allocated comment
	bool WrongEndian;
	unsigned int CurValue;
};
extern AddressWatcher rswatches[256];
extern int WatchCount; // number of valid items in rswatches
#define MAX_WATCH_COUNT (sizeof(rswatches)/sizeof(*rswatches))

extern char Watch_Dir[1024];

bool InsertWatch(const AddressWatcher& Watch, char *Comment);
bool InsertWatch(const AddressWatcher& Watch, HWND parent=NULL); // asks user for comment
void Update_RAM_Watch();

#endif