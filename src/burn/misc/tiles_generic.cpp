/*================================================================================================
Generic Tile Rendering Module - Uses the Colour-Depth Independent Image Transfer Method

Supports 8 x 8, 16 x 16 and 32 x 32 with or without masking and with full flipping. The functions fully
support varying colour-depths and palette offsets as well as all the usual variables.

Call GenericTilesInit() in the driver Init function to store the drivers screen size for clipping.
This function also calls BurnTransferInit().

Call GenericTilesExit() in the driver Exit function to clear the screen size variables.
Again, this function also calls BurnTransferExit().

Otherwise, use the Transfer code as usual.
================================================================================================*/

#include "tiles_generic.h"

unsigned char* pTileData;
unsigned short nScreenWidth, nScreenHeight;

int GenericTilesInit()
{
	int nRet;

	if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
		BurnDrvGetVisibleSize(&nScreenHeight, &nScreenWidth);
	} else {
		BurnDrvGetVisibleSize(&nScreenWidth, &nScreenHeight);
	}

	nRet = BurnTransferInit();

	return nRet;
}

int GenericTilesExit()
{
	nScreenWidth = nScreenHeight = 0;
	BurnTransferExit();

	return 0;
}

/*================================================================================================
Graphics Decoding
================================================================================================*/

inline static int readbit(const UINT8 *src, int bitnum)
{
	return src[bitnum / 8] & (0x80 >> (bitnum % 8));
}

void GfxDecode(int num, int numPlanes, int xSize, int ySize, int planeoffsets[], int xoffsets[], int yoffsets[], int modulo, unsigned char *pSrc, unsigned char *pDest)
{
	int c;
	
	for (c = 0; c < num; c++) {
		int plane, x, y;
	
		UINT8 *dp = pDest + (c * xSize * ySize);
		memset(dp, 0, xSize * ySize);
	
		for (plane = 0; plane < numPlanes; plane++) {
			int planebit = 1 << (numPlanes - 1 - plane);
			int planeoffs = (c * modulo) + planeoffsets[plane];
		
			for (y = 0; y < ySize; y++) {
				int yoffs = planeoffs + yoffsets[y];
				dp = pDest + (c * xSize * ySize) + (y * xSize);
			
				for (x = 0; x < xSize; x++) {
					if (readbit(pSrc, yoffs + xoffsets[x])) dp[x] |= planebit;
				}
			}
		}
	}	
}

#ifdef BUILD_PSP

void DecodeGfxRom(int num, int numPlanes, int xSize, int ySize, int planeoffsets[], int xoffsets[], int yoffsets[], int modulo, unsigned char *pSrc, unsigned char *pDest)
{
	extern void ui_update_progress2(float size, const char * txt);
	
	int c;
	
	for (c = 0; c < num; c++) {
		int plane, x, y;
	
		UINT8 *dp = pDest + (c * xSize * ySize);
		memset(dp, 0, xSize * ySize);
	
		if((c % (num / 16)) == 0) {
			ui_update_progress2(1.0 / 16, c ? NULL : "decoding graphics data..." );
		}
		
		for (plane = 0; plane < numPlanes; plane++) {
			int planebit = 1 << (numPlanes - 1 - plane);
			int planeoffs = (c * modulo) + planeoffsets[plane];
		
			for (y = 0; y < ySize; y++) {
				int yoffs = planeoffs + yoffsets[y];
				dp = pDest + (c * xSize * ySize) + (y * xSize);
			
				for (x = 0; x < xSize; x++) {
					if (readbit(pSrc, yoffs + xoffsets[x])) dp[x] |= planebit;
				}
			}
		}
	}	
}
#else
	#define DecodeGfxRom(num, numPlanes, xSize, ySize, planeoffsets, xoffsets, yoffsets, modulo, pSrc, pDest) \
			GfxDecode(num, numPlanes, xSize, ySize, planeoffsets, xoffsets, yoffsets, modulo, pSrc, pDest)
#endif

//================================================================================================

#define PLOTPIXEL(x) pPixel[x] = nPalette | pTileData[x];
#define PLOTPIXEL_FLIPX(x, a) pPixel[x] = nPalette | pTileData[a];
#define PLOTPIXEL_MASK(x, mc) if (pTileData[x] != mc) {pPixel[x] = nPalette | pTileData[x];}
#define PLOTPIXEL_MASK_FLIPX(x, a, mc) if (pTileData[a] != mc) {pPixel[x] = nPalette | pTileData[a] ;}
#define CLIPPIXEL(x, sx, mx, a) if ((sx + x) >= 0 && (sx + x) < mx) { a; };

/*================================================================================================
8 x 8 Functions
================================================================================================*/

void Render8x8Tile(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth ) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		PLOTPIXEL(0);
		PLOTPIXEL(1);
		PLOTPIXEL(2);
		PLOTPIXEL(3);
		PLOTPIXEL(4);
		PLOTPIXEL(5);
		PLOTPIXEL(6);
		PLOTPIXEL(7);
	}
}

void Render8x8Tile_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL(0));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL(1));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL(2));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL(3));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL(4));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL(5));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL(6));
		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL(7));
	}
}

void Render8x8Tile_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		PLOTPIXEL_FLIPX(7, 0);
		PLOTPIXEL_FLIPX(6, 1);
		PLOTPIXEL_FLIPX(5, 2);
		PLOTPIXEL_FLIPX(4, 3);
		PLOTPIXEL_FLIPX(3, 4);
		PLOTPIXEL_FLIPX(2, 5);
		PLOTPIXEL_FLIPX(1, 6);
		PLOTPIXEL_FLIPX(0, 7);
	}
}

void Render8x8Tile_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_FLIPX(7, 0));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_FLIPX(6, 1));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_FLIPX(5, 2));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_FLIPX(4, 3));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_FLIPX(3, 4));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_FLIPX(2, 5));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_FLIPX(1, 6));
		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_FLIPX(0, 7));
	}
}

void Render8x8Tile_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		PLOTPIXEL(0);
		PLOTPIXEL(1);
		PLOTPIXEL(2);
		PLOTPIXEL(3);
		PLOTPIXEL(4);
		PLOTPIXEL(5);
		PLOTPIXEL(6);
		PLOTPIXEL(7);
	}
}

void Render8x8Tile_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL(0));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL(1));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL(2));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL(3));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL(4));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL(5));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL(6));
		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL(7));
	}
}

void Render8x8Tile_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		PLOTPIXEL_FLIPX(7, 0);
		PLOTPIXEL_FLIPX(6, 1);
		PLOTPIXEL_FLIPX(5, 2);
		PLOTPIXEL_FLIPX(4, 3);
		PLOTPIXEL_FLIPX(3, 4);
		PLOTPIXEL_FLIPX(2, 5);
		PLOTPIXEL_FLIPX(1, 6);
		PLOTPIXEL_FLIPX(0, 7);
	}
}

void Render8x8Tile_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_FLIPX(7, 0));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_FLIPX(6, 1));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_FLIPX(5, 2));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_FLIPX(4, 3));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_FLIPX(3, 4));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_FLIPX(2, 5));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_FLIPX(1, 6));
		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_FLIPX(0, 7));
	}
}

/*================================================================================================
8 x 8 Functions with Masking
================================================================================================*/

void Render8x8Tile_Mask(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		PLOTPIXEL_MASK(0, nMaskColour);
		PLOTPIXEL_MASK(1, nMaskColour);
		PLOTPIXEL_MASK(2, nMaskColour);
		PLOTPIXEL_MASK(3, nMaskColour);
		PLOTPIXEL_MASK(4, nMaskColour);
		PLOTPIXEL_MASK(5, nMaskColour);
		PLOTPIXEL_MASK(6, nMaskColour);
		PLOTPIXEL_MASK(7, nMaskColour);
	}
}

void Render8x8Tile_Mask_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_MASK(0, nMaskColour));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_MASK(1, nMaskColour));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_MASK(2, nMaskColour));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_MASK(3, nMaskColour));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_MASK(4, nMaskColour));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_MASK(5, nMaskColour));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_MASK(6, nMaskColour));
		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_MASK(7, nMaskColour));
	}
}

void Render8x8Tile_Mask_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		PLOTPIXEL_MASK_FLIPX(7, 0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(6, 1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(5, 2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(4, 3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(3, 4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(2, 5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(1, 6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(0, 7, nMaskColour);
	}
}

void Render8x8Tile_Mask_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 8; y++, pPixel += nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(7, 0, nMaskColour));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(6, 1, nMaskColour));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(5, 2, nMaskColour));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(4, 3, nMaskColour));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(3, 4, nMaskColour));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(2, 5, nMaskColour));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(1, 6, nMaskColour));
		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(0, 7, nMaskColour));
	}
}

void Render8x8Tile_Mask_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		PLOTPIXEL_MASK(0, nMaskColour);
		PLOTPIXEL_MASK(1, nMaskColour);
		PLOTPIXEL_MASK(2, nMaskColour);
		PLOTPIXEL_MASK(3, nMaskColour);
		PLOTPIXEL_MASK(4, nMaskColour);
		PLOTPIXEL_MASK(5, nMaskColour);
		PLOTPIXEL_MASK(6, nMaskColour);
		PLOTPIXEL_MASK(7, nMaskColour);
	}
}

void Render8x8Tile_Mask_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_MASK(0, nMaskColour));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_MASK(1, nMaskColour));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_MASK(2, nMaskColour));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_MASK(3, nMaskColour));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_MASK(4, nMaskColour));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_MASK(5, nMaskColour));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_MASK(6, nMaskColour));
		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_MASK(7, nMaskColour));
	}
}

void Render8x8Tile_Mask_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		PLOTPIXEL_MASK_FLIPX(7, 0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(6, 1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(5, 2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(4, 3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(3, 4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(2, 5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(1, 6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(0, 7, nMaskColour);
	}
}

void Render8x8Tile_Mask_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 6);

	unsigned short* pPixel = pDestDraw + ((StartY + 7) * nScreenWidth) + StartX;

	for (int y = 7; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 8) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(7, 0, nMaskColour));
		CLIPPIXEL(6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(6, 1, nMaskColour));
		CLIPPIXEL(5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(5, 2, nMaskColour));
		CLIPPIXEL(4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(4, 3, nMaskColour));
		CLIPPIXEL(3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(3, 4, nMaskColour));
		CLIPPIXEL(2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(2, 5, nMaskColour));
		CLIPPIXEL(1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(1, 6, nMaskColour));
		CLIPPIXEL(0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(0, 7, nMaskColour));
	}
}

/*================================================================================================
16 x 16 Functions
================================================================================================*/

void Render16x16Tile(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		PLOTPIXEL( 0);
		PLOTPIXEL( 1);
		PLOTPIXEL( 2);
		PLOTPIXEL( 3);
		PLOTPIXEL( 4);
		PLOTPIXEL( 5);
		PLOTPIXEL( 6);
		PLOTPIXEL( 7);
		PLOTPIXEL( 8);
		PLOTPIXEL( 9);
		PLOTPIXEL(10);
		PLOTPIXEL(11);
		PLOTPIXEL(12);
		PLOTPIXEL(13);
		PLOTPIXEL(14);
		PLOTPIXEL(15);
	}
}

void Render16x16Tile_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}
		
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL( 0));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL( 1));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL( 2));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL( 3));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL( 4));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL( 5));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL( 6));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL( 7));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL( 8));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL( 9));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL(10));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL(11));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL(12));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL(13));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL(14));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL(15));
	}
}

void Render16x16Tile_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		PLOTPIXEL_FLIPX(15,  0);
		PLOTPIXEL_FLIPX(14,  1);
		PLOTPIXEL_FLIPX(13,  2);
		PLOTPIXEL_FLIPX(12,  3);
		PLOTPIXEL_FLIPX(11,  4);
		PLOTPIXEL_FLIPX(10,  5);
		PLOTPIXEL_FLIPX( 9,  6);
		PLOTPIXEL_FLIPX( 8,  7);
		PLOTPIXEL_FLIPX( 7,  8);
		PLOTPIXEL_FLIPX( 6,  9);
		PLOTPIXEL_FLIPX( 5, 10);
		PLOTPIXEL_FLIPX( 4, 11);
		PLOTPIXEL_FLIPX( 3, 12);
		PLOTPIXEL_FLIPX( 2, 13);
		PLOTPIXEL_FLIPX( 1, 14);
		PLOTPIXEL_FLIPX( 0, 15);
	}
}

void Render16x16Tile_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_FLIPX(15,  0));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_FLIPX(14,  1));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_FLIPX(13,  2));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_FLIPX(12,  3));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_FLIPX(11,  4));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_FLIPX(10,  5));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 9,  6));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 8,  7));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 7,  8));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 6,  9));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 5, 10));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 4, 11));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 3, 12));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 2, 13));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 1, 14));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 0, 15));
	}
}

void Render16x16Tile_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		PLOTPIXEL( 0);
		PLOTPIXEL( 1);
		PLOTPIXEL( 2);
		PLOTPIXEL( 3);
		PLOTPIXEL( 4);
		PLOTPIXEL( 5);
		PLOTPIXEL( 6);
		PLOTPIXEL( 7);
		PLOTPIXEL( 8);
		PLOTPIXEL( 9);
		PLOTPIXEL(10);
		PLOTPIXEL(11);
		PLOTPIXEL(12);
		PLOTPIXEL(13);
		PLOTPIXEL(14);
		PLOTPIXEL(15);
	}
}

void Render16x16Tile_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL( 0));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL( 1));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL( 2));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL( 3));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL( 4));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL( 5));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL( 6));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL( 7));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL( 8));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL( 9));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL(10));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL(11));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL(12));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL(13));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL(14));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL(15));
	}
}

void Render16x16Tile_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		PLOTPIXEL_FLIPX(15,  0);
		PLOTPIXEL_FLIPX(14,  1);
		PLOTPIXEL_FLIPX(13,  2);
		PLOTPIXEL_FLIPX(12,  3);
		PLOTPIXEL_FLIPX(11,  4);
		PLOTPIXEL_FLIPX(10,  5);
		PLOTPIXEL_FLIPX( 9,  6);
		PLOTPIXEL_FLIPX( 8,  7);
		PLOTPIXEL_FLIPX( 7,  8);
		PLOTPIXEL_FLIPX( 6,  9);
		PLOTPIXEL_FLIPX( 5, 10);
		PLOTPIXEL_FLIPX( 4, 11);
		PLOTPIXEL_FLIPX( 3, 12);
		PLOTPIXEL_FLIPX( 2, 13);
		PLOTPIXEL_FLIPX( 1, 14);
		PLOTPIXEL_FLIPX( 0, 15);
	}
}

void Render16x16Tile_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_FLIPX(15,  0));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_FLIPX(14,  1));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_FLIPX(13,  2));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_FLIPX(12,  3));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_FLIPX(11,  4));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_FLIPX(10,  5));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 9,  6));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 8,  7));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 7,  8));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 6,  9));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 5, 10));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 4, 11));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 3, 12));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 2, 13));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 1, 14));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 0, 15));
	}
}

/*================================================================================================
16 x 16 Functions with Masking
================================================================================================*/

void Render16x16Tile_Mask(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);
	
	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		PLOTPIXEL_MASK( 0, nMaskColour);
		PLOTPIXEL_MASK( 1, nMaskColour);
		PLOTPIXEL_MASK( 2, nMaskColour);
		PLOTPIXEL_MASK( 3, nMaskColour);
		PLOTPIXEL_MASK( 4, nMaskColour);
		PLOTPIXEL_MASK( 5, nMaskColour);
		PLOTPIXEL_MASK( 6, nMaskColour);
		PLOTPIXEL_MASK( 7, nMaskColour);
		PLOTPIXEL_MASK( 8, nMaskColour);
		PLOTPIXEL_MASK( 9, nMaskColour);
		PLOTPIXEL_MASK(10, nMaskColour);
		PLOTPIXEL_MASK(11, nMaskColour);
		PLOTPIXEL_MASK(12, nMaskColour);
		PLOTPIXEL_MASK(13, nMaskColour);
		PLOTPIXEL_MASK(14, nMaskColour);
		PLOTPIXEL_MASK(15, nMaskColour);
	}
}

void Render16x16Tile_Mask_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK( 0, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK( 1, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK( 2, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK( 3, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK( 4, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK( 5, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK( 6, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK( 7, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK( 8, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK( 9, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK(10, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK(11, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK(12, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK(13, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK(14, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK(15, nMaskColour));
	}
}

void Render16x16Tile_Mask_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		PLOTPIXEL_MASK_FLIPX(15,  0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(14,  1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(13,  2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(12,  3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(11,  4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(10,  5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 9,  6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 8,  7, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 7,  8, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 6,  9, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 5, 10, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 4, 11, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 3, 12, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 2, 13, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 1, 14, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 0, 15, nMaskColour);
	}
}

void Render16x16Tile_Mask_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 16; y++, pPixel += nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(15,  0, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(14,  1, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(13,  2, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(12,  3, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(11,  4, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(10,  5, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 9,  6, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 8,  7, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 7,  8, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 6,  9, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 5, 10, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 4, 11, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 3, 12, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 2, 13, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 1, 14, nMaskColour));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 0, 15, nMaskColour));
	}
}

void Render16x16Tile_Mask_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		PLOTPIXEL_MASK( 0, nMaskColour);
		PLOTPIXEL_MASK( 1, nMaskColour);
		PLOTPIXEL_MASK( 2, nMaskColour);
		PLOTPIXEL_MASK( 3, nMaskColour);
		PLOTPIXEL_MASK( 4, nMaskColour);
		PLOTPIXEL_MASK( 5, nMaskColour);
		PLOTPIXEL_MASK( 6, nMaskColour);
		PLOTPIXEL_MASK( 7, nMaskColour);
		PLOTPIXEL_MASK( 8, nMaskColour);
		PLOTPIXEL_MASK( 9, nMaskColour);
		PLOTPIXEL_MASK(10, nMaskColour);
		PLOTPIXEL_MASK(11, nMaskColour);
		PLOTPIXEL_MASK(12, nMaskColour);
		PLOTPIXEL_MASK(13, nMaskColour);
		PLOTPIXEL_MASK(14, nMaskColour);
		PLOTPIXEL_MASK(15, nMaskColour);
	}
}

void Render16x16Tile_Mask_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK( 0, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK( 1, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK( 2, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK( 3, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK( 4, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK( 5, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK( 6, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK( 7, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK( 8, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK( 9, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK(10, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK(11, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK(12, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK(13, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK(14, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK(15, nMaskColour));
	}
}

void Render16x16Tile_Mask_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		PLOTPIXEL_MASK_FLIPX(15,  0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(14,  1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(13,  2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(12,  3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(11,  4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(10,  5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 9,  6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 8,  7, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 7,  8, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 6,  9, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 5, 10, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 4, 11, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 3, 12, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 2, 13, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 1, 14, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 0, 15, nMaskColour);
	}
}

void Render16x16Tile_Mask_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 8);

	unsigned short* pPixel = pDestDraw + ((StartY + 15) * nScreenWidth) + StartX;

	for (int y = 15; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 16) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(15,  0, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(14,  1, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(13,  2, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(12,  3, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(11,  4, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(10,  5, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 9,  6, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 8,  7, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 7,  8, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 6,  9, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 5, 10, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 4, 11, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 3, 12, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 2, 13, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 1, 14, nMaskColour));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 0, 15, nMaskColour));
	}
}

/*================================================================================================
32 x 32 Functions
================================================================================================*/

void Render32x32Tile(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		PLOTPIXEL( 0);
		PLOTPIXEL( 1);
		PLOTPIXEL( 2);
		PLOTPIXEL( 3);
		PLOTPIXEL( 4);
		PLOTPIXEL( 5);
		PLOTPIXEL( 6);
		PLOTPIXEL( 7);
		PLOTPIXEL( 8);
		PLOTPIXEL( 9);
		PLOTPIXEL(10);
		PLOTPIXEL(11);
		PLOTPIXEL(12);
		PLOTPIXEL(13);
		PLOTPIXEL(14);
		PLOTPIXEL(15);
		PLOTPIXEL(16);
		PLOTPIXEL(17);
		PLOTPIXEL(18);
		PLOTPIXEL(19);
		PLOTPIXEL(20);
		PLOTPIXEL(21);
		PLOTPIXEL(22);
		PLOTPIXEL(23);
		PLOTPIXEL(24);
		PLOTPIXEL(25);
		PLOTPIXEL(26);
		PLOTPIXEL(27);
		PLOTPIXEL(28);
		PLOTPIXEL(29);
		PLOTPIXEL(30);
		PLOTPIXEL(31);
	}
}

void Render32x32Tile_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL( 0));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL( 1));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL( 2));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL( 3));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL( 4));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL( 5));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL( 6));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL( 7));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL( 8));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL( 9));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL(10));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL(11));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL(12));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL(13));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL(14));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL(15));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL(16));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL(17));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL(18));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL(19));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL(20));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL(21));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL(22));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL(23));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL(24));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL(25));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL(26));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL(27));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL(28));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL(29));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL(30));
		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL(31));
	}
}

void Render32x32Tile_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		PLOTPIXEL_FLIPX(31,  0);
		PLOTPIXEL_FLIPX(30,  1);
		PLOTPIXEL_FLIPX(29,  2);
		PLOTPIXEL_FLIPX(28,  3);
		PLOTPIXEL_FLIPX(27,  4);
		PLOTPIXEL_FLIPX(26,  5);
		PLOTPIXEL_FLIPX(25,  6);
		PLOTPIXEL_FLIPX(24,  7);
		PLOTPIXEL_FLIPX(23,  8);
		PLOTPIXEL_FLIPX(22,  9);
		PLOTPIXEL_FLIPX(21, 10);
		PLOTPIXEL_FLIPX(20, 11);
		PLOTPIXEL_FLIPX(19, 12);
		PLOTPIXEL_FLIPX(18, 13);
		PLOTPIXEL_FLIPX(17, 14);
		PLOTPIXEL_FLIPX(16, 15);
		PLOTPIXEL_FLIPX(15, 16);
		PLOTPIXEL_FLIPX(14, 17);
		PLOTPIXEL_FLIPX(13, 18);
		PLOTPIXEL_FLIPX(12, 19);
		PLOTPIXEL_FLIPX(11, 20);
		PLOTPIXEL_FLIPX(10, 21);
		PLOTPIXEL_FLIPX( 9, 22);
		PLOTPIXEL_FLIPX( 8, 23);
		PLOTPIXEL_FLIPX( 7, 24);
		PLOTPIXEL_FLIPX( 6, 25);
		PLOTPIXEL_FLIPX( 5, 26);
		PLOTPIXEL_FLIPX( 4, 27);
		PLOTPIXEL_FLIPX( 3, 28);
		PLOTPIXEL_FLIPX( 2, 29);
		PLOTPIXEL_FLIPX( 1, 30);
		PLOTPIXEL_FLIPX( 0, 31);
	}
}

void Render32x32Tile_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_FLIPX(31,  0));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_FLIPX(30,  1));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_FLIPX(29,  2));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_FLIPX(28,  3));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_FLIPX(27,  4));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_FLIPX(26,  5));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_FLIPX(25,  6));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_FLIPX(24,  7));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_FLIPX(23,  8));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_FLIPX(22,  9));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_FLIPX(21, 10));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_FLIPX(20, 11));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_FLIPX(19, 12));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_FLIPX(18, 13));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_FLIPX(17, 14));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_FLIPX(16, 15));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_FLIPX(15, 16));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_FLIPX(14, 17));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_FLIPX(13, 18));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_FLIPX(12, 19));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_FLIPX(11, 20));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_FLIPX(10, 21));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 9, 22));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 8, 23));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 7, 24));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 6, 25));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 5, 26));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 4, 27));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 3, 28));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 2, 29));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 1, 30));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 0, 31));
	}
}

void Render32x32Tile_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		PLOTPIXEL( 0);
		PLOTPIXEL( 1);
		PLOTPIXEL( 2);
		PLOTPIXEL( 3);
		PLOTPIXEL( 4);
		PLOTPIXEL( 5);
		PLOTPIXEL( 6);
		PLOTPIXEL( 7);
		PLOTPIXEL( 8);
		PLOTPIXEL( 9);
		PLOTPIXEL(10);
		PLOTPIXEL(11);
		PLOTPIXEL(12);
		PLOTPIXEL(13);
		PLOTPIXEL(14);
		PLOTPIXEL(15);
		PLOTPIXEL(16);
		PLOTPIXEL(17);
		PLOTPIXEL(18);
		PLOTPIXEL(19);
		PLOTPIXEL(20);
		PLOTPIXEL(21);
		PLOTPIXEL(22);
		PLOTPIXEL(23);
		PLOTPIXEL(24);
		PLOTPIXEL(25);
		PLOTPIXEL(26);
		PLOTPIXEL(27);
		PLOTPIXEL(28);
		PLOTPIXEL(29);
		PLOTPIXEL(30);
		PLOTPIXEL(31);
	}
}

void Render32x32Tile_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL( 0));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL( 1));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL( 2));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL( 3));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL( 4));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL( 5));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL( 6));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL( 7));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL( 8));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL( 9));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL(10));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL(11));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL(12));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL(13));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL(14));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL(15));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL(16));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL(17));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL(18));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL(19));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL(20));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL(21));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL(22));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL(23));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL(24));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL(25));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL(26));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL(27));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL(28));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL(29));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL(30));
		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL(31));
	}
}

void Render32x32Tile_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		PLOTPIXEL_FLIPX(31,  0);
		PLOTPIXEL_FLIPX(30,  1);
		PLOTPIXEL_FLIPX(29,  2);
		PLOTPIXEL_FLIPX(28,  3);
		PLOTPIXEL_FLIPX(27,  4);
		PLOTPIXEL_FLIPX(26,  5);
		PLOTPIXEL_FLIPX(25,  6);
		PLOTPIXEL_FLIPX(24,  7);
		PLOTPIXEL_FLIPX(23,  8);
		PLOTPIXEL_FLIPX(22,  9);
		PLOTPIXEL_FLIPX(21, 10);
		PLOTPIXEL_FLIPX(20, 11);
		PLOTPIXEL_FLIPX(19, 12);
		PLOTPIXEL_FLIPX(18, 13);
		PLOTPIXEL_FLIPX(17, 14);
		PLOTPIXEL_FLIPX(16, 15);
		PLOTPIXEL_FLIPX(15, 16);
		PLOTPIXEL_FLIPX(14, 17);
		PLOTPIXEL_FLIPX(13, 18);
		PLOTPIXEL_FLIPX(12, 19);
		PLOTPIXEL_FLIPX(11, 20);
		PLOTPIXEL_FLIPX(10, 21);
		PLOTPIXEL_FLIPX( 9, 22);
		PLOTPIXEL_FLIPX( 8, 23);
		PLOTPIXEL_FLIPX( 7, 24);
		PLOTPIXEL_FLIPX( 6, 25);
		PLOTPIXEL_FLIPX( 5, 26);
		PLOTPIXEL_FLIPX( 4, 27);
		PLOTPIXEL_FLIPX( 3, 28);
		PLOTPIXEL_FLIPX( 2, 29);
		PLOTPIXEL_FLIPX( 1, 30);
		PLOTPIXEL_FLIPX( 0, 31);
	}
}

void Render32x32Tile_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_FLIPX(31,  0));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_FLIPX(30,  1));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_FLIPX(29,  2));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_FLIPX(28,  3));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_FLIPX(27,  4));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_FLIPX(26,  5));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_FLIPX(25,  6));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_FLIPX(24,  7));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_FLIPX(23,  8));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_FLIPX(22,  9));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_FLIPX(21, 10));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_FLIPX(20, 11));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_FLIPX(19, 12));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_FLIPX(18, 13));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_FLIPX(17, 14));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_FLIPX(16, 15));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_FLIPX(15, 16));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_FLIPX(14, 17));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_FLIPX(13, 18));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_FLIPX(12, 19));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_FLIPX(11, 20));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_FLIPX(10, 21));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 9, 22));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 8, 23));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 7, 24));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 6, 25));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 5, 26));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 4, 27));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 3, 28));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 2, 29));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 1, 30));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_FLIPX( 0, 31));
	}
}

/*================================================================================================
32 x 32 Functions with Masking
================================================================================================*/

void Render32x32Tile_Mask(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);
	
	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		PLOTPIXEL_MASK( 0, nMaskColour);
		PLOTPIXEL_MASK( 1, nMaskColour);
		PLOTPIXEL_MASK( 2, nMaskColour);
		PLOTPIXEL_MASK( 3, nMaskColour);
		PLOTPIXEL_MASK( 4, nMaskColour);
		PLOTPIXEL_MASK( 5, nMaskColour);
		PLOTPIXEL_MASK( 6, nMaskColour);
		PLOTPIXEL_MASK( 7, nMaskColour);
		PLOTPIXEL_MASK( 8, nMaskColour);
		PLOTPIXEL_MASK( 9, nMaskColour);
		PLOTPIXEL_MASK(10, nMaskColour);
		PLOTPIXEL_MASK(11, nMaskColour);
		PLOTPIXEL_MASK(12, nMaskColour);
		PLOTPIXEL_MASK(13, nMaskColour);
		PLOTPIXEL_MASK(14, nMaskColour);
		PLOTPIXEL_MASK(15, nMaskColour);
		PLOTPIXEL_MASK(16, nMaskColour);
		PLOTPIXEL_MASK(17, nMaskColour);
		PLOTPIXEL_MASK(18, nMaskColour);
		PLOTPIXEL_MASK(19, nMaskColour);
		PLOTPIXEL_MASK(20, nMaskColour);
		PLOTPIXEL_MASK(21, nMaskColour);
		PLOTPIXEL_MASK(22, nMaskColour);
		PLOTPIXEL_MASK(23, nMaskColour);
		PLOTPIXEL_MASK(24, nMaskColour);
		PLOTPIXEL_MASK(25, nMaskColour);
		PLOTPIXEL_MASK(26, nMaskColour);
		PLOTPIXEL_MASK(27, nMaskColour);
		PLOTPIXEL_MASK(28, nMaskColour);
		PLOTPIXEL_MASK(29, nMaskColour);
		PLOTPIXEL_MASK(30, nMaskColour);
		PLOTPIXEL_MASK(31, nMaskColour);
	}
}

void Render32x32Tile_Mask_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK( 0, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK( 1, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK( 2, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK( 3, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK( 4, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK( 5, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK( 6, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK( 7, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK( 8, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK( 9, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK(10, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK(11, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK(12, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK(13, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK(14, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK(15, nMaskColour));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_MASK(16, nMaskColour));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_MASK(17, nMaskColour));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_MASK(18, nMaskColour));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_MASK(19, nMaskColour));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_MASK(20, nMaskColour));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_MASK(21, nMaskColour));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_MASK(22, nMaskColour));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_MASK(23, nMaskColour));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_MASK(24, nMaskColour));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_MASK(25, nMaskColour));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_MASK(26, nMaskColour));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_MASK(27, nMaskColour));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_MASK(28, nMaskColour));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_MASK(29, nMaskColour));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_MASK(30, nMaskColour));
		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_MASK(31, nMaskColour));
	}
}

void Render32x32Tile_Mask_FlipX(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		PLOTPIXEL_MASK_FLIPX(31,  0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(30,  1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(29,  2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(28,  3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(27,  4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(26,  5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(25,  6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(24,  7, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(23,  8, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(22,  9, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(21, 10, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(20, 11, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(19, 12, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(18, 13, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(17, 14, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(16, 15, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(15, 16, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(14, 17, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(13, 18, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(12, 19, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(11, 20, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(10, 21, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 9, 22, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 8, 23, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 7, 24, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 6, 25, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 5, 26, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 4, 27, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 3, 28, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 2, 29, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 1, 30, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 0, 31, nMaskColour);
	}
}

void Render32x32Tile_Mask_FlipX_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + (StartY * nScreenWidth) + StartX;

	for (int y = 0; y < 32; y++, pPixel += nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(31,  0, nMaskColour));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(30,  1, nMaskColour));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(29,  2, nMaskColour));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(28,  3, nMaskColour));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(27,  4, nMaskColour));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(26,  5, nMaskColour));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(25,  6, nMaskColour));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(24,  7, nMaskColour));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(23,  8, nMaskColour));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(22,  9, nMaskColour));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(21, 10, nMaskColour));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(20, 11, nMaskColour));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(19, 12, nMaskColour));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(18, 13, nMaskColour));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(17, 14, nMaskColour));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(16, 15, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(15, 16, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(14, 17, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(13, 18, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(12, 19, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(11, 20, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(10, 21, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 9, 22, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 8, 23, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 7, 24, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 6, 25, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 5, 26, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 4, 27, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 3, 28, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 2, 29, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 1, 30, nMaskColour));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 0, 31, nMaskColour));
	}
}

void Render32x32Tile_Mask_FlipY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		PLOTPIXEL_MASK( 0, nMaskColour);
		PLOTPIXEL_MASK( 1, nMaskColour);
		PLOTPIXEL_MASK( 2, nMaskColour);
		PLOTPIXEL_MASK( 3, nMaskColour);
		PLOTPIXEL_MASK( 4, nMaskColour);
		PLOTPIXEL_MASK( 5, nMaskColour);
		PLOTPIXEL_MASK( 6, nMaskColour);
		PLOTPIXEL_MASK( 7, nMaskColour);
		PLOTPIXEL_MASK( 8, nMaskColour);
		PLOTPIXEL_MASK( 9, nMaskColour);
		PLOTPIXEL_MASK(10, nMaskColour);
		PLOTPIXEL_MASK(11, nMaskColour);
		PLOTPIXEL_MASK(12, nMaskColour);
		PLOTPIXEL_MASK(13, nMaskColour);
		PLOTPIXEL_MASK(14, nMaskColour);
		PLOTPIXEL_MASK(15, nMaskColour);
		PLOTPIXEL_MASK(16, nMaskColour);
		PLOTPIXEL_MASK(17, nMaskColour);
		PLOTPIXEL_MASK(18, nMaskColour);
		PLOTPIXEL_MASK(19, nMaskColour);
		PLOTPIXEL_MASK(20, nMaskColour);
		PLOTPIXEL_MASK(21, nMaskColour);
		PLOTPIXEL_MASK(22, nMaskColour);
		PLOTPIXEL_MASK(23, nMaskColour);
		PLOTPIXEL_MASK(24, nMaskColour);
		PLOTPIXEL_MASK(25, nMaskColour);
		PLOTPIXEL_MASK(26, nMaskColour);
		PLOTPIXEL_MASK(27, nMaskColour);
		PLOTPIXEL_MASK(28, nMaskColour);
		PLOTPIXEL_MASK(29, nMaskColour);
		PLOTPIXEL_MASK(30, nMaskColour);
		PLOTPIXEL_MASK(31, nMaskColour);
	}
}

void Render32x32Tile_Mask_FlipY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK( 0, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK( 1, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK( 2, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK( 3, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK( 4, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK( 5, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK( 6, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK( 7, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK( 8, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK( 9, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK(10, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK(11, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK(12, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK(13, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK(14, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK(15, nMaskColour));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_MASK(16, nMaskColour));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_MASK(17, nMaskColour));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_MASK(18, nMaskColour));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_MASK(19, nMaskColour));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_MASK(20, nMaskColour));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_MASK(21, nMaskColour));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_MASK(22, nMaskColour));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_MASK(23, nMaskColour));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_MASK(24, nMaskColour));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_MASK(25, nMaskColour));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_MASK(26, nMaskColour));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_MASK(27, nMaskColour));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_MASK(28, nMaskColour));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_MASK(29, nMaskColour));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_MASK(30, nMaskColour));
		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_MASK(31, nMaskColour));
	}
}

void Render32x32Tile_Mask_FlipXY(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		PLOTPIXEL_MASK_FLIPX(31,  0, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(30,  1, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(29,  2, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(28,  3, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(27,  4, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(26,  5, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(25,  6, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(24,  7, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(23,  8, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(22,  9, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(21, 10, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(20, 11, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(19, 12, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(18, 13, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(17, 14, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(16, 15, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(15, 16, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(14, 17, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(13, 18, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(12, 19, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(11, 20, nMaskColour);
		PLOTPIXEL_MASK_FLIPX(10, 21, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 9, 22, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 8, 23, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 7, 24, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 6, 25, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 5, 26, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 4, 27, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 3, 28, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 2, 29, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 1, 30, nMaskColour);
		PLOTPIXEL_MASK_FLIPX( 0, 31, nMaskColour);
	}
}

void Render32x32Tile_Mask_FlipXY_Clip(unsigned short* pDestDraw, int nTileNumber, int StartX, int StartY, int nTilePalette, int nColourDepth, int nMaskColour, int nPaletteOffset, unsigned char *pTile)
{
	UINT32 nPalette = (nTilePalette << nColourDepth) | nPaletteOffset;
	pTileData = pTile + (nTileNumber << 10);

	unsigned short* pPixel = pDestDraw + ((StartY + 31) * nScreenWidth) + StartX;

	for (int y = 31; y >= 0; y--, pPixel -= nScreenWidth, pTileData += 32) {
		if ((StartY + y) < 0 || (StartY + y) >= nScreenHeight) {
			continue;
		}

		CLIPPIXEL(31, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(31,  0, nMaskColour));
		CLIPPIXEL(30, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(30,  1, nMaskColour));
		CLIPPIXEL(29, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(29,  2, nMaskColour));
		CLIPPIXEL(28, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(28,  3, nMaskColour));
		CLIPPIXEL(27, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(27,  4, nMaskColour));
		CLIPPIXEL(26, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(26,  5, nMaskColour));
		CLIPPIXEL(25, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(25,  6, nMaskColour));
		CLIPPIXEL(24, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(24,  7, nMaskColour));
		CLIPPIXEL(23, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(23,  8, nMaskColour));
		CLIPPIXEL(22, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(22,  9, nMaskColour));
		CLIPPIXEL(21, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(21, 10, nMaskColour));
		CLIPPIXEL(20, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(20, 11, nMaskColour));
		CLIPPIXEL(19, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(19, 12, nMaskColour));
		CLIPPIXEL(18, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(18, 13, nMaskColour));
		CLIPPIXEL(17, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(17, 14, nMaskColour));
		CLIPPIXEL(16, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(16, 15, nMaskColour));
		CLIPPIXEL(15, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(15, 16, nMaskColour));
		CLIPPIXEL(14, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(14, 17, nMaskColour));
		CLIPPIXEL(13, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(13, 18, nMaskColour));
		CLIPPIXEL(12, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(12, 19, nMaskColour));
		CLIPPIXEL(11, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(11, 20, nMaskColour));
		CLIPPIXEL(10, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX(10, 21, nMaskColour));
		CLIPPIXEL( 9, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 9, 22, nMaskColour));
		CLIPPIXEL( 8, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 8, 23, nMaskColour));
		CLIPPIXEL( 7, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 7, 24, nMaskColour));
		CLIPPIXEL( 6, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 6, 25, nMaskColour));
		CLIPPIXEL( 5, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 5, 26, nMaskColour));
		CLIPPIXEL( 4, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 4, 27, nMaskColour));
		CLIPPIXEL( 3, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 3, 28, nMaskColour));
		CLIPPIXEL( 2, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 2, 29, nMaskColour));
		CLIPPIXEL( 1, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 1, 30, nMaskColour));
		CLIPPIXEL( 0, StartX, nScreenWidth, PLOTPIXEL_MASK_FLIPX( 0, 31, nMaskColour));
	}
}

#undef PLOTPIXEL
#undef PLOTPIXEL_FLIPX
#undef PLOTPIXEL_MASK
#undef CLIPPIXEL
