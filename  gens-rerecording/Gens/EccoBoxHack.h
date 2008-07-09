#ifndef _ECCOBOXHACK_H
#define _ECCOBOXHACK_H
#include "hackdefs.h"
#ifdef ECCOBOXHACK
#define CAMXPOS	0xFFAE08
#define CAMYPOS 0xFFAE06
void EccoDraw3D();
void EccoDrawBoxes();
void EccoAutofire();
#endif
#endif