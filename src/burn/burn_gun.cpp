#include "burnint.h"
#include "burn_gun.h"

// Generic Light Gun support for FBA
// written by Barry Harris (Treble Winner) based on the code in Kev's opwolf driver

int nBurnGunNumPlayers = 0;
static bool bBurnGunDrawTargets = true;

static unsigned short nBurnGunMaxX = 0;
static unsigned short nBurnGunMaxY = 0;

int BurnGunX[MAX_GUNS];
int BurnGunY[MAX_GUNS];

#define P1Colour	0xfc, 0x12, 0xee
#define P2Colour	0x1c, 0xfc, 0x1c
#define P3Colour	0x15, 0x93, 0xfd
#define P4Colour	0xf7, 0xfa, 0x0e

#define a 0,
#define b 1,

unsigned char BurnGunTargetData[18][18] = {
	{ a a a a  a a a a  b a a a  a a a a  a a },
	{ a a a a  a a b b  b b b a  a a a a  a a },
	{ a a a a  b b a a  b a a b  b a a a  a a },
	{ a a a b  a a a a  b a a a  a b a a  a a },
	{ a a b a  a a a a  b a a a  a a b a  a a },
	{ a a b a  a a a b  b b a a  a a b a  a a },
	{ a b a a  a a b b  b b b a  a a a b  a a },
	{ a b a a  a b b a  a a a b  a a a b  a a },
	{ b b b b  b b b a  a a b b  b b b b  b a },
	{ a b a a  a b b a  a a a b  b a a b  a a },
	{ a b a a  a a b a  b a b b  a a a b  a a },
	{ a a b a  a a a b  b b b a  a a b a  a a },
	{ a a b a  a a a a  b b a a  a a b a  a a },
	{ a a a b  a a a a  b a a a  a b a a  a a },
	{ a a a a  b b a a  b a a b  b a a a  a a },
	{ a a a a  a a b b  b b b a  a a a a  a a },
	{ a a a a  a a a a  b a a a  a a a a  a a },
	{ a a a a  a a a a  a a a a  a a a a  a a },
};
#undef b
#undef a

unsigned char BurnGunReturnX(int num)
{
	if (num > MAX_GUNS - 1) return 0xff;

	float temp = (float)((BurnGunX[num] >> 8) + 8) / nBurnGunMaxX * 0xff;
	return (unsigned char)temp;
}

unsigned char BurnGunReturnY(int num)
{
	if (num > MAX_GUNS - 1) return 0xff;
	
	float temp = (float)((BurnGunY[num] >> 8) + 8) / nBurnGunMaxY * 0xff;
	return (unsigned char)temp;
}

void BurnGunMakeInputs(int num, short x, short y)
{
	if (num > MAX_GUNS - 1) return;
	
	const int MinX = (-8 << 8);
	const int MinY = (-8 << 8);
	
	BurnGunX[num] += x;
	BurnGunY[num] += y;
	
	if (BurnGunX[num] < MinX) BurnGunX[num] = MinX;
	if (BurnGunX[num] > MinX + (nBurnGunMaxX << 8)) BurnGunX[num] = MinX + (nBurnGunMaxX << 8);
	if (BurnGunY[num] < MinY) BurnGunY[num] = MinY;
	if (BurnGunY[num] > MinY + (nBurnGunMaxY << 8)) BurnGunY[num] = MinY + (nBurnGunMaxY << 8);
}
	
void BurnGunInit(int nNumPlayers, bool bDrawTargets)
{
	if (nNumPlayers > MAX_GUNS) nNumPlayers = MAX_GUNS;
	nBurnGunNumPlayers = nNumPlayers;
	bBurnGunDrawTargets = bDrawTargets;
	
	if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL) {
		BurnDrvGetVisibleSize(&nBurnGunMaxY, &nBurnGunMaxX);
	} else {
		BurnDrvGetVisibleSize(&nBurnGunMaxX, &nBurnGunMaxY);
	}
	
	for (int i = 0; i < MAX_GUNS; i++) {
		BurnGunX[i] = ((nBurnGunMaxX >> 1) - 7) << 8;
		BurnGunY[i] = ((nBurnGunMaxY >> 1) - 8) << 8;
	}
}

void BurnGunExit()
{
	nBurnGunNumPlayers = 0;
	bBurnGunDrawTargets = true;
	
	nBurnGunMaxX = 0;
	nBurnGunMaxY = 0;
	
	for (int i = 0; i < MAX_GUNS; i++) {
		BurnGunX[i] = 0;
		BurnGunY[i] = 0;
	}
}

void BurnGunScan()
{
	SCAN_VAR(BurnGunX);
	SCAN_VAR(BurnGunY);
}

void BurnGunDrawTarget(int num, int x, int y)
{
	if (bBurnGunDrawTargets == false) return;
	
	if (num > MAX_GUNS - 1) return;
	
#ifndef BUILD_PSP
	unsigned char *pTile = pBurnDraw + nBurnGunMaxX * nBurnBpp * (y - 1) + nBurnBpp * x;
#else
	unsigned char *pTile = pBurnDraw + ((y - 1) << 10) + (x << 1);
#endif
	
	unsigned int nTargetCol = 0;
	if (num == 0) nTargetCol = BurnHighCol(P1Colour, 0);
	if (num == 1) nTargetCol = BurnHighCol(P2Colour, 0);
	if (num == 2) nTargetCol = BurnHighCol(P3Colour, 0);
	if (num == 3) nTargetCol = BurnHighCol(P4Colour, 0);

	for (int y2 = 0; y2 <= 16; y2++) {
		
#ifndef BUILD_PSP
		pTile += (nBurnGunMaxX * nBurnBpp);
#else
		pTile += (2 << 9);
#endif
		
		if ((y + y2) < 0 || (y + y2) > nBurnGunMaxY - 1) {
			continue;
		}
		
		for (int x2 = 0; x2 <= 16; x2++) {
			
			if ((x + x2) < 0 || (x + x2) > nBurnGunMaxX - 1) {
				continue;
			}
			
			if (BurnGunTargetData[y2][x2]) {
				if (nBurnBpp == 2) {
					((unsigned short *)pTile)[x2] = (unsigned short)nTargetCol;
				} else {
					((unsigned int *)pTile)[x2] = nTargetCol;
				}
			}
		}
	}
}


#undef P1Colour
#undef P2Colour
#undef P3Colour
#undef P4Colour
