/*
adpcm decoding - a simplifed adpcm.cpp.
This has one channel per 'chip' with a current maximum of 4 chips.

Needs redsigning as some of the things we are doing here are not needed.
*/

#include <math.h>
#include "burnint.h"
#include "msm5205.h"

unsigned char* MSM5205ROM;
unsigned char* MSM5205SampleInfo[MAX_MSM5205];
unsigned char* MSM5205SampleData[MAX_MSM5205];

unsigned int ALIGN_DATA nMSM5205Status[MAX_MSM5205];

struct MSM5205ChannelInfo {
	int nOutput;
	int nVolume;
	unsigned int nPosition;
	unsigned int nSampleCount;
	int nSample;
	int nStep;
	int nDelta;
	int nBufPos;
};

static struct {
	int nVolume;
	int nSampleRate;
	int nSampleSize;
	int nFractionalPosition;
	MSM5205ChannelInfo ChannelInfo;
} MSM5205[MAX_MSM5205] ALIGN_DATA;

static unsigned int ALIGN_DATA MSM5205VolumeTable[16];
static int ALIGN_DATA MSM5205DeltaTable[49 * 16];
static int ALIGN_DATA MSM5205StepShift[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

static int* MSM5205ChannelData[MAX_MSM5205];

static int* pBuffer = NULL;
static int nLastChip;

static bool bAdd;

void MSM5205Reset(int nChip)
{
	memset(MSM5205ChannelData[nChip], 0, 0x0100 * sizeof(int));

	// Set initial bank information
	MSM5205SampleInfo[nChip]= MSM5205ROM;
	MSM5205SampleData[nChip]= MSM5205ROM;

	MSM5205[nChip].ChannelInfo.nBufPos = 4;
	MSM5205[nChip].ChannelInfo.nVolume = MSM5205VolumeTable[0];
	MSM5205[nChip].ChannelInfo.nPosition = 0;
	MSM5205[nChip].ChannelInfo.nSampleCount = 0;
	MSM5205[nChip].ChannelInfo.nSample = 0;
	MSM5205[nChip].ChannelInfo.nStep = 0;
	nMSM5205Status[nChip] = 0;

	//MSM5205Reset(nChip);
}

int MSM5205Scan(int nChip, int /*nAction*/)
{
	SCAN_VAR(MSM5205[nChip]);
	SCAN_VAR(nMSM5205Status[nChip]);
	for (int i = 0; i < 4; i++) {
		MSM5205SampleInfo[nChip][i] -= (unsigned int)MSM5205ROM;
		SCAN_VAR(MSM5205SampleInfo[nChip][i]);
		MSM5205SampleInfo[nChip][i] += (unsigned int)MSM5205ROM;

		MSM5205SampleData[nChip][i] -= (unsigned int)MSM5205ROM;
		SCAN_VAR(MSM5205SampleData[nChip][i]);
		MSM5205SampleData[nChip][i] += (unsigned int)MSM5205ROM;
	}

	return 0;
}

static void MSM5205Render_Linear(int nChip, int* pBuf, int nSegmentLength)
{
	static int nPreviousSample[MAX_MSM5205], nCurrentSample[MAX_MSM5205];
	int nVolume = MSM5205[nChip].nVolume;
	int nFractionalPosition = MSM5205[nChip].nFractionalPosition;

	int nDelta, nSample;
	MSM5205ChannelInfo* pChannelInfo;

	while (nSegmentLength--) {
		if (nFractionalPosition >= 0x1000) {

			nPreviousSample[nChip] = nCurrentSample[nChip];

			do {
				nCurrentSample[nChip] = 0;

					if (nMSM5205Status[nChip]) {
						pChannelInfo = &MSM5205[nChip].ChannelInfo;

						// Check for end of sample
						if (pChannelInfo->nSampleCount-- == 0) {
							nMSM5205Status[nChip] = 0;
							continue;
						}

						// Get new delta from ROM
						if (pChannelInfo->nPosition & 1) {
							nDelta = pChannelInfo->nDelta & 0x0F;
						} else {
							pChannelInfo->nDelta = MSM5205SampleData[nChip][(pChannelInfo->nPosition >> 1) & 0xFFFF];
							nDelta = pChannelInfo->nDelta >> 4;
						}

						// Compute new sample
						nSample = pChannelInfo->nSample + MSM5205DeltaTable[(pChannelInfo->nStep << 4) + nDelta];
#ifndef BUILD_PSP
						if (nSample > 2047) {
							nSample = 2047;
						} else {
							if (nSample < -2048) {
								nSample = -2048;
							}
						}
#else
						nSample = __builtin_allegrex_min(nSample,  2047);
						nSample = __builtin_allegrex_max(nSample, -2048);
#endif
						pChannelInfo->nSample = nSample;
						pChannelInfo->nOutput = (nSample * pChannelInfo->nVolume);

						// Update step value
						pChannelInfo->nStep = pChannelInfo->nStep + MSM5205StepShift[nDelta & 7];
#ifndef BUILD_PSP
						if (pChannelInfo->nStep > 48) {
							pChannelInfo->nStep = 48;
						} else {
							if (pChannelInfo->nStep < 0) {
								pChannelInfo->nStep = 0;
							}
						}
#else
						pChannelInfo->nStep = __builtin_allegrex_min(pChannelInfo->nStep, 48);
						pChannelInfo->nStep = __builtin_allegrex_max(pChannelInfo->nStep,  0);
#endif

						nCurrentSample[nChip] += pChannelInfo->nOutput;

						// Advance sample position
						pChannelInfo->nPosition++;
					}
				nFractionalPosition -= 0x1000;

			} while (nFractionalPosition >= 0x1000);

			// Scale all 4 channels
			nCurrentSample[nChip] *= nVolume;
			nCurrentSample[nChip] /= 0x4000;

		}

		// Compute linearly interpolated sample
		nSample = nPreviousSample[nChip] + (((nCurrentSample[nChip] - nPreviousSample[nChip]) * nFractionalPosition) >> 12);

		*pBuf++ += nSample;

		nFractionalPosition += MSM5205[nChip].nSampleSize;
	}

	MSM5205[nChip].nFractionalPosition = nFractionalPosition;
}


int MSM5205Render(int nChip, short* pSoundBuf, int nSegmentLength)
{
	if (nChip == 0) {
		memset(pBuffer, 0, nSegmentLength * sizeof(int));
	}

	MSM5205Render_Linear(nChip, pBuffer, nSegmentLength);

	if (nChip == nLastChip)	{
		int nOutput;

		int* pBuf = pBuffer;

		if (bAdd) {
			while (nSegmentLength--) {

				// Left side
				nOutput = *pBuf + *pSoundBuf;

				// Clamp summed channels
#ifndef BUILD_PSP
				if (nOutput > 32767) {
					nOutput = 32767;
				} else {
					if (nOutput < -32768) {
						nOutput = -32768;
					}
				}
#else
				nOutput = __builtin_allegrex_min(nOutput,  32767);
				nOutput = __builtin_allegrex_max(nOutput, -32768);
#endif

				*pSoundBuf++ = (short)nOutput;

				// Right side
				nOutput = *pBuf++ + *pSoundBuf;

				// Clamp summed channels
#ifndef BUILD_PSP
				if (nOutput > 32767) {
					nOutput = 32767;
				} else {
					if (nOutput < -32768) {
						nOutput = -32768;
					}
				}
#else
				nOutput = __builtin_allegrex_min(nOutput,  32767);
				nOutput = __builtin_allegrex_max(nOutput, -32768);
#endif

				*pSoundBuf++ = (short)nOutput;
			}
		} else {
			while (nSegmentLength--) {

				nOutput = *pBuf++;

				// Clamp summed channels
#ifndef BUILD_PSP
				if (nOutput > 32767) {
					nOutput = 32767;
				} else {
					if (nOutput < -32768) {
						nOutput = -32768;
					}
				}
#else
				nOutput = __builtin_allegrex_min(nOutput,  32767);
				nOutput = __builtin_allegrex_max(nOutput, -32768);
#endif

				*pSoundBuf++ = (short)nOutput;
				*pSoundBuf++ = (short)nOutput;
			}
		}
	}

	return 0;
}


void MSM5205Exit(int nChip)
{
	free(pBuffer);
	pBuffer = NULL;

	for (int nChannel = 0; nChannel < 4; nChannel++) {
		free(MSM5205ChannelData[nChip]);
		MSM5205ChannelData[nChip]= NULL;
	}
}

int MSM5205Init(int nChip, int nSamplerate, float fMaxVolume, bool bAddSignal)
{
	if (nBurnSoundRate <= 0) {
		return 1;
	}

	bAdd = bAddSignal;

	// Convert volume from percentage
	MSM5205[nChip].nVolume = int(fMaxVolume * 1024.0 / 100.0 + 0.5);

	MSM5205[nChip].nSampleRate = nSamplerate;
	MSM5205[nChip].nSampleSize = (nSamplerate << 12) / nBurnSoundRate;

	MSM5205[nChip].nFractionalPosition = 0;

	nMSM5205Status[nChip] = 0;
	if (nChip == 0) {
		nLastChip = 0;
	} else {
		if (nLastChip < nChip) {
			nLastChip = nChip;
		}
	}

	// Compute sample deltas
	for (int i = 0; i < 49; i++) {
		int nStep = (int)(powf(1.1, (float)i) * 16.0);
		for (int n = 0; n < 16; n++) {
			int nDelta = nStep >> 3;
			if (n & 1) {
				nDelta += nStep >> 2;
			}
			if (n & 2) {
				nDelta += nStep >> 1;
			}
			if (n & 4) {
				nDelta += nStep;
			}
			if (n & 8) {
				nDelta = -nDelta;
			}
			MSM5205DeltaTable[(i << 4) + n] = nDelta;
		}
	}

	// Compute volume levels
	for (int i = 0; i < 16; i++) {
		float nVolume = 256.0;
		for (int n = i; n > 0; n--) {
			nVolume /= 1.412537545;
		}
		MSM5205VolumeTable[i] = (unsigned int)(nVolume + 0.5);
	}

	if (pBuffer == NULL) {
		pBuffer = (int*)malloc(nBurnSoundRate * sizeof(int));
	}
	memset(pBuffer, 0, nBurnSoundRate * sizeof(int));

	MSM5205ChannelData[nChip] = (int*)malloc(0x0100 * sizeof(int));
	memset(MSM5205ChannelData[nChip], 0, 0x0100 * sizeof(int));

	return 0;
}

void MSM5205Play(unsigned int nStart, unsigned int nEnd, int nChip)
{

	if (nEnd < 0x80000) {
		// Start playing channel
		MSM5205SampleInfo[nChip] = MSM5205ROM+nStart;
		MSM5205SampleData[nChip] = MSM5205ROM+nStart;
		MSM5205[nChip].ChannelInfo.nVolume = MSM5205VolumeTable[4];
		MSM5205[nChip].ChannelInfo.nPosition = 0;
		MSM5205[nChip].ChannelInfo.nSampleCount = nEnd;
		MSM5205[nChip].ChannelInfo.nSample = 0;
		MSM5205[nChip].ChannelInfo.nStep = 0;
		nMSM5205Status[nChip] = 1;
	}
}

