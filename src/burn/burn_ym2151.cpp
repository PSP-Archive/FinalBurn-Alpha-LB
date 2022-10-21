#include "burnint.h"
#include "burn_ym2151.h"

UINT8 ALIGN_DATA BurnYM2151Registers[0x0100];
UINT8 nBurnCurrentYM2151Register;

static int nBurnYM2151SoundRate;

static INT16 *pBuffer = NULL;
static INT16 *pYM2151Buffer[2];

static UINT32 nYM2151Volume;

void BurnYM2151Render(INT16 *pSoundBuf, UINT16 nSegmentLength)
{
	pYM2151Buffer[0] = pBuffer;
	pYM2151Buffer[1] = pBuffer + nSegmentLength;
	
	YM2151UpdateOne(0, pYM2151Buffer, nSegmentLength);
	
	for (int n = 0; n < nSegmentLength; n++) {
		pSoundBuf[(n << 1) + 0] = ((INT64)pYM2151Buffer[0][n] * nYM2151Volume) >> 16;
		pSoundBuf[(n << 1) + 1] = ((INT64)pYM2151Buffer[1][n] * nYM2151Volume) >> 16;
	}
}

void BurnYM2151Reset()
{
	YM2151ResetChip(0);
}

void BurnYM2151Exit()
{
	YM2151Shutdown();

	free(pBuffer);
	pBuffer = NULL;
}

INT8 BurnYM2151Init(int nClockFrequency, float fVolume)
{
	if (nBurnSoundRate <= 0) {
		YM2151Init(1, nClockFrequency, 11025);
		return 0;
	}

	nBurnYM2151SoundRate = nBurnSoundRate;

	nYM2151Volume = (UINT32)(65536.0 * fVolume / 100.0 + 0.5);

	YM2151Init(1, nClockFrequency, nBurnYM2151SoundRate);

	pBuffer = (INT16 *)malloc(65536 * 2 * sizeof(INT16));
	memset(pBuffer, 0, 65536 * 2 * sizeof(INT16));

	return 0;
}

void BurnYM2151Scan(int nAction)
{
	if ((nAction & ACB_DRIVER_DATA) == 0) {
		return;
	}
	SCAN_VAR(nBurnCurrentYM2151Register);
	SCAN_VAR(BurnYM2151Registers);

	if (nAction & ACB_WRITE) {
		for (int i = 0; i < 0x0100; i++) {
			YM2151WriteReg(0, i, BurnYM2151Registers[i]);
		}
	}
}

