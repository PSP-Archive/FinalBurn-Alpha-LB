// Yamaha YMZ280B module

#ifndef YMZ280B_H
#define YMZ280B_H

INT8 YMZ280BInit(int nClock, void (*IRQCallback)(int), UINT8 nChannels);
void YMZ280BReset();
INT8 YMZ280BScan();
void YMZ280BExit();
INT8 YMZ280BRender(INT16 *pSoundBuf, UINT16 nSegmenLength);
void YMZ280BWriteRegister(UINT8 nValue);
UINT8 YMZ280BReadStatus();

extern UINT8 *YMZ280BROM;

inline static void YMZ280BSelectRegister(UINT8 nRegister)
{
	extern UINT8 nYMZ280BRegister;

	nYMZ280BRegister = nRegister;
}

#endif
