#include <math.h>
#include "burnint.h"
#include "msm6295.h"
#include "burn_sound.h"


#define FRAC_BIT   12
#define FRAC_SIZE  (1 << FRAC_BIT)

UINT8 *MSM6295ROM;
UINT8 *MSM6295SampleInfo[MAX_MSM6295][4];
UINT8 *MSM6295SampleData[MAX_MSM6295][4];

UINT8 ALIGN_DATA nMSM6295Status[MAX_MSM6295];

typedef struct {
	UINT16 nVolume;
	int nOutput;
	int nSample;
	UINT32 nPosition;
	UINT32 nSampleCount;
	INT8  nStep;
	UINT8 nDelta;

} MSM6295ChannelInfo;

typedef struct {
	// All current settings for each channel
	MSM6295ChannelInfo ChannelInfo[4];

	// Used for sending commands
	bool bIsCommand;
	UINT16 nSampleInfo;

	UINT16 nVolume;
	int nSampleRate;
	int nSampleSize;

	int nPreviousSample;
	int nCurrentSample;
	int nFractionalPosition;

} OkiM6295;

static OkiM6295 ALIGN_DATA MSM6295[MAX_MSM6295];

static UINT16 ALIGN_DATA VolumeTable[16];
static INT16  ALIGN_DATA DeltaTable[49 * 16];
static const INT8 ALIGN_DATA StepShift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static int *pBuffer = NULL;

static UINT8 nLastChip;
static bool bAdd;

void MSM6295Reset(UINT8 nChip)
{
	nMSM6295Status[nChip] = 0;

	MSM6295[nChip].bIsCommand = false;

	MSM6295[nChip].nPreviousSample = 0;
	MSM6295[nChip].nCurrentSample  = 0;
	MSM6295[nChip].nFractionalPosition = 0;

	for (INT8 channel = 0; channel < 4; channel++) {
		// Set initial bank information
		MSM6295SampleInfo[nChip][channel] = MSM6295ROM + (nChip * 0x0100000) + (channel << 8);
		MSM6295SampleData[nChip][channel] = MSM6295ROM + (nChip * 0x0100000) + (channel << 16);
	}
}

INT8 MSM6295Scan(UINT8 nChip, int /*nAction*/)
{
	int nSampleSize = MSM6295[nChip].nSampleSize;
	SCAN_VAR(MSM6295[nChip]);
	MSM6295[nChip].nSampleSize = nSampleSize;

	SCAN_VAR(nMSM6295Status[nChip]);

	for (int i = 0; i < 4; i++) {
		MSM6295SampleInfo[nChip][i] -= (UINT32)MSM6295ROM;
		SCAN_VAR(MSM6295SampleInfo[nChip][i]);
		MSM6295SampleInfo[nChip][i] += (UINT32)MSM6295ROM;

		MSM6295SampleData[nChip][i] -= (UINT32)MSM6295ROM;
		SCAN_VAR(MSM6295SampleData[nChip][i]);
		MSM6295SampleData[nChip][i] += (UINT32)MSM6295ROM;
	}

	return 0;
}

static void MSM6295Render_Linear(UINT8 nChip, INT32 *pBuf, UINT16 nSegmentLength)
{
	OkiM6295 *pM6295 = &MSM6295[nChip];
	
	int prev_sample = pM6295->nPreviousSample;
	int curr_sample = pM6295->nCurrentSample;
	int stream_pos  = pM6295->nFractionalPosition;
	UINT16 volume   = pM6295->nVolume;
	
	MSM6295ChannelInfo *pChannelInfo;
	
	UINT8 channel, delta;
	int sample;
	
	while (nSegmentLength--) {
		
		if (stream_pos >= FRAC_SIZE) {
			
			prev_sample = curr_sample;
			
			do {
				curr_sample = 0;
				
				for (channel = 0; channel < 4; channel++) {
					
					if (nMSM6295Status[nChip] & (1 << channel)) {
						pChannelInfo = &pM6295->ChannelInfo[channel];
						
						// Check for end of sample
						if (pChannelInfo->nSampleCount-- == 0) {
							nMSM6295Status[nChip] &= ~(1 << channel);
							continue;
						}
						
						// Get new delta from ROM
						if (pChannelInfo->nPosition & 1) {
							delta = pChannelInfo->nDelta & 0x0F;
						} else {
							pChannelInfo->nDelta = MSM6295SampleData[nChip][(pChannelInfo->nPosition >> 17) & 3][(pChannelInfo->nPosition >> 1) & 0xFFFF];
							delta = pChannelInfo->nDelta >> 4;
						}
						
						// Compute new sample
						sample = pChannelInfo->nSample + DeltaTable[(pChannelInfo->nStep << 4) + delta];
#ifndef BUILD_PSP
						if (sample > 2047) {
							sample = 2047;
						} else {
							if (sample < -2048) {
								sample = -2048;
							}
						}
#else
						sample = __builtin_allegrex_min(sample,  2047);
						sample = __builtin_allegrex_max(sample, -2048);
#endif
						pChannelInfo->nSample = sample;
						pChannelInfo->nOutput = sample * pChannelInfo->nVolume;
						
						// Update step value
						pChannelInfo->nStep = pChannelInfo->nStep + StepShift[delta & 0x07];
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
						curr_sample += (pChannelInfo->nOutput >> 4);
						
						// Advance sample position
						pChannelInfo->nPosition++;
					}
				}
				
				stream_pos -= FRAC_SIZE;
				
			} while (stream_pos >= FRAC_SIZE);
		}
		
		// Compute linearly interpolated sample
		sample = prev_sample + (((curr_sample - prev_sample) * stream_pos) >> FRAC_BIT);
		
		// Scale all 4 channels
		sample *= volume;
		
		*pBuf++ += sample;
		
		stream_pos += pM6295->nSampleSize;
	}
	
	pM6295->nPreviousSample = prev_sample;
	pM6295->nCurrentSample  = curr_sample;
	pM6295->nFractionalPosition = stream_pos;
}

INT8 MSM6295Render(UINT8 nChip, INT16 *pSoundBuf, UINT16 nSegmentLength)
{
	if (nChip == 0) {
		memset(pBuffer, 0, nSegmentLength * sizeof(int));
	}

	MSM6295Render_Linear(nChip, pBuffer, nSegmentLength);

	if (nChip == nLastChip)	{
		if (bAdd) {
			BurnSoundCopyClamp_Mono_Add_C(pBuffer, pSoundBuf, nSegmentLength);
		} else {
			BurnSoundCopyClamp_Mono_C(pBuffer, pSoundBuf, nSegmentLength);
		}
	}

	return 0;
}

void MSM6295Command(UINT8 nChip, UINT8 nCommand)
{
	OkiM6295 *M6295 = &MSM6295[nChip];
	
	if (M6295->bIsCommand) {
		// Process second half of command
		INT8 channel;
		UINT32 nSampleStart, nSampleStop;
		UINT8 volume = nCommand & 0x0F;
		
		nCommand >>= 4;
		M6295->bIsCommand = false;
		
		// it's not possible to specify multiple channels at the same time.
		if (nCommand != 1 && nCommand != 2 && nCommand != 4 && nCommand != 8)
			return;
		
		for (channel = 0; channel < 4; channel++) {
			if (nCommand & (0x01 << channel)) break;
		}
		
		UINT8 nBank = (M6295->nSampleInfo >> 8) & 0x03;
		M6295->nSampleInfo &= 0xFF;
		
		UINT8 *base = &MSM6295SampleInfo[nChip][nBank][M6295->nSampleInfo];
		nSampleStart = ((base[0] << 16) + (base[1] << 8) + base[2]) & 0x3FFFF;
		nSampleStop  = ((base[3] << 16) + (base[4] << 8) + base[5]) & 0x3FFFF;
		
		if (nSampleStart < nSampleStop) {
			if (!(nMSM6295Status[nChip] & nCommand)) {
				// Start playing channel
				MSM6295ChannelInfo *pChannelInfo = &M6295->ChannelInfo[channel];
				
				nMSM6295Status[nChip] |= nCommand;
				
				pChannelInfo->nVolume = VolumeTable[volume];
				pChannelInfo->nPosition = nSampleStart << 1;
				pChannelInfo->nSampleCount = (nSampleStop - nSampleStart + 1) << 1;
				
				pChannelInfo->nStep = 0;
				pChannelInfo->nSample = -2;
				pChannelInfo->nOutput = 0;
			}
		} else {
			// invalid sample
			nMSM6295Status[nChip] &= ~nCommand;
		}
	} else {
		// Process command
		if (nCommand & 0x80) {
			M6295->nSampleInfo = (nCommand & 0x7F) << 3;
			M6295->bIsCommand = true;
		} else {
			// Stop playing samples
			nCommand >>= 3;
			nMSM6295Status[nChip] &= ~nCommand;
		}
	}
}

void MSM6295Exit(UINT8 nChip)
{
	free(pBuffer);
	pBuffer = NULL;
}

static void init_tables(void)
{
	// Compute sample deltas
	for (INT16 i = 0; i < 49; i++) {
		INT16 step = (INT16)(powf(1.1, (float)i) * 16.0);
		for (INT16 n = 0; n < 16; n++) {
			INT16 delta = step >> 3;
			if (n & 1) {
				delta += step >> 2;
			}
			if (n & 2) {
				delta += step >> 1;
			}
			if (n & 4) {
				delta += step;
			}
			if (n & 8) {
				delta = -delta;
			}
			DeltaTable[(i << 4) + n] = delta;
		}
	}

	// Compute volume levels
	for (INT16 i = 0; i < 16; i++) {
		float volume = 256.0;
		for (INT16 n = i; n > 0; n--) {
			volume /= 1.412537545;
		}
		VolumeTable[i] = (UINT16)(volume + 0.5);
	}
}

INT8 MSM6295Init(UINT8 nChip, int nSamplerate, float fVolume, bool bAddSignal)
{
	if (nBurnSoundRate > 0) {
		if (pBuffer == NULL) {
			pBuffer = (int *)malloc(nBurnSoundRate * sizeof(int));
			memset(pBuffer, 0, nBurnSoundRate * sizeof(int));
		}
	}
	
	bAdd = bAddSignal;
	
	// Convert volume from percentage
	MSM6295[nChip].nVolume = (UINT16)(fVolume * 256.0 / 100.0 + 0.5);
	
	MSM6295[nChip].nSampleRate = nSamplerate;
	if (nBurnSoundRate > 0) {
		MSM6295[nChip].nSampleSize = (nSamplerate << FRAC_BIT) / nBurnSoundRate;
	} else {
		MSM6295[nChip].nSampleSize = (nSamplerate << FRAC_BIT) / 11025;
	}
	
	if (nChip == 0) {
		nLastChip = 0;
		init_tables();
	} else {
		if (nLastChip < nChip) {
			nLastChip = nChip;
		}
	}
	
	MSM6295Reset(nChip);
	
	return 0;
}

