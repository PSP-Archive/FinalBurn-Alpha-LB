#include "burnint.h"
#include "eeprom.h"
#include "UniCache.h"

#define CAVE_REFRESHRATE (15625.0 / 271.5)
#define CAVE_VBLANK_LINES (271.5 - 240.0)

// cave.cpp
extern UINT16 nCaveXSize, nCaveYSize;
extern INT16 nCaveXOffset, nCaveYOffset;
extern INT16 nCaveExtraXOffset, nCaveExtraYOffset;
extern INT16 nCaveRowModeOffset;

INT8 CaveScanGraphics();
void CaveClearScreen(UINT32 nColour);

void CaveNibbleSwap1(UINT8 *pData, UINT32 nLen);
void CaveNibbleSwap2(UINT8 *pData, UINT32 nLen);
void CaveNibbleSwap3(UINT8 *pData, UINT32 nLen);
void CaveNibbleSwap4(UINT8 *pData, UINT32 nLen);


// cave_palette.cpp
extern UINT32 *CavePalette;

extern UINT8 *CavePalSrc;
extern UINT8 CaveRecalcPalette;

INT8 CavePalInit(UINT16 nPalSize);
INT8 CavePalExit();
INT8 CavePalUpdate4Bit(UINT16 nOffset, UINT8 nNumPalettes);
INT8 CavePalUpdate8Bit(UINT16 nOffset, UINT8 nNumPalettes);

void CavePalWriteByte(UINT32 nAddress, UINT8  byteValue);
void CavePalWriteWord(UINT32 nAddress, UINT16 wordValue);


// cave_tiles.cpp
extern UINT32 CaveTileROMOffset[4];
extern UINT8 *CaveTileRAM[4];

extern UINT16 CaveTileReg[4][3];
extern UINT8 nCaveTileBank;

INT8 CaveTileRender(UINT8 nMode);
void CaveTileExit();
INT8 CaveTileInit();
INT8 CaveTileInitLayer(UINT8 nLayer, UINT32 nROMSize, UINT8 nBitdepth, UINT16 nOffset);


// cave_sprite.cpp
extern INT16 CaveSpriteVisibleXOffset;

extern UINT32 CaveSpriteROMOffset;
extern UINT8* CaveSpriteRAM;

extern UINT16 nCaveSpriteBank;
extern UINT16 nCaveSpriteBankDelay;

extern INT8 (*CaveSpriteBuffer)();
extern INT8 CaveSpriteRender(UINT8 nLowPriority, UINT8 nHighPriority);
void CaveSpriteExit();
INT8 CaveSpriteInit(UINT8 nType, UINT32 nROMSize);


inline static void CaveClearOpposites(UINT16 *nJoystickInputs)
{
	if ((*nJoystickInputs & 0x0003) == 0x0003) {
		*nJoystickInputs &= ~0x0003;
	}
	if ((*nJoystickInputs & 0x000C) == 0x000C) {
		*nJoystickInputs &= ~0x000C;
	}
}


inline static UINT32 CaveCalcCol(UINT16 nColour)
{
#ifdef BUILD_PSP

	return ((nColour & 0x03E0) >> 5) | ((nColour & 0x7C00) >> 4) | ((nColour & 0x001F) << 11);

#else

	UINT32 r, g, b;

	r = (nColour & 0x03E0) >> 2;	// Red
	r |= r >> 5;
	g = (nColour & 0x7C00) >> 7;  	// Green
	g |= g >> 5;
	b = (nColour & 0x001F) << 3;	// Blue
	b |= b >> 5;

	return BurnHighCol(r, g, b, 0);

#endif
}

