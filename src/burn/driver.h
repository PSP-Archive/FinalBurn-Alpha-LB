/*
 * For the MAME sound cores
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>

#if !defined (_WIN32)
 #define __cdecl
#endif

#ifndef INLINE
 #define INLINE static __inline__
#endif

#define ALIGN_DATA  __attribute__((aligned(4)))

#define FBA

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed int INT32;
typedef unsigned int UINT32;
typedef signed long long INT64;
typedef unsigned long long UINT64;
#define OSD_CPU_H

/* OPN */
#define HAS_YM2203  1
#define HAS_YM2608  0
#define HAS_YM2610  0
#define HAS_YM2610B 0
#define HAS_YM2612  0
#define HAS_YM3438  0
/* OPL */
#define HAS_YM3812  0
#define HAS_YM3526  0
#define HAS_Y8950   0

enum {
	CLEAR_LINE = 0,
	ASSERT_LINE,
	HOLD_LINE,
	PULSE_LINE
};

#define timer_get_time() BurnTimerGetTime()

#define READ8_HANDLER(name) 	UINT8 name(void)
#define WRITE8_HANDLER(name) 	void  name(UINT8 data)

#ifdef __cplusplus
 extern "C" {
#endif
  float BurnTimerGetTime(void);

  typedef UINT8 (*read8_handler)(UINT32 offset);
  typedef void (*write8_handler)(UINT32 offset, UINT32 data);

 #ifdef MAME_USE_LOGERROR
  void __cdecl logerror(char* szFormat, ...);
 #else
  #define logerror
 #endif
#ifdef __cplusplus
 }
#endif

#endif /* DRIVER_H */
