// FB Alpha - Emulator for MC68000/Z80 based arcade games
//            Refer to the "license.txt" file for more info

// Burner emulation library

#ifndef BURN_H
  #define BURN_H

#ifdef __cplusplus
 extern "C" {
#endif

#if !defined (_WIN32)
 #define __cdecl
#endif

#include <time.h>
#include <malloc.h>
#include "cheat.h"
#include "state.h"

#define ALIGN_DATA  __attribute__((aligned(4)))


// Enable the MAME logerror() function in debug builds
// #define MAME_USE_LOGERROR

// Give access to the CPUID function for various compilers
#if defined (__GNUC__)
 #define CPUID(f,ra,rb,rc,rd) __asm__ __volatile__ ("cpuid"											\
 													: "=a" (ra), "=b" (rb), "=c" (rc), "=d" (rd)	\
 													: "a"  (f)										\
 												   );
#elif defined (_MSC_VER)
 #define CPUID(f,ra,rb,rc,rd) __asm { __asm mov		eax, f		\
									  __asm cpuid				\
									  __asm mov		ra, eax		\
									  __asm mov		rb, ebx		\
									  __asm mov		rc, ecx		\
									  __asm mov		rd, edx }
#else
 #define CPUID(f,ra,rb,rc,rd)
#endif

#ifdef _UNICODE
 #define SEPERATOR_1 " \u2022 "
 #define SEPERATOR_2 " \u25E6 "
#else
 #define SEPERATOR_1 " ~ "
 #define SEPERATOR_2 " ~ "
#endif

#ifdef _UNICODE
 #define WRITE_UNICODE_BOM(file) { UINT16 BOM[] = { 0xFEFF }; fwrite(BOM, 2, 1, file); }
#else
 #define WRITE_UNICODE_BOM(file)
#endif

#define INCLUDE_NEOGEO_MULTISLOT 1

typedef signed char INT8;
typedef unsigned char UINT8;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed int INT32;
typedef unsigned int UINT32;

extern int nBurnVer;						// Version number of the library

// ---------------------------------------------------------------------------
// Callbacks

// Application-defined rom loading function
extern int (__cdecl *BurnExtLoadRom)(UINT8 *Dest, int *pnWrote, int i);

// Application-defined progress indicator functions
extern int (__cdecl *BurnExtProgressRangeCallback)(float dProgressRange);
extern int (__cdecl *BurnExtProgressUpdateCallback)(float dProgress, const TCHAR *pszText, bool bAbs);

// Application-defined colour conversion function
extern UINT32 (__cdecl *BurnHighCol) (int r, int g, int b, int i);

// ---------------------------------------------------------------------------

extern UINT32 nCurrentFrame;

inline static UINT32 GetCurrentFrame() {
	return nCurrentFrame;
}

inline static void SetCurrentFrame(const UINT32 n) {
	nCurrentFrame = n;
}

// ---------------------------------------------------------------------------
// Driver info structures

// ROMs

#define BRF_PRG				(1 << 20)
#define BRF_GRA				(1 << 21)
#define BRF_SND				(1 << 22)

#define BRF_ESS				(1 << 24)
#define BRF_BIOS			(1 << 25)
#define BRF_SELECT			(1 << 26)
#define BRF_OPT				(1 << 27)
#define BRF_NODUMP			(1 << 28)

struct BurnRomInfo {
	const char *szName;
	UINT32 nLen;
	UINT32 nCrc;
	UINT32 nType;
};

// Inputs

#define BIT_DIGITAL			(1)

#define BIT_GROUP_ANALOG	(4)
#define BIT_ANALOG_REL		(4)
#define BIT_ANALOG_ABS		(5)

#define BIT_GROUP_CONSTANT	(8)
#define BIT_CONSTANT		(8)
#define BIT_DIPSWITCH		(9)

struct BurnInputInfo {
	const char *szName;
	UINT8 nType;
	union {
		UINT8  *pVal;					// Most inputs use a char*
		UINT16 *pShortVal;				// All analog inputs use a short*
	};
	const char *szInfo;
};

// DIPs

struct BurnDIPInfo {
	int nInput;
	UINT8 nFlags;
	UINT8 nMask;
	UINT8 nSetting;
	const char *szText;
};

// ---------------------------------------------------------------------------

extern bool bBurnUseMMX;
extern bool bBurnUseASMCPUEmulation;

extern UINT32 nFramesEmulated;
extern UINT32 nFramesRendered;
extern clock_t starttime;				// system time when emulation started and after roms loaded

extern bool bForce60Hz;

extern int nBurnFPS;
extern int nBurnCPUSpeedAdjust;

extern UINT32 nBurnDrvCount;			// Count of game drivers
extern UINT32 nBurnDrvSelect;			// Which game driver is selected

extern int nMaxPlayers;

extern UINT8 *pBurnDraw;				// Pointer to correctly sized bitmap
extern int nBurnPitch;					// Pitch between each line
extern int nBurnBpp;					// Bytes per pixel (2, 3, or 4)

extern UINT8 nBurnLayer;				// Can be used externally to select which layers to show
extern UINT8 nSpriteEnable;				// Can be used externally to select which Sprites to show

extern int nBurnSoundRate;				// Samplerate of sound
extern UINT16 nBurnSoundLen;			// Length in samples per frame
extern INT16 *pBurnSoundOut;			// Pointer to output buffer

extern int nInterpolation;				// Desired interpolation level for ADPCM/PCM sound
extern int nFMInterpolation;			// Desired interpolation level for FM sound

#define PRINT_NORMAL	(0)
#define PRINT_UI		(1)
#define PRINT_IMPORTANT (2)
#define PRINT_ERROR		(3)

extern int (__cdecl *bprintf) (int nStatus, TCHAR *szFormat, ...);

int BurnLibInit();
int BurnLibExit();

int BurnDrvInit();
int BurnDrvExit();
int BurnDrvFrame();
int BurnDrvRedraw();
int BurnRecalcPal();

int BurnSetProgressRange(float dProgressRange);
int BurnUpdateProgress(float dProgressStep, const TCHAR *pszText, bool bAbs);

// ---------------------------------------------------------------------------
// Retrieve driver information

#define DRV_NAME		 (0)
#define DRV_DATE		 (1)
#define DRV_FULLNAME	 (2)
#define DRV_ALTNAME		 (3)
#define DRV_COMMENT		 (4)
#define DRV_MANUFACTURER (5)
#define DRV_SYSTEM		 (6)
#define DRV_PARENT		 (7)
#define DRV_BOARDROM	 (8)

#define DRV_NEXTNAME	 (1 << 8)
#define DRV_ASCIIONLY	 (1 << 12)
#define DRV_UNICODEONLY	 (1 << 13)

const TCHAR *BurnDrvGetText(UINT32 i);
const char *BurnDrvGetTextA(UINT32 i);

int BurnDrvGetZipName(char **pszName, UINT32 i);
int BurnDrvGetRomInfo(struct BurnRomInfo *pri, UINT32 i);
int BurnDrvGetRomName(const char **pszName, UINT32 i, int nAka);
int BurnDrvGetInputInfo(struct BurnInputInfo *pii, UINT32 i);
int BurnDrvGetDIPInfo(struct BurnDIPInfo *pdi, UINT32 i);
int BurnDrvGetVisibleSize(UINT16 *pnWidth, UINT16* pnHeight);
int BurnDrvGetVisibleOffs(int *pnLeft, int *pnTop);
int BurnDrvGetFullSize(UINT16 *pnWidth, UINT16 *pnHeight);
int BurnDrvGetAspect(UINT8 *pnXAspect, UINT8 *pnYAspect);
int BurnDrvGetHardwareCode();
int BurnDrvGetFlags();
bool BurnDrvIsWorking();
int BurnDrvGetMaxPlayers();
int BurnDrvSetVisibleSize(UINT16 pnWidth, UINT16 pnHeight);
int BurnDrvSetAspect(UINT8 pnXAspect, UINT8 pnYAspect);

#define JUKEBOX_SOUND_NULL	0
#define JUKEBOX_SOUND_STOP	1
#define JUKEBOX_SOUND_PLAY	2
extern UINT32 JukeboxSoundLatch;
extern UINT32 JukeboxSoundCommand;
int BurnJukeboxGetFlags();
int BurnJukeboxInit();
int BurnJukeboxExit();
int BurnJukeboxFrame();

extern bool bSaveCRoms;

#define MAX_NEO_SLOTS 6
extern UINT32 nNeoSlotDrvNum[MAX_NEO_SLOTS];

// ---------------------------------------------------------------------------
// Flags used with the Burndriver structure

// Flags for the flags member
#define BDF_GAME_WORKING			(1 << 0)
#define BDF_ORIENTATION_FLIPPED		(1 << 1)
#define BDF_ORIENTATION_VERTICAL	(1 << 2)
#define BDF_BOARDROM				(1 << 3)
#define BDF_CLONE					(1 << 4)
#define BDF_BOOTLEG					(1 << 5)
#define BDF_PROTOTYPE				(1 << 6)
#define BDF_16BIT_ONLY				(1 << 7)
#define BDF_HACK					(1 << 8)
#define BDF_HOMEBREW				(1 << 9)
#define BDF_DEMO					(1 << 10)

// Flags for the hardware member
// Format: 0xDDEEFFFF, where EE: Manufacturer, DD: Hardware platform, FFFF: Flags (used by driver)

#define HARDWARE_PUBLIC_MASK			(0xFFFF0000)
#define HARDWARE_PREFIX_MISC_PRE90S		(0x00000000)
#define HARDWARE_PREFIX_CAPCOM			(0x01000000)
#define HARDWARE_PREFIX_SEGA			(0x02000000)
#define HARDWARE_PREFIX_KONAMI			(0x03000000)
#define HARDWARE_PREFIX_TOAPLAN			(0x04000000)
#define HARDWARE_PREFIX_SNK				(0x05000000)
#define HARDWARE_PREFIX_CAVE			(0x06000000)
#define HARDWARE_PREFIX_CPS2			(0x07000000)
#define HARDWARE_PREFIX_IGS_PGM			(0x08000000)
#define HARDWARE_PREFIX_CPS3			(0x09000000)
#define HARDWARE_PREFIX_MISC_POST90S	(0x0a000000)
#define HARDWARE_PREFIX_TAITO			(0x0b000000)
#define HARDWARE_PREFIX_SEGA_MEGADRIVE	(0x0c000000)
#define HARDWARE_PREFIX_PSIKYO			(0x0d000000)
#define HARDWARE_PREFIX_KANEKO16		(0x0e000000)
#define HARDWARE_PREFIX_PACMAN			(0x0f000000)

#define HARDWARE_MISC_PRE90S			(HARDWARE_PREFIX_MISC_PRE90S)
#define HARDWARE_MISC_POST90S			(HARDWARE_PREFIX_MISC_POST90S)

#define HARDWARE_CAPCOM_CPS1			(HARDWARE_PREFIX_CAPCOM | 0x00010000)
#define HARDWARE_CAPCOM_CPS1_QSOUND		(HARDWARE_PREFIX_CAPCOM | 0x00020000)
#define HARDWARE_CAPCOM_CPS1_GENERIC	(HARDWARE_PREFIX_CAPCOM | 0x00030000)
#define HARDWARE_CAPCOM_CPSCHANGER		(HARDWARE_PREFIX_CAPCOM | 0x00040000)
#define HARDWARE_CAPCOM_CPS2			(HARDWARE_PREFIX_CPS2   | 0x00010000)
#define HARDWARE_CAPCOM_CPS2_SIMM		(0x0002)

#define HARDWARE_SEGA_SYSTEMX			(HARDWARE_PREFIX_SEGA | 0x00010000)
#define HARDWARE_SEGA_SYSTEMY			(HARDWARE_PREFIX_SEGA | 0x00020000)
#define HARDWARE_SEGA_SYSTEM16A			(HARDWARE_PREFIX_SEGA | 0x00030000)
#define HARDWARE_SEGA_SYSTEM16B 		(HARDWARE_PREFIX_SEGA | 0x00040000)
#define HARDWARE_SEGA_SYSTEM16M			(HARDWARE_PREFIX_SEGA | 0x00050000)
#define HARDWARE_SEGA_SYSTEM18			(HARDWARE_PREFIX_SEGA | 0x00060000)
#define HARDWARE_SEGA_HANGON			(HARDWARE_PREFIX_SEGA | 0x00070000)
#define HARDWARE_SEGA_OUTRUN			(HARDWARE_PREFIX_SEGA | 0x00080000)
#define HARDWARE_SEGA_SYSTEM1			(HARDWARE_PREFIX_SEGA | 0x00090000)

#define HARDWARE_SEGA_FD1089A_ENC		(0x0002)
#define HARDWARE_SEGA_FD1089B_ENC		(0x0004)
#define HARDWARE_SEGA_5358				(0x0008)
#define HARDWARE_SEGA_MC8123_ENC		(0x0010)
#define HARDWARE_SEGA_BAYROUTE_MEMMAP	(0x0020)
#define HARDWARE_SEGA_ALT_MEMMAP		(0x0040)
#define HARDWARE_SEGA_FD1094_ENC		(0x0080)
#define HARDWARE_SEGA_SPRITE_LOAD32		(0x0100)
#define HARDWARE_SEGA_YM2203			(0x0200)
#define HARDWARE_SEGA_INVERT_TILES		(0x0400)
#define HARDWARE_SEGA_5521				(0x0800)
#define HARDWARE_SEGA_5797				(0x1000)

#define HARDWARE_KONAMI_68K_Z80		(HARDWARE_PREFIX_KONAMI | 0x00010000)
#define HARDWARE_KONAMI_68K_ONLY	(HARDWARE_PREFIX_KONAMI | 0x00020000)

#define HARDWARE_TOAPLAN_RAIZING	(HARDWARE_PREFIX_TOAPLAN | 0x00010000)
#define HARDWARE_TOAPLAN_68K_Zx80	(HARDWARE_PREFIX_TOAPLAN | 0x00020000)
#define HARDWARE_TOAPLAN_68K_ONLY	(HARDWARE_PREFIX_TOAPLAN | 0x00030000)

#define HARDWARE_SNK_NEOGEO			(HARDWARE_PREFIX_SNK | 0x00100000)
#define HARDWARE_SNK_SRAM			(0x0002)	// SRAM protection
#define HARDWARE_SNK_SWAPP			(0x0004)	// Swap code roms
#define HARDWARE_SNK_SWAPV			(0x0008)	// Swap sound roms
#define HARDWARE_SNK_SWAPC			(0x0010)	// Swap sprite roms
#define HARDWARE_SNK_ENCRYPTED_A	(0x0020)	// KOF99 encryption scheme
#define HARDWARE_SNK_ENCRYPTED_B	(0x0040)	// KOF2000 encryption scheme
#define HARDWARE_SNK_ALTERNATE_TEXT	(0x0080)	// KOF2000 text layer banks
#define HARDWARE_SNK_SMA_PROTECTION	(0x0100)	// SMA protection
#define HARDWARE_SNK_CUSTOM_BANKING (0x0200)	// Uses custom banking
#define HARDWARE_SNK_P32			(0x0400)	// Uses 32Bit 68000 roms
#define HARDWARE_SNK_PVC_PROT		(0x0800)
#define HARDWARE_SNK_ENCRYPTED_M1	(0x1000)

#define HARDWARE_SNK_CONTROLMASK	(0xF0000)
#define HARDWARE_SNK_JOYSTICK		(0x00000)	// Uses joysticks
#define HARDWARE_SNK_PADDLE			(0x10000)	// Uses joysticks or paddles
#define HARDWARE_SNK_TRACKBALL		(0x20000)	// Uses a trackball
#define HARDWARE_SNK_4_JOYSTICKS	(0x30000)	// Uses 4 joysticks
#define HARDWARE_SNK_MAHJONG		(0x40000)	// Uses a special mahjong controller
#define HARDWARE_SNK_GAMBLING		(0x50000)	// Uses gambling controls
#define HARDWARE_SNK_PCB			(0x60000)
#define HARDWARE_SNK_MVSCARTRIDGE	(0x70000)

#define HARDWARE_SNK_NEOCD			(HARDWARE_PREFIX_SNK | 0x00200000)

#define HARDWARE_CAVE_68K_ONLY		(HARDWARE_PREFIX_CAVE)
#define HARDWARE_CAVE_68K_Z80		(HARDWARE_PREFIX_CAVE | 0x0001)
#define HARDWARE_CAVE_M6295			(0x0002)
#define HARDWARE_CAVE_YM2151		(0x0004)

#define HARDWARE_IGS_PGM			(HARDWARE_PREFIX_IGS_PGM)
#define HARDWARE_IGS_USE_ARM_CPU	(0x0001)
#define HARDWARE_IGS_JAMMAPCB		(0x0002)

#define HARDWARE_CAPCOM_CPS3		(HARDWARE_PREFIX_CPS3)
#define HARDWARE_CAPCOM_CPS3_NO_CD	(0x0001)

#define HARDWARE_TAITO_TAITOZ		(HARDWARE_PREFIX_TAITO | 0x00010000)
#define HARDWARE_TAITO_TAITOF2		(HARDWARE_PREFIX_TAITO | 0x00020000)
#define HARDWARE_TAITO_MISC			(HARDWARE_PREFIX_TAITO | 0x00030000)
#define HARDWARE_TAITO_TAITOX		(HARDWARE_PREFIX_TAITO | 0x00040000)

#define HARDWARE_SEGA_MEGADRIVE		(HARDWARE_PREFIX_SEGA_MEGADRIVE)

#define HARDWARE_PSIKYO				(HARDWARE_PREFIX_PSIKYO)

#define HARDWARE_KANEKO16			(HARDWARE_PREFIX_KANEKO16)

#define HARDWARE_PACMAN				(HARDWARE_PREFIX_PACMAN)

// flags for the jukebox member
#define JBF_GAME_WORKING		(1 << 0)

// flags for the genre member
#define GBF_HORSHOOT		(1 << 0)
#define GBF_VERSHOOT		(1 << 1)
#define GBF_SCRFIGHT		(1 << 2)
#define GBF_VSFIGHT			(1 << 3)
#define GBF_BIOS			(1 << 4)
#define GBF_BREAKOUT		(1 << 5)
#define GBF_CASINO			(1 << 6)
#define GBF_BALLPADDLE		(1 << 7)
#define GBF_MAZE			(1 << 8)
#define GBF_MINIGAMES		(1 << 9)
#define GBF_PINBALL			(1 << 10)
#define GBF_PLATFORM		(1 << 11)
#define GBF_PUZZLE			(1 << 12)
#define GBF_QUIZ			(1 << 13)
#define GBF_SPORTSMISC		(1 << 14)
#define GBF_SPORTSFOOTBALL	(1 << 15)
#define GBF_MISC			(1 << 16)
#define GBF_MAHJONG			(1 << 17)
#define GBF_RACING			(1 << 18)
#define GBF_SHOOT			(1 << 19)

// flags for the family member
#define FBF_MSLUG			(1 << 0)
#define FBF_SF				(1 << 1)
#define FBF_KOF				(1 << 2)
#define FBF_DSTLK			(1 << 3)
#define FBF_FATFURY			(1 << 4)
#define FBF_SAMSHO			(1 << 5)
#define FBF_19XX			(1 << 6)
#define FBF_SONICWI			(1 << 7)
#define FBF_PWRINST			(1 << 8)

#ifdef __cplusplus
 } // End of extern "C"
#endif

#endif /* BURN_H */
