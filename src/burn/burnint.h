// Burn - Arcade emulator library - internal code

#ifndef BURNINT_H
#define BURNINT_H

// Standard headers
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tchar.h"
#include "burn.h"


#ifndef MAX_PATH
 #define MAX_PATH (260)
#endif

// ---------------------------------------------------------------------------
// CPU emulation interfaces

// sek.cpp
#include "sek.h"

// zet.cpp
#include "zet.h"

// ---------------------------------------------------------------------------
// Driver information

struct BurnDriver {
	const char *szShortName;			// The filename of the zip file (without extension)
	const char *szParent;				// The filename of the parent (without extension, NULL if not applicable)
	const char *szBoardROM;			// The filename of the board ROMs (without extension, NULL if not applicable)
	const char *szDate;

	// szFullNameA, szCommentA, szManufacturerA and szSystemA should always contain valid info
	// szFullNameW, szCommentW, szManufacturerW and szSystemW should be used only if characters or scripts are needed that ASCII can't handle
	const char    *szFullNameA; const char    *szCommentA; const char    *szManufacturerA; const char    *szSystemA;
	const wchar_t *szFullNameW; const wchar_t *szCommentW; const wchar_t *szManufacturerW; const wchar_t *szSystemW;

	int flags;			// See burn.h
	int players;		// Max number of players a game supports (so we can remove single player games from netplay)
	int hardware;		// Which type of hardware the game runs on
	int genre;
	int family;
	int (*GetZipName)(char **pszName, UINT32 i);					// Function to get possible zip names
	int (*GetRomInfo)(struct BurnRomInfo *pri, UINT32 i);			// Function to get the length and crc of each rom
	int (*GetRomName)(const char **pszName, UINT32 i, int nAka);	// Function to get the possible names for each rom
	int (*GetInputInfo)(struct BurnInputInfo *pii, UINT32 i);		// Function to get the input info for the game
	int (*GetDIPInfo)(struct BurnDIPInfo *pdi, UINT32 i);			// Function to get the input info for the game
	int (*Init)(); int (*Exit)(); int (*Frame)(); int (*Redraw)(); int (*AreaScan)(int nAction, int* pnMin);
	int JukeboxFlags; int (*JukeboxInit)(); int (*JukeboxExit)(); int (*JukeboxFrame)();
	UINT8 *pRecalcPal;												// Set to 1 if the palette needs to be fully re-calculated
	UINT16 nWidth, nHeight; UINT8 nXAspect, nYAspect;				// Screen width, height, x/y aspect
};

#define BurnDriverD BurnDriver		// Debug status
#define BurnDriverX BurnDriver		// Exclude from build

// Standard functions for dealing with ROM and input info structures
#include "stdfunc.h"

// ---------------------------------------------------------------------------

// burn.cpp
int BurnSetRefreshRate(float dRefreshRate);
int BurnByteswap(UINT8 *pm,int nLen);
int BurnClearScreen();

// load.cpp
int BurnLoadRom(UINT8 *Dest,int i, int nGap);
int BurnXorRom(UINT8 *Dest,int i, int nGap);
int BurnLoadBitField(UINT8 *pDest, UINT8 *pSrc, int nField, int nSrcLen);

// ---------------------------------------------------------------------------
// Colour-depth independant image transfer

extern UINT16 *pTransDraw;

void BurnTransferClear();
int BurnTransferCopy(UINT32 *pPalette);
void BurnTransferExit();
int BurnTransferInit();

// ---------------------------------------------------------------------------
// Plotting pixels

inline static void PutPix(UINT8 *pPix, UINT32 c)
{
	if (nBurnBpp >= 4) {
		*((UINT32 *)pPix) = c;
	} else {
		if (nBurnBpp == 2) {
			*((UINT16 *)pPix) = (UINT16)c;
		} else {
			pPix[0] = (UINT8)(c >>  0);
			pPix[1] = (UINT8)(c >>  8);
			pPix[2] = (UINT8)(c >> 16);
		}
	}
}

#endif
