#ifndef _SONICHACKSUITE_H
#define _SONICHACKSUITE_H
#include "hackdefs.h"
#ifdef SONICMAPHACK
#define SCROLLSPEED 8
#define POWEROFTWO 0
void Update_RAM_Cheats();
extern long x,xg,y,yg;
#elif defined SONICROUTEHACK
#define POWEROFTWO 0
void Update_RAM_Cheats();
#endif
#ifdef SONICCAMHACK
int SonicCamHack();
extern "C" int disableSound, disableSound2, disableRamSearchUpdate;
#endif
#endif