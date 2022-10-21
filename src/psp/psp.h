#ifndef _H_PSP_
#define _H_PSP_

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <psprtc.h>
#include <pspiofilemgr.h>
#include <pspsdk.h>
#include <pspiofilemgr.h>
#include <psprtc.h>
#include <pspimpose_driver.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include "libpower/psppower.h"

#include "burnint.h"

#include "font.h"
#include "UniCache.h"

#ifdef ENABLE_PRX
	#include "ku_bridge.h"
	#include "exception.h"
#endif

#define PSP_LINE_SIZE (512)
#define SCREEN_WIDTH  (480)
#define SCREEN_HEIGHT (272)

#define ALIGN_PSPDATA __attribute__((aligned(16)))
#define ALIGN_DATA    __attribute__((aligned(4)))

// no use cache
#define GU_FRAME_ADDR(frame)   (UINT16 *)((UINT32)frame | 0x44000000)
// use cache
#define GUC_FRAME_ADDR(frame)  (UINT16 *)((UINT32)frame | 0x04000000)


/* main.cpp */

extern int nGameStage;
extern int bGameRunning;

extern char currentPath[];
extern char szAppCachePath[];
extern char szAppStatePath[];
extern char szAppNvramPath[];

extern UINT32 devkit_version;

extern UINT8 sleep_flag;

extern UINT32 real_frame_count;
extern UINT32 virtual_frame_count;

// frameskip type
#define AUTO_FRAMESKIP   0
#define MANUAL_FRAMESKIP 1
#define NO_FRAMESKIP     2

extern UINT8 option_frameskip_type;
extern UINT8 option_frameskip_value;


int SetClockFrequency(int pllfreq, int cpufreq, int busfreq);


/* ui.cpp */
#define UI_COLOR	R8G8B8_to_B5G6R5(0xffc090)
#define UI_BGCOLOR	R8G8B8_to_B5G6R5(0x102030)

extern char ui_current_path[];

extern int screen_rotate;
extern UINT8 game_reset;

int do_ui_key(unsigned int key);

void draw_ui_main();
void draw_ui_browse(bool rebuiltlist);

void draw_status();

void ui_update_progress(float size, char * txt);
void ui_update_progress2(float size, const char * txt);


/* roms.cpp */

int findRomsInDir(bool force);
char *getRomsFileName(int idx);
int getRomsFileStat(int idx);


/* gui.cpp */

extern void *show_frame;
extern void *draw_frame;
extern void *work_frame;
extern void *tex_frame;

void init_gui();
void exit_gui();
void update_gui();
void clear_gui_texture(int color, UINT16 w, UINT16 h);

void get_screen_size();
void screen_initialise();

#ifdef ENABLE_PRX
	int draw_volume_status(UINT8 draw);
#endif


/* bzip */
extern char szAppRomPath[];


/*  */
int DrvInit(int nDrvNum, bool bRestore);
int DrvExit();


/* input.cpp */
int InpInit();
int InpExit();
void InpDIP();
int DoInputBlank(int bDipSwitch);


/* snd.cpp */

#define SND_RATE        (22050)
#define SND_FRAME_SIZE  ((SND_RATE * 100 + 3000) / 6000)

int sound_start();
int sound_stop();
void sound_next();
void sound_clear();

#endif	// _H_PSP_
