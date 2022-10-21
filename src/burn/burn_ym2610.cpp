#include "burnint.h"
#include "burn_ym2610.h"

void (*BurnYM2610Update)(short* pSoundBuf, int nSegmentEnd);

static int (*BurnYM2610StreamCallback)(int nSoundRate);

static int nBurnYM2610SoundRate;

#define SAFETY (32)
static short* pBuffer = NULL;
static short* pYM2610Buffer[5];

static int nYM2610Position;
static int nAY8910Position;

static int nYM2610Volume;
static int nAY8910Volume;

static unsigned int nSampleSize;
static unsigned int nFractionalPosition;

static int bYM2610AddSignal;

// ----------------------------------------------------------------------------
// Dummy functions

static void YM2610UpdateDummy(short*, int /* nSegmentEnd */)
{
	return;
}

static int YM2610StreamCallbackDummy(int /* nSoundRate */)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Execute YM2610 for part of a frame

static void AY8910Render(int nSegmentLength)
{
	if (nAY8910Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    AY8910 render %6i -> %6i\n"), nAY8910Position, nSegmentLength);

	nSegmentLength -= nAY8910Position;

	pYM2610Buffer[2] = pBuffer + 2 * 4096 + 4 + nAY8910Position;
	pYM2610Buffer[3] = pBuffer + 3 * 4096 + 4 + nAY8910Position;
	pYM2610Buffer[4] = pBuffer + 4 * 4096 + 4 + nAY8910Position;

	AY8910Update(0, &pYM2610Buffer[2], nSegmentLength);

	nAY8910Position += nSegmentLength;
}

static void YM2610Render(int nSegmentLength)
{
	if (nYM2610Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    YM2610 render %6i -> %6i\n", nYM2610Position, nSegmentLength));

	nSegmentLength -= nYM2610Position;

	pYM2610Buffer[0] = pBuffer + 0 * 4096 + 4 + nYM2610Position;
	pYM2610Buffer[1] = pBuffer + 1 * 4096 + 4 + nYM2610Position;

	YM2610UpdateOne(0, &pYM2610Buffer[0], nSegmentLength);

	nYM2610Position += nSegmentLength;
}

// ----------------------------------------------------------------------------
// Update the sound buffer

static void YM2610UpdateNormal(short* pSoundBuf, int nSegmentEnd)
{
	int nSegmentLength = nSegmentEnd;

//	bprintf(PRINT_NORMAL, _T("    YM2610 update        -> %6i\n", nSegmentLength));

	if (nSegmentEnd < nAY8910Position) {
		nSegmentEnd = nAY8910Position;
	}
	if (nSegmentEnd < nYM2610Position) {
		nSegmentEnd = nYM2610Position;
	}

	if (nSegmentLength > nBurnSoundLen) {
		nSegmentLength = nBurnSoundLen;
	}

	YM2610Render(nSegmentEnd);
	AY8910Render(nSegmentEnd);

	pYM2610Buffer[0] = pBuffer + 4 + 0 * 4096;
	pYM2610Buffer[1] = pBuffer + 4 + 1 * 4096;
	pYM2610Buffer[2] = pBuffer + 4 + 2 * 4096;
	pYM2610Buffer[3] = pBuffer + 4 + 3 * 4096;
	pYM2610Buffer[4] = pBuffer + 4 + 4 * 4096;


	for (int n = nFractionalPosition; n < nSegmentLength; n++) {
		int nAYSample, nTotalSample;

		nAYSample  = pYM2610Buffer[2][n];
		nAYSample += pYM2610Buffer[3][n];
		nAYSample += pYM2610Buffer[4][n];

		nAYSample  *= nAY8910Volume;
		nAYSample >>= 12;

		nTotalSample = pYM2610Buffer[0][n];

		nTotalSample  *= nYM2610Volume;
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

		if (bYM2610AddSignal) {
			pSoundBuf[(n << 1) + 0] += nTotalSample;
		} else {
			pSoundBuf[(n << 1) + 0] = nTotalSample;
		}

		nTotalSample = pYM2610Buffer[1][n];

		nTotalSample  *= nYM2610Volume;
		nTotalSample >>= 12;

		nTotalSample += nAYSample;

		if (nTotalSample < -32768) {
			nTotalSample = -32768;
		} else {
			if (nTotalSample > 32767) {
				nTotalSample = 32767;
			}
		}
			
		if (bYM2610AddSignal) {
			pSoundBuf[(n << 1) + 1] += nTotalSample;
		} else {
			pSoundBuf[(n << 1) + 1] = nTotalSample;
		}
	}

	nFractionalPosition = nSegmentLength;

	if (nSegmentEnd >= nBurnSoundLen) {
		int nExtraSamples = nSegmentEnd - nBurnSoundLen;

		for (int i = 0; i < nExtraSamples; i++) {
			pYM2610Buffer[0][i] = pYM2610Buffer[0][nBurnSoundLen + i];
			pYM2610Buffer[1][i] = pYM2610Buffer[1][nBurnSoundLen + i];
			pYM2610Buffer[2][i] = pYM2610Buffer[2][nBurnSoundLen + i];
			pYM2610Buffer[3][i] = pYM2610Buffer[3][nBurnSoundLen + i];
			pYM2610Buffer[4][i] = pYM2610Buffer[4][nBurnSoundLen + i];
		}

		nFractionalPosition = 0;

		nYM2610Position = nExtraSamples;
		nAY8910Position = nExtraSamples;

		dTime += 100.0 / nBurnFPS;
	}
}

// ----------------------------------------------------------------------------
// Callbacks for YM2610 core

void BurnYM2610UpdateRequest()
{
	YM2610Render(BurnYM2610StreamCallback(nBurnYM2610SoundRate));
}

static void BurnAY8910UpdateRequest()
{
	AY8910Render(BurnYM2610StreamCallback(nBurnYM2610SoundRate));
}

// ----------------------------------------------------------------------------
// Initialisation, etc.

void BurnYM2610Reset()
{
	BurnTimerReset();

	YM2610ResetChip(0);
}

void BurnYM2610Exit()
{
	YM2610Shutdown();
	AY8910Exit(0);

	BurnTimerExit();

	free(pBuffer);
	pBuffer = NULL;
	
	bYM2610AddSignal = 0;
}

int BurnYM2610Init(int nClockFrequency, float volume_psg, float volume_fm,
                   unsigned char* YM2610ADPCMAROM, int* nYM2610ADPCMASize, unsigned char* YM2610ADPCMBROM, int* nYM2610ADPCMBSize,
                   FM_IRQHANDLER IRQCallback, int (*StreamCallback)(int), float (*GetTimeCallback)(), int bAddSignal)
{
	BurnTimerInit(&YM2610TimerOver, GetTimeCallback);

	if (nBurnSoundRate <= 0) {
		BurnYM2610StreamCallback = YM2610StreamCallbackDummy;

		BurnYM2610Update = YM2610UpdateDummy;

		AY8910InitYM(0, nClockFrequency, 11025, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
		YM2610Init(1, nClockFrequency, 11025, (void**)(&YM2610ADPCMAROM), nYM2610ADPCMASize, (void**)(&YM2610ADPCMBROM), nYM2610ADPCMBSize, &BurnOPNTimerCallback, IRQCallback);
		return 0;
	}

	BurnYM2610StreamCallback = StreamCallback;

	nBurnYM2610SoundRate = nBurnSoundRate;
	BurnYM2610Update = YM2610UpdateNormal;

	AY8910InitYM(0, nClockFrequency, nBurnYM2610SoundRate, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
	YM2610Init(1, nClockFrequency, nBurnYM2610SoundRate, (void**)(&YM2610ADPCMAROM), nYM2610ADPCMASize, (void**)(&YM2610ADPCMBROM), nYM2610ADPCMBSize, &BurnOPNTimerCallback, IRQCallback);

	pBuffer = (short*)malloc((4096 + SAFETY) * 5 * sizeof(short));
	memset(pBuffer, 0, (4096 + SAFETY) * 5 * sizeof(short));

	nYM2610Volume = (int)(4096.0 * volume_fm  / 100.0 + 0.5);
	nAY8910Volume = (int)(4096.0 * volume_psg / 100.0 + 0.5);

	nYM2610Position = 0;
	nAY8910Position = 0;

	nFractionalPosition = 0;
	bYM2610AddSignal = bAddSignal;

	return 0;
}

void BurnYM2610Scan(int nAction, int* pnMin)
{
	BurnTimerScan(nAction, pnMin);
	AY8910Scan(nAction, pnMin);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(nYM2610Position);
		SCAN_VAR(nAY8910Position);
	}
}

