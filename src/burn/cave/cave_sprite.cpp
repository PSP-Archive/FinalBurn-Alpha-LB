// Cave hardware sprites
#include "cave.h"

INT16 CaveSpriteVisibleXOffset;

UINT32 CaveSpriteROMOffset = 0;

UINT8 *CaveSpriteRAM = NULL;

UINT16 nCaveSpriteBank;
UINT16 nCaveSpriteBankDelay;

static UINT32 nSpriteAddressMask;

typedef struct {
	UINT8 flip;
	UINT8 priority;
	UINT16 palette;
	INT16 x; INT16 y;
	UINT16 xsize; UINT16 ysize;
	UINT16 xzoom; UINT16 yzoom;
	UINT32 address;
} CaveSprite;

static CaveSprite *pSpriteList = NULL;

INT8 (*CaveSpriteBuffer)();

static UINT8 *pRow;
static UINT8 *pPixel;
//static UINT32 *pSpriteData;
static UINT32 nSpriteDataOffset;
static UINT32 *pSpritePalette;

static UINT16 *pZBuffer = NULL;
static UINT16 *pZRow;
static UINT16 *pZPixel;

static int nSpriteRow, nSpriteRowSize;
static int nXPos, nYPos, nZPos;
static int nXSize, nYSize;
static int nSpriteXZoomSize, nSpriteYZoomSize;
static int nSpriteXOffset, nSpriteYOffset;

static int ALIGN_DATA nFirstSprite[4], nLastSprite[4];

static int nTopSprite;
static int nZOffset;

typedef void (*RenderSpriteFunction)();
static RenderSpriteFunction* RenderSprite;

// Include the sprite rendering functions
#include "cave_sprite_func.h"

INT8 CaveSpriteRender(UINT8 nLowPriority, UINT8 nHighPriority)
{
	static INT16 nMaskLeft, nMaskRight, nMaskTop, nMaskBottom;
	CaveSprite *pBuffer;

	UINT8 nPriorityMask = 0;
	int nMaxZPos = -1;
	int nCurrentZPos = 0x00010000;
	int nUseBuffer = 0x00010000;
	UINT8 nFunction;

	if (nLowPriority == 0) {
		nZPos = -1;
		nTopSprite = -1;

		nMaskLeft = nMaskTop = 9999;
		nMaskRight = nMaskBottom = -1;
	}

	if ((nBurnLayer & 1) == 0) {
		return 0;
	}

	if (nHighPriority < 3) {
		for (INT8 i = nHighPriority + 1; i < 4; i++) {
			if (nUseBuffer > nFirstSprite[i]) {
				nUseBuffer = nFirstSprite[i];
			}
		}
	}

	for (INT8 i = nLowPriority; i <= nHighPriority; i++) {
		if (nCurrentZPos > nFirstSprite[i]) {
			nCurrentZPos = nFirstSprite[i];
		}
		if (nMaxZPos < nLastSprite[i]) {
			nMaxZPos = nLastSprite[i];
		}
		nPriorityMask |= (8 >> i);
	}

	nPriorityMask &= nSpriteEnable;
	if (nPriorityMask == 0) {
		return 0;
	}

	for (pBuffer = pSpriteList + nCurrentZPos; nCurrentZPos <= nMaxZPos; pBuffer++, nCurrentZPos++) {

		if ((pBuffer->priority & nPriorityMask) == 0) {
			continue;
		}

		nXPos = pBuffer->x;
		nYPos = pBuffer->y;

		nSpriteDataOffset = CaveSpriteROMOffset + ((pBuffer->address << 8) & nSpriteAddressMask);
		pSpritePalette = CavePalette + pBuffer->palette;

		nXSize = pBuffer->xsize;
		nYSize = pBuffer->ysize;

		if ((pBuffer->xzoom == 0x0100) && (pBuffer->yzoom == 0x0100)) {				// This sprite doesn't use zooming

			nSpriteRowSize = pBuffer->xsize >> 2;

			if (pBuffer->flip & 1) {												// Y Flip
				nSpriteDataOffset = nSpriteDataOffset + ((nSpriteRowSize * (nYSize - 1)) << 2);
				nSpriteRowSize = -nSpriteRowSize;
			}

			if (nYPos >= 0x0200) {
				nYPos -= 0x0400;
			}

			if (nYPos < 0) {
				nSpriteDataOffset = nSpriteDataOffset + ((nSpriteRowSize * -nYPos) << 2);
				nYSize += nYPos;
				nYPos = 0;
			}

			if ((nYPos + nYSize) > nCaveYSize) {
				nYSize -= (nYPos + nYSize) - nCaveYSize;
			}

			if (nXPos >= 0x0200) {
				nXPos -= 0x0400;
			}

			if (nXPos < 0) {
				if ((pBuffer->flip & 2) == 0) {
					nSpriteDataOffset = nSpriteDataOffset + (-nXPos & ~0x0F);
				}
				nXSize -= -nXPos & 0xFFF0;
				nXPos += -nXPos & 0xFFF0;
			}

			if ((nXPos + nXSize) >= nCaveXSize) {
				if (pBuffer->flip & 2) {
					nSpriteDataOffset = nSpriteDataOffset + ((nXPos + nXSize - nCaveXSize) & ~0x0F);
				}
				nXSize -= (nXPos + nXSize - nCaveXSize) & 0xFFF0;
			}

#ifdef BUILD_PSP
			pRow = pBurnDraw + (nYPos << 10) + (nXPos << 1);
#else
			pRow = pBurnDraw + (nYPos * nBurnPitch) + (nXPos * nBurnBpp);
#endif

			nFunction = (pBuffer->flip & 2) << 1;									// X Flip

			if (nTopSprite > nCurrentZPos) {										// Test ZBuffer
				if ((nXPos < nMaskRight) && ((nXPos + nXSize) >= nMaskLeft) && (nYPos < nMaskBottom) && ((nYPos + nYSize) >= nMaskTop)) {
					nFunction |= 1;
				}
			}

			if (nUseBuffer < nCurrentZPos) {										// Write ZBuffer
				nFunction |= 2;

				if (nXPos < nMaskLeft) {
					nMaskLeft = nXPos;
				}
				if ((nXPos + nXSize) > nMaskRight) {
					nMaskRight = nXPos + nXSize;
				}
				if (nYPos < nMaskTop) {
					nMaskTop = nYPos;
				}
				if ((nYPos + nYSize) > nMaskBottom) {
					nMaskBottom = nYPos + nYSize;
				}
			}

			if (nFunction & 3) {
				pZRow = pZBuffer + (nYPos * 320) + nXPos;
				nZPos = nCurrentZPos + nZOffset;
			}

			nXSize = nXSize >> 2;

			RenderSprite[nFunction]();
		} else {																	// This sprite uses zooming
			nSpriteXZoomSize = 0x01000000;											// * zoom factor = size of each screen pixel

			nXSize *= pBuffer->xzoom;
			nXSize >>= 8;															// Round to multiple of whole pixel
			if (nXSize < 1) {														// Make sure the sprite is at least one pixel wide
				nXSize = 1;
			} else {
				nSpriteXZoomSize /= pBuffer->xzoom;
			}
			if (nSpriteXZoomSize > (pBuffer->xsize << 16)) {
				nSpriteXZoomSize = pBuffer->xsize << 16;
			}
			nSpriteXOffset = nSpriteXZoomSize >> 1;									// Make certain the pixels displayed are centered

			if (pBuffer->flip & 2) {												// X Flip
				nXPos += pBuffer->xsize - nXSize;

				nSpriteXOffset = (pBuffer->xsize << 16) - nSpriteXOffset;
				nSpriteXZoomSize = -nSpriteXZoomSize;
			}

			if (nXPos >= 0x0200) {
				nXPos -= 0x0400;
			}

			if (nXPos < 0) {
				if (nXPos + nXSize <= 0) {
					continue;
				}
				nXPos = -nXPos;
				nSpriteXOffset += nXPos * nSpriteXZoomSize;
				nXSize -= nXPos;
				nXPos = 0;
			}

			if ((nXPos + nXSize) >= nCaveXSize) {
				if (nXPos >= nCaveXSize) {
					continue;
				}
				nXSize = nCaveXSize - nXPos;
			}

			nSpriteRowSize = pBuffer->xsize;										// Size of each sprite row in memory
			nSpriteYZoomSize = 0x01000000;											// * zoom factor = size of each screen pixel

			nYSize *= pBuffer->yzoom;
			nYSize >>= 8;															// Round to multiple of whole pixel
			if (nYSize < 1) {														// Make certain the sprite is at least one pixel high
				nYSize = 1;
			} else {
				nSpriteYZoomSize /= pBuffer->yzoom;
			}
			if (nSpriteYZoomSize > (pBuffer->ysize << 16)) {
				nSpriteYZoomSize = pBuffer->ysize << 16;
			}
			nSpriteYOffset = nSpriteYZoomSize >> 1;									// Make certain the pixels displayed are centered

			if (pBuffer->flip & 1) {												// Y Flip
				nYPos += pBuffer->ysize - nYSize;

				nSpriteYOffset = (pBuffer->ysize << 16) - nSpriteYOffset;
				nSpriteYZoomSize = -nSpriteYZoomSize;
			}

			if (nYPos >= 0x0200) {
				nYPos -= 0x0400;
			}

			if (nYPos < 0) {
				if ((nYPos + nYSize) <= 0) {
					continue;
				}
				nYPos = -nYPos;
				nSpriteYOffset += nYPos * nSpriteYZoomSize;
				nYSize -= nYPos;
				nYPos = 0;
			}

			if ((nYPos + nYSize) >= nCaveYSize) {
				if (nYPos >= nCaveYSize) {
					continue;
				}
				nYSize = nCaveYSize - nYPos;
			}

#ifdef BUILD_PSP
			pRow = pBurnDraw + (nYPos << 10) + (nXPos << 1);
#else
			pRow = pBurnDraw + (nYPos * nBurnPitch) + (nXPos * nBurnBpp);
#endif

			nFunction = 8;

			if ((pBuffer->xzoom > 0x0100) || (pBuffer->yzoom > 0x0100)) {
				nFunction |= 4;
			}

			if (nTopSprite > nCurrentZPos) {										// Test ZBuffer
				if ((nXPos < nMaskRight) && ((nXPos + nXSize) >= nMaskLeft) && (nYPos < nMaskBottom) && ((nYPos + nYSize) >= nMaskTop)) {
					nFunction |= 1;
				}
			}

			if (nUseBuffer < nCurrentZPos) {										// Write ZBuffer
				nFunction |= 2;

				if (nXPos < nMaskLeft) {
					nMaskLeft = nXPos;
				}
				if ((nXPos + nXSize) > nMaskRight) {
					nMaskRight = nXPos + nXSize;
				}
				if (nYPos < nMaskTop) {
					nMaskTop = nYPos;
				}
				if ((nYPos + nYSize) > nMaskBottom) {
					nMaskBottom = nYPos + nYSize;
				}
			}

			if (nFunction & 3) {
				pZRow = pZBuffer + (nYPos * nCaveXSize) + nXPos;
				nZPos = nCurrentZPos + nZOffset;
			}

			nXSize <<= 16;
			nYSize <<= 16;

			RenderSprite[nFunction]();
		}
	}

	if (nMaxZPos > nTopSprite) {
		nTopSprite = nMaxZPos;
	}

	if (nHighPriority == 3) {
		if (nZPos >= 0) {
			nZOffset += nTopSprite;
			if (nZOffset > 0xFC00) {
				memset(pZBuffer, 0, nCaveXSize * nCaveYSize * sizeof(UINT16));
				nZOffset = 0;
			}
		}
	}

	return 0;
}

// Donpachi/DoDonpachi sprite format (no zooming)
static INT8 CaveSpriteBuffer_NoZoom()
{
	UINT16 *pSprite = (UINT16 *)(CaveSpriteRAM + (nCaveSpriteBank << 14));
	CaveSprite *pBuffer = pSpriteList;
	UINT8 nPriority;

	nFirstSprite[0] = 0x00010000;
	nFirstSprite[1] = 0x00010000;
	nFirstSprite[2] = 0x00010000;
	nFirstSprite[3] = 0x00010000;

	nLastSprite[0] = -1;
	nLastSprite[1] = -1;
	nLastSprite[2] = -1;
	nLastSprite[3] = -1;

	UINT16 word;
	INT16 x, y, xs, ys;

	for (INT16 i = 0, z = 0; i < 0x0400; i++, pSprite += 8) {

		word = pSprite[4];

		xs = (word >> 4) & 0x01F0;
		ys = (word << 4) & 0x01F0;
		if ((ys == 0) || (xs == 0)) {
			continue;
		}

#if 0
		x = (pSprite[2] + nCaveExtraXOffset) & 0x03FF;
#else
		x = (pSprite[2] + CaveSpriteVisibleXOffset) & 0x03FF;
#endif
		if (x >= 320) {
			if ((x + xs) <= 0x0400) {
				continue;
			}
		}

#if 0
		y = (pSprite[3] + nCaveExtraYOffset) & 0x03FF;
#else
		y = pSprite[3] & 0x03FF;
#endif
		if (y >= 240) {
			if ((y + ys) <= 0x0400) {
				continue;
			}
		}

		// Sprite is both active and onscreen, so add it to the buffer

		word = pSprite[0];

		nPriority = (word >> 4) & 0x03;
		if (nLastSprite[nPriority] == -1) {
			nFirstSprite[nPriority] = z;
		}
		nLastSprite[nPriority] = z;

		pBuffer->priority = 8 >> nPriority;

		pBuffer->flip = (word >> 2) & 0x03;
		pBuffer->palette = word & 0x3F00;

		pBuffer->address = pSprite[1] | ((word & 3) << 16);

		pBuffer->x = x;
		pBuffer->y = y;

		pBuffer->xsize = xs;
		pBuffer->ysize = ys;

		pBuffer++;
		z++;
	}

	return 0;
}

// Normal sprite format (zooming)
static INT8 CaveSpriteBuffer_ZoomA()
{
	UINT16 *pSprite = (UINT16 *)(CaveSpriteRAM + (nCaveSpriteBank << 14));
	CaveSprite *pBuffer = pSpriteList;
	UINT8 nPriority;

	nFirstSprite[0] = 0x00010000;
	nFirstSprite[1] = 0x00010000;
	nFirstSprite[2] = 0x00010000;
	nFirstSprite[3] = 0x00010000;

	nLastSprite[0] = -1;
	nLastSprite[1] = -1;
	nLastSprite[2] = -1;
	nLastSprite[3] = -1;

	UINT16 word;
	INT16 x, y, xs, ys;

	for (INT16 i = 0, z = 0; i < 0x0400; i++, pSprite += 8) {

		word = pSprite[6];

		xs = (word >> 4) & 0x01F0;
		ys = (word << 4) & 0x01F0;
		if ((ys == 0) || (xs == 0)) {
			continue;
		}

		word = pSprite[2];

		nPriority = (word >> 4) & 0x03;

		x = ((pSprite[0] >> 6) + CaveSpriteVisibleXOffset) & 0x03FF;
#if 0
		y = ((pSprite[1] >> 6) + nCaveExtraYOffset) & 0x03FF;
#else
		y = (pSprite[1] >> 6) & 0x03FF;
#endif

		if ((pSprite[4] <= 0x0100) && (pSprite[5] <= 0x0100)) {
			if (x >= 320) {
				if ((x + xs) <= 0x0400) {
					continue;
				}
			}
			if (y >= 240) {
				if ((y + ys) <= 0x0400) {
					continue;
				}
			}
		}

		// Sprite is active and most likely on screen, so add it to the buffer

		if (nLastSprite[nPriority] == -1) {
			nFirstSprite[nPriority] = z;
		}
		nLastSprite[nPriority] = z;

		pBuffer->priority = 8 >> nPriority;

		pBuffer->xzoom = pSprite[4];
		pBuffer->yzoom = pSprite[5];

		pBuffer->xsize = xs;
		pBuffer->ysize = ys;

		pBuffer->x = x;
		pBuffer->y = y;

		pBuffer->flip = (word >> 2) & 0x03;
		pBuffer->palette = word & 0x3F00;

		pBuffer->address = pSprite[3] | ((word & 3) << 16);

		pBuffer++;
		z++;
	}

	return 0;
}

// Normal sprite format (zooming, alternate position handling)
static INT8 CaveSpriteBuffer_ZoomB()
{
	UINT16 *pSprite = (UINT16 *)(CaveSpriteRAM + (nCaveSpriteBank << 14));
	CaveSprite *pBuffer = pSpriteList;
	UINT8 nPriority;

	nFirstSprite[0] = 0x00010000;
	nFirstSprite[1] = 0x00010000;
	nFirstSprite[2] = 0x00010000;
	nFirstSprite[3] = 0x00010000;

	nLastSprite[0] = -1;
	nLastSprite[1] = -1;
	nLastSprite[2] = -1;
	nLastSprite[3] = -1;

	UINT16 word;
	INT16 x, y, xs, ys;
	
	for (INT16 i = 0, z = 0; i < 0x0400; i++, pSprite += 8) {

		word = pSprite[6];

		xs = (word >> 4) & 0x01F0;
		ys = (word << 4) & 0x01F0;
		if ((ys == 0) || (xs == 0)) {
			continue;
		}

		word = pSprite[2];

		nPriority = (word >> 4) & 0x03;

#if 0
		x = (pSprite[0] + nCaveExtraXOffset) & 0x03FF;
# else
		x = (pSprite[0] + CaveSpriteVisibleXOffset) & 0x03FF;
#endif
#if 0
		y = (pSprite[1] + nCaveExtraYOffset) & 0x03FF;
#else
		y = pSprite[1] & 0x03FF;
#endif

		if ((pSprite[4] <= 0x0100) && (pSprite[5] <= 0x0100)) {
			if (x >= nCaveXSize) {
				if ((x + xs) <= 0x0400) {
					continue;
				}
			}
			if (y >= nCaveYSize) {
				if ((y + ys) <= 0x0400) {
					continue;
				}
			}
		}

		// Sprite is active and most likely on screen, so add it to the buffer

		if (nLastSprite[nPriority] == -1) {
			nFirstSprite[nPriority] = z;
		}
		nLastSprite[nPriority] = z;

		pBuffer->priority = 8 >> nPriority;

		pBuffer->xzoom = pSprite[4];
		pBuffer->yzoom = pSprite[5];

		pBuffer->xsize = xs;
		pBuffer->ysize = ys;

		pBuffer->x = x;
		pBuffer->y = y;

		pBuffer->flip = (word >> 2) & 0x03;
		pBuffer->palette = word & 0x3F00;

		pBuffer->address = pSprite[3] | ((word & 3) << 16);

		pBuffer++;
		z++;
	}

	return 0;
}

// Power Instinct 2 sprite format (no zooming)
static INT8 CaveSpriteBuffer_PowerInstinct()
{
	UINT16 *pSprite = (UINT16 *)(CaveSpriteRAM + (nCaveSpriteBank << 14));
	CaveSprite *pBuffer = pSpriteList;
	UINT8 nPriority;

	nFirstSprite[0] = 0x00010000;
	nFirstSprite[1] = 0x00010000;
	nFirstSprite[2] = 0x00010000;
	nFirstSprite[3] = 0x00010000;

	nLastSprite[0] = -1;
	nLastSprite[1] = -1;
	nLastSprite[2] = -1;
	nLastSprite[3] = -1;

	UINT16 word;
	INT16 x, y, xs, ys;

	for (INT16 i = 0, z = 0; i < 0x0400; i++, pSprite += 8) {

		word = pSprite[4];

		xs = (word >> 4) & 0x01F0;
		ys = (word << 4) & 0x01F0;
		if ((ys == 0) || (xs == 0)) {
			continue;
		}

		x = (pSprite[2] + nCaveExtraXOffset) & 0x03FF;
		if (x >= 320) {
			if ((x + xs) <= 0x0400) {
				continue;
			}
		}

		y = (pSprite[3] + nCaveExtraYOffset) & 0x03FF;
		if (y >= 240) {
			if ((y + ys) <= 0x0400) {
				continue;
			}
		}

		// Sprite is both active and onscreen, so add it to the buffer

		word = pSprite[0];

		nPriority = ((word >> 4) & 0x01) | 2;
		if (nLastSprite[nPriority] == -1) {
			nFirstSprite[nPriority] = z;
		}
		nLastSprite[nPriority] = z;

		pBuffer->priority = 8 >> nPriority;

		pBuffer->flip = (word >> 2) & 0x03;
		pBuffer->palette = ((word >> 4) & 0x03F0) + ((word << 5) & 0xC00);

		pBuffer->address = pSprite[1] | ((word & 3) << 16);

		pBuffer->x = x;
		pBuffer->y = y;

		pBuffer->xsize = xs;
		pBuffer->ysize = ys;

		pBuffer++;
		z++;
	}

	return 0;
}

void CaveSpriteExit()
{
	free(pSpriteList);
	pSpriteList = NULL;

	free(pZBuffer);
	pZBuffer = NULL;
	
	CaveSpriteVisibleXOffset = 0;

	return;
}

INT8 CaveSpriteInit(UINT8 nType, UINT32 nROMSize)
{
	free(pSpriteList);
	pSpriteList = (CaveSprite *)memalign(4, 0x0401 * sizeof(CaveSprite));
	if (pSpriteList == NULL) {
		CaveSpriteExit();
		return 1;
	}
	memset(pSpriteList, 0, 0x0401 * sizeof(CaveSprite));

	for (INT16 i = 0; i < 0x0400; i++) {
		pSpriteList[i].xzoom = 0x0100;
		pSpriteList[i].yzoom = 0x0100;
	}
	for (INT16 i = 0; i < 4; i++) {
		nFirstSprite[i] = 0x00010000;
		nLastSprite[i] = -1;
	}

	free(pZBuffer);
	pZBuffer = (UINT16 *)memalign(4, nCaveXSize * nCaveYSize * sizeof(UINT16));
	if (pZBuffer == NULL) {
		CaveSpriteExit();
		return 1;
	}
	memset(pZBuffer, 0, nCaveXSize * nCaveYSize * sizeof(UINT16));
	nZOffset = 0;

	for (nSpriteAddressMask = 1; nSpriteAddressMask < nROMSize; nSpriteAddressMask <<= 1) {}
	nSpriteAddressMask--;

	switch (nType) {
		case 0:
			CaveSpriteBuffer = &CaveSpriteBuffer_NoZoom;
			break;
		case 1:
			CaveSpriteBuffer = &CaveSpriteBuffer_ZoomA;
			break;
		case 2:
			CaveSpriteBuffer = &CaveSpriteBuffer_ZoomB;
			break;
		case 3:
			CaveSpriteBuffer = &CaveSpriteBuffer_PowerInstinct;
			break;
		default:
			CaveSpriteExit();
			return 1;
	}

	nCaveSpriteBank = 0;
	nCaveSpriteBankDelay = 0;

	RenderSprite = RenderSprite_ROT0[(nCaveXSize == 320) ? 0 : 1];

	return 0;
}

