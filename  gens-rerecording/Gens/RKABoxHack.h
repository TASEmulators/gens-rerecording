#ifndef _RKABOXHACK_H
#define _RKABOXHACK_H
#include "hackdefs.h"
#ifdef RKABOXHACK
#define POSOFFSET 0xC000
#define SSTLEN 0x2000
#define SPRITESIZE 0x80
#define CAMXPOS	0xB1D2
#define CAMYPOS 0xB1D6
#define XPo 0x06
#define YPo 0x0A
#define Wo 0x36
#define Ho 0x38
#define XVo 0x0E
#define YVo 0x12
void RKADrawBoxes();
#endif
#endif