#ifndef _ECCOBOXHACK_H
#define _ECCOBOXHACK_H
#include "hackdefs.h"
#if (defined ECCOBOXHACK) || (defined ECCO1BOXHACK)
#define CAMXPOS	0xFFB836
#define CAMYPOS 0xFFB834
void EccoDraw3D();
void EccoDrawBoxes();
void EccoAutofire();
#endif
#endif