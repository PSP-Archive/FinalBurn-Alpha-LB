#include "psp.h"

static INT16 ALIGN_DATA mixbuf[SND_FRAME_SIZE * 2 * 8];

static INT16 *pMixBuf[] = {
	&mixbuf[SND_FRAME_SIZE * 2 * 0],
	&mixbuf[SND_FRAME_SIZE * 2 * 1],
	&mixbuf[SND_FRAME_SIZE * 2 * 2],
	&mixbuf[SND_FRAME_SIZE * 2 * 3],
	&mixbuf[SND_FRAME_SIZE * 2 * 4],
	&mixbuf[SND_FRAME_SIZE * 2 * 5],
	&mixbuf[SND_FRAME_SIZE * 2 * 6],
	&mixbuf[SND_FRAME_SIZE * 2 * 7]
};

static UINT8 mixbufid  = 0;
static UINT8 playbufid = 3;

static SceUID sound_thread;
static UINT8 sound_active;
static UINT8 sound_sleep;


static void sound_thread_wakeup()
{
	if (sound_sleep) {
		sceKernelWakeupThread(sound_thread);
	}
}

static int sound_release()
{
	while(sceAudioOutput2GetRestSample() > 0);
	
	return sceAudioSRCChRelease();
}

static int sound_frequency(UINT16 freq)
{
	switch (freq) {
		case  8000: case 11025: case 12000:
		case 16000: case 22050: case 24000:
		case 32000: case 44100: case 48000:
			break;
		
		default:
			return -1;
	}
	
	sound_release();
	
	return sceAudioSRCChReserve(SND_FRAME_SIZE, freq, 2);
}

static int sound_update_thread(SceSize args, void *argp)
{
	while (sound_active) {
		
		if (sleep_flag) {
			do {
				sceKernelDelayThread(500000);
			} while (sleep_flag);
			
			sound_frequency(SND_RATE);
		}
		
		if (playbufid == mixbufid) {
			sound_sleep = 1;
			sceKernelSleepThread();
			
			sound_sleep = 0;
			sceKernelDelayThread(1);
			continue;
		}
		
		sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX, pMixBuf[playbufid]);
		
		playbufid = (playbufid + 1) & 0x07;
	}
	
	sceKernelExitThread(0);
	
	return 0;
}

int sound_start()
{
	nInterpolation = 0;
	pBurnSoundOut = NULL;
	nBurnSoundRate = SND_RATE;
	nBurnSoundLen = SND_FRAME_SIZE;
	
	memset(mixbuf, 0, sizeof(mixbuf));
	
	sound_release();
	
	int aures = sound_frequency(SND_RATE);
	if (aures) return aures;
	
	sound_thread = sceKernelCreateThread("sound_thread", sound_update_thread, 0x08, 0x10000, 0, NULL);
	if (sound_thread < 0) {
		sound_release();
		return -1;
	}
	
	mixbufid = 0;
	pBurnSoundOut = &mixbuf[mixbufid];
	
	sound_active = 1;
	sceKernelStartThread(sound_thread, 0, 0);
	
	return 0;
}

int sound_stop()
{
	sound_active = 0;
	
	if (sound_thread >= 0) {
		sound_thread_wakeup();
		sceKernelWaitThreadEnd(sound_thread, NULL);
		sceKernelDeleteThread(sound_thread);
		sound_thread = -1;
		sound_release();
	}
	
	return 0;
}

void sound_next()
{
	mixbufid = (mixbufid + 1) & 0x07;
	pBurnSoundOut = pMixBuf[mixbufid];
	sound_thread_wakeup();
	
	UINT8 diff;
	if (mixbufid > playbufid) {
		diff = mixbufid - playbufid;
	} else if (mixbufid < playbufid) {
		diff = mixbufid + (8 - playbufid);
	} else {
		diff = 0;
	}
	if (diff > 4) {
		sceDisplayWaitVblankStart();
		real_frame_count = 0;
		virtual_frame_count = 0;
	} else if (diff < 2) {
		// frameskip force.
		real_frame_count++;
	}
}

void sound_clear()
{
	memset(mixbuf, 0, sizeof(mixbuf));
}

