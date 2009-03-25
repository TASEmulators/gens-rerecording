#ifndef _M68KD_H_
#define _M68KD_H_


#ifdef __cplusplus
extern "C" {
#endif

char *M68KDisasm(unsigned short (*NW)(), unsigned int (*NL)());
char *M68KDisasm2(unsigned short (*NW)(), unsigned int (*NL)(), unsigned int hook_pc);


#ifdef __cplusplus
};
#endif

#endif

