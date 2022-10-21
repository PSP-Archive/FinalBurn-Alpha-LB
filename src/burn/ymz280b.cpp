// Yamaha YMZ280B module

#include "burnint.h"
#include "ymz280b.h"
#include "burn_sound.h"

UINT8 *YMZ280BROM;

UINT8 nYMZ280BStatus;
UINT8 nYMZ280BRegister;

static bool bYMZ280BEnable;

static UINT8 bYMZ280BIRQEnable;
static UINT8 nYMZ280BIRQMask;
static UINT8 nYMZ280BIRQStatus;
void (*YMZ280BIRQCallback)(int nStatus) = NULL;

static int *pBuffer = NULL;

static int nYMZ280BSampleRate;
static int nYMZ280BFrequency;
static UINT8 nOutputChannels;

static INT8 ALIGN_DATA YMZ280BDeltaTable[16];
static const INT16 ALIGN_DATA YMZ280BStepShift[8] = { 0x0E6, 0x0E6, 0x0E6, 0x0E6, 0x133, 0x199, 0x200, 0x266 };

typedef struct {
	bool bEnabled;
	bool bPlaying;
	bool bLoop;
	UINT8 nMode;
	UINT16 nFrequency;
	int nSample;
	int nLoopSample;
	UINT32 nSampleSize;
	UINT32 nPosition;
	int nFractionalPosition;
	int nStep;
	int nLoopStep;
	UINT32 nSampleStart;
	UINT32 nSampleStop;
	UINT32 nLoopStart;
	UINT32 nLoopStop;
	UINT8 nVolume;
	UINT8 nVolumeLeft;
	UINT8 nVolumeRight;
	UINT8 nPan;
	
	int nCurrentOutput;
	int nPreviousOutput;

} sYMZ280BChannelInfo;

static UINT8 nActiveChannel;
static int nSample, nCount;
static int *buf;
static int nRamp;

sYMZ280BChannelInfo ALIGN_DATA YMZ280BChannelInfo[8];
static sYMZ280BChannelInfo *channelInfo;

void YMZ280BReset()
{
	memset(&YMZ280BChannelInfo[0], 0, sizeof(YMZ280BChannelInfo));
	
	nYMZ280BIRQMask = 0;
	nYMZ280BIRQStatus = 0;
	nYMZ280BStatus = 0;
	bYMZ280BEnable = false;
}

// nMode
// 1:  4-bit ADPCM - 0.172 to 44.1kHz in 256 steps
// 2:  8-bit PCM   - 0.172 to 88.2kHz in 512 steps
// 3: 16-bit PCM   - 0.172 to 88.2kHz in 512 steps

inline void YMZ280BSetSampleSize(const int nChannel)
{
	float rate = (float)(YMZ280BChannelInfo[nChannel].nFrequency + 1) * nYMZ280BFrequency * 512;
	rate /= nYMZ280BSampleRate * 3;
	
	YMZ280BChannelInfo[nChannel].nSampleSize = (UINT32)rate;
}

INT8 YMZ280BScan()
{
	SCAN_VAR(nYMZ280BStatus);
	SCAN_VAR(nYMZ280BRegister);
	
	SCAN_VAR(bYMZ280BEnable);
	
	SCAN_VAR(bYMZ280BIRQEnable);
	SCAN_VAR(nYMZ280BIRQMask);
	SCAN_VAR(nYMZ280BIRQStatus);
	
	for (int j = 0; j < 8; j++) {
		SCAN_VAR(YMZ280BChannelInfo[j]);
		YMZ280BSetSampleSize(j);
	}
	
	return 0;
}

INT8 YMZ280BInit(int nClock, void (*IRQCallback)(int), UINT8 nChannels)
{
	nYMZ280BFrequency = nClock;
	
	if (nBurnSoundRate > 0) {
		nYMZ280BSampleRate = nBurnSoundRate;
	} else {
		nYMZ280BSampleRate = 11025;
	}
	
	// Compute sample deltas
	for (int n = 0; n < 16; n++) {
		INT8 delta = (n & 7) * 2 + 1;
		if (n & 8) {
			delta = -delta;
		}
		YMZ280BDeltaTable[n] = delta;
	}
	
	YMZ280BIRQCallback = IRQCallback;
	
	free(pBuffer);
	pBuffer = (int *)malloc(nYMZ280BSampleRate * 2 * sizeof(int));
	memset(pBuffer, 0, nYMZ280BSampleRate * 2 * sizeof(int));
	
	nOutputChannels = nChannels;
	
	nRamp = 64 * 32678 / nYMZ280BSampleRate;
	
	YMZ280BReset();
	
	return 0;
}

void YMZ280BExit()
{
	free(pBuffer);
	pBuffer = NULL;
	
	YMZ280BIRQCallback = NULL;
}

inline static void UpdateIRQStatus()
{
	nYMZ280BIRQStatus = 0;
	if (bYMZ280BIRQEnable && (nYMZ280BStatus & nYMZ280BIRQMask)) {
		nYMZ280BIRQStatus = 1;
	}
	
	if ((YMZ280BIRQCallback != NULL)) {
		YMZ280BIRQCallback(nYMZ280BIRQStatus);
	}
}

inline static void ComputeVolume(sYMZ280BChannelInfo *channel)
{
	if (channel->nPan == 8) {
		channel->nVolumeLeft  = channel->nVolume;
		channel->nVolumeRight = channel->nVolume;
	} else {
		if (channel->nPan < 8) {
			channel->nVolumeLeft  = channel->nVolume;
			channel->nVolumeRight =((UINT16)channel->nVolume * channel->nPan) >> 3;
		} else {
			channel->nVolumeLeft  = ((UINT16)channel->nVolume * (15 - channel->nPan)) >> 3;
			channel->nVolumeRight = channel->nVolume;
		}
	}
}

inline static void RampChannel()
{
#if 1
	if (channelInfo->nSample != 0) {
		if (channelInfo->nSample > 0) {
			while (nCount-- && channelInfo->nSample > nRamp) {
				channelInfo->nSample -= nRamp;
				*buf++ += channelInfo->nSample * channelInfo->nVolumeLeft;
				*buf++ += channelInfo->nSample * channelInfo->nVolumeRight;
			}
			if (channelInfo->nSample < nRamp) {
				channelInfo->nSample = 0;
			}
		} else {
			while (nCount-- && channelInfo->nSample < -nRamp) {
				channelInfo->nSample += nRamp;
				*buf++ += channelInfo->nSample * channelInfo->nVolumeLeft;
				*buf++ += channelInfo->nSample * channelInfo->nVolumeRight;
			}
			if (channelInfo->nSample > -nRamp) {
				channelInfo->nSample = 0;
			}
		}
	}
#endif
}

inline static void DecodeOneSample()
{
	// Get next value & compute delta
	UINT8 nDelta = YMZ280BROM[channelInfo->nPosition >> 1];
	if (channelInfo->nPosition & 1) {
		nDelta &= 0x0F;
	} else {
		nDelta >>= 4;
	}
	
	nSample = channelInfo->nSample + channelInfo->nStep * YMZ280BDeltaTable[nDelta] / 8;
	
#ifndef BUILD_PSP
	if (nSample < -32768) {
		nSample = -32768;
	} else {
		if (nSample > 32767) {
			nSample = 32767;
		}
	}
#else
	nSample = __builtin_allegrex_min(nSample,  32767);
	nSample = __builtin_allegrex_max(nSample, -32768);
#endif
	
	channelInfo->nSample = nSample;
	
	channelInfo->nStep = (channelInfo->nStep * YMZ280BStepShift[nDelta & 7]) >> 8;
#ifndef BUILD_PSP
	if (channelInfo->nStep > 0x6000) {
		channelInfo->nStep = 0x6000;
	} else {
		if (channelInfo->nStep < 127) {
			channelInfo->nStep = 127;
		}
	}
#else
	channelInfo->nStep = __builtin_allegrex_min(channelInfo->nStep, 0x6000);
	channelInfo->nStep = __builtin_allegrex_max(channelInfo->nStep, 127);
#endif
}

inline static void ComputeOutput_Linear()
{
	nSample = channelInfo->nPreviousOutput + (channelInfo->nCurrentOutput - channelInfo->nPreviousOutput) * (channelInfo->nFractionalPosition >> 12) / (0x01000000 >> 12);
	
	*buf++ += (nSample * channelInfo->nVolumeLeft);
	*buf++ += (nSample * channelInfo->nVolumeRight);
}

inline static void RenderADPCM_Linear()
{
	while (nCount--) {
		
		if (channelInfo->nFractionalPosition >= 0x01000000) {
			
			channelInfo->nPreviousOutput = channelInfo->nCurrentOutput;
			
			do {
				// Check for end of sample
				if (channelInfo->nPosition >= channelInfo->nSampleStop) {
					channelInfo->bPlaying = false;
					
					RampChannel();
					
					nYMZ280BStatus |= 1 << nActiveChannel;
					UpdateIRQStatus();
					
					return;
				} else {
					
					DecodeOneSample();
					
					// Advance sample position
					channelInfo->nPosition++;
					channelInfo->nFractionalPosition -= 0x01000000;
				}
				
			} while (channelInfo->nFractionalPosition >= 0x01000000);
			
			channelInfo->nCurrentOutput = channelInfo->nSample;
		}
		
		ComputeOutput_Linear();
		
		channelInfo->nFractionalPosition += channelInfo->nSampleSize;
	}
}

inline static void RenderADPCMLoop_Linear()
{
	while (nCount--) {
		
		if (channelInfo->nFractionalPosition >= 0x01000000) {
			
			channelInfo->nPreviousOutput = channelInfo->nCurrentOutput;
			
			do {
				// Check for end of sample
				if (channelInfo->nPosition == channelInfo->nLoopStop) {
					channelInfo->nStep = channelInfo->nLoopStep;
					channelInfo->nSample = channelInfo->nLoopSample;
					channelInfo->nPosition = channelInfo->nLoopStart;
				} else {
					// Store the state of the channel at the point where the loop starts
					if (channelInfo->nPosition == channelInfo->nLoopStart) {
						channelInfo->nLoopStep = channelInfo->nStep;
						channelInfo->nLoopSample = channelInfo->nSample;
					}
				}
				
				DecodeOneSample();
				
				// Advance sample position
				channelInfo->nPosition++;
				channelInfo->nFractionalPosition -= 0x01000000;
				
			} while (channelInfo->nFractionalPosition >= 0x01000000);
			
			channelInfo->nCurrentOutput = channelInfo->nSample;
		}
		
		ComputeOutput_Linear();
		
		channelInfo->nFractionalPosition += channelInfo->nSampleSize;
	}
}

INT8 YMZ280BRender(INT16 *pSoundBuf, UINT16 nSegmentLength)
{
	memset(pBuffer, 0, nSegmentLength * 2 * sizeof(int));
	
	for (nActiveChannel = 0; nActiveChannel < 8; nActiveChannel++) {
		nCount = nSegmentLength;
		buf = pBuffer;
		channelInfo = &YMZ280BChannelInfo[nActiveChannel];
		
		if (channelInfo->bPlaying) {
			if (channelInfo->bEnabled && channelInfo->bLoop) {
				RenderADPCMLoop_Linear();
			} else {
				RenderADPCM_Linear();
			}
		} else {
			RampChannel();
		}
	}
	
	switch (nOutputChannels) {
		case 1:										// Use only the left output channel
		case 2: {									// Use only the right output channel
			nCount = nSegmentLength;
			buf = pBuffer + (nOutputChannels >> 1);
			while (nCount--) {
				nSample = *buf >> 8;
				buf += 2;
				
#ifndef BUILD_PSP
				if (nSample > 32767) {
					nSample = 32767;
				} else {
					if (nSample < -32768) {
						nSample = -32768;
					}
				}
#else
				nSample = __builtin_allegrex_min(nSample,  32767);
				nSample = __builtin_allegrex_max(nSample, -32768);
#endif
				*pSoundBuf++ = (INT16)nSample;
				*pSoundBuf++ = (INT16)nSample;
			}
			break;
		}
		case 3: {									// Use both output channels
			BurnSoundCopyClamp_C(pBuffer, pSoundBuf, nSegmentLength);
			break;
		}
	}
	
	return 0;
}

void YMZ280BWriteRegister(UINT8 nValue)
{
	if (nYMZ280BRegister < 0x80) {
		int nWriteChannel = (nYMZ280BRegister >> 2) & 0x07;
		
		switch (nYMZ280BRegister & 0x63) {
			
			// Miscellaneous
			case 0:															// Frequency
				YMZ280BChannelInfo[nWriteChannel].nFrequency &= 0x0100;
				YMZ280BChannelInfo[nWriteChannel].nFrequency |= nValue;
				YMZ280BSetSampleSize(nWriteChannel);
				break;
			case 1:	{														// Start/mode/freq
				YMZ280BChannelInfo[nWriteChannel].nMode = ((nValue >> 5) & 0x03);
				YMZ280BChannelInfo[nWriteChannel].bLoop = (nValue & 0x10);
				
				YMZ280BChannelInfo[nWriteChannel].nFrequency &= 0x00FF;
				YMZ280BChannelInfo[nWriteChannel].nFrequency |= ((nValue & 1) << 8);
				YMZ280BSetSampleSize(nWriteChannel);
				
				if ((nValue & 0x80) == 0) {
					YMZ280BChannelInfo[nWriteChannel].bEnabled = false;
					if (!YMZ280BChannelInfo[nWriteChannel].bLoop) {
						YMZ280BChannelInfo[nWriteChannel].bPlaying = false;
					}
				} else {
					if (!YMZ280BChannelInfo[nWriteChannel].bEnabled) {
						YMZ280BChannelInfo[nWriteChannel].bEnabled = true;
						YMZ280BChannelInfo[nWriteChannel].bPlaying = true;
						YMZ280BChannelInfo[nWriteChannel].nPosition = YMZ280BChannelInfo[nWriteChannel].nSampleStart;
						YMZ280BChannelInfo[nWriteChannel].nStep = 127;
#ifdef DEBUG
						if (YMZ280BChannelInfo[nWriteChannel].nMode > 1) {
							printf("Yikes!!! -- PCM ");
							printf("Sample Start: %08X - Stop: %08X.\n",YMZ280BChannelInfo[nWriteChannel].nSampleStart, YMZ280BChannelInfo[nWriteChannel].nSampleStop);
						}
#endif
						YMZ280BChannelInfo[nWriteChannel].nSample = 0;
						YMZ280BChannelInfo[nWriteChannel].nFractionalPosition = 0;
						YMZ280BChannelInfo[nWriteChannel].nPreviousOutput = 0;
						YMZ280BChannelInfo[nWriteChannel].nCurrentOutput = 0;
					}
				}
				
				break;
			}
			case 2:																	// Volume
				YMZ280BChannelInfo[nWriteChannel].nVolume = nValue;
				ComputeVolume(&YMZ280BChannelInfo[nWriteChannel]);
				break;
			case 3:																	// Pan
				YMZ280BChannelInfo[nWriteChannel].nPan = nValue & 0x0F;
				ComputeVolume(&YMZ280BChannelInfo[nWriteChannel]);
				break;
				
			// Hi bits
			case 0x20:
				YMZ280BChannelInfo[nWriteChannel].nSampleStart &= 0x0001FFFE;
				YMZ280BChannelInfo[nWriteChannel].nSampleStart |= (nValue << 17);
				break;
			case 0x21:
				YMZ280BChannelInfo[nWriteChannel].nLoopStart &= 0x0001FFFE;
				YMZ280BChannelInfo[nWriteChannel].nLoopStart |= (nValue << 17);
				break;
			case 0x22:
				YMZ280BChannelInfo[nWriteChannel].nLoopStop &= 0x0001FFFE;
				YMZ280BChannelInfo[nWriteChannel].nLoopStop |= (nValue << 17);
				break;
			case 0x23:
				YMZ280BChannelInfo[nWriteChannel].nSampleStop &= 0x0001FFFE;
				YMZ280BChannelInfo[nWriteChannel].nSampleStop |= (nValue << 17);
				break;
				
			// Mid bits
			case 0x40:
				YMZ280BChannelInfo[nWriteChannel].nSampleStart &= 0x01FE01FE;
				YMZ280BChannelInfo[nWriteChannel].nSampleStart |= (nValue << 9);
				break;
			case 0x41:
				YMZ280BChannelInfo[nWriteChannel].nLoopStart &= 0x01FE01FE;
				YMZ280BChannelInfo[nWriteChannel].nLoopStart |= (nValue << 9);
				break;
			case 0x42:
				YMZ280BChannelInfo[nWriteChannel].nLoopStop &= 0x01FE01FE;
				YMZ280BChannelInfo[nWriteChannel].nLoopStop |= (nValue << 9);
				break;
			case 0x43:
				YMZ280BChannelInfo[nWriteChannel].nSampleStop &= 0x01FE01FE;
				YMZ280BChannelInfo[nWriteChannel].nSampleStop |= (nValue << 9);
				break;
				
			// Lo bits
			case 0x60:
				YMZ280BChannelInfo[nWriteChannel].nSampleStart &= 0x01FFFE00;
				YMZ280BChannelInfo[nWriteChannel].nSampleStart |= (nValue << 1);
				break;
			case 0x61:
				YMZ280BChannelInfo[nWriteChannel].nLoopStart &= 0x01FFFE00;
				YMZ280BChannelInfo[nWriteChannel].nLoopStart |= (nValue << 1);
				break;
			case 0x62:
				YMZ280BChannelInfo[nWriteChannel].nLoopStop &= 0x01FFFE00;
				YMZ280BChannelInfo[nWriteChannel].nLoopStop |= (nValue << 1);
				break;
			case 0x63:
				YMZ280BChannelInfo[nWriteChannel].nSampleStop &= 0x01FFFE00;
				YMZ280BChannelInfo[nWriteChannel].nSampleStop |= (nValue << 1);
				break;
		}
	} else {
		if (nYMZ280BRegister == 0xFE) {
			// Set IRQ mask
			nYMZ280BIRQMask = nValue;
			UpdateIRQStatus();
		} else {
			if (nYMZ280BRegister == 0xFF) {
				// Start/stop playing, enable/disable IRQ
				bYMZ280BIRQEnable = (nValue >> 4) & 0x01;
				UpdateIRQStatus();
				
				if (bYMZ280BEnable && !(nValue & 0x80)) {
					bYMZ280BEnable = false;
					for (int n = 0; n < 8; n++) {
						YMZ280BChannelInfo[n].bPlaying = false;
					}
				} else {
					if (!bYMZ280BEnable && (nValue & 0x80)) {
						bYMZ280BEnable = true;
						for (int n = 0; n < 8; n++) {
							if (YMZ280BChannelInfo[n].bEnabled && YMZ280BChannelInfo[n].bLoop) {
								YMZ280BChannelInfo[n].bPlaying = true;
							}
						}
					}
				}
			}
		}
	}
}

UINT8 YMZ280BReadStatus()
{
	UINT8 nStatus = nYMZ280BStatus;
	nYMZ280BStatus = 0;
	
	UpdateIRQStatus();
	
	return nStatus;
}

