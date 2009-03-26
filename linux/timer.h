#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif
void init_timer();
void GetPerformanceFrequency(long long *freq);
void GetPerformanceCounter(long long *now);
unsigned long gettime();
#ifdef __cplusplus
}
#endif
#endif
