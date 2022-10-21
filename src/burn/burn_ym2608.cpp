#include "burnint.h"
#include "burn_ym2608.h"

void (*BurnYM2608Update)(short* pSoundBuf, int nSegmentEnd);

static int (*BurnYM2608StreamCallback)(int nSoundRate);

static int nBurnYM2608SoundRate;

#define SAFETY (32)
static short *pBuffer = NULL;
static short *pYM2608Buffer[5];

static int nYM2608Position;
static int nAY8910Position;

static int nYM2608Volume;
static int nAY8910Volume;

static UINT32 nSampleSize;
static UINT32 nFractionalPosition;

static UINT8 bYM2608AddSignal;

// ----------------------------------------------------------------------------
// Dummy functions

static void YM2608UpdateDummy(short*, int /* nSegmentEnd */)
{
	return;
}

static int YM2608StreamCallbackDummy(int /* nSoundRate */)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Execute YM2608 for part of a frame

static void AY8910Render(int nSegmentLength)
{
	if (nAY8910Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    AY8910 render %6i -> %6i\n"), nAY8910Position, nSegmentLength);

	nSegmentLength -= nAY8910Position;

	pYM2608Buffer[2] = pBuffer + 2 * 4096 + 4 + nAY8910Position;
	pYM2608Buffer[3] = pBuffer + 3 * 4096 + 4 + nAY8910Position;
	pYM2608Buffer[4] = pBuffer + 4 * 4096 + 4 + nAY8910Position;

	AY8910Update(0, &pYM2608Buffer[2], nSegmentLength);

	nAY8910Position += nSegmentLength;
}

static void YM2608Render(int nSegmentLength)
{
	if (nYM2608Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    YM2608 render %6i -> %6i\n", nYM2608Position, nSegmentLength));

	nSegmentLength -= nYM2608Position;

	pYM2608Buffer[0] = pBuffer + 0 * 4096 + 4 + nYM2608Position;
	pYM2608Buffer[1] = pBuffer + 1 * 4096 + 4 + nYM2608Position;

	YM2608UpdateOne(0, &pYM2608Buffer[0], nSegmentLength);

	nYM2608Position += nSegmentLength;
}

// ----------------------------------------------------------------------------
// Update the sound buffer

static void YM2608UpdateNormal(short *pSoundBuf, int nSegmentEnd)
{
	int nSegmentLength = nSegmentEnd;

//	bprintf(PRINT_NORMAL, _T("    YM2608 update        -> %6i\n", nSegmentLength));

	if (nSegmentEnd < nAY8910Position) {
		nSegmentEnd = nAY8910Position;
	}
	if (nSegmentEnd < nYM2608Position) {
		nSegmentEnd = nYM2608Position;
	}

	if (nSegmentLength > nBurnSoundLen) {
		nSegmentLength = nBurnSoundLen;
	}

	YM2608Render(nSegmentEnd);
	AY8910Render(nSegmentEnd);

	pYM2608Buffer[0] = pBuffer + 4 + 0 * 4096;
	pYM2608Buffer[1] = pBuffer + 4 + 1 * 4096;
	pYM2608Buffer[2] = pBuffer + 4 + 2 * 4096;
	pYM2608Buffer[3] = pBuffer + 4 + 3 * 4096;
	pYM2608Buffer[4] = pBuffer + 4 + 4 * 4096;

	for (int n = nFractionalPosition; n < nSegmentLength; n++) {
		int nAYSample, nTotalSample;

		nAYSample  = pYM2608Buffer[2][n];
		nAYSample += pYM2608Buffer[3][n];
		nAYSample += pYM2608Buffer[4][n];

		nAYSample  *= nAY8910Volume;
		nAYSample >>= 12;

		nTotalSample = pYM2608Buffer[0][n];

		nTotalSample  *= nYM2608Volume;
		nTotalSample >>= 12;

		nTotalSample += nAYSample;

#ifndef BUILD_PSP
		if (nTotalSample < -32768) {
			nTotalSample = -32768;
		} else {
			if (nTotalSample > 32767) {
				nTotalSample = 32767;
			}
		}
#else
		nTotalSample = __builtin_allegrex_min(nTotalSample,  32767);
		nTotalSample = __builtin_allegrex_max(nTotalSample, -32768);
#endif

		if (bYM2608AddSignal) {
			pSoundBuf[(n << 1) + 0] += nTotalSample;
		} else {
			pSoundBuf[(n << 1) + 0] = nTotalSample;
		}

		nTotalSample = pYM2608Buffer[1][n];

		nTotalSample  *= nYM2608Volume;
		nTotalSample >>= 12;

		nTotalSample += nAYSample;

		if (nTotalSample < -32768) {
			nTotalSample = -32768;
		} else {
			if (nTotalSample > 32767) {
				nTotalSample = 32767;
			}
		}
		
		if (bYM2608AddSignal) {
			pSoundBuf[(n << 1) + 1] += nTotalSample;
		} else {
			pSoundBuf[(n << 1) + 1] = nTotalSample;
		}
	}

	nFractionalPosition = nSegmentLength;

	if (nSegmentEnd >= nBurnSoundLen) {
		int nExtraSamples = nSegmentEnd - nBurnSoundLen;

		for (int i = 0; i < nExtraSamples; i++) {
			pYM2608Buffer[0][i] = pYM2608Buffer[0][nBurnSoundLen + i];
			pYM2608Buffer[1][i] = pYM2608Buffer[1][nBurnSoundLen + i];
			pYM2608Buffer[2][i] = pYM2608Buffer[2][nBurnSoundLen + i];
			pYM2608Buffer[3][i] = pYM2608Buffer[3][nBurnSoundLen + i];
			pYM2608Buffer[4][i] = pYM2608Buffer[4][nBurnSoundLen + i];
		}

		nFractionalPosition = 0;

		nYM2608Position = nExtraSamples;
		nAY8910Position = nExtraSamples;

		dTime += 100.0 / nBurnFPS;
	}
}

// ----------------------------------------------------------------------------
// Callbacks for YM2608 core

void BurnYM2608UpdateRequest()
{
	YM2608Render(BurnYM2608StreamCallback(nBurnYM2608SoundRate));
}

static void BurnAY8910UpdateRequest()
{
	AY8910Render(BurnYM2608StreamCallback(nBurnYM2608SoundRate));
}

// ----------------------------------------------------------------------------
// Initialisation, etc.

void BurnYM2608Reset()
{
	BurnTimerReset();

	YM2608ResetChip(0);
}

void BurnYM2608Exit()
{
	YM2608Shutdown();
	AY8910Exit(0);

	BurnTimerExit();

	free(pBuffer);
	pBuffer = NULL;
	
	bYM2608AddSignal = 0;
}

int BurnYM2608Init(int nClockFrequency, float volume_psg, float volume_fm, unsigned char *YM2608ADPCMROM, int *nYM2608ADPCMSize,
                   FM_IRQHANDLER IRQCallback, int (*StreamCallback)(int), float (*GetTimeCallback)(), UINT8 bAddSignal)
{
	BurnTimerInit(&YM2608TimerOver, GetTimeCallback);

	if (nBurnSoundRate <= 0) {
		BurnYM2608StreamCallback = YM2608StreamCallbackDummy;

		BurnYM2608Update = YM2608UpdateDummy;

		AY8910InitYM(0, nClockFrequency, 11025, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
		YM2608Init(1, nClockFrequency, 11025, (void **)(&YM2608ADPCMROM), nYM2608ADPCMSize, &BurnOPNTimerCallback, IRQCallback);
		return 0;
	}

	BurnYM2608StreamCallback = StreamCallback;

	nBurnYM2608SoundRate = nBurnSoundRate;
	BurnYM2608Update = YM2608UpdateNormal;

	AY8910InitYM(0, nClockFrequency, nBurnYM2608SoundRate, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
	YM2608Init(1, nClockFrequency, nBurnYM2608SoundRate, (void **)(&YM2608ADPCMROM), nYM2608ADPCMSize, &BurnOPNTimerCallback, IRQCallback);

	pBuffer = (short *)malloc((4096 + SAFETY) * 5 * sizeof(short));
	memset(pBuffer, 0, (4096 + SAFETY) * 5 * sizeof(short));

	nYM2608Volume = (int)(4096.0 * volume_fm  / 100.0 + 0.5);
	nAY8910Volume = (int)(4096.0 * volume_psg / 100.0 + 0.5);
	
	nYM2608Position = 0;
	nAY8910Position = 0;
	
	bYM2608AddSignal = bAddSignal;

	return 0;
}

void BurnYM2608Scan(int nAction, int* pnMin)
{
	BurnTimerScan(nAction, pnMin);
	AY8910Scan(nAction, pnMin);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(nYM2608Position);
		SCAN_VAR(nAY8910Position);
	}
}

