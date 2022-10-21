#include "cave.h"

UINT8 *CavePalSrc;
UINT8 CaveRecalcPalette;	// Set to 1 to force recalculation of the entire palette

UINT32 *CavePalette = NULL;
static UINT16 *CavePalCopy = NULL;

INT8 CavePalInit(UINT16 nPalSize)
{
	CavePalette = (UINT32 *)memalign(4, nPalSize * sizeof(UINT32));
	memset(CavePalette, 0, nPalSize * sizeof(UINT32));

	CavePalCopy = (UINT16 *)memalign(4, nPalSize * sizeof(UINT16));
	memset(CavePalCopy, 0, nPalSize * sizeof(UINT16));

	return 0;
}

INT8 CavePalExit()
{
	free(CavePalette);
	CavePalette = NULL;

	free(CavePalCopy);
	CavePalCopy = NULL;

	return 0;
}

INT8 CavePalUpdate4Bit(UINT16 nOffset, UINT8 nNumPalettes)
{
	INT16 i, j;

	UINT16 *ps = (UINT16*)CavePalSrc + nOffset;
	UINT16 *pc;
	UINT32 *pd;

	UINT16 c;

	if (CaveRecalcPalette) {

		for (i = 0; i < 0 + nNumPalettes; i++) {

			pc = CavePalCopy + (i << 8);
			pd = CavePalette + (i << 8);

			for (j = 0; j < 16; j++, ps++, pc++, pd++) {

				c = *ps;
				*pc = c;
				*pd = CaveCalcCol(c);

			}
		}

		CaveRecalcPalette = 0;
		return 0;
	}


	for (i = 0; i < 0 + nNumPalettes; i++) {

		pc = CavePalCopy + (i << 8);
		pd = CavePalette + (i << 8);

		for (j = 0; j < 16; j++, ps++, pc++, pd++) {

			c = *ps;
			if (*pc != c) {
				*pc = c;
				*pd = CaveCalcCol(c);
			}

		}
	}

	return 0;
}

INT8 CavePalUpdate8Bit(UINT16 nOffset, UINT8 nNumPalettes)
{
	if (CaveRecalcPalette) {

		UINT16 *ps = (UINT16 *)CavePalSrc + nOffset;
		UINT16 *pc;
		UINT32* pd;

		UINT16 c;

		for (INT16 i = 0; i < nNumPalettes; i++) {

			pc = CavePalCopy + nOffset + (i << 8);
			pd = CavePalette + nOffset + (i << 8);

			for (INT16 j = 0; j < 256; j++, ps++, pc++, pd++) {

				c = *ps;
				*pc = c;
				*pd = CaveCalcCol(c);

			}
		}

		CaveRecalcPalette = 0;
	}

	return 0;
}

// Update the PC copy of the palette on writes to the palette memory
void CavePalWriteByte(UINT32 nAddress, UINT8 byteValue)
{
	nAddress ^= 1;
	CavePalSrc[nAddress] = byteValue;							// write byte

	if (*((UINT8 *)(CavePalCopy + nAddress)) != byteValue) {
		*((UINT8 *)(CavePalCopy + nAddress)) = byteValue;
		nAddress >>= 1;
		CavePalette[nAddress] = CaveCalcCol(*((UINT16 *)CavePalSrc + nAddress));
	}
}

void CavePalWriteWord(UINT32 nAddress, UINT16 wordValue)
{
	nAddress >>= 1;
	((UINT16 *)CavePalSrc)[nAddress] = wordValue;		// write word

	if (CavePalCopy[nAddress] != wordValue) {
		CavePalCopy[nAddress] = wordValue;
		CavePalette[nAddress] = CaveCalcCol(wordValue);
	}
}

