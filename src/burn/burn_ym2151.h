// burn_ym2151.h
#ifndef BURN_YM2152_H
#define BURN_YM2152_H

#include "driver.h"
extern "C" {
 #include "ym2151.h"
}

INT8 BurnYM2151Init(int nClockFrequency, float fVolume);
void BurnYM2151Reset();
void BurnYM2151Exit();
void BurnYM2151Render(INT16 *pSoundBuf, UINT16 nSegmentLength);
void BurnYM2151Scan(int nAction);

inline static void BurnYM2151SelectRegister(const UINT8 nRegister)
{
	extern UINT8 nBurnCurrentYM2151Register;

	nBurnCurrentYM2151Register = nRegister;
}

inline static void BurnYM2151WriteRegister(const UINT8 nValue)
{
	extern UINT8 nBurnCurrentYM2151Register;
	extern UINT8 BurnYM2151Registers[0x0100];

	BurnYM2151Registers[nBurnCurrentYM2151Register] = nValue;
	YM2151WriteReg(0, nBurnCurrentYM2151Register, nValue);
}

inline static UINT8 BurnYM2151ReadStatus(UINT8 a)
{
	if (a == 0) {
		return 0xFF;
	}
	
	return YM2151ReadStatus(0);
}

#define BurnYM2151SetIrqHandler(h) YM2151SetIrqHandler(0, h)
#define BurnYM2151SetPortHandler(h) YM2151SetPortWriteHandler(0, h)

#endif
