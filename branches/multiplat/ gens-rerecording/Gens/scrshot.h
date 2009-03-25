#ifndef SCRSHOT_H
#define SCRSHOT_H

extern char ScrShot_Dir[1024];
extern char AVIFileName[1024];
extern int AVIRecording,AVISound,AVIWaitMovie,AVIBreakMovie,AVISplit,AVIHeight224IfNotPAL,ShotPNGFormat;

int Save_Shot(void* Screen,int mode, int Hmode, int Vmode);
int Save_Shot_AVI(void* VideoBuf,int mode, int Hmode, int Vmode,HWND hWnd);
int Close_AVI();
int InitAVI(HWND hWnd);
int UpdateSoundAVI(unsigned char * buf,unsigned int length,int Rate, int Stereo);

#endif