// burn_ym2608.h
#ifndef BURN_YM2608_H
#define BURN_YM2608_H

#include "driver.h"
extern "C" {
 #include "ay8910.h"
 #include "fm.h"
}
#include "timer.h"

extern "C" void BurnYM2608UpdateRequest();

int BurnYM2608Init(int nClockFrequency, float volume_psg, float volume_fm, unsigned char *YM2608ADPCMROM, int *nYM2608ADPCMSize,
                   FM_IRQHANDLER IRQCallback, int (*StreamCallback)(int), float (*GetTimeCallback)(), UINT8 bAddSignal);
void BurnYM2608Reset();
void BurnYM2608Exit();
extern void (*BurnYM2608Update)(INT16 *pSoundBuf, int nSegmentEnd);
void BurnYM2608Scan(int nAction, int *pnMin);

#define BurnYM2608Write(a, n) YM2608Write(0, a, n)
#define BurnYM2608Read(a) YM2608Read(0, a)

#endif
