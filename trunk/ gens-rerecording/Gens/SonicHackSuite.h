#ifndef _SONICHACKSUITE_H
#define _SONICHACKSUITE_H
#include "hackdefs.h"
#ifdef SONICMAPHACK
#define POWEROFTWO 0
#define SCROLLSPEED 8
void Update_RAM_Cheats();
extern long x,xg,y,yg;
#endif
#ifdef SONICCAMHACK
int SonicCamHack();
extern "C" int disableSound, disableSound2, disableRamSearchUpdate;
#endif
#endif