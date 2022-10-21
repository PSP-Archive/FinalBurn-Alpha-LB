#include "cave.h"

UINT16 nCaveXSize = 0, nCaveYSize = 0;
INT16 nCaveXOffset = 0, nCaveYOffset = 0;
INT16 nCaveExtraXOffset = 0, nCaveExtraYOffset = 0;
INT16 nCaveRowModeOffset = 0;

INT8 CaveScanGraphics()
{
	SCAN_VAR(nCaveXOffset);
	SCAN_VAR(nCaveYOffset);

	SCAN_VAR(nCaveTileBank);

	SCAN_VAR(nCaveSpriteBank);
	SCAN_VAR(nCaveSpriteBankDelay);

	for (INT8 i = 0; i < 4; i++) {
		SCAN_VAR(CaveTileReg[i][0]);
		SCAN_VAR(CaveTileReg[i][1]);
		SCAN_VAR(CaveTileReg[i][2]);
	}

	return 0;
}

// This function fills the screen with the background colour
void CaveClearScreen(UINT32 nColour)
{
#ifdef BUILD_PSP

	extern void clear_gui_texture(int color, UINT16 w, UINT16 h);
	clear_gui_texture(((nColour & 0x001f ) << 3) | ((nColour & 0x07e0 ) << 5) | ((nColour & 0xf800 ) << 8), nCaveXSize, nCaveYSize);

#else

	if (nColour) {
		UINT32 *pClear = (UINT32 *)pBurnDraw;
		nColour = nColour | (nColour << 16);
		for (int i = nCaveXSize * nCaveYSize / 16; i > 0 ; i--) {
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
			*pClear++ = nColour;
		}
	} else {
		memset(pBurnDraw, 0, nCaveXSize * nCaveYSize * sizeof(short));
	}

#endif
}


void CaveNibbleSwap1(UINT8 *pData, UINT32 nLen)
{
	UINT8 *pOrg = pData + nLen - 1;
	UINT8 *pDest = pData + ((nLen - 1) << 1);

	for (int i = 0; i < nLen; i++, pOrg--, pDest -= 2) {
		pDest[0] = *pOrg & 0x0F;
		pDest[1] = *pOrg >> 4;
	}
}

void CaveNibbleSwap2(UINT8 *pData, UINT32 nLen)
{
	UINT8 *pOrg = pData + nLen - 1;
	UINT8 *pDest = pData + ((nLen - 1) << 1);

	for (int i = 0; i < nLen; i++, pOrg--, pDest -= 2) {
		pDest[1] = *pOrg & 0x0F;
		pDest[0] = *pOrg >> 4;
	}
}

void CaveNibbleSwap3(UINT8 *pData, UINT32 nLen)
{
	nLen >>= 1;
	for (int i = 0; i < nLen; i++, pData += 2) {
		UINT8 n1 = pData[0];
		UINT8 n2 = pData[1];

		pData[0] = (n1 << 4) | (n2 & 0x0F);
		pData[1] = (n1 & 0xF0) | (n2 >> 4);
	}
}

void CaveNibbleSwap4(UINT8 *pData, UINT32 nLen)
{
	nLen >>= 1;
	for (int i = 0; i < nLen; i++, pData += 2) {
		UINT8 n1 = pData[0];
		UINT8 n2 = pData[1];

		pData[1] = (n2 << 4) | (n1 & 0x0F);
		pData[0] = (n2 & 0xF0) | (n1 >> 4);
	}
}

