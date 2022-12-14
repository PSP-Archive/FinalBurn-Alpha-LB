#include "toaplan.h"

unsigned char* RomZ80;
unsigned char* RamZ80;

// Used to keep track of emulated CPU cycles
int ALIGN_DATA nCyclesDone[2], nCyclesTotal[2];
int nCyclesSegment;

int nToaCyclesScanline;
int nToaCyclesDisplayStart;
int nToaCyclesVBlankStart;

// Variables for the graphics drawing
bool bRotatedScreen;
unsigned char* pBurnBitmap;
int nBurnColumn;
int nBurnRow;

// This function loads interleaved code ROMs.
// All even roms should be first, followed by all odd ROMs.
int ToaLoadCode(unsigned char* pROM, int nStart, int nCount)
{
	nCount >>= 1;

	for (int nOdd = 0; nOdd < 2; nOdd++) {
		unsigned char* pLoad = pROM + (nOdd ^ 1);			// ^1 for byteswapped

		for (int i = 0; i < nCount; i++) {
			struct BurnRomInfo ri;
			// Load these even/odd bytes
			if (BurnLoadRom(pLoad, nStart + i, 2)) {
				return 1;
			}

			// Increment position by the length of the rom * 2
			ri.nLen = 0;
			BurnDrvGetRomInfo(&ri, nStart + i);
			pLoad += ri.nLen << 1;
		}
		nStart += nCount;
	}
	return 0;
}

// This function decodes the tile data for the GP9001 chip in place.
int ToaLoadGP9001Tiles(unsigned char* pDest, int nStart, int nNumFiles, int nROMSize, bool bSwap)	// bSwap = false
{
	unsigned char* pTile;
	int nSwap;

	for (int i = 0; i < (nNumFiles >> 1); i++) {
		BurnLoadRom(pDest + (i * 2 * nROMSize / nNumFiles), nStart + i, 2);
		BurnLoadRom(pDest + (i * 2 * nROMSize / nNumFiles) + 1, nStart + (nNumFiles >> 1) + i, 2);
	}

	BurnUpdateProgress(0.0, _T("Decoding graphics..."), 0);

	if (bSwap) {
		nSwap = 2;
	} else {
		nSwap = 0;
	}

	for (pTile = pDest; pTile < (pDest + nROMSize); pTile += 4) {
		unsigned char data[4];
		for (int n = 0; n < 4; n++) {
			int m = 7 - (n << 1);
			unsigned char nPixels = ((pTile[0 ^ nSwap] >> m) & 1) << 0;
			nPixels |= ((pTile[2 ^ nSwap] >> m) & 1) << 1;
			nPixels |= ((pTile[1 ^ nSwap] >> m) & 1) << 2;
			nPixels |= ((pTile[3 ^ nSwap] >> m) & 1) << 3;
			nPixels |= ((pTile[0 ^ nSwap] >> (m - 1)) & 1) << 4;
			nPixels |= ((pTile[2 ^ nSwap] >> (m - 1)) & 1) << 5;
			nPixels |= ((pTile[1 ^ nSwap] >> (m - 1)) & 1) << 6;
			nPixels |= ((pTile[3 ^ nSwap] >> (m - 1)) & 1) << 7;

			data[n] = nPixels;
		}

		for (int n = 0; n < 4; n++) {
			pTile[n] = data[n];
		}
	}
	return 0;
}

// This function fills the screen with the first palette entry
void ToaClearScreen()
{
#ifdef BUILD_PSP

	extern void clear_gui_texture(int color, UINT16 w, UINT16 h);
	
	unsigned int nColour = *ToaPalette;
	nColour = ((nColour & 0x001f ) << 3) | ((nColour & 0x07e0 ) << 5) | ((nColour & 0xf800 ) << 8);
	clear_gui_texture(nColour, 320, 240);
	
#else

	if (*ToaPalette) {
		switch (nBurnBpp) {
			case 4: {
				unsigned int* pClear = (unsigned int*)pBurnDraw;
				unsigned int nColour = *ToaPalette;
				for (int i = 0; i < 320 * 240 / 8; i++) {
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
				}
				break;
			}

			case 3: {
				unsigned char* pClear = pBurnDraw;
				unsigned char r = *ToaPalette;
				unsigned char g = (r >> 8) & 0xFF;
				unsigned char b = (r >> 16) & 0xFF;
				r &= 0xFF;
				for (int i = 0; i < 320 * 240; i++) {
					*pClear++ = r;
					*pClear++ = g;
					*pClear++ = b;
				}
				break;
			}

			case 2: {
				unsigned int* pClear = (unsigned int*)pBurnDraw;
				unsigned int nColour = *ToaPalette | *ToaPalette << 16;
				for (int i = 0; i < 320 * 240 / 16; i++) {
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
					*pClear++ = nColour;
				}
				break;
			}
		}
	} else {
		memset(pBurnDraw, 0, 320 * 240 * nBurnBpp);
	}
#endif
}

void ToaZExit()
{
	ZetExit();
}

void play_oki_sound(unsigned char game_sound, unsigned char data)
{
	unsigned int status = MSM6295ReadStatus(0);

//	logerror("Playing sample %02x from command %02x\n",game_sound,data);

	if (game_sound != 0)
	{
		if ((status & 0x01) == 0) {
			MSM6295Command(0, 0x80 | game_sound);
			MSM6295Command(0, 0x11);
		}
		else if ((status & 0x02) == 0) {
			MSM6295Command(0, 0x80 | game_sound);
			MSM6295Command(0, 0x21);
		}
		else if ((status & 0x04) == 0) {
			MSM6295Command(0, 0x80 | game_sound);
			MSM6295Command(0, 0x41);
		}
		else if ((status & 0x08) == 0) {
			MSM6295Command(0, 0x80 | game_sound);
			MSM6295Command(0, 0x81);
		}
	}
}

