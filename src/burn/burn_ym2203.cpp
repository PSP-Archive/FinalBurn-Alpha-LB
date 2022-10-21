#include "burnint.h"
#include "burn_ym2203.h"

#define MAX_YM2203 (2)

void (*BurnYM2203Update)(INT16 *pSoundBuf, UINT16 nSegmentEnd);

static int (*BurnYM2203StreamCallback)(int nSoundRate);

static int nBurnYM2203SoundRate;

#define SAFETY (32)
#define YM2203_BUFFER_SIZE ((4096 + SAFETY) * 4 * sizeof(INT16))
static INT16 *pBuffer = NULL;
static INT16 *pYM2203Buffer[4 * MAX_YM2203];

static UINT16 nYM2203Position;
static UINT16 nAY8910Position;

static UINT16 nFractionalPosition;

static UINT16 nYM2203Volume;
static UINT16 nAY8910Volume;

static UINT8 nNumChips = 0;

static UINT8 bYM2203AddSignal;

// ----------------------------------------------------------------------------
// Dummy functions

static void YM2203UpdateDummy(INT16*, UINT16 /* nSegmentEnd */)
{
	return;
}

static int YM2203StreamCallbackDummy(int /* nSoundRate */)
{
	return 0;
}

// ----------------------------------------------------------------------------
// Execute YM2203 for part of a frame

static void AY8910Render(UINT16 nSegmentLength)
{
	if (nAY8910Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    AY8910 render %6i -> %6i\n"), nAY8910Position, nSegmentLength);

	nSegmentLength -= nAY8910Position;

	pYM2203Buffer[1] = pBuffer + 1 * 4096 + 4 + nAY8910Position;
	pYM2203Buffer[2] = pBuffer + 2 * 4096 + 4 + nAY8910Position;
	pYM2203Buffer[3] = pBuffer + 3 * 4096 + 4 + nAY8910Position;

	AY8910Update(0, &pYM2203Buffer[1], nSegmentLength);
	
	if (nNumChips > 1) {
		pYM2203Buffer[5] = pBuffer + 5 * 4096 + 4 + nAY8910Position;
		pYM2203Buffer[6] = pBuffer + 6 * 4096 + 4 + nAY8910Position;
		pYM2203Buffer[7] = pBuffer + 7 * 4096 + 4 + nAY8910Position;

		AY8910Update(1, &pYM2203Buffer[5], nSegmentLength);
	}

	nAY8910Position += nSegmentLength;
}

static void YM2203Render(UINT16 nSegmentLength)
{
	if (nYM2203Position >= nSegmentLength) {
		return;
	}

//	bprintf(PRINT_NORMAL, _T("    YM2203 render %6i -> %6i\n", nYM2203Position, nSegmentLength));

	nSegmentLength -= nYM2203Position;

	pYM2203Buffer[0] = pBuffer + 0 * 4096 + 4 + nYM2203Position;

	YM2203UpdateOne(0, pYM2203Buffer[0], nSegmentLength);
	
	if (nNumChips > 1) {
		pYM2203Buffer[4] = pBuffer + 4 * 4096 + 4 + nYM2203Position;

		YM2203UpdateOne(1, pYM2203Buffer[4], nSegmentLength);
	}

	nYM2203Position += nSegmentLength;
}

// ----------------------------------------------------------------------------
// Update the sound buffer

static void YM2203UpdateNormal(INT16 *pSoundBuf, UINT16 nSegmentEnd)
{
	UINT16 nSegmentLength = nSegmentEnd;
	
//	bprintf(PRINT_NORMAL, _T("    YM2203 update        -> %6i\n", nSegmentLength));
	
	if (nSegmentEnd < nAY8910Position) {
		nSegmentEnd = nAY8910Position;
	}
	if (nSegmentEnd < nYM2203Position) {
		nSegmentEnd = nYM2203Position;
	}
	
	if (nSegmentLength > nBurnSoundLen) {
		nSegmentLength = nBurnSoundLen;
	}
	
	YM2203Render(nSegmentEnd);
	AY8910Render(nSegmentEnd);
	
	pYM2203Buffer[0] = pBuffer + 4 + 0 * 4096;
	pYM2203Buffer[1] = pBuffer + 4 + 1 * 4096;
	pYM2203Buffer[2] = pBuffer + 4 + 2 * 4096;
	pYM2203Buffer[3] = pBuffer + 4 + 3 * 4096;
	if (nNumChips > 1) {
		pYM2203Buffer[4] = pBuffer + 4 + 4 * 4096;
		pYM2203Buffer[5] = pBuffer + 4 + 5 * 4096;
		pYM2203Buffer[6] = pBuffer + 4 + 6 * 4096;
		pYM2203Buffer[7] = pBuffer + 4 + 7 * 4096;
	}
	
	for (int n = nFractionalPosition; n < nSegmentLength; n++) {
		int nAYSample, nTotalSample;
		
		nAYSample  = pYM2203Buffer[1][n];
		nAYSample += pYM2203Buffer[2][n];
		nAYSample += pYM2203Buffer[3][n];
		if (nNumChips > 1) {
			nAYSample += pYM2203Buffer[5][n];
			nAYSample += pYM2203Buffer[6][n];
			nAYSample += pYM2203Buffer[7][n];
		}
		
		nAYSample  *= nAY8910Volume;
		nAYSample >>= 12;
		
		nTotalSample = pYM2203Buffer[0][n];
		if (nNumChips > 1) {
			nTotalSample += pYM2203Buffer[4][n];
		}
		
		nTotalSample  *= nYM2203Volume;
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
		
		if (bYM2203AddSignal) {
			pSoundBuf[(n << 1) + 0] += nTotalSample;
			pSoundBuf[(n << 1) + 1] += nTotalSample;
		} else {
			pSoundBuf[(n << 1) + 0] = nTotalSample;
			pSoundBuf[(n << 1) + 1] = nTotalSample;
		}
	}
	
	nFractionalPosition = nSegmentLength;
	
	if (nSegmentEnd >= nBurnSoundLen) {
		UINT16 nExtraSamples = nSegmentEnd - nBurnSoundLen;
		
		for (int i = 0; i < nExtraSamples; i++) {
			pYM2203Buffer[0][i] = pYM2203Buffer[0][nBurnSoundLen + i];
			pYM2203Buffer[1][i] = pYM2203Buffer[1][nBurnSoundLen + i];
			pYM2203Buffer[2][i] = pYM2203Buffer[2][nBurnSoundLen + i];
			pYM2203Buffer[3][i] = pYM2203Buffer[3][nBurnSoundLen + i];
			if (nNumChips > 1) {
				pYM2203Buffer[4][i] = pYM2203Buffer[4][nBurnSoundLen + i];
				pYM2203Buffer[5][i] = pYM2203Buffer[5][nBurnSoundLen + i];
				pYM2203Buffer[6][i] = pYM2203Buffer[6][nBurnSoundLen + i];
				pYM2203Buffer[7][i] = pYM2203Buffer[7][nBurnSoundLen + i];
			}
		}
		
		nFractionalPosition = 0;
		
		nYM2203Position = nExtraSamples;
		nAY8910Position = nExtraSamples;
		
		dTime += 100.0 / nBurnFPS;
	}
}

// ----------------------------------------------------------------------------
// Callbacks for YM2203 core

void BurnYM2203UpdateRequest()
{
	YM2203Render(BurnYM2203StreamCallback(nBurnYM2203SoundRate));
}

static void BurnAY8910UpdateRequest()
{
	AY8910Render(BurnYM2203StreamCallback(nBurnYM2203SoundRate));
}

// ----------------------------------------------------------------------------
// Initialisation, etc.

void BurnYM2203Reset()
{
	BurnTimerReset();
	
	for (INT8 i = 0; i < nNumChips; i++) {
		YM2203ResetChip(i);
		AY8910Reset(i);
	}
}

void BurnYM2203Exit()
{
	YM2203Shutdown();
	
	for (INT8 i = 0; i < nNumChips; i++) {
		AY8910Exit(i);
	}

	BurnTimerExit();

	free(pBuffer);
	pBuffer = NULL;
	
	nNumChips = 0;
	bYM2203AddSignal = 0;
}

INT8 BurnYM2203Init(UINT8 num, int nClockFrequency, float volume_psg, float volume_fm,
                    FM_IRQHANDLER IRQCallback, int (*StreamCallback)(int), float (*GetTimeCallback)(), UINT8 bAddSignal)
{
	if (num > MAX_YM2203) num = MAX_YM2203;
	
	BurnTimerInit(&YM2203TimerOver, GetTimeCallback);

	if (nBurnSoundRate <= 0) {
		BurnYM2203StreamCallback = YM2203StreamCallbackDummy;

		BurnYM2203Update = YM2203UpdateDummy;

		for (INT8 i = 0; i < num; i++) {
			AY8910InitYM(i, nClockFrequency, 11025, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
		}

		YM2203Init(num, nClockFrequency, 11025, &BurnOPNTimerCallback, IRQCallback);

		return 0;
	}

	BurnYM2203StreamCallback = StreamCallback;
	
	nBurnYM2203SoundRate = nBurnSoundRate;
	BurnYM2203Update = YM2203UpdateNormal;

	for (INT8 i = 0; i < num; i++) {
		AY8910InitYM(i, nClockFrequency, nBurnYM2203SoundRate, NULL, NULL, NULL, NULL, BurnAY8910UpdateRequest);
	}
	
	YM2203Init(num, nClockFrequency, nBurnYM2203SoundRate, &BurnOPNTimerCallback, IRQCallback);

	pBuffer = (INT16 *)malloc(YM2203_BUFFER_SIZE * num);
	memset(pBuffer, 0, YM2203_BUFFER_SIZE * num);

	nYM2203Volume = (UINT16)(4096.0 * volume_fm  / 100.0 + 0.5);
	nAY8910Volume = (UINT16)(4096.0 * volume_psg / 100.0 + 0.5);
	
	nYM2203Position = 0;
	nAY8910Position = 0;
	nFractionalPosition = 0;
	
	nNumChips = num;
	bYM2203AddSignal = bAddSignal;

	return 0;
}

void BurnYM2203Scan(int nAction, int *pnMin)
{
	BurnTimerScan(nAction, pnMin);
	AY8910Scan(nAction, pnMin);

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(nYM2203Position);
		SCAN_VAR(nAY8910Position);
	}
}

#undef MAX_YM2203
