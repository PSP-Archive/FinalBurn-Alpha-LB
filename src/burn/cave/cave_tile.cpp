// Cave hardware tilemaps

#include "cave.h"

UINT32 ALIGN_DATA CaveTileROMOffset[4] = { 0, };
UINT8 *CaveTileRAM[4] = { NULL, };

UINT16 ALIGN_DATA CaveTileReg[4][3];
UINT8 nCaveTileBank = 0;


typedef struct {
	INT16 x; INT16 y; UINT32 tile;
} CaveTile;

static CaveTile *CaveTileQueueMemory[4] = { NULL, };
static CaveTile *CaveTileQueue[4][4];

static INT8 *CaveTileAttrib[4] = { NULL, };

// Used when row-select mode is enabled
static UINT16 *pRowScroll[4] = { NULL, };
static UINT16 *pRowSelect[4] = { NULL, };

static INT16  ALIGN_DATA nLayerXOffset[4];
static INT16  nLayerYOffset;
static UINT16 ALIGN_DATA nPaletteOffset[4];
static UINT8  ALIGN_DATA nPaletteSize[4];

static UINT16 ALIGN_DATA CaveTileCount[4];
static UINT16 ALIGN_DATA CaveTileMax[4];
static UINT32 ALIGN_DATA nTileMask[4];

static UINT8  *pTile;
static UINT32 *pTileData;
static UINT32 *pTilePalette;
static UINT16 *pTileRowInfo;

typedef void (*RenderTileFunction)();
static RenderTileFunction *RenderTile;

static INT16 nTileXPos, nTileYPos, nRowOffset;

static UINT16 nClipX8, nClipX16;
static UINT16 nClipY8, nClipY16;

// Include the tile rendering functions
#include "cave_tile_func.h"

static void CaveQueue8x8Layer_Normal(UINT8 nLayer)
{
	INT16 x, y;
	UINT16 bx, by, cx, cy, mx;
	UINT16 nTileColumn, nTileRow;
	UINT32 nTileNumber;

	UINT8 *pTileRAM = CaveTileRAM[nLayer];

	UINT32 nTileOffset = 0;
	if (nCaveTileBank) {
		nTileOffset = 0x040000;
	}

	mx = (nCaveXSize >> 3) << 2;

	bx = CaveTileReg[nLayer][0] - 0x0A + nLayerXOffset[nLayer];
	bx &= 0x01FF;
	cx = (bx >> 3) << 2;
	bx &= 0x0007;

	by = CaveTileReg[nLayer][1] + nLayerYOffset;
	by &= 0x01FF;
	cy = (by >> 3) << 8;
	by &= 0x0007;

	for (y = 0; y < (31 << 8); y += (1 << 8)) {
		nTileRow = (cy + y) & (0x3F << 8);
		nTileYPos = (y >> 5) - by;

		if ((nTileYPos <= -8) || (nTileYPos >= nCaveYSize)) {
			continue;
		}

		for (x = mx; x >= 0; x -= (1 << 2)) {
			nTileColumn = (cx + x) & (0x3F << 2);
			nTileNumber  = *((UINT16 *)(pTileRAM + 0x4000 + nTileRow + nTileColumn)) << 16;
			nTileNumber |= *((UINT16 *)(pTileRAM + 0x4002 + nTileRow + nTileColumn));

			nTileNumber |= nTileOffset;

			if (CaveTileAttrib[nLayer][nTileNumber & nTileMask[nLayer]]) {
				continue;
			}

			nTileXPos = (x << 1) - bx;

			if ((nTileXPos <= -8) || (nTileXPos >= nCaveXSize)) {
				continue;
			}

			CaveTileQueue[nLayer][nTileNumber >> 30]->x = nTileXPos;
			CaveTileQueue[nLayer][nTileNumber >> 30]->y = nTileYPos;
			CaveTileQueue[nLayer][nTileNumber >> 30]->tile = nTileNumber;
			CaveTileQueue[nLayer][nTileNumber >> 30]++;
		}
	}

	return;
}

static void Cave8x8Layer_Normal(UINT8 nLayer, UINT8 nPriority)
{
	UINT32 nTileNumber;

	UINT32 *pPalette = CavePalette + nPaletteOffset[nLayer];
	UINT32 nPaletteMask = 0x3F000000;
	if (nPaletteSize[nLayer] == 6) {
		nPaletteMask = 0x0F000000;
	}
	UINT8 nPaletteShift = 24 - nPaletteSize[nLayer];

	CaveTile *TileQueue = CaveTileQueue[nLayer][nPriority];

	while ((nTileXPos = TileQueue->x) < 9999) {
		nTileYPos = TileQueue->y;
		nTileNumber = TileQueue->tile;
		pTilePalette = pPalette + ((nTileNumber & nPaletteMask) >> nPaletteShift);
		nTileNumber &= nTileMask[nLayer];

#ifdef BUILD_PSP
		pTile = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);
#else
		pTile = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);
#endif

		pTileData = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 6), 64);

		if ((nTileYPos < 0) || (nTileYPos > nClipY8) || (nTileXPos < 0) || (nTileXPos > nClipX8)) {
			RenderTile[1]();
		} else {
			RenderTile[0]();
		}

		TileQueue++;
	}

	return;
}

static void Cave8x8Layer_RowScroll(UINT8 nLayer, UINT8 nPriority)
{
	INT16 x, y;
	UINT16 bx, by, cy;
	UINT16 nTileColumn, nTileRow;
	UINT32 nTileNumber;

    UINT8  *pTileRAM = CaveTileRAM[nLayer];
	UINT32 *pPalette = CavePalette + nPaletteOffset[nLayer];
	UINT8 nPaletteShift = 24 - nPaletteSize[nLayer];

	UINT16 *rowscroll = pRowScroll[nLayer];

	UINT16 count = CaveTileCount[nLayer];

	if (count >= (64 * 31)) {
		return;
	}

	UINT32 nTileOffset = 0;
	if (nCaveTileBank) {
		nTileOffset = 0x040000;
	}

	bx = CaveTileReg[nLayer][0] - 0x0A + nLayerXOffset[nLayer];
	bx &= 0x01FF;

	by = CaveTileReg[nLayer][1] + nLayerYOffset;
	by &= 0x01FF;
	cy = (by >> 3) << 8;
	by &= 0x0007;

	if (nPriority == 0) {
		INT16 dy = CaveTileReg[nLayer][1] + 0x12 - nCaveRowModeOffset;

		for (y = 0; y < nCaveYSize; y++) {
			rowscroll[y] = *((UINT16 *)(pTileRAM + 0x1000 + (((dy + y) & 0x01FF) << 2))) + bx;
		}
	}

	for (y = 0; y < (31 << 8); y += (1 << 8)) {
		nTileYPos = (y >> 5) - by;

		nTileRow = (cy + y) & (0x3F << 8);
		pTileRowInfo = rowscroll + nTileYPos;

		for (x = 0; x < (64 << 2); x += (1 << 2)) {
			nTileColumn = x & (0x3F << 2);
			nTileNumber = *((UINT16 *)(pTileRAM + 0x4000 + nTileRow + nTileColumn)) << 16;
			if ((nTileNumber >> 30) != nPriority) {
				continue;
			}
			nTileNumber |= *((UINT16 *)(pTileRAM + 0x4002 + nTileRow + nTileColumn));
			pTilePalette = pPalette + ((nTileNumber & 0x3F000000) >> nPaletteShift);

			nTileNumber |= nTileOffset;
			nTileNumber &= nTileMask[nLayer];

			count++;

			if ((nTileYPos <= -8) || (nTileYPos >= nCaveYSize)) {
				continue;
			}

			if (CaveTileAttrib[nLayer][nTileNumber]) {
				continue;
			}

			nTileXPos = (x << 1);

#ifdef BUILD_PSP
			pTile = pBurnDraw + (nTileYPos << 10);
#else
			pTile = pBurnDraw + (nTileYPos * nBurnPitch);
#endif

			pTileData = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 6), 64);

			if ((nTileYPos < 0) || (nTileYPos > nClipY8) || (nTileXPos < 0) || (nTileXPos > nClipX8)) {
				RenderTile[3]();
			} else {
				RenderTile[2]();
			}
		}
	}

	if (count >= (64 * 31)) {
		CaveTileMax[0] -= 0x0123;
		CaveTileMax[1] -= 0x0123;
		CaveTileMax[2] -= 0x0123;
		CaveTileMax[3] -= 0x0123;
	}
	CaveTileCount[nLayer] = count;

	return;
}

static void CaveQueue16x16Layer_Normal(UINT8 nLayer)
{
	INT16 x, y;
	UINT16 bx, by, cx, cy, mx;
	UINT16 nTileColumn, nTileRow;
	UINT32 nTileNumber;

    UINT8 *pTileRAM = CaveTileRAM[nLayer];

	mx = (nCaveXSize >> 4) << 2;

	bx = CaveTileReg[nLayer][0] - 0x12 + nLayerXOffset[nLayer];
	bx &= 0x01FF;
	cx = (bx >> 4) << 2;
	bx &= 0x000F;

	by = CaveTileReg[nLayer][1] + nLayerYOffset;
	by &= 0x01FF;
	cy = (by >> 4) << 7;
	by &= 0x000F;

	for (y = 0; y < (16 << 7); y += (1 << 7)) {
		nTileRow = (cy + y) & (0x1F << 7);
		nTileYPos = (y >> 3) - by;

		if ((nTileYPos <= -16) || (nTileYPos >= nCaveYSize)) {
			continue;
		}

		for (x = mx; x >= 0; x -= (1 << 2)) {
			nTileColumn = (cx + x) & (0x1F << 2);
			nTileNumber  = *((UINT16 *)(pTileRAM + 0x0000 + nTileRow + nTileColumn)) << 16;
			nTileNumber |= *((UINT16 *)(pTileRAM + 0x0002 + nTileRow + nTileColumn));

			if (*((UINT32 *)(CaveTileAttrib[nLayer] + ((nTileNumber << 2) & nTileMask[nLayer]))) == 0x01010101) {
				continue;
			}

			nTileXPos = (x << 2) - bx;

			if ((nTileXPos <= -16) || (nTileXPos >= nCaveXSize)) {
				continue;
			}

			CaveTileQueue[nLayer][nTileNumber >> 30]->x = nTileXPos;
			CaveTileQueue[nLayer][nTileNumber >> 30]->y = nTileYPos;
			CaveTileQueue[nLayer][nTileNumber >> 30]->tile = nTileNumber;
			CaveTileQueue[nLayer][nTileNumber >> 30]++;
		}
	}

	return;
}

static void Cave16x16Layer_Normal(UINT8 nLayer, UINT8 nPriority)
{
	UINT32 nTileNumber, nAttrib;

	UINT32 *pPalette = CavePalette + nPaletteOffset[nLayer];
	UINT8 nPaletteShift = 24 - nPaletteSize[nLayer];
	UINT32 nPaletteMask = 0x3F000000;
	if (nPaletteSize[nLayer] == 6) {
		nPaletteMask = 0x0F000000;
	}

	CaveTile *TileQueue = CaveTileQueue[nLayer][nPriority];

	while ((nTileXPos = TileQueue->x) < 9999) {
		nTileYPos = TileQueue->y;
		nTileNumber = TileQueue->tile;
		pTilePalette = pPalette + ((nTileNumber & nPaletteMask) >> nPaletteShift);
		nTileNumber <<= 2;
		nTileNumber &= nTileMask[nLayer];

		nAttrib = *((UINT32 *)(CaveTileAttrib[nLayer] + nTileNumber));

#ifdef BUILD_PSP
		pTile = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);
#else
		pTile = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);
#endif

		UINT32 *pTileBase = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 6), 256);

		if ((nTileXPos < 0) || (nTileXPos > nClipX16) || (nTileYPos < 0) || (nTileYPos > nClipY16)) {

			if ((nAttrib & 0x000000FF) == 0) {
				if ((nTileXPos > -8) && (nTileXPos < nCaveXSize) && (nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
					pTileData = pTileBase;
					if ((nTileXPos >= 0) && (nTileXPos <= nClipX8) && (nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
						RenderTile[0]();
					} else {
						RenderTile[1]();
					}
				}
			}

			nTileXPos += 8;
			pTile += (nBurnBpp << 3);
			if ((nAttrib & 0x0000FF00) == 0) {
				if ((nTileXPos > -8) && (nTileXPos < nCaveXSize) && (nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
					pTileData = pTileBase + 16;
					if ((nTileXPos >= 0) && (nTileXPos <= nClipX8) && (nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
						RenderTile[0]();
					} else{
						RenderTile[1]();
					}
				}
			}

			nTileXPos -= 8;
			nTileYPos += 8;

#ifdef BUILD_PSP
			pTile = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);
#else
			pTile = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);
#endif
			if ((nAttrib & 0x00FF0000) == 0) {
				if ((nTileXPos > -8) && (nTileXPos < nCaveXSize) && (nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
					pTileData = pTileBase + 32;
					if ((nTileXPos >= 0) && (nTileXPos <= nClipX8) && (nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
						RenderTile[0]();
					} else {
						RenderTile[1]();
					}
				}
			}

			nTileXPos += 8;
			pTile += (nBurnBpp << 3);
			if ((nAttrib & 0xFF000000) == 0) {
				if ((nTileXPos > -8) && (nTileXPos < nCaveXSize) && (nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
					pTileData = pTileBase + 48;
					if ((nTileXPos >= 0) && (nTileXPos <= nClipX8) && (nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
						RenderTile[0]();
					} else {
						RenderTile[1]();
					}
				}
			}

		} else {

			if ((nAttrib & 0x000000FF) == 0) {
				pTileData = pTileBase;
				RenderTile[0]();
			}
			nTileXPos += 8;
			pTile += (nBurnBpp << 3);
			if ((nAttrib & 0x0000FF00) == 0) {
				pTileData = pTileBase + 16;
				RenderTile[0]();
			}
			nTileXPos -= 8;
			nTileYPos += 8;

#ifdef BUILD_PSP
			pTile = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);
#else
			pTile = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);
#endif
			if ((nAttrib & 0x00FF0000) == 0) {
				pTileData = pTileBase + 32;
				RenderTile[0]();
			}
			nTileXPos += 8;
			pTile += (nBurnBpp << 3);
			if ((nAttrib & 0xFF000000) == 0) {
				pTileData = pTileBase + 48;
				RenderTile[0]();
			}
		}

		TileQueue++;
	}

	return;
}

static void Cave16x16Layer_RowScroll(UINT8 nLayer, UINT8 nPriority)
{
	INT16 x, y;
	UINT16 bx, by, cy;
	UINT16 nTileColumn, nTileRow;
	UINT32 nTileNumber, nAttrib;

    UINT8  *pTileRAM = CaveTileRAM[nLayer];
	UINT32 *pPalette = CavePalette + nPaletteOffset[nLayer];
	UINT8 nPaletteShift = 24 - nPaletteSize[nLayer];
	UINT32 nPaletteMask = 0x3F000000;
	if (nPaletteSize[nLayer] == 6) {
		nPaletteMask = 0x0F000000;
	}

	UINT16 *rowscroll = pRowScroll[nLayer];

	UINT16 count = CaveTileCount[nLayer];

	if (count >= (32 * 16)) {
		return;
	}

	bx = CaveTileReg[nLayer][0] - 0x12 + nLayerXOffset[nLayer];
	bx &= 0x01FF;

	by = CaveTileReg[nLayer][1] + nLayerYOffset;
	by &= 0x01FF;
	cy = (by >> 4) << 7;
	by &= 0x000F;

	if (nPriority == 0) {
		INT16 dy = CaveTileReg[nLayer][1] + 0x12 - nCaveRowModeOffset;

		for (y = 0; y < nCaveYSize; y++) {
			rowscroll[y] = *((UINT16 *)(pTileRAM + 0x1000 + (((dy + y) & 0x01FF) << 2))) + bx;
		}
	}

	for (y = 0; y < (16 << 7); y += (1 << 7)) {
		nTileRow = (cy + y) & (0x1F << 7);
		nTileYPos = (y >> 3) - by;

		pTileRowInfo = rowscroll + nTileYPos;

		for (x = 0; x < (32 << 2); x += (1 << 2)) {
			nTileColumn = x & (0x1F << 2);
			nTileNumber = *((UINT16 *)(pTileRAM + 0x0000 + nTileRow + nTileColumn)) << 16;
			if ((nTileNumber >> 30) != nPriority) {
				continue;
			}
			nTileNumber |= *((UINT16 *)(pTileRAM + 0x0002 + nTileRow + nTileColumn));
			pTilePalette = pPalette + ((nTileNumber & nPaletteMask) >> nPaletteShift);
			nTileNumber <<= 2;
			nTileNumber &= nTileMask[nLayer];

			count++;

			nAttrib = *((UINT32 *)(CaveTileAttrib[nLayer] + nTileNumber));
			if (nAttrib == 0x01010101) {
				continue;
			}

			nTileXPos = (x << 2);

#ifdef BUILD_PSP
			pTile = pBurnDraw + (nTileYPos << 10);
#else
			pTile = pBurnDraw + (nTileYPos * nBurnPitch);
#endif
			UINT32 *pTileBase = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 6), 256);

			if ((nTileYPos < 0) || (nTileYPos > nClipY16)) {

				if ((nAttrib & 0x000000FF) == 0) {
					if ((nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
						pTileData = pTileBase;
						if ((nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
							RenderTile[2]();
						} else {
							RenderTile[3]();
						}
					}
				}
				nTileXPos += 8;
				if ((nAttrib & 0x0000FF00) == 0) {
					if ((nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
						pTileData = pTileBase + 16;
						if ((nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
							RenderTile[2]();
						} else {
							RenderTile[3]();
						}
					}
				}
				nTileXPos -= 8;
				nTileYPos += 8;

    			pTileRowInfo += 8;

#ifdef BUILD_PSP
				pTile = pBurnDraw + (nTileYPos << 10);
#else
				pTile = pBurnDraw + (nTileYPos * nBurnPitch);
#endif
				if ((nAttrib & 0x00FF0000) == 0) {
					if ((nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
						pTileData = pTileBase + 32;
						if ((nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
							RenderTile[2]();
						} else {
							RenderTile[3]();
						}
					}
				}
				nTileXPos += 8;
				if ((nAttrib & 0xFF000000) == 0) {
					if ((nTileYPos > -8) && (nTileYPos < nCaveYSize)) {
						pTileData = pTileBase + 48;
						if ((nTileYPos >= 0) && (nTileYPos <= nClipY8)) {
							RenderTile[2]();
						} else {
							RenderTile[3]();
						}
					}
				}
			} else {

				if ((nAttrib & 0x000000FF) == 0) {
					pTileData = pTileBase;
					RenderTile[2]();
				}
				nTileXPos += 8;
				if ((nAttrib & 0x0000FF00) == 0) {
					pTileData = pTileBase + 16;
					RenderTile[2]();
				}
				nTileXPos -= 8;
				nTileYPos += 8;

    			pTileRowInfo += 8;

#ifdef BUILD_PSP
				pTile = pBurnDraw + (nTileYPos << 10);
#else
				pTile = pBurnDraw + (nTileYPos * nBurnPitch);
#endif
				if ((nAttrib & 0x00FF0000) == 0) {
					pTileData = pTileBase + 32;
					RenderTile[2]();
				}
				nTileXPos += 8;
				if ((nAttrib & 0xFF000000) == 0) {
					pTileData = pTileBase + 48;
					RenderTile[2]();
				}
			}
			nTileYPos -= 8;

			pTileRowInfo -= 8;
		}
	}

	if (count >= (32 * 16)) {
		CaveTileMax[0] -= 0x0123;
		CaveTileMax[1] -= 0x0123;
		CaveTileMax[2] -= 0x0123;
		CaveTileMax[3] -= 0x0123;
	}
	CaveTileCount[nLayer] = count;

	return;
}

static void Cave16x16Layer_RowSelect(UINT8 nLayer, UINT8 nPriority)
{
	INT16 x, y, ry, rx;
	UINT16 bx, by, mx;
	UINT16 nTileColumn, nTileRow;
	UINT32 nTileNumber;

    UINT8  *pTileRAM = CaveTileRAM[nLayer];
	UINT32 *pPalette = CavePalette + nPaletteOffset[nLayer];
	UINT8 nPaletteShift = 24 - nPaletteSize[nLayer];
	UINT32 nPaletteMask = 0x3F000000;
	if (nPaletteSize[nLayer] == 6) {
		nPaletteMask = 0x0F000000;
	}

	UINT16 *rowselect = pRowSelect[nLayer];
	UINT16 *rowscroll = pRowScroll[nLayer];

	UINT16 count = CaveTileCount[nLayer];

	mx = nCaveXSize >> 4;

	if (count >= (nCaveYSize * (mx + 1))) {
		return;
	}

	bx = CaveTileReg[nLayer][0] - 0x12 + nLayerXOffset[nLayer];
	bx &= 0x01FF;

	by = CaveTileReg[nLayer][1] + nLayerYOffset;
	by &= 0x01FF;

	if (nPriority == 0) {
		INT16 cy = CaveTileReg[nLayer][1] + 0x12 - nCaveRowModeOffset;

		if (CaveTileReg[nLayer][0] & 0x4000) {	// Row-scroll mode
			for (y = 0; y < nCaveYSize; y++) {
				rowselect[y] = *((UINT16 *)(pTileRAM + 0x1002 + (((cy + y) & 0x01FF) << 2)));
				rowscroll[y] = *((UINT16 *)(pTileRAM + 0x1000 + (((cy + y) & 0x01FF) << 2))) + bx;
			}
		} else {
			for (y = 0; y < nCaveYSize; y++) {
				rowselect[y] = *((UINT16 *)(pTileRAM + 0x1002 + (((cy + y) & 0x01FF) << 2)));
			}
		}
	}

	for (y = 0; y < nCaveYSize; y++) {
		ry = rowselect[y];
		nTileRow = ((ry >> 4) & 0x1F) << 7;
		nTileYPos = y;

		if (CaveTileReg[nLayer][0] & 0x4000) {	// Row-scroll mode
			rx = rowscroll[y];
			ry = (((ry & 8) << 1) + (ry & 7)) << 1;

			for (x = 0; x <= mx; x++) {

				nTileColumn = (((rx >> 4) + x) & 0x1F) << 2;
				nTileNumber = *((UINT16 *)(pTileRAM + 0x0000 + nTileRow + nTileColumn)) << 16;
				if ((nTileNumber >> 30) != nPriority) {
					continue;
				}
				nTileNumber |= *((UINT16 *)(pTileRAM + 0x0002 + nTileRow + nTileColumn));
				pTilePalette = pPalette + ((nTileNumber & nPaletteMask) >> nPaletteShift);
				nTileNumber &= nTileMask[nLayer];

				count++;

				if (((UINT32 *)CaveTileAttrib[nLayer])[nTileNumber] == 0x01010101) {
					continue;
				}

				nTileXPos = (x << 4) - (rx & 15);

				if ((nTileXPos <= -16) || (nTileXPos >= nCaveXSize)) {
					continue;
				}

#ifdef BUILD_PSP
				pTile = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);
#else
				pTile = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);
#endif

				UINT32 *pTileBase = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 8) + (ry << 2), 128);

				pTileData = pTileBase;
				if ((nTileXPos >= 0) && (nTileXPos <= nClipX8)) {
					RenderTile[4]();
				} else {
					RenderTile[5]();
				}
				nTileXPos += 8;
				pTile += (nBurnBpp << 3);

				pTileData = pTileBase + 16;
				if ((nTileXPos >= 0) && (nTileXPos <= nClipX16)) {
					RenderTile[4]();
				} else {
					RenderTile[5]();
				}
			}
		} else {
			nTileXPos = - (bx & 15);

#ifdef BUILD_PSP
			pTile = pBurnDraw + (nTileYPos << 10) - ((bx & 15) << 1);
#else
			pTile = pBurnDraw + (nTileYPos * nBurnPitch) - ((bx & 15) * nBurnBpp);
#endif
			ry = (((ry & 8) << 1) + (ry & 7)) << 1;

			for (x = 0; x <= mx; x++) {
				nTileColumn = (((bx >> 4) + x) & 0x1F) << 2;
				nTileNumber = *((UINT16 *)(pTileRAM + 0x0000 + nTileRow + nTileColumn)) << 16;
				if ((nTileNumber >> 30) != nPriority) {
					nTileXPos += 16;
					pTile += (nBurnBpp << 4);
					continue;
				}
				nTileNumber |= *((UINT16 *)(pTileRAM + 0x0002 + nTileRow + nTileColumn));
				pTilePalette = pPalette + ((nTileNumber & nPaletteMask) >> nPaletteShift);
				nTileNumber &= nTileMask[nLayer];

				count++;

				if (((UINT32 *)CaveTileAttrib[nLayer])[nTileNumber] == 0x01010101) {
					nTileXPos += 16;
					pTile += (nBurnBpp << 4);
					continue;
				}

				UINT32 *pTileBase = (UINT32 *)getBlock(CaveTileROMOffset[nLayer] + (nTileNumber << 8) + (ry << 2), 128);

				pTileData = pTileBase;
				if ((nTileXPos >= 0) && (nTileXPos <= nClipX8)) {
					RenderTile[4]();
				} else {
					RenderTile[5]();
				}
				nTileXPos += 8;
				pTile += (nBurnBpp << 3);

				pTileData = pTileBase + 16;
				if ((nTileXPos >= 0) && (nTileXPos <= nClipX8)) {
					RenderTile[4]();
				} else {
					RenderTile[5]();
				}
				nTileXPos += 8;
				pTile += (nBurnBpp << 3);
			}
		}
	}

	if (count >= (nCaveYSize * (mx + 1))) {
		CaveTileMax[0] -= 0x0123;
		CaveTileMax[1] -= 0x0123;
		CaveTileMax[2] -= 0x0123;
		CaveTileMax[3] -= 0x0123;
	}
	CaveTileCount[nLayer] = count;

	return;
}

static void Cave16x16Layer(UINT8 nLayer, UINT8 nPriority)
{
	if (CaveTileReg[nLayer][1] & 0x4000) {			// Row-select mode
		Cave16x16Layer_RowSelect(nLayer, nPriority);
//		bprintf(PRINT_NORMAL, "   Layer %d row-select mode enabled (16x16)\n", nLayer);
		return;
	}

	if (CaveTileReg[nLayer][0] & 0x4000) {			// Row-scroll mode
		Cave16x16Layer_RowScroll(nLayer, nPriority);
//		bprintf(PRINT_NORMAL, "   Layer %d row-scroll mode enabled (16x16)\n", nLayer);
		return;
	}

	Cave16x16Layer_Normal(nLayer, nPriority);

	return;
}

static void Cave8x8Layer(UINT8 nLayer, UINT8 nPriority)
{
#if 0
	if (CaveTileReg[nLayer][1] & 0x4000) {			// Row-select mode
		Cave8x8Layer_RowSelect(nLayer, nPriority);
//		bprintf(PRINT_NORMAL, "   Layer %d row-select mode enabled ( 8x 8)\n", nLayer);
		return;
	}
#else
	if (CaveTileReg[nLayer][1] & 0x4000) {
//		bprintf(PRINT_ERROR, "   Layer %d row-select mode enabled ( 8x 8, UNSUPPORTED!)\n", nLayer);
	}
#endif

	if (CaveTileReg[nLayer][0] & 0x4000) {			// Row-scroll mode
		Cave8x8Layer_RowScroll(nLayer, nPriority);
//		bprintf(PRINT_NORMAL, "   Layer %d row-scroll mode enabled ( 8x 8)\n", nLayer);
		return;
	}

	Cave8x8Layer_Normal(nLayer, nPriority);

	return;
}

INT8 CaveTileRender(UINT8 nMode)
{
	UINT8 nPriority;
	UINT8 nLowPriority;
	UINT8 nLayer;
//	UINT8 nOffset = 0;

	CaveTileCount[0] = CaveTileCount[1] = CaveTileCount[2] = CaveTileCount[3] = 0;
	CaveTileMax[0] = CaveTileMax[1] = CaveTileMax[2] = CaveTileMax[3] = 0;

	nLayerYOffset = 0x12 - nCaveYOffset - nCaveExtraYOffset;

	for (nLayer = 0; nLayer < 4; nLayer++) {

		nLayerXOffset[nLayer] = nLayer /* + nOffset */ - nCaveXOffset - nCaveExtraXOffset;
		
		if ((CaveTileReg[nLayer][2] & 0x0010) == 0) {

			for (nPriority = 0; nPriority < 4; nPriority++) {
				CaveTileQueue[nLayer][nPriority] = &CaveTileQueueMemory[nLayer][nPriority * 1536];
			}
			if (nBurnLayer & (8 >> nLayer)) {
				if (CaveTileReg[nLayer][1] & 0x2000) {
					if (nMode && ((CaveTileReg[nLayer][0] | CaveTileReg[nLayer][1]) & 0x4000)) {
						CaveTileMax[0] += 0x0123;
						CaveTileMax[1] += 0x0123;
						CaveTileMax[2] += 0x0123;
						CaveTileMax[3] += 0x0123;
					} else {
						CaveQueue16x16Layer_Normal(nLayer);
					}
				} else {
					if (nMode && (CaveTileReg[nLayer][0] & 0x4000)) {
						CaveTileMax[0] += 0x0123;
						CaveTileMax[1] += 0x0123;
						CaveTileMax[2] += 0x0123;
						CaveTileMax[3] += 0x0123;
					} else {
						CaveQueue8x8Layer_Normal(nLayer);
					}

//					nOffset += 8;
				}
			}

			for (nPriority = 0; nPriority < 4; nPriority++) {
				CaveTileQueue[nLayer][nPriority]->x = 9999;
				CaveTileQueue[nLayer][nPriority] = &CaveTileQueueMemory[nLayer][nPriority * 1536];
				if (CaveTileQueue[nLayer][nPriority]->x < 9999) {
					CaveTileMax[nPriority] += 0x123;
				}
			}
		}
	}

	nLowPriority = 0;

	for (nPriority = 0; nPriority < 4; nPriority++) {

		if (CaveTileMax[nPriority] || (nPriority == 3)) {
			CaveSpriteRender(nLowPriority, nPriority);
			nLowPriority = nPriority + 1;
		}

		for (INT8 i = 0; i < 4; i++) {
			for (nLayer = 0; nLayer < 4; nLayer++) {
				if ((CaveTileReg[nLayer][2] & 0x0003) == i) {
					if ((CaveTileReg[nLayer][2] & 0x0010) || ((nBurnLayer & (8 >> nLayer)) == 0)) {
						continue;
					}

					if (CaveTileReg[nLayer][1] & 0x2000) {
						if (nMode) {
							Cave16x16Layer(nLayer, nPriority);
						} else {
							Cave16x16Layer_Normal(nLayer, nPriority);
						}
					} else {
						if (nMode) {
							Cave8x8Layer(nLayer, nPriority);
						} else {
							Cave8x8Layer_Normal(nLayer, nPriority);
						}
					}
				}
			}
		}
	}

	return 0;
}

void CaveTileExit()
{
	for (INT8 nLayer = 0; nLayer < 4; nLayer++) {
		free(CaveTileAttrib[nLayer]);
		CaveTileAttrib[nLayer] = NULL;

		free(CaveTileQueueMemory[nLayer]);
		CaveTileQueueMemory[nLayer] = NULL;

		free(pRowScroll[nLayer]);
		pRowScroll[nLayer] = NULL;
		free(pRowSelect[nLayer]);
		pRowSelect[nLayer] = NULL;
	}

	nCaveXOffset = nCaveYOffset = 0;
	nCaveRowModeOffset = 0;
	nCaveExtraXOffset = nCaveExtraYOffset = 0;

	return;
}

INT8 CaveTileInit()
{
	for (INT8 nLayer = 0; nLayer < 4; nLayer++) {
		CaveTileReg[nLayer][0] = 0x0000;
		CaveTileReg[nLayer][1] = 0x0000;
		CaveTileReg[nLayer][2] = 0x0010;							// Disable layer
	}

	nCaveTileBank = 0;

	BurnDrvGetFullSize(&nCaveXSize, &nCaveYSize);

	nClipX8  = nCaveXSize -  8;
	nClipX16 = nCaveXSize - 16;
	nClipY8  = nCaveYSize -  8;
	nClipY16 = nCaveYSize - 16;

	RenderTile = RenderTile_ROT0[(nCaveXSize == 320) ? 0 : 1];

	return 0;
}

#define nTileSize 64

INT8 CaveTileInitLayer(UINT8 nLayer, UINT32 nROMSize, UINT8 nBitdepth, UINT16 nOffset)
{
	UINT32 nNumTiles = nROMSize / nTileSize;
	UINT8 *tileRom;

	for (nTileMask[nLayer] = 1; nTileMask[nLayer] < nNumTiles; nTileMask[nLayer] <<= 1) { }
	nTileMask[nLayer]--;

	free(CaveTileAttrib[nLayer]);
	CaveTileAttrib[nLayer] = (INT8 *)memalign(4, nTileMask[nLayer] + 1);
	if (CaveTileAttrib[nLayer] == NULL) {
		return 1;
	}
	memset(CaveTileAttrib[nLayer], 0, nTileMask[nLayer] + 1);

	for (INT32 i = 0; i < nNumTiles; i++) {
		bool bTransparent = true;
		for (INT32 j = i * nTileSize; j < (i + 1) * nTileSize; j++) {
			tileRom = getBlockSmallData(CaveTileROMOffset[nLayer] + j);
			if ((tileRom != 0) && ((*tileRom) != 0)) {
				bTransparent = false;
				break;
			}
		}
		if (bTransparent) {
			CaveTileAttrib[nLayer][i] = 1;
		} else {
			CaveTileAttrib[nLayer][i] = 0;
		}
	}

	for (INT32 i = nNumTiles; i <= nTileMask[nLayer]; i++) {
		CaveTileAttrib[nLayer][i] = 1;
	}

	free(CaveTileQueueMemory[nLayer]);
	CaveTileQueueMemory[nLayer] = (CaveTile *)memalign(4, 4 * 1536 * sizeof(CaveTile));
	if (CaveTileQueueMemory[nLayer] == NULL) {
		return 1;
	}
	memset(CaveTileQueueMemory[nLayer], 0, 4 * 1536 * sizeof(CaveTile));

	free(pRowScroll[nLayer]);
	pRowScroll[nLayer] = (UINT16 *)memalign(4, nCaveYSize * sizeof(UINT16));
	if (pRowScroll[nLayer] == NULL) {
		return 1;
	}
	memset(pRowScroll[nLayer], 0, nCaveYSize * sizeof(UINT16));

	free(pRowSelect[nLayer]);
	pRowSelect[nLayer] = (UINT16 *)memalign(4, nCaveYSize * sizeof(UINT16));
	if (pRowSelect[nLayer] == NULL) {
		return 1;
	}
	memset(pRowSelect[nLayer], 0, nCaveYSize * sizeof(UINT16));

	nPaletteSize[nLayer] = nBitdepth;
	nPaletteOffset[nLayer] = nOffset;

	CaveTileReg[nLayer][2] = 0x0000;							// Enable layer

	return 0;
}

#undef nTileSize

