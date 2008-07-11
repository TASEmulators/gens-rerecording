#ifndef _HACKSCOMMON_H
#define _HACKSCOMMON_H
#include "mem_m68k.h"
#include "hackdefs.h"
template <typename T>
T CheatRead (unsigned int address)
{
	T val = 0;
	for(int i = 0 ; i < sizeof(T) ; i++)
		val <<= 8, val |= (T)(M68K_RB(address+i));
	return val;
}
template <typename T>
void CheatWrite (unsigned int address, T Value)
{
	for (int i = sizeof(T) - 1; i >= 0; i--)
	{
		M68K_WB(address + i, (unsigned char)(Value & 0xFF));
		Value >>= 8;
	}
}
extern unsigned short CamX;
#ifdef SONICCAMHACK
extern short CamY;
#else
extern unsigned short CamY;
#endif
#endif