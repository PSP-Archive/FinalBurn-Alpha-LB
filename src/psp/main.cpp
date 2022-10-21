#include "psp.h"

// #define DISPLAY_FPS

PSP_MODULE_INFO(PBPNAME, PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_MAX();

int nGameStage = 1;
int bGameRunning = 1;

int DrvInit(int nDrvNum, bool bRestore);
int DrvExit();
int InpInit();
int InpExit();
int InpMake(UINT32);
void InpDIP();

char currentPath[MAX_PATH];

extern char szAppRomPath[];
char szAppCachePath[256];
char szAppStatePath[256];
char szAppNvramPath[256];

UINT32 devkit_version;

UINT8 sleep_flag = 0;

UINT32 real_frame_count = 0;
UINT32 virtual_frame_count = 0;

UINT8 option_frameskip_type = AUTO_FRAMESKIP;
UINT8 option_frameskip_value = 9;

static UINT8 skip_next_frame;
static UINT8 num_skipped_frames;
static UINT8 frameskip_counter;

#ifdef DISPLAY_FPS
	static char str_fps[32];
	static UINT32 vblank_count;
	static UINT8 frames;
	static UINT8 interval_skipped_frames;
#endif


#define scePowerSetClockFrequency371  scePower_EBD177D6

static int (*__scePowerSetClockFrequency)(int pllfreq, int cpufreq, int busfreq);

int SetClockFrequency(int pllfreq, int cpufreq, int busfreq)
{
	if (__scePowerSetClockFrequency) {
		return (*__scePowerSetClockFrequency)(pllfreq, cpufreq, busfreq);
	}
	return -1;
}


static int power_callback(int unknown, int powerInfo, void *arg)
{
	if (powerInfo & PSP_POWER_CB_SUSPENDING) {
		if (cacheFile) {
			sceIoClose(cacheFile);
		}
		cacheFile = -1;
		sleep_flag = 1;
	} else if (powerInfo & PSP_POWER_CB_RESUME_COMPLETE) {
		sleep_flag = 0;
	}
	return 0;
}

static int CallbackThread(SceSize args, void *argp)
{
	SceUID cbid;
	cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);
	
	sceKernelSleepThreadCB();
	return 0;
}


static void vblank_interrupt_handler(UINT32 sub, UINT32 *parg)
{
	real_frame_count++;
	
#ifdef DISPLAY_FPS
	vblank_count++;
#endif
}


static void *video_frame_addr(void *frame, int x, int y)
{
	return (void *)(((unsigned int)frame | 0x4000000) + ((x + (y << 9)) << 1));
}

static void video_flip_screen(void)
{
	show_frame = draw_frame;
	draw_frame = sceGuSwapBuffers();
}


static UINT32 HighCol16(int r, int g, int b, int  /* i */)
{
	UINT32 t;
	t  = (b << 8) & 0xF800;
	t |= (g << 3) & 0x07E0;
	t |= (r >> 3) & 0x001F;
	
	return t;
}

static void chech_and_mk_dir(const char *dir)
{
	SceUID fd = sceIoDopen(dir);
	if (fd >= 0) sceIoDclose(fd);
	else sceIoMkdir(dir, 0777);
}

static void setup_main(void)
{
#ifdef ENABLE_PRX
	initExceptionHandler();
#endif
	
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	
	sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0, (void *)vblank_interrupt_handler, NULL);
	sceKernelEnableSubIntr(PSP_VBLANK_INT, 0);
	
	devkit_version = sceKernelDevkitVersion();
	
	if (devkit_version < 0x05000010) {
		__scePowerSetClockFrequency = scePowerSetClockFrequency;
	} else {
		__scePowerSetClockFrequency = scePowerSetClockFrequency371;
	}
	
	getcwd(currentPath, MAX_PATH - 1);
	strcat(currentPath, "/");
	
	strcpy(szAppRomPath, currentPath);
	strcat(szAppRomPath, "ROMS");
	chech_and_mk_dir( szAppRomPath );
	strcat(szAppRomPath, "/");
	
	strcpy(szAppCachePath, currentPath);
	strcat(szAppCachePath, "CACHE");
	chech_and_mk_dir( szAppCachePath );
	strcat(szAppCachePath, "/");
	
	strcpy(szAppNvramPath, currentPath);
	strcat(szAppNvramPath, "NVRAM");
	chech_and_mk_dir( szAppNvramPath );
	strcat(szAppNvramPath, "/");
	
	strcpy(szAppStatePath, currentPath);
	strcat(szAppStatePath, "STATE");
	chech_and_mk_dir( szAppStatePath );
	strcat(szAppStatePath, "/");
	
	init_gui();
	
	SceUID thid = sceKernelCreateThread("Update Thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	} else {
		bGameRunning = 0;
	}
	
#ifdef ENABLE_PRX
	char prx_path[MAX_PATH];
	sprintf(prx_path, "%sku_bridge.prx", currentPath);
	
	SceUID modID = pspSdkLoadStartModule(prx_path, PSP_MEMORY_PARTITION_KERNEL);
	if (modID >= 0) {
		init_ku_bridge(devkit_version);
	} else {
		sprintf(prx_path, "Error 0x%08X  start ku_bridge.prx.", modID);
		drawString(prx_path, GU_FRAME_ADDR(draw_frame), 10, 10, R8G8B8_to_B5G6R5(0xffffff));
		video_flip_screen();
		sceKernelDelayThread(5000000);
		bGameRunning = 0;
	}
#endif
	
	BurnLibInit();
	nBurnDrvSelect = ~0U;
	bBurnUseASMCPUEmulation = false;
	
	sound_start();
	
	nBurnBpp = 2;
	nBurnPitch  = 512 * nBurnBpp;
	BurnHighCol = HighCol16;
	
	pBurnDraw = (UINT8 *)video_frame_addr(tex_frame, 0, 0);
}


inline static void synchronize(void)
{
#ifdef DISPLAY_FPS
	
	static UINT8 fps = 60;
	static UINT8 frames_drawn = 60;
	
	frames++;
	
	if (frames == 60) {
		fps = 3600 / vblank_count;
		frames_drawn = 60 - interval_skipped_frames;
		
		vblank_count = 0;
		frames = 0;
		interval_skipped_frames = 0;
	}
	sprintf(str_fps, "%02d(%02d)", (int)fps, (int)frames_drawn);
	
#endif
	
	skip_next_frame = 0;
	virtual_frame_count++;
	
	if (real_frame_count >= virtual_frame_count) {
		if ((real_frame_count > virtual_frame_count) &&
			(option_frameskip_type == AUTO_FRAMESKIP) &&
			(num_skipped_frames < option_frameskip_value)) {
			skip_next_frame = 1;
			num_skipped_frames++;
		} else {
			real_frame_count = 0;
			virtual_frame_count = 0;
			num_skipped_frames = 0;
		}
	} else {
		sceDisplayWaitVblankStart();
		real_frame_count = 0;
		virtual_frame_count = 0;
		num_skipped_frames = 0;
	}
	
	if (option_frameskip_type == MANUAL_FRAMESKIP) {
		frameskip_counter = (frameskip_counter + 1) % (option_frameskip_value + 1);
		if (frameskip_counter) {
			skip_next_frame = 1;
		}
	}
	
#ifdef DISPLAY_FPS
	interval_skipped_frames += skip_next_frame;
#endif

}


int main(int argc, char **argv)
{
	SceCtrlData pad;
	
	bGameRunning = 1;
	nGameStage = 1;
	
	setup_main();
	draw_ui_main();
	
	nCurrentFrame = 0;
	
	while( bGameRunning ) {
		
#ifdef ENABLE_PRX
		kuCtrlPeekBufferPositive(&pad, 1);
#else
		sceCtrlPeekBufferPositive(&pad, 1);
#endif
		pad.Buttons &= 0x0003FFFF;
		
		if ( nGameStage ) {
			
			do_ui_key( pad.Buttons );
			
			update_gui();
			draw_status();
			
#ifdef ENABLE_PRX
			draw_volume_status(1);
#endif
			sceDisplayWaitVblankStart();
			video_flip_screen();
			
		} else {
			
			if (pad.Buttons & PSP_CTRL_HOME) {
				
				nGameStage = 1;
				SetClockFrequency(222, 222, 111);
				
				if (cacheFile) sceIoClose(cacheFile);
				cacheFile = -1;
				
				draw_ui_main();
				sound_clear();
				
				continue;
			}
			
			if (sleep_flag) {
				do {
					sceKernelDelayThread(500000);
				} while (sleep_flag);
			}
			
			InpMake(pad.Buttons);
			
			if (game_reset) {
				InpMake(1U << 28);
				game_reset = 0;
			}
			
			nCurrentFrame++;
			
			UINT8 skip_this_frame = skip_next_frame;
			
			if (!skip_this_frame) {
				pBurnDraw = (UINT8 *)video_frame_addr(tex_frame, 0, 0);
				BurnDrvFrame();
				pBurnDraw = NULL;
				
				update_gui();
				
#ifdef ENABLE_PRX
				draw_volume_status(1);
#endif

#ifdef DISPLAY_FPS
				drawString(str_fps, GU_FRAME_ADDR(draw_frame), 3, 3, R8G8B8_to_B5G6R5(0x404040));
				drawString(str_fps, GU_FRAME_ADDR(draw_frame), 2, 2, R8G8B8_to_B5G6R5(0xdddddd));
#endif

			} else {
				pBurnDraw = NULL;
				BurnDrvFrame();
				
#ifdef ENABLE_PRX
				draw_volume_status(0);
#endif
			}
			
			synchronize();
			sound_next();
			
			if (!skip_this_frame) {
				video_flip_screen();
			}
		}
	}
	
	sound_stop();
	exit_gui();
	
	DrvExit();
	BurnLibExit();
	InpExit();
	
	sceKernelDisableSubIntr(PSP_VBLANK_INT, 0);
	sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT, 0);

	SetClockFrequency(222, 222, 111);
	scePowerUnregisterCallback(0);
	
	sceKernelExitGame();
	
	return 0;
}

