#ifndef HACKDEFS
#define HACKDEFS

// TODO: it would be nice if these were runtime-togglable instead of hardcoded defines
// Agreed, I've been working out how auto-detection of games and rom version would work for some of these for a while.

//#define SONICCAMHACK // enables camhack (see Sonic offscreen) in sonic games, and hitbox/solidity display
//#define SONICSPEEDHACK // enables speed/position printout in sonic games
//#define SONICMAPHACK // enables (not fully automatic) dumping of sonic level maps to bitmap
//#define SONICROUTEHACK // enables semi-automatic dumping of sonic routes maps to bitmap
#define SONICNOHITBOXES // disables hitbox display of SONICCAMHACK
#define SONICNOSOLIDITYDISPLAY // disables solidity display of SONICCAMHACK
//#define SK // specifies that the sonic game being used is Sonic 3-based (S3, SK, S3&K)
#define S2 // specifies that the sonic game being used is Sonic 2-based
//#define S1 // specifies that the sonic game being used is Sonic 1-based (S1, S1MM, S1MM_CD)
//#define GAME_SCD // specifies that the sonic game being used is Sonic CD-based. Renamed from SCD because an emulation struct is called SCD and that was creating confusion
//#define S1MM_CD
//#define RISTAR
//#define RKABOXHACK
//#define ECCOBOXHACK
//#define ECCO1BOXHACK

#endif
