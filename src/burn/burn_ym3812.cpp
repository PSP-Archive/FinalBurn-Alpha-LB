#include "burnint.h"
#include "burn_ym3812.h"

void (*BurnYM3812Update)(short* pSoundBuf, int nSegmentEnd);

static int (*BurnYM3812StreamCallback)(int nSoundRate);

static int nBurnYM3812SoundRate;

#define SAFETY (32)
static short* pBuffer = NULL;
static short* pYM3812Buffer;

static int nYM3812Position;

static unsigned int nSampleSize;
static unsigned int nFractionalPosition;

static int bYM3812AddSignal;

// ----------------------------------------------------------------------------
// Dummy functions

static void YM3812UpdateDummy(short* , int /* nSegmentEnd */)
{
	return;
}

static int YM3812StreamCallbackDummy(int /* nSoundRate */)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Execute YM3812 for part of a frame

static void YM3812Render(int nSegmentLength)
{
	if (nYM3812Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    YM3812 render %6i -> %6i\n", nYM3812Position, nSegmentLength));

	nSegmentLength -= nYM3812Position;

	YM3812UpdateOne(0, pBuffer + 0 * 4096 + 4 + nYM3812Position, nSegmentLength);

	nYM3812Position += nSegmentLength;
}

// ----------------------------------------------------------------------------
// Update the sound buffer

static void YM3812UpdateNormal(short* pSoundBuf, int nSegmentEnd)
{
	int nSegmentLength = nSegmentEnd;

//	bprintf(PRINT_NORMAL, _T("    YM3812 render %6i -> %6i\n"), nYM3812Position, nSegmentEnd);

	if (nSegmentEnd < nYM3812Position) {
		nSegmentEnd = nYM3812Position;
	}

	if (nSegmentLength > nBurnSoundLen) {
		nSegmentLength = nBurnSoundLen;
	}

	YM3812Render(nSegmentEnd);

	pYM3812Buffer = pBuffer + 4 + 0 * 4096;

	for (int i = nFractionalPosition; i < nSegmentLength; i++) {
		if (bYM3812AddSignal) {
			pSoundBuf[(i << 1) + 0] += pYM3812Buffer[i];
			pSoundBuf[(i << 1) + 1] += pYM3812Buffer[i];
		} else {
			pSoundBuf[(i << 1) + 0] = pYM3812Buffer[i];
			pSoundBuf[(i << 1) + 1] = pYM3812Buffer[i];
		}
	}

	nFractionalPosition = nSegmentLength;

	if (nSegmentEnd >= nBurnSoundLen) {
		int nExtraSamples = nSegmentEnd - nBurnSoundLen;

		for (int i = 0; i < nExtraSamples; i++) {
			pYM3812Buffer[i] = pYM3812Buffer[nBurnSoundLen + i];
		}

		nFractionalPosition = 0;

		nYM3812Position = nExtraSamples;

	}
}

// ----------------------------------------------------------------------------
// Callbacks for YM3812 core

void BurnYM3812UpdateRequest(int, int)
{
	YM3812Render(BurnYM3812StreamCallback(nBurnYM3812SoundRate));
}

// ----------------------------------------------------------------------------
// Initialisation, etc.

void BurnYM3812Reset()
{
	BurnTimerReset();

	YM3812ResetChip(0);
}

void BurnYM3812Exit()
{
	YM3812Shutdown();

	BurnTimerExit();

	free(pBuffer);
	pBuffer = NULL;
	
	bYM3812AddSignal = 0;
}

int BurnYM3812Init(int nClockFrequency, OPL_IRQHANDLER IRQCallback, int (*StreamCallback)(int), int bAddSignal)
{
	BurnTimerInit(&YM3812TimerOver, NULL);

	if (nBurnSoundRate <= 0) {
		BurnYM3812StreamCallback = YM3812StreamCallbackDummy;

		BurnYM3812Update = YM3812UpdateDummy;

		YM3812Init(1, nClockFrequency, 11025);
		return 0;
	}

	BurnYM3812StreamCallback = StreamCallback;

	nBurnYM3812SoundRate = nBurnSoundRate;
	BurnYM3812Update = YM3812UpdateNormal;

	YM3812Init(1, nClockFrequency, nBurnYM3812SoundRate);
	YM3812SetIRQHandler(0, IRQCallback, 0);
	YM3812SetTimerHandler(0, &BurnOPLTimerCallback, 0);
	YM3812SetUpdateHandler(0, &BurnYM3812UpdateRequest, 0);

	pBuffer = (short*)malloc((4096 + SAFETY) * sizeof(short));
	memset(pBuffer, 0, (4096 + SAFETY) * sizeof(short));

	nYM3812Position = 0;

	nFractionalPosition = 0;
	
	bYM3812AddSignal = bAddSignal;

	return 0;
}

void BurnYM3812Scan(int nAction, int* pnMin)
{
	BurnTimerScan(nAction, pnMin);
}

