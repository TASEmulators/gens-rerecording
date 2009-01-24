#ifndef SCRSHOT_H
#define SCRSHOT_H

extern char ScrShot_Dir[1024];
extern char AVIFileName[1024];
extern int AVIRecording,AVISound,AVIWaitMovie,AVIBreakMovie,AVISplit,AVICorrect256AspectRatio;

int Save_Shot(unsigned char *Screen, int mode, int X, int Y, int Pitch);
int Save_Shot_AVI(void* VideoBuf,int mode, int Hmode, int Vmode,HWND hWnd);
int Close_AVI();
int InitAVI(HWND hWnd);
int UpdateSoundAVI(unsigned char * buf,unsigned int length,int Rate, int Stereo);

#endif