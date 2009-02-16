#include "Rom.h"

#ifndef _GENS_H
#define _GENS_H

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

//#define CLOCK_NTSC 53700000			// More accurate for division round
//#define CLOCK_PAL  53200000

#define CLOCK_NTSC 53693175
#define CLOCK_PAL  53203424

#define GENS_DEBUG

extern int Debug;
extern int Frame_Skip;
extern int Frame_Number;
extern int Inside_Frame;
extern int DAC_Improv;
extern int RMax_Level;
extern int GMax_Level;
extern int BMax_Level;
extern int Contrast_Level;
extern int Brightness_Level;
extern int Greyscale;
extern int Invert_Color;
extern int XRay;
extern unsigned int Pal32_XRAY[0x10000];

int Round_Double(double val);
void Init_Tab(void);
void Recalculate_Palettes(void);
void Check_Country_Order(void);
char* Detect_Country_SegaCD(void);
void Detect_Country_Genesis(void);

void Init_Genesis_Bios(void);
int Init_Genesis(struct Rom *MD_Rom);
void Reset_Genesis();
int Do_VDP_Only(void);
int Do_Genesis_Frame_No_VDP(void);
int Do_Genesis_Frame(void);

int Init_32X(struct Rom *MD_Rom);
void Reset_32X();
int Do_32X_Frame_No_VDP(void);
int Do_32X_Frame(void);

int Init_SegaCD(char *iso_name);
int Reload_SegaCD(char *iso_name);
void Reset_SegaCD(void);
int Do_SegaCD_Frame_No_VDP(void);
int Do_SegaCD_Frame(void);
int Do_SegaCD_Frame_Cycle_Accurate(void);
int Do_SegaCD_Frame_No_VDP_Cycle_Accurate(void);

BOOL IsAsyncAllowed(void);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

// disable warnings about all common string operations being deprecated in newer VS versions
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
	#pragma warning(disable : 4996)
#endif

#endif