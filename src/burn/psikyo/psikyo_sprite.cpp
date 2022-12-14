// Psikyo hardware sprites
#include "psikyo.h"

unsigned char* PsikyoSpriteROM = NULL;
unsigned char* PsikyoSpriteRAM = NULL;
unsigned char* PsikyoSpriteLUT = NULL;

struct PsikyoSprite {
	char flip;
	char priority;
	short palette;
	int x; int y;
	int xsize; int ysize;
	int xzoom; int yzoom;
	int address;
};

static int nFrame;

static char* PsikyoSpriteAttrib = NULL;
static int nSpriteAddressMask;

static PsikyoSprite* pSpriteLists = NULL;
static PsikyoSprite* pSpriteList = NULL;

static int* PsikyoZoomXTable = NULL;
static int* PsikyoZoomYTable = NULL;

static unsigned char* pTile;
static unsigned char* pTileData;
static unsigned int* pTilePalette;

static unsigned short* pZBuffer = NULL;
static unsigned short* pZTile;

static int *pXZoomInfo, *pYZoomInfo;

static int nTileXPos, nTileYPos, nZPos, nTileXSize, nTileYSize;
static int nXSize, nYSize;

static int nFirstSprites[8], nLastSprites[8];
static int *nFirstSprite, *nLastSprite;

static int nTopSprite;
static int nZOffset;

typedef void (*RenderSpriteFunction)();

// Include the tile rendering functions
#include "psikyo_sprite_func.h"

static void GetBuffers(int nBuffer)
{
	pSpriteList = pSpriteLists + (nBuffer << 10);
	nFirstSprite = nFirstSprites + (nBuffer << 2);
	nLastSprite = nLastSprites + (nBuffer << 2);
}

int PsikyoSpriteRender(int nLowPriority, int nHighPriority)
{
	static int nMaskLeft, nMaskRight, nMaskTop, nMaskBottom;
	PsikyoSprite* pBuffer;
	int nRenderFunction;

	unsigned char* pStart;
	unsigned short* pZStart = NULL;

	int nAddress, nTransColour, nNextTileX, nNextTileY, nXZoom, nYZoom, nStartXPos;
	int nPriorityMask = 0;
	int nMaxZPos = -1;
	int nMinZPos = 0x00010000;
	int nUseBuffer = 0x00010000;
	int nCurrentZPos;

	if ((nBurnLayer & 1) == 0) {
		return 0;
	}

	if (nLowPriority == 0) {
		nZPos = -1;
		nTopSprite = -1;

		nMaskLeft = nMaskTop = 9999;
		nMaskRight = nMaskBottom = -1;

		GetBuffers(nFrame ^ 1);
	}

	if (nHighPriority < 3) {
		for (int i = nHighPriority + 1; i < 4; i++) {
			if (nUseBuffer > nFirstSprite[i]) {
				nUseBuffer = nFirstSprite[i];
			}
		}
	}

	nRenderFunction = (((unsigned short*)PsikyoSpriteRAM)[0x0FFF] & 4) << 4;
	nTransColour = (nRenderFunction & 64) ? 0 : 15;

	for (int i = nLowPriority; i <= nHighPriority; i++) {
		if (nMinZPos > nFirstSprite[i]) {
			nMinZPos = nFirstSprite[i];
		}
		if (nMaxZPos < nLastSprite[i]) {
			nMaxZPos = nLastSprite[i];
		}
		nPriorityMask |= 1 << i;
	}

	nCurrentZPos = nMinZPos;

	nPriorityMask &= nSpriteEnable;
	if (nPriorityMask == 0) {
		return 0;
	}

	for (pBuffer = pSpriteList + nCurrentZPos; nCurrentZPos <= nMaxZPos; pBuffer++, nCurrentZPos++) {

		if ((pBuffer->priority & nPriorityMask) == 0) {
			continue;
		}

		nTileXPos = pBuffer->x;
		nTileYPos = pBuffer->y;

		pTilePalette = PsikyoPalette + pBuffer->palette;

		nXSize = pBuffer->xsize;
		nYSize = pBuffer->ysize;

		nRenderFunction &= 64;
		nRenderFunction |= pBuffer->flip << 3;

		nAddress = pBuffer->address;

		switch (pBuffer->flip) {
			case 1:
				nNextTileX = -1;
				nNextTileY = nXSize << 1;
				nAddress -= nXSize + 1;
				break;
			case 2:
				nNextTileX = 1;
				nNextTileY = -nXSize << 1;
				nAddress += (nYSize + 1) * nXSize;
				break;
			case 3:
				nNextTileX = -1;
				nNextTileY = 0;
				nAddress += nYSize * nXSize - 1;
				break;
			default:
				nNextTileX = 1;
				nNextTileY = 0;
		}

		if (pBuffer->xzoom == 0 && pBuffer->yzoom == 0) {						// This sprite doesn't use zooming

			if (nTopSprite > nCurrentZPos) {										// Test ZBuffer
				if (nTileXPos < nMaskRight && (nTileXPos + nXSize * 16) >= nMaskLeft && nTileYPos < nMaskBottom && (nTileYPos + nYSize * 16) >= nMaskTop) {
					nRenderFunction |= 2;
				}
			}

			if (nUseBuffer < nCurrentZPos) {										// Write ZBuffer
				nRenderFunction |= 4;

				if (nTileXPos < nMaskLeft) {
					nMaskLeft = nTileXPos;
				}
				if ((nTileXPos + nXSize) > nMaskRight) {
					nMaskRight = nTileXPos + nXSize * 16;
				}
				if (nTileYPos < nMaskTop) {
					nMaskTop = nTileYPos;
				}
				if ((nTileYPos + nYSize) > nMaskBottom) {
					nMaskBottom = nTileYPos + nYSize * 16;
				}
			}

			if (nRenderFunction & 6) {
				pZStart = pZBuffer + (nTileYPos * 320) + nTileXPos;
				nZPos = nZOffset + nCurrentZPos;
			}

#ifdef BUILD_PSP
			pStart = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);

			for (int y = 0; y < nYSize; y++, nTileYPos += 16, pStart += (16 << 10), pZStart += 16 * 320) {
#else
			pStart = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);

			for (int y = 0; y < nYSize; y++, nTileYPos += 16, pStart += 16 * nBurnPitch, pZStart += 16 * 320) {
#endif
				nTileXPos = pBuffer->x;

				pTile = pStart;
				pZTile = pZStart;

				nAddress += nNextTileY;

				for (int x = 0; x < nXSize; x++, nTileXPos += 16, nAddress += nNextTileX, pTile += nBurnBpp << 4, pZTile += 16) {

					if (PsikyoSpriteAttrib[((unsigned short*)PsikyoSpriteLUT)[nAddress]] == nTransColour) {
						continue;
					}

					pTileData = PsikyoSpriteROM + ((((unsigned short*)PsikyoSpriteLUT)[nAddress] << 8) & nSpriteAddressMask);
					if (nTileYPos < 0 || nTileYPos > 208 || nTileXPos < 0 || nTileXPos > 304) {
						if (nTileYPos > -16 && nTileYPos < 224 && nTileXPos > -16 && nTileXPos < 320) {
							RenderSprite[nRenderFunction + 1]();
						}
					} else {
						RenderSprite[nRenderFunction + 0]();
					}
				}
			}
		} else {																	// This sprite uses zooming
			nXZoom = pBuffer->xzoom;
			nYZoom = pBuffer->yzoom;

			nTileXPos += (nXSize * nXZoom + 3) >> 2;
			nTileYPos += (nYSize * nYZoom + 3) >> 2;

			nXZoom = 32 - nXZoom;
			nYZoom = 32 - nYZoom;

			if (nTopSprite > nCurrentZPos) {										// Test ZBuffer
				if (nTileXPos < nMaskRight && (nTileXPos + nXSize * nXZoom / 2) >= nMaskLeft && nTileYPos < nMaskBottom && (nTileYPos + nYSize * nXZoom / 2) >= nMaskTop) {
					nRenderFunction |= 2;
				}
			}

			if (nUseBuffer < nCurrentZPos) {										// Write ZBuffer
				nRenderFunction |= 4;

				if (nTileXPos < nMaskLeft) {
					nMaskLeft = nTileXPos;
				}
				if ((nTileXPos + nXSize) > nMaskRight) {
					nMaskRight = nTileXPos + nXSize * nXZoom / 2;
				}
				if (nTileYPos < nMaskTop) {
					nMaskTop = nTileYPos;
				}
				if ((nTileYPos + nYSize) > nMaskBottom) {
					nMaskBottom = nTileYPos + nYSize * nYZoom / 2;
				}
			}

			if (nRenderFunction & 6) {
				pZStart = pZBuffer + (nTileYPos * 320) + nTileXPos;
				nZPos = nZOffset + nCurrentZPos;
			}

			nStartXPos = nTileXPos;
#ifdef BUILD_PSP
			pStart = pBurnDraw + (nTileYPos << 10) + (nTileXPos << 1);

			for (int y = 0; y < nYSize; y++, nTileYPos += nTileYSize, pStart += (nTileYSize << 10), pZStart += nTileYSize * 320) {
#else
			pStart = pBurnDraw + (nTileYPos * nBurnPitch) + (nTileXPos * nBurnBpp);

			for (int y = 0; y < nYSize; y++, nTileYPos += nTileYSize, pStart += nTileYSize * nBurnPitch, pZStart += nTileYSize * 320) {
#endif
				nTileXPos = nStartXPos;

				pTile = pStart;
				pZTile = pZStart;

				nAddress += nNextTileY;

				nTileYSize = (y + 1) * nYZoom / 2 - (y * nYZoom / 2);
				pYZoomInfo = PsikyoZoomYTable + (nTileYSize << 4);

				for (int x = 0; x < nXSize; x++, nTileXPos += nTileXSize, nAddress += nNextTileX, pTile += nBurnBpp * nTileXSize, pZTile += nTileXSize) {

					nTileXSize = (x + 1) * nXZoom / 2 - (x * nXZoom / 2);

					if (PsikyoSpriteAttrib[((unsigned short*)PsikyoSpriteLUT)[nAddress]] == nTransColour) {
						continue;
					}

					pXZoomInfo = PsikyoZoomXTable + (nTileXSize << 4);
					pTileData = PsikyoSpriteROM + ((((unsigned short*)PsikyoSpriteLUT)[nAddress] << 8) & nSpriteAddressMask);
					if (nTileYPos < 0 || nTileYPos > (224 - nTileYSize) || nTileXPos < 0 || nTileXPos > (320 - nTileXSize)) {
						if (nTileYPos > -nTileYSize && nTileYPos < 224 && nTileXPos > -nTileXSize && nTileXPos < 320) {
							RenderSprite[nRenderFunction + 33]();
						}
					} else {
						RenderSprite[nRenderFunction + 32]();
					}
				}
			}
		}
	}

	if (nMaxZPos > nTopSprite) {
		nTopSprite = nMaxZPos;
	}

	if (nHighPriority == 3) {
		if (nZPos >= 0) {
			nZOffset += nTopSprite;
			if (nZOffset > 0xFC00) {
				memset(pZBuffer, 0, 320 * 224 * sizeof(short));
				nZOffset = 0;
			}
		}
	}

	return 0;
}

int PsikyoSpriteBuffer()
{
	unsigned short* pSprite;
	PsikyoSprite* pBuffer;
	int nPriority;

	unsigned short word;
	int x, y, xs, ys;

	nFrame ^= 1;

	GetBuffers(nFrame);

	pBuffer = pSpriteList;

	nFirstSprite[0] = 0x00010000;
	nFirstSprite[1] = 0x00010000;
	nFirstSprite[2] = 0x00010000;
	nFirstSprite[3] = 0x00010000;

	nLastSprite[0] = -1;
	nLastSprite[1] = -1;
	nLastSprite[2] = -1;
	nLastSprite[3] = -1;

	// Check if sprites are disabled
	if (((unsigned short*)PsikyoSpriteRAM)[0x0FFF] & 1) {
		return 0;
	}

	for (int i = 0x0C00, z = 0; i < 0x0FFF; i++) {

		word = ((unsigned short*)PsikyoSpriteRAM)[i];

		// Check for end-marker
		if (word == 0xFFFF) {
			break;
		}

		if (word >= 0x0300) {
			continue;
		}

		// Point to sprite
		pSprite = &(((unsigned short*)PsikyoSpriteRAM)[word * 4]);

		x = pSprite[1] & 0x01FF;
		y = pSprite[0] & 0x01FF;

		xs = ((pSprite[1] >> 9) & 0x0007) + 1;
		ys = ((pSprite[0] >> 9) & 0x0007) + 1;

		if (x >= 320) {
			x -= 512;
			if (x + (xs << 4) < 0) {
				continue;
			}
		}
		if (y >= 224) {
			y -= 512;
			if (y + (ys << 4) < 0) {
				continue;
			}
		}

		// Sprite is active and most likely on screen, so add it to the buffer

		word = pSprite[2];

		nPriority = 3 - ((word >> 6) & 0x03);

		if (nLastSprite[nPriority] == -1) {
			nFirstSprite[nPriority] = z;
		}
		nLastSprite[nPriority] = z;

		pBuffer->priority = 1 << nPriority;

		pBuffer->xzoom = pSprite[1] >> 12;
		pBuffer->yzoom = pSprite[0] >> 12;

		pBuffer->xsize = xs;
		pBuffer->ysize = ys;

		pBuffer->x = x;
		pBuffer->y = y;

		pBuffer->flip = (word >> 14) & 0x03;
		pBuffer->palette = (word >> 4) & 0x01F0;

		pBuffer->address = pSprite[3] | ((word & 1) << 16);

		pBuffer++;
		z++;
	}

	return 0;
}

void PsikyoSpriteExit()
{
	free(PsikyoZoomXTable);
	PsikyoZoomXTable = NULL;
	free(PsikyoZoomYTable);
	PsikyoZoomYTable = NULL;

	free(PsikyoSpriteAttrib);
	PsikyoSpriteAttrib = NULL;

	free(pSpriteLists);
	pSpriteLists = NULL;

	free(pZBuffer);
	pZBuffer = NULL;

	return;
}

int PsikyoSpriteInit(int nROMSize)
{
	const int nTileSize = 256;
	int nNumTiles = nROMSize / nTileSize;

	free(pSpriteLists);
	pSpriteLists = (PsikyoSprite*)malloc(0x0800 * sizeof(PsikyoSprite));
	if (pSpriteLists == NULL) {
		PsikyoSpriteExit();
		return 1;
	}
	memset(pSpriteLists, 0, 0x0800 * sizeof(PsikyoSprite));

	for (int i = 0; i < 8; i++) {
		nFirstSprites[i] = 0x00010000;
		nLastSprites[i] = -1;
	}

	free(pZBuffer);
	pZBuffer = (unsigned short*)malloc(320 * 224 * sizeof(short));
	if (pZBuffer == NULL) {
		PsikyoSpriteExit();
		return 1;
	}

	memset(pZBuffer, 0, 320 * 224 * sizeof(short));
	nZOffset = 0;

	for (nSpriteAddressMask = 1; nSpriteAddressMask < nROMSize; nSpriteAddressMask <<= 1) {}
	nSpriteAddressMask--;

	free(PsikyoSpriteAttrib);
	PsikyoSpriteAttrib = (char*)malloc(nSpriteAddressMask + 1);
	if (PsikyoSpriteAttrib == NULL) {
		return 1;
	}
	memset(PsikyoSpriteAttrib, 0, nSpriteAddressMask + 1);

	for (int i = 0; i < nNumTiles; i++) {
		bool bTransparent0 = true;
		bool bTransparent15 = true;
		for (int j = i * nTileSize; j < (i + 1) * nTileSize; j++) {
			if (PsikyoSpriteROM[j] != 0x00) {
				bTransparent0 = false;
				if (!bTransparent15) {
					break;
				}
			}
			if (PsikyoSpriteROM[j] != 0xFF) {
				bTransparent15 = false;
				if (!bTransparent0) {
					break;
				}
			}
		}
		PsikyoSpriteAttrib[i] = 0xFF;
		if (bTransparent0) {
			PsikyoSpriteAttrib[i] = 0;
		}
		if (bTransparent15) {
			PsikyoSpriteAttrib[i] = 15;
		}
	}

	for (int i = nNumTiles; i <= nSpriteAddressMask; i++) {
		PsikyoSpriteAttrib[i] = 0xFF;
	}

	PsikyoZoomXTable = (int*)malloc(272 * sizeof(int));
	PsikyoZoomYTable = (int*)malloc(272 * sizeof(int));
	if (PsikyoZoomXTable == NULL || PsikyoZoomYTable == NULL) {
		PsikyoSpriteExit();
		return 1;
	}

	memset(PsikyoZoomXTable, 0, 272 * sizeof(int));
	memset(PsikyoZoomYTable, 0, 272 * sizeof(int));

	for (int z = 8; z < 16; z++) {
		for (int i = 0, x = 0; i < z; i++, x += 0x100000 / z) {
			PsikyoZoomXTable[(z << 4) + i] = (x + 0x8000) >> 16;
		}
		for (int i = 1; i < z; i++) {
			PsikyoZoomYTable[(z << 4) + i - 1] = (PsikyoZoomXTable[(z << 4) + i] - PsikyoZoomXTable[(z << 4) + i - 1]) << 4;
//			bprintf(PRINT_NORMAL, "%2i:%2i - ", PsikyoZoomXTable[(z << 4) + i - 1], PsikyoZoomYTable[(z << 4) + i - 1]);
		}
		PsikyoZoomYTable[(z << 4) + z - 1] = PsikyoZoomYTable[(z << 4)];
//		bprintf(PRINT_NORMAL, "%2i:%2i\n", PsikyoZoomXTable[(z << 4) + z - 1], PsikyoZoomYTable[(z << 4) + z - 1]);
	}
	for (int i = 0; i < 16; i++) {
		PsikyoZoomXTable[256 + i] = i;
		PsikyoZoomYTable[256 + i] = 16;
	}

	nFrame = 0;

	return 0;
}

