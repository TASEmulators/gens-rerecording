#ifndef __PORT_H__
#define __PORT_H__

#ifdef WITH_GTK
#ifndef __PORT__
#error WITH_GTK is defined while __PORT__ is not !
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */
#ifdef __PORT__
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef void VOID;
typedef int HWND;
typedef int HINSTANCE;
typedef int WNDCLASS;
typedef int HMENU;
typedef struct _POINT {
	int x;
	int y;
} POINT;
#define FAR
#define SEPERATOR '/'
#define default_path "./"
#define CHAR_SEP "/"
enum { MB_OK, MB_ICONEXCLAMATION };
void SetWindowText(int hwnd, const char *text);
void SetCurrentDirectory(const char *directory);
int GetCurrentDirectory(int size,char* buf);
int stricmp(const char *str1, const char *str2);
int	strnicmp(const char *str1, const char *str2,
	unsigned long nchars);
void MessageBox(int parent, const char *text,
	const char *error, int type);
unsigned long GetTickCount();
int GetPrivateProfileInt(const char *section, const char *var,
	int def, const char *filename);
void GetPrivateProfileString(const char *section, const char *var,
	const char *def, char *get, int length, const char *filename);
void WritePrivateProfileString(const char *section, const char *var,
	const char *var_name, const char *filename);
void DestroyMenu(HMENU hmenu);
void SetMenu(HWND hwnd, HMENU hmenu);
int Exists(const char *filename);
#else
#include <windows.h>
#define SEPERATOR '\\'
#define default_path ".\\"
#define CHAR_SEP "\\"
#endif
#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif
