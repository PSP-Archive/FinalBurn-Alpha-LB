/*
  File: fm.h -- header file for software emulation for FM sound generator

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

/* --- select emulation chips --- */
#define BUILD_YM2203  (HAS_YM2203)					/* build YM2203(OPN)   emulator */
#define BUILD_YM2608  (HAS_YM2608)					/* build YM2608(OPNA)  emulator */
#define BUILD_YM2610  (HAS_YM2610)					/* build YM2610(OPNB)  emulator */
#define BUILD_YM2610B (HAS_YM2610B)					/* build YM2610B(OPNB?)emulator */
#define BUILD_YM2612  (HAS_YM2612 || HAS_YM3438)	/* build YM2612(OPN2)  emulator */

//#define BUILD_YM2151  (HAS_YM2151)				/* build YM2151(OPM)   emulator */

/* select bit size of output : 8 or 16 */
#define FM_SAMPLE_BITS 16

/* select timer system internal or external */
#define FM_INTERNAL_TIMER 0

/* --- speedup optimize --- */
/* busy flag enulation , The definition of FM_GET_TIME_NOW() is necessary. */
#define FM_BUSY_FLAG_SUPPORT 1

/* --- external SSG(YM2149/AY-3-8910)emulator interface port */
/* used by YM2203,YM2608,and YM2610 */

/* SSGClk   : Set SSG Clock      */
/* UINT8 n  = chip number        */
/* int clk  = MasterClock(Hz)    */
#define SSGClk(chip, clock) AY8910_set_clock((chip) + ay8910_index_ym, clock)

/* SSGWrite : Write SSG port     */
/* UINT8 n  = chip number        */
/* UINT8 a  = address            */
/* UINT8 v  = data               */
#define SSGWrite(n, a, v) AY8910Write((n) + ay8910_index_ym, a, v)

/* SSGRead  : Read SSG port */
/* UINT8 n  = chip number   */
/* return   = Read data     */
#define SSGRead(n) AY8910Read((n) + ay8910_index_ym)

/* SSGReset : Reset SSG chip */
/* UINT8 n  = chip number   */
#define SSGReset(chip) AY8910Reset((chip) + ay8910_index_ym)


/* --- external callback funstions for realtime update --- */

/* for busy flag emulation , function FM_GET_TIME_NOW() should */
/* return present time in seconds with "double" precision  */
  /* in timer.c */
  #define FM_GET_TIME_NOW() timer_get_time()

#if BUILD_YM2203
  /* in 2203intf.c */
  void BurnYM2203UpdateRequest(void);
  #define YM2203UpdateReq(chip) BurnYM2203UpdateRequest()
#endif

#if BUILD_YM2608
  /* in 2608intf.c */
  void BurnYM2608UpdateRequest(void);
  #define YM2608UpdateReq(chip) BurnYM2608UpdateRequest()
#endif

#if BUILD_YM2610
  /* in 2610intf.c */
  void BurnYM2610UpdateRequest(void);
  #define YM2610UpdateReq(chip) BurnYM2610UpdateRequest()
#endif

#if BUILD_YM2612
  /* in 2612intf.c */
  void BurnYM2612UpdateRequest(void);
  #define YM2612UpdateReq(chip) BurnYM2612UpdateRequest()
#endif

#if 0 //BUILD_YM2151
  /* in 2151intf.c */
  #define YM2151UpdateReq(chip) YM2151UpdateRequest(chip);
#endif


#ifndef INLINE
  #define INLINE static __inline__
#endif


#if (FM_SAMPLE_BITS==16)
typedef INT16 FMSAMPLE;
#endif

#if (FM_SAMPLE_BITS==8)
typedef unsigned char  FMSAMPLE;
#endif

typedef void (*FM_TIMERHANDLER)(int n, int c, int cnt, float stepTime);
typedef void (*FM_IRQHANDLER)(int n, int irq);
/* FM_TIMERHANDLER : Stop or Start timer         */
/* int n          = chip number                  */
/* int c          = Channel 0=TimerA,1=TimerB    */
/* int count      = timer count (0=stop)         */
/* doube stepTime = step time of one count (sec.)*/

/* FM_IRQHHANDLER : IRQ level changing sense     */
/* int n       = chip number                     */
/* int irq     = IRQ level 0=OFF,1=ON            */

//#if BUILD_YM2203
#if 1
/* -------------------- YM2203(OPN) Interface -------------------- */

/*
** Initialize YM2203 emulator(s).
**
** 'num'           is the number of virtual YM2203's to allocate
** 'baseclock'
** 'rate'          is sampling rate
** 'TimerHandler'  timer callback handler when timer start and clear
** 'IRQHandler'    IRQ callback handler when changed IRQ level
** return      0 = success
*/
INT8 YM2203Init(UINT8 num, int baseclock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler);

/*
** shutdown the YM2203 emulators
*/
void YM2203Shutdown(void);

/*
** reset all chip registers for YM2203 number 'num'
*/
void YM2203ResetChip(UINT8 num);

/*
** update one of chip
*/
void YM2203UpdateOne(UINT8 num, INT16 *buffer, UINT16 length);

/*
** Write
** return : InterruptLevel
*/
UINT8 YM2203Write(UINT8 n, UINT8 a, UINT8 v);

/*
** Read
** return : InterruptLevel
*/
UINT8 YM2203Read(UINT8 n, UINT8 a);

/*
**	Timer OverFlow
*/
UINT8 YM2203TimerOver(UINT8 n, UINT8 c);

#endif /* BUILD_YM2203 */


#if BUILD_YM2608
/* -------------------- YM2608(OPNA) Interface -------------------- */
INT8 YM2608Init(UINT8 num, int baseclock, int rate,
               void **pcmroma,int *pcmsizea,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler);
void YM2608Shutdown(void);
void YM2608ResetChip(UINT8 num);
void YM2608UpdateOne(UINT8 num, INT16 **buffer, UINT16 length);

UINT8 YM2608Write(UINT8 n, UINT8 a, UINT8 v);
UINT8 YM2608Read(UINT8 n, UINT8 a);
UINT8 YM2608TimerOver(UINT8 n, UINT8 c);
#endif /* BUILD_YM2608 */


#if (BUILD_YM2610||BUILD_YM2610B)
/* -------------------- YM2610(OPNB) Interface -------------------- */
INT8 YM2610Init(UINT8 num, int baseclock, int rate,
                void **pcmroma, int *pcmasize, void **pcmromb, int *pcmbsize,
                FM_TIMERHANDLER TimerHandler, FM_IRQHANDLER IRQHandler);
void YM2610Shutdown(void);
void YM2610ResetChip(UINT8 num);
void YM2610UpdateOne(UINT8 num, INT16 **buffer, UINT16 length);
#if BUILD_YM2610B
void YM2610BUpdateOne(UINT8 num, INT16 **buffer, UINT16 length);
#endif

UINT8 YM2610Write(UINT8 n, UINT8 a, UINT8 v);
UINT8 YM2610Read(UINT8 n, UINT8 a);
UINT8 YM2610TimerOver(UINT8 n, UINT8 c);
#endif /* BUILD_YM2610 */


#if BUILD_YM2612
INT8 YM2612Init(UINT8 num, int baseclock, int rate, FM_TIMERHANDLER TimerHandler, FM_IRQHANDLER IRQHandler);
void YM2612Shutdown(void);
void YM2612ResetChip(UINT8 num);
void YM2612UpdateOne(UINT8 num, INT16 **buffer, UINT16 length);

UINT8 YM2612Write(UINT8 n, UINT8 a, UINT8 v);
UINT8 YM2612Read(UINT8 n, UINT8 a);
UINT8 YM2612TimerOver(UINT8 n, UINT8 c);
#endif /* BUILD_YM2612 */


#if 0 //BUILD_YM2151
/* -------------------- YM2151(OPM) Interface -------------------- */
int OPMInit(int num, int baseclock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler);
void OPMShutdown(void);
void OPMResetChip(int num);

void OPMUpdateOne(UINT8 num, INT16 **buffer, UINT16 length );
/* ---- set callback hander when port CT0/1 write ----- */
/* CT.bit0 = CT0 , CT.bit1 = CT1 */
/*
typedef void (*write8_handler)(int offset, int data);
*/
void OPMSetPortHander(UINT8 n, write8_handler PortWrite);
/* JB 981119  - so it will match MAME's memory write functions scheme*/

int YM2151Write(UINT8 n, UINT8 a, UINT8 v);
UINT8 YM2151Read(UINT8 n, UINT8 a);
int YM2151TimerOver(UINT8 n, UINT8 c);
#endif /* BUILD_YM2151 */

#endif /* _H_FM_FM_ */
