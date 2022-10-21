// burn_ym2203.h
#ifndef BURN_YM2203_H
#define BURN_YM2203_H

#include "driver.h"
extern "C" {
 #include "ay8910.h"
 #include "fm.h"
}
#include "timer.h"

extern "C" void BurnYM2203UpdateRequest();

INT8 BurnYM2203Init(UINT8 num, int nClockFrequency, float volume_psg, float volume_fm,
                    FM_IRQHANDLER IRQCallback, int (*StreamCallback)(int), float (*GetTimeCallback)(), UINT8 bAddSignal);
void BurnYM2203Reset();
void BurnYM2203Exit();
extern void (*BurnYM2203Update)(INT16 *pSoundBuf, UINT16 nSegmentEnd);
void BurnYM2203Scan(int nAction, int *pnMin);

#define BurnYM2203Write(i, a, n) YM2203Write(i, a, n)
#define BurnYM2203Read(i, a) YM2203Read(i, a)

#define BurnYM2203SetPorts(c, read0, read1, write0, write1)	\
	AY8910SetPorts(c, read0, read1, write0, write1)

#endif
