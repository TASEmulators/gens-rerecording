#ifndef RAMWATCH_H
#define RAMWATCH_H
void ResetWatches();
void OpenRWRecentFile(int memwRFileNumber);
extern bool AutoRWLoad;
extern char rw_recent_files[5][1024];
extern bool AskSave();
extern int ramw_x;
extern int ramw_y;
extern bool RWfileChanged;
#endif