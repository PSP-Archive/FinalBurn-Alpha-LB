#include "burnint.h"
#include "ics2115.h"

// pgm_run.cpp
extern UINT8 PgmJoy1[];
extern UINT8 PgmJoy2[];
extern UINT8 PgmJoy3[];
extern UINT8 PgmJoy4[];
extern UINT8 PgmBtn1[];
extern UINT8 PgmBtn2[];
extern UINT8 PgmInput[];
extern UINT8 PgmReset;

extern UINT8 nCavePgm;
extern UINT8 nPGMEnableIRQ4;

extern UINT32 nPGM68KROMLen;
extern UINT32 nPGMTileROMLen;
extern UINT32 nPGMTileROMExpLen;

extern UINT32 *PgmBgRam, *PgmTxRam, *PgmPalette;
extern UINT16 *PgmRsRam, *PgmPalRam, *PgmVidReg, *PgmSprBuf;

extern UINT8 *PGM68KROM;
extern UINT8 *PGMUSER0, *PGMUSER1;

extern UINT8 nPgmPalRecalc;

extern void (*pPgmInitCallback)();
extern void (*pPgmResetCallback)();
extern int  (*pPgmScanCallback)(int, int*);

extern int pgmInit();
extern int pgmExit();
extern int pgmFrame();
extern int pgmDraw();
extern int pgmScan(int nAction, int *pnMin);

extern UINT32 nPGMTileROMOffset;
extern UINT32 nPGMTileROMExpOffset;
extern UINT32 nPGMSPRColROMOffset;
extern UINT32 nPGMSPRMaskROMOffset;

// pgm_draw
void pgmInitDraw();
void pgmExitDraw();
int pgmDraw();

// pgm_prot.cpp

void install_protection_ket();
void install_protection_ddp2();
void install_protection_ddp3();

void reset_ddp3();

// pgm_crypt.cpp

void pgm_decrypt_ddp2();
void pgm_decrypt_py2k2();
void pgm_decrypt_ket();
void pgm_decrypt_espgal();


/* PGM Palette */
inline static UINT32 PgmCalcCol(UINT16 nColour)
{
#ifdef BUILD_PSP

	return ((nColour & 0x001f) << 11) | ((nColour & 0x03e0) <<  1) | ((nColour & 0x7c00) >> 10);

#else

	int r, g, b;

	r = (nColour & 0x001F) << 3;	// Red
	r |= r >> 5;
	g = (nColour & 0x03E0) >> 2;  // Green
	g |= g >> 5;
	b = (nColour & 0x7C00) >> 7;	// Blue
	b |= b >> 5;

	return BurnHighCol(b, g, r, 0);

#endif
}

