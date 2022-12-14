#include "toaplan.h"

unsigned char* ExtraTROM;
unsigned char* ExtraTRAM;
unsigned char* ExtraTScroll;
unsigned char* ExtraTSelect;

int	nExtraTXOffset = 0x9999;
int nTileXPos;

static unsigned char* pTile;
static unsigned char* pTileData;
static unsigned int* pTilePalette;

typedef void (*RenderTileFunction)();
static RenderTileFunction ALIGN_DATA RenderTile[4];

static int nLastBPP = 0;

#define ROT 0

#define ROWMODE 0

#define DOCLIP 0

#define BPP 16
#include "toa_extratext.h"
#undef BPP
#define BPP 24
#include "toa_extratext.h"
#undef BPP
#define BPP 32
#include "toa_extratext.h"
#undef BPP

#undef DOCLIP
#define DOCLIP 1

#define BPP 16
#include "toa_extratext.h"
#undef BPP
#define BPP 24
#include "toa_extratext.h"
#undef BPP
#define BPP 32
#include "toa_extratext.h"
#undef BPP

#undef DOCLIP

#undef ROWMODE
#define ROWMODE 1

#define DOCLIP 0

#define BPP 16
#include "toa_extratext.h"
#undef BPP
#define BPP 24
#include "toa_extratext.h"
#undef BPP
#define BPP 32
#include "toa_extratext.h"
#undef BPP

#undef DOCLIP
#define DOCLIP 1

#define BPP 16
#include "toa_extratext.h"
#undef BPP
#define BPP 24
#include "toa_extratext.h"
#undef BPP
#define BPP 32
#include "toa_extratext.h"
#undef BPP

#undef DOCLIP

#undef ROWMODE

#undef ROT

#ifdef DRIVER_ROTATION
 #define ROT 270

 #define ROWMODE 0

 #define DOCLIP 0

 #define BPP 16
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 24
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 32
 #include "toa_extratext.h"
 #undef BPP

 #undef DOCLIP
 #define DOCLIP 1

 #define BPP 16
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 24
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 32
 #include "toa_extratext.h"
 #undef BPP

 #undef DOCLIP

 #undef ROWMODE
 #define ROWMODE 1

 #define DOCLIP 0

 #define BPP 16
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 24
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 32
 #include "toa_extratext.h"
 #undef BPP

 #undef DOCLIP
 #define DOCLIP 1

 #define BPP 16
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 24
 #include "toa_extratext.h"
 #undef BPP
 #define BPP 32
 #include "toa_extratext.h"
 #undef BPP

 #undef DOCLIP

 #undef ROWMODE

 #undef ROT
#endif

int ToaExtraTextLayer()
{
	if (nLastBPP != nBurnBpp ) {
		nLastBPP = nBurnBpp;

#ifdef DRIVER_ROTATION
		switch (nBurnBpp) {
			case 2:
				if (bRotatedScreen) {
					RenderTile[0] = *RenderTile16_ROT270_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile16_ROT270_CLIP_NORMAL;
					RenderTile[2] = *RenderTile16_ROT270_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile16_ROT270_CLIP_ROWSEL;
				} else {
					RenderTile[0] = *RenderTile16_ROT0_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile16_ROT0_CLIP_NORMAL;
					RenderTile[2] = *RenderTile16_ROT0_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile16_ROT0_CLIP_ROWSEL;
				}
				break;
			case 3:
				if (bRotatedScreen) {
					RenderTile[0] = *RenderTile24_ROT270_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile24_ROT270_CLIP_NORMAL;
					RenderTile[2] = *RenderTile24_ROT270_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile24_ROT270_CLIP_ROWSEL;
				} else {
					RenderTile[0] = *RenderTile24_ROT0_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile24_ROT0_CLIP_NORMAL;
					RenderTile[2] = *RenderTile24_ROT0_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile24_ROT0_CLIP_ROWSEL;
				}
				break;
			case 4:
				if (bRotatedScreen) {
					RenderTile[0] = *RenderTile32_ROT270_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile32_ROT270_CLIP_NORMAL;
					RenderTile[2] = *RenderTile32_ROT270_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile32_ROT270_CLIP_ROWSEL;
				} else {
					RenderTile[0] = *RenderTile32_ROT0_NOCLIP_NORMAL;
					RenderTile[1] = *RenderTile32_ROT0_CLIP_NORMAL;
					RenderTile[2] = *RenderTile32_ROT0_NOCLIP_ROWSEL;
					RenderTile[3] = *RenderTile32_ROT0_CLIP_ROWSEL;
				}
				break;
			default:
				return 1;
		}
#else
		switch (nBurnBpp) {
			case 2:
				RenderTile[0] = *RenderTile16_ROT0_NOCLIP_NORMAL;
				RenderTile[1] = *RenderTile16_ROT0_CLIP_NORMAL;
				RenderTile[2] = *RenderTile16_ROT0_NOCLIP_ROWSEL;
				RenderTile[3] = *RenderTile16_ROT0_CLIP_ROWSEL;
				break;
			case 3:
				RenderTile[0] = *RenderTile24_ROT0_NOCLIP_NORMAL;
				RenderTile[1] = *RenderTile24_ROT0_CLIP_NORMAL;
				RenderTile[2] = *RenderTile24_ROT0_NOCLIP_ROWSEL;
				RenderTile[3] = *RenderTile24_ROT0_CLIP_ROWSEL;
				break;
			case 4:
				RenderTile[0] = *RenderTile32_ROT0_NOCLIP_NORMAL;
				RenderTile[1] = *RenderTile32_ROT0_CLIP_NORMAL;
				RenderTile[2] = *RenderTile32_ROT0_NOCLIP_ROWSEL;
				RenderTile[3] = *RenderTile32_ROT0_CLIP_ROWSEL;
				break;
			default:
				return 1;
		}
#endif
	}

	unsigned int* pTextPalette = &ToaPalette[0x0400];
	unsigned char* pCurrentRow = pBurnBitmap;

	int nTileLeft = nBurnColumn << 3;
	int nTileDown = nBurnRow << 3;
	unsigned short* pTileRow;

	int nOffset, nLine, nStartX;
	int x, y, i;

#if 1
	y = 0;
	do {
		nLine = ((unsigned short*)ExtraTSelect)[y];
		nOffset = ((unsigned short*)ExtraTScroll)[y];

		if (y < 233) {
			for (i = 1; i < 8 && ((unsigned short*)ExtraTSelect)[y + i] == (nLine + i) && ((unsigned short*)ExtraTScroll)[y + i] == nOffset; i++) { }

			// draw whole tiles in one go
			if (i == 8) {

				nOffset += nExtraTXOffset;
				nStartX = (nOffset >> 3) & 0x3F;
				pTileRow = ((unsigned short*)ExtraTRAM) + ((nLine & (0x1F << 3)) << 3);
				nOffset &= 7;

				for (x = 0, pTile = pCurrentRow - nOffset * nBurnColumn; x < 41; x++, pTile += nTileLeft) {
					unsigned int nTile = pTileRow[(x + nStartX) & 0x3F];
					if (nTile && (nTile != 0x20)) {
						pTileData = ExtraTROM + ((nTile & 0x3FF) << 5);
						pTilePalette = &pTextPalette[((nTile >> 6) & 0x03F0)];
						if ((x == 0) || (x == 40)) {
							nTileXPos = 0 - nOffset + (x << 3);
							RenderTile[1]();
						} else {
							RenderTile[0]();
						}
					}
				}

				pCurrentRow += nTileDown;
				y += 8;
				continue;
			}
		}

		// Draw each line seperately

		nOffset += nExtraTXOffset;
		nStartX = (nOffset >> 3) & 0x3F;
		pTileRow = ((unsigned short*)ExtraTRAM) + ((nLine & (0x1F << 3)) << 3);
		nOffset &= 7;

		for (x = 0, pTile = pCurrentRow - nOffset * nBurnColumn; x < 41; x++, pTile += nTileLeft) {
			unsigned int nTile = pTileRow[(x + nStartX) & 0x3F];
			if (nTile && (nTile != 0x20)) {
				pTileData = ExtraTROM + ((nTile & 0x3FF) << 5) + ((nLine & 7) << 2);
				pTilePalette = &pTextPalette[((nTile >> 6) & 0x03F0)];
				if ((x == 0) || (x == 40)) {
					nTileXPos = 0 - nOffset + (x << 3);
					RenderTile[3]();
				} else {
					RenderTile[2]();
				}
			}
		}

		pCurrentRow += nBurnRow;
		y++;

	} while (y < 240);
#else
	pTileRow = (unsigned short*)(ExtraTRAM + ((nOffset >> 3) << 1));
	pCurrentRow = pBurnBitmap - (nOffset & 7) * nBurnColumn;

	nOffset = ((unsigned short*)(ExtraTScroll + nExtraTXOffset))[y];
	nStartX = nOffset >> 3;
	nOffset &= 7;

	for (y = 0; y < 30; y++, pCurrentRow += nTileDown, pTileRow += 0x40) {
		for (x = 0, pTile = pCurrentRow; x < 41; x++, pTile += nTileLeft) {
			unsigned int nTile = pTileRow[(x + nStartX) & 0x3F];
			if (nTile && (nTile != 0x20)) {
				pTileData = ExtraTROM + ((nTile & 0x3FF) << 5);
				pTilePalette = &pTextPalette[((nTile >> 6) & 0x03F0)];
				if ((x == 0) || (x == 40)) {
					nTileXPos = 0 - nOffset + (x << 3);
					RenderTile[1]();
				} else {
					RenderTile[0]();
				}
			}
		}
	}
#endif

	return 0;
}

int ToaExtraTextInit()
{
	if (nExtraTXOffset == 0x9999) {
		nExtraTXOffset = 0x2B;
	}

	return 0;
}

void ToaExtraTextExit()
{
	nExtraTXOffset = 0x9999;
}

