// FM timers

#ifndef TIMER_H
#define TIMER_H

#define TIMER_TICKS_PER_SECOND (2048000000)
#define MAKE_TIMER_TICKS(n, m) ((long long)(n) * TIMER_TICKS_PER_SECOND / (m))
#define MAKE_CPU_CYLES(n, m)   ((long long)(n) * (m) / TIMER_TICKS_PER_SECOND)

extern "C" float BurnTimerGetTime();

// Callbacks for various sound chips
void BurnOPNTimerCallback(int n, int c, int cnt, float stepTime);	// period = cnt * stepTime in s
void BurnOPLTimerCallback(int c, float period);					// period in  s
void BurnYMFTimerCallback(int n, int c, float period);				// period in us

// Start / stop a timer
void BurnTimerSetRetrig(int c, float period);						// period in  s
void BurnTimerSetOneshot(int c, float period);						// period in  s

extern float dTime;

void BurnTimerExit();
void BurnTimerReset();
int BurnTimerInit(unsigned char (*pOverCallback)(unsigned char, unsigned char), float (*pTimeCallback)());
int BurnTimerAttachSek(int nClockspeed);
int BurnTimerAttachZet(int nClockspeed);
void BurnTimerScan(int nAction, int* pnMin);
unsigned char BurnTimerUpdate(int nCycles);
void BurnTimerUpdateEnd();
void BurnTimerEndFrame(int nCycles);

#endif
