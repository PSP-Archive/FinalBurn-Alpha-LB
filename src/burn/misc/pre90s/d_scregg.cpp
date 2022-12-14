#include "tiles_generic.h"
#include "m6502.h"
#include "bitswap.h"
#include "driver.h"
extern "C" {
#include "ay8910.h"
}

static unsigned char *Mem, *Rom, *Gfx0, *Gfx1, *Prom;
static unsigned char ALIGN_DATA DrvJoy1[8], DrvJoy2[8], DrvDips[2], DrvReset;
static unsigned int *Palette, *DrvPalette;
static unsigned char DrvRecalcPal = 0;
static short *pFMBuffer, *pAY8910Buffer[6];

static int flipscreen = 0;
static int VBLK = 0;

static int scregg = 0, rockduck = 0;

static struct BurnInputInfo DrvInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 6,	"p1 coin"  },
	{"P1 start"  ,    BIT_DIGITAL  , DrvJoy2 + 6,	"p1 start" },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 0, 	"p1 right" },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 1, 	"p1 left"  },
	{"P1 Up",	  BIT_DIGITAL,   DrvJoy1 + 2,   "p1 up",   },
	{"P1 Down",	  BIT_DIGITAL,   DrvJoy1 + 3,   "p1 down", },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 4,	"p1 fire 1"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy1 + 7,	"p2 coin"  },
	{"P2 start"  ,    BIT_DIGITAL  , DrvJoy2 + 7,	"p2 start" },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 0, 	"p2 right" },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 1, 	"p2 left"  },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy2 + 2,   "p2 up",   },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy2 + 3,   "p2 down", },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 4,	"p2 fire 1"},

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,	"dip 1"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,	"dip 2"	   },
};

STDINPUTINFO(Drv);

static struct BurnDIPInfo DrvDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0x7f, NULL                     },

	{0   , 0xfe, 0   , 4   , "Coin A"                 },
	{0x0f, 0x01, 0x03, 0x00, "2C 1C"     		  },
	{0x0f, 0x01, 0x03, 0x03, "1C 1C"    		  },
	{0x0f, 0x01, 0x03, 0x01, "1C 2C"     		  },
	{0x0f, 0x01, 0x03, 0x02, "1C 3C"    		  },

	{0   , 0xfe, 0   , 4   , "Coin B"                 },
	{0x0f, 0x01, 0x0c, 0x00, "2C 1C"     		  },
	{0x0f, 0x01, 0x0c, 0x0c, "1C 3C"    		  },
	{0x0f, 0x01, 0x0c, 0x04, "1C 2C"     		  },
	{0x0f, 0x01, 0x0c, 0x08, "1C 3C"    		  },

	{0   , 0xfe, 0   , 2   , "Cabinet"                },
	{0x0f, 0x01, 0x40, 0x40, "Cocktail"       	  },
	{0x0f, 0x01, 0x40, 0x00, "Upright"     		  },

	// Default Values
	{0x10, 0xff, 0xff, 0xff, NULL                     },

	{0   , 0xfe, 0   , 2   , "Lives"                  },
	{0x10, 0x01, 0x01, 0x01, "3"     		  },
	{0x10, 0x01, 0x01, 0x00, "5"    		  },

	{0   , 0xfe, 0   , 4   , "Bonus Life"             },
	{0x10, 0x01, 0x06, 0x04, "30000"     		  },
	{0x10, 0x01, 0x06, 0x02, "50000"    		  },
	{0x10, 0x01, 0x06, 0x06, "70000"     		  },
	{0x10, 0x01, 0x06, 0x00, "Never"    		  },

	{0   , 0xfe, 0   , 2   , "Difficulty"             },
	{0x10, 0x01, 0x80, 0x80, "Easy"     		  },
	{0x10, 0x01, 0x80, 0x00, "Hard"    		  },
};

STDDIPINFO(Drv);

inline static int calc_mirror_offset(unsigned short address)
{
	int x, y, offset;
	offset = address & 0x3ff;

	x = offset >> 5;
	y = offset & 0x1f;

	return ((y << 5) | x);
}

unsigned char eggs_readmem(unsigned short address)
{
	unsigned char ret = 0xff;

	if ((address & 0xf800) == 0x1800) {
    		return Rom[0x2000 + (address & 0x400) + calc_mirror_offset(address)];
	}

	switch (address)
	{
		case 0x2000:
			return VBLK | DrvDips[0];

		case 0x2001:
			return DrvDips[1];

		case 0x2002:
		{
			for (int i = 0; i < 8; i++) ret ^= DrvJoy1[i] << i;

			return ret;
		}

		case 0x2003:
		{
			for (int i = 0; i < 8; i++) ret ^= DrvJoy2[i] << i;

			return ret;
		}
	}

	return 0;
}

void eggs_writemem(unsigned short address, unsigned char data)
{
	if ((address & 0xf800) == 0x1800) {
    		Rom[0x2000 + (address & 0x400) + calc_mirror_offset(address)] = data;
		return;
	}

	switch (address)
	{
		case 0x2000:
			flipscreen = data & 1;
		break;

		case 0x2001:
		break;

		case 0x2004:
		case 0x2005:
		case 0x2006:
		case 0x2007:
			AY8910Write((address >> 1) & 1, address & 1, data);
		break;
	}
}

unsigned char dommy_readmem(unsigned short address)
{
	unsigned char ret = 0xff;

	if (address >= 0x2800 && address <= 0x2bff) {   
    		return Rom[0x2000 + calc_mirror_offset(address)];
	}

	switch (address)
	{
		case 0x4000:
			return DrvDips[0] | VBLK;

		case 0x4001:
			return DrvDips[1];

		case 0x4002:
		{
			for (int i = 0; i < 8; i++) ret ^= DrvJoy1[i] << i;

			return ret;
		}

		case 0x4003:
		{
			for (int i = 0; i < 8; i++) ret ^= DrvJoy2[i] << i;

			return ret;
		}
	}

	return 0;
}

void dommy_writemem(unsigned short address, unsigned char data)
{
	if (address >= 0x2800 && address <= 0x2bff) {   
    		Rom[0x2000 + calc_mirror_offset(address)] = data;
		return;
	}

	switch (address)
	{
		case 0x4000:
		break;

		case 0x4001:
			flipscreen = data & 1;
		break;

		case 0x4004:
		case 0x4005:
		case 0x4006:
		case 0x4007:
			AY8910Write((address >> 1) & 1, address & 1, data);
		break;
	}
}


static int DrvDoReset()
{
	memset (Rom, 0, 0x3000);

	flipscreen = 0;

	m6502Open(0);
	m6502Reset();
	m6502Close();

	AY8910Reset(0);
	AY8910Reset(1);

	return 0;
}


static int scregg_gfx_convert()
{
	static int PlaneOffsets[3]   = { 0x20000, 0x10000, 0 };
	static int XOffsets[16]      = { 128, 129, 130, 131, 132, 133, 134, 135, 0, 1, 2, 3, 4, 5, 6, 7 };
	static int YOffsets[16]      = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120 };

	unsigned char *tmp = (unsigned char*)malloc(0x6000);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, Gfx0, 0x6000);

	GfxDecode(0x400, 3,  8,  8, PlaneOffsets, XOffsets + 8, YOffsets, 0x040, tmp, Gfx0);
	GfxDecode(0x100, 3, 16, 16, PlaneOffsets, XOffsets + 0, YOffsets, 0x100, tmp, Gfx1);

	free (tmp);

	return 0;
}

static int scregg_palette_init()
{
	for (int i = 0;i < 8;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		bit0 = (Prom[i] >> 0) & 0x01;
		bit1 = (Prom[i] >> 1) & 0x01;
		bit2 = (Prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (Prom[i] >> 3) & 0x01;
		bit1 = (Prom[i] >> 4) & 0x01;
		bit2 = (Prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = 0;
		bit1 = (Prom[i] >> 6) & 0x01;
		bit2 = (Prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		Palette[i] = (r << 16) | (g << 8) | b;
	}

	return 0;
}

static int DrvInit()
{
	Mem = (unsigned char*)malloc(0x10000 + 0x10000 + 0x10000 + 0x20 + 0x40);
	if (Mem == NULL) {
		return 1;
	}
	memset(Mem, 0, (0x10000 + 0x10000 + 0x10000 + 0x20 + 0x40));

	pFMBuffer = (short *)malloc (nBurnSoundLen * 6 * sizeof(short));
	if (pFMBuffer == NULL) {
		return 1;
	}
	memset(pFMBuffer, 0, nBurnSoundLen * 6 * sizeof(short));

	Rom  = Mem + 0x00000;
	Gfx0 = Mem + 0x10000;
	Gfx1 = Mem + 0x20000;
	Prom = Mem + 0x30000;
	Palette    = (unsigned int*)(Mem + 0x30020);
	DrvPalette = (unsigned int*)(Mem + 0x30040);

	if (scregg)
	{
		if (rockduck) {
			if (BurnLoadRom(Rom  + 0x4000, 0, 1)) return 1;
			if (BurnLoadRom(Rom  + 0x6000, 1, 1)) return 1;
			if (BurnLoadRom(Rom  + 0x8000, 2, 1)) return 1;
			memcpy (Rom + 0x3000, Rom + 0x5000, 0x1000);
			memcpy (Rom + 0x5000, Rom + 0x7000, 0x1000);
			memcpy (Rom + 0xe000, Rom + 0x8000, 0x2000);
			memcpy (Rom + 0x7000, Rom + 0x9000, 0x1000);

			if (BurnLoadRom(Gfx0 + 0x0000, 3, 1)) return 1;
			if (BurnLoadRom(Gfx0 + 0x2000, 4, 1)) return 1;
			if (BurnLoadRom(Gfx0 + 0x4000, 5, 1)) return 1;

			if (BurnLoadRom(Prom + 0x0000, 6, 1)) return 1;

			for (int i = 0x2000; i < 0x6000; i++)
				Gfx0[i] = BITSWAP08(Gfx0[i],2,0,3,6,1,4,7,5);

		} else {
			for (int i = 0; i < 5; i++)
				if (BurnLoadRom(Rom + 0x3000 + i * 0x1000, i, 1)) return 1;

			memcpy (Rom + 0xf000, Rom + 0x7000, 0x1000);

			for (int i = 0; i < 6; i++)
				if (BurnLoadRom(Gfx0 + i * 0x1000, i + 5, 1)) return 1;

			if (BurnLoadRom(Prom, 11, 1)) return 1;
		}

		m6502Init(1);
		m6502Open(0);
		m6502SetReadHandler(eggs_readmem);
		m6502SetWriteHandler(eggs_writemem);
		m6502MapMemory(Rom + 0x0000, 0x0000, 0x07ff, M6502_RAM);
		m6502MapMemory(Rom + 0x2000, 0x1000, 0x17ff, M6502_RAM);
		m6502MapMemory(Rom + 0x3000, 0x3000, 0x7fff, M6502_ROM);
		m6502MapMemory(Rom + 0xf000, 0xf000, 0xffff, M6502_ROM);
		m6502Close();
	} else {
		for (int i = 0; i < 3; i++) {
			if (BurnLoadRom(Rom  + 0xa000 + i * 0x2000,     i, 1)) return 1;
			if (BurnLoadRom(Gfx0 + 0x0000 + i * 0x2000, 3 + i, 1)) return 1;
		}

		if (BurnLoadRom(Prom, 6, 1)) return 1;
		memcpy (Prom, Prom + 0x18, 8);

		m6502Init(1);
		m6502Open(0);
		m6502SetReadHandler(dommy_readmem);
		m6502SetWriteHandler(dommy_writemem);
		m6502MapMemory(Rom + 0x0000, 0x0000, 0x07ff, M6502_RAM);
		m6502MapMemory(Rom + 0x2000, 0x2000, 0x27ff, M6502_RAM);
		m6502MapMemory(Rom + 0xa000, 0xa000, 0xffff, M6502_ROM);
		m6502Close();
	}

	scregg_gfx_convert();
	scregg_palette_init();

//	BurnSetRefreshRate(57); // Proper rate, but the AY8910 can't handle it

	GenericTilesInit();

	pAY8910Buffer[0] = pFMBuffer + nBurnSoundLen * 0;
	pAY8910Buffer[1] = pFMBuffer + nBurnSoundLen * 1;
	pAY8910Buffer[2] = pFMBuffer + nBurnSoundLen * 2;
	pAY8910Buffer[3] = pFMBuffer + nBurnSoundLen * 3;
	pAY8910Buffer[4] = pFMBuffer + nBurnSoundLen * 4;
	pAY8910Buffer[5] = pFMBuffer + nBurnSoundLen * 5;

	AY8910Init(0, 1500000, nBurnSoundRate, NULL, NULL, NULL, NULL);
	AY8910Init(1, 1500000, nBurnSoundRate, NULL, NULL, NULL, NULL);

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	m6502Exit();
	AY8910Exit(0);
	AY8910Exit(1);
	GenericTilesExit();

	free (Mem);
	DrvRecalcPal = 0;
	flipscreen = 0;

	scregg = rockduck = 0;
	VBLK = 0;

	return 0;
}

static void draw_chars()
{
	for (int offs = 0; offs < 0x400; offs++)
	{
		int sx = (~offs >> 2) & 0xf8;
		int sy = ( offs & 0x1f) << 3;

        	unsigned short code = Rom[0x2000 | offs] | ((Rom[0x2400 | offs] & 3) << 8);

		if (flipscreen)
		{
			sx ^= 0xf8;
			sy ^= 0xf8;
		}

			    sy -= 8;
		if (scregg) sx -= 8;

		if (flipscreen) {
			Render8x8Tile_FlipXY_Clip(pTransDraw, code, sx, sy, 0, 3, 0, Gfx0);
		} else {
			Render8x8Tile_Clip(pTransDraw, code, sx, sy, 0, 3, 0, Gfx0);
		}
	}
}

static void draw_sprites()
{
	for (int i = 0, offs = 0; i < 8; i++, offs += 4*0x20)
	{
		int sx, sy, code;
		unsigned char flipx,flipy;

		if (!(Rom[0x2000 + offs] & 0x01)) continue;

		sx = 240 - Rom[0x2000 + offs + 0x60];
		sy = 240 - Rom[0x2000 + offs + 0x40];

		flipx = Rom[0x2000 + offs] & 0x04;
		flipy = Rom[0x2000 + offs] & 0x02;

		code = Rom[0x2000 + offs + 0x20];

		if (flipscreen)
		{
			sx = 240 - sx;
			sy = 240 - sy;

			flipx = !flipx;
			flipy = !flipy;
		}

			    sy -= 8;
		if (scregg) sx -= 8;

		if (flipy) {
			if (flipx) {
				Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy, 0, 3, 0, 0, Gfx1);
			} else {
				Render16x16Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy, 0, 3, 0, 0, Gfx1);
			}
		} else {
			if (flipx) {
				Render16x16Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy, 0, 3, 0, 0, Gfx1);
			} else {
				Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, 0, 3, 0, 0, Gfx1);
			}
		}
	}
}

static int DrvDraw()
{
	// check to see if we need to recalculate the palette
	if (DrvRecalcPal) {
		for (int i = 0; i < 8; i++) {
			int col = Palette[i];
			DrvPalette[i] = BurnHighCol(col >> 16, col >> 8, col, 0);
		}
	}

	draw_chars();
	draw_sprites();

	BurnTransferCopy(DrvPalette);

	return 0;
}

inline static void scregg_interrupt_handler(int scanline)
{
	if (scanline == 0)
		VBLK = 0;

	if (scanline == 14)
		VBLK = 0x80;

	m6502SetIRQ(M6502_IRQ);
}

static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	int nTotalCycles = (int)((float)(1500000 / 57));
	int nCyclesRun = 0;

	m6502Open(0);

	for (int i = 0; i < 16; i++) {
		nCyclesRun += m6502Run((nTotalCycles - nCyclesRun) / (16 - i));
		scregg_interrupt_handler(i);
	}

	m6502Close();

	if (pBurnSoundOut) {
		int nSample;
		int nSegmentLength = nBurnSoundLen;
		short* pSoundBuf = pBurnSoundOut;
		if (nSegmentLength) {
			AY8910Update(0, &pAY8910Buffer[0], nSegmentLength);
			AY8910Update(1, &pAY8910Buffer[3], nSegmentLength);
			for (int n = 0; n < nSegmentLength; n++) {
				nSample  = pAY8910Buffer[0][n] >> 2;
				nSample += pAY8910Buffer[1][n] >> 2;
				nSample += pAY8910Buffer[2][n] >> 2;
				nSample += pAY8910Buffer[3][n] >> 2;
				nSample += pAY8910Buffer[4][n] >> 2;
				nSample += pAY8910Buffer[5][n] >> 2;

				if (nSample < -32768) {
					nSample = -32768;
				} else {
					if (nSample > 32767) {
						nSample = 32767;
					}
				}

				pSoundBuf[(n << 1) + 0] = nSample;
				pSoundBuf[(n << 1) + 1] = nSample;
 			}
		}
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}


static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029521;
	}

	if (nAction & ACB_VOLATILE) {		
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = Rom + 0x0000;
		ba.nLen	  = 0x3000;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		m6502Scan(nAction);
		AY8910Scan(nAction, pnMin);

		// Scan critical driver variables
		SCAN_VAR(flipscreen);
	}

	return 0;
}


// Dommy

static struct BurnRomInfo dommyRomDesc[] = {
	{ "dommy.e01",  0x2000, 0x9ae064ed, 1 | BRF_ESS | BRF_PRG }, //  0 M6502 Code
	{ "dommy.e11",  0x2000, 0x7c4fad5c, 1 | BRF_ESS | BRF_PRG }, //  1
	{ "dommy.e21",  0x2000, 0xcd1a4d55, 1 | BRF_ESS | BRF_PRG }, //  2

	{ "dommy.e50",  0x2000, 0x5e9db0a4, 2 | BRF_GRA },	     //  3 Graphics
	{ "dommy.e40",  0x2000, 0x4d1c36fb, 2 | BRF_GRA },	     //  4
	{ "dommy.e30",  0x2000, 0x4e68bb12, 2 | BRF_GRA },	     //  5

	{ "dommy.e70",  0x0020, 0x50c1d86e, 3 | BRF_GRA },	     //  6 Palette

	{ "dommy.e60",  0x0020, 0x24da2b63, 4 | BRF_OPT },	     //  7
};

STD_ROM_PICK(dommy);
STD_ROM_FN(dommy);

struct BurnDriver BurnDrvdommy = {
	"dommy", NULL, NULL, "198?",
	"Dommy\0", NULL, "Technos", "misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_MAZE, 0,
	NULL, dommyRomInfo, dommyRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalcPal,
	240, 248, 3, 4
};


// Scrambled Egg

static struct BurnRomInfo screggRomDesc[] = {
	{ "scregg.e14",   0x1000, 0x29226d77, 1 | BRF_ESS | BRF_PRG }, //  0 M6502 Code
	{ "scregg.d14",   0x1000, 0xeb143880, 1 | BRF_ESS | BRF_PRG }, //  1
	{ "scregg.c14",   0x1000, 0x4455f262, 1 | BRF_ESS | BRF_PRG }, //  2
	{ "scregg.b14",   0x1000, 0x044ac5d2, 1 | BRF_ESS | BRF_PRG }, //  3
	{ "scregg.a14",   0x1000, 0xb5a0814a, 1 | BRF_ESS | BRF_PRG }, //  4

	{ "scregg.j12",   0x1000, 0xa485c10c, 2 | BRF_GRA },	       //  5 Graphics
	{ "scregg.j10",   0x1000, 0x1fd4e539, 2 | BRF_GRA },	       //  6
	{ "scregg.h12",   0x1000, 0x8454f4b2, 2 | BRF_GRA },	       //  7
	{ "scregg.h10",   0x1000, 0x72bd89ee, 2 | BRF_GRA },	       //  8
	{ "scregg.g12",   0x1000, 0xff3c2894, 2 | BRF_GRA },	       //  9
	{ "scregg.g10",   0x1000, 0x9c20214a, 2 | BRF_GRA },	       // 10

	{ "screggco.c6",  0x0020, 0xff23bdd6, 3 | BRF_GRA },	       // 11 Palette

	{ "screggco.b4",  0x0020, 0x7cc4824b, 0 | BRF_OPT },	       // 12
};

STD_ROM_PICK(scregg);
STD_ROM_FN(scregg);

static int screggInit()
{
	scregg = 1;

	return DrvInit();
}

struct BurnDriver BurnDrvscregg = {
	"scregg", NULL, NULL, "1983",
	"Scrambled Egg\0", NULL, "Technos", "misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_MAZE, 0,
	NULL, screggRomInfo, screggRomName, DrvInputInfo, DrvDIPInfo,
	screggInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalcPal,
	240, 240, 3, 4
};


// Eggs

static struct BurnRomInfo eggsRomDesc[] = {
	{ "e14.bin",      0x1000, 0x4e216f9d, 1 | BRF_ESS | BRF_PRG }, //  0 M6502 Code
	{ "d14.bin",      0x1000, 0x4edb267f, 1 | BRF_ESS | BRF_PRG }, //  1
	{ "c14.bin",      0x1000, 0x15a5c48c, 1 | BRF_ESS | BRF_PRG }, //  2
	{ "b14.bin",      0x1000, 0x5c11c00e, 1 | BRF_ESS | BRF_PRG }, //  3
	{ "a14.bin",      0x1000, 0x953faf07, 1 | BRF_ESS | BRF_PRG }, //  4

	{ "j12.bin",      0x1000, 0xce4a2e46, 2 | BRF_GRA },	       //  5 Graphics
	{ "j10.bin",      0x1000, 0xa1bcaffc, 2 | BRF_GRA },	       //  6
	{ "h12.bin",      0x1000, 0x9562836d, 2 | BRF_GRA },	       //  7
	{ "h10.bin",      0x1000, 0x3cfb3a8e, 2 | BRF_GRA },	       //  8
	{ "g12.bin",      0x1000, 0x679f8af7, 2 | BRF_GRA },	       //  9
	{ "g10.bin",      0x1000, 0x5b58d3b5, 2 | BRF_GRA },	       // 10

	{ "eggs.c6",      0x0020, 0xe8408c81, 3 | BRF_GRA },	       // 11 Palette

	{ "screggco.b4",  0x0020, 0x7cc4824b, 0 | BRF_OPT },	       // 12
};

STD_ROM_PICK(eggs);
STD_ROM_FN(eggs);

struct BurnDriver BurnDrveggs = {
	"eggs", "scregg", NULL, "1983",
	"Eggs\0", NULL, "[Technos] Universal USA", "misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_MAZE, 0,
	NULL, eggsRomInfo, eggsRomName, DrvInputInfo, DrvDIPInfo,
	screggInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalcPal,
	240, 240, 3, 4
};


// Rock Duck (prototype?)

static struct BurnRomInfo rockduckRomDesc[] = {
	{ "rde.bin",	  0x2000, 0x56e2a030, 1 | BRF_ESS | BRF_PRG }, //  0 M6502 Code
	{ "rdc.bin",	  0x2000, 0x482d9a0c, 1 | BRF_ESS | BRF_PRG }, //  1
	{ "rdb.bin",	  0x2000, 0x974626f2, 1 | BRF_ESS | BRF_PRG }, //  2


	{ "rd3.rdg",	  0x2000, 0x8a3f1e53, 2 | BRF_GRA },	       //  3 Graphics
	{ "rd2.rdh",	  0x2000, 0xe94e673e, 2 | BRF_GRA },	       //  4
	{ "rd1.rdj",	  0x2000, 0x654afff2, 2 | BRF_GRA },	       //  5

	{ "eggs.c6",      0x0020, 0xe8408c81, 3 | BRF_GRA },  //  6 Palette 
};

STD_ROM_PICK(rockduck);
STD_ROM_FN(rockduck);

static int rockduckInit()
{
	scregg = 1;
	rockduck = 1;

	return DrvInit();
}

struct BurnDriver BurnDrvrockduck = {
	"rockduck", NULL, NULL, "1983",
	"Rock Duck (prototype?)\0", "incorrect colors", "Datel SAS", "misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_MAZE, 0,
	NULL, rockduckRomInfo, rockduckRomName, DrvInputInfo, DrvDIPInfo,
	rockduckInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalcPal,
	240, 240, 3, 4
};
