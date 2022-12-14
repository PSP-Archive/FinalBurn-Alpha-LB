// FB Alpha Block Out driver module
// Based on MAME driver by Nicola Salmoria

#include "tiles_generic.h"
#include "burn_ym2151.h"
#include "msm6295.h"

static unsigned char *AllMem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *Drv68KROM;
static unsigned char *DrvZ80ROM;
static unsigned char *DrvSndROM;
static unsigned char *Drv68KRAM0;
static unsigned char *Drv68KRAM1;
static unsigned char *Drv68KRAM2;
static unsigned char *DrvZ80RAM;
static unsigned char *DrvPalRAM;
static unsigned char *DrvVidRAM0;
static unsigned char *DrvVidRAM1;
static unsigned short*DrvTmpBmp;
static unsigned int  *DrvPalette;

static unsigned char *soundlatch;

static unsigned char DrvRecalc;

static unsigned char DrvJoy1[16];
static unsigned char DrvJoy2[16];
static unsigned char DrvJoy3[16];
static unsigned char DrvJoy5[16];
static unsigned char DrvDips[2];
static unsigned char DrvReset;
static unsigned char DrvInputs[5];

static struct BurnInputInfo BlockoutInputList[] = {
	{"Coin 1",		BIT_DIGITAL,	DrvJoy3 + 1,	"p1 coin"  },
	{"Coin 2",		BIT_DIGITAL,	DrvJoy3 + 2,	"p2 coin"  },
	{"Coin 3",		BIT_DIGITAL,	DrvJoy3 + 3,	"p3 coin"  },

	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 7,	"p1 start" },
	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 up"    },
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 down"  },
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 left"  },
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 right" },
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy5 + 6,	"p1 fire 1"},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},
	{"P1 Button 4",		BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 4"},

	{"P2 Start",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 start" },
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 up"    },
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 3,	"p2 down"  },
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 left"  },
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 right" },
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy5 + 7,	"p2 fire 1"},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},
	{"P2 Button 4",		BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 4"},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"    },
	{"Dip 1",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"      },
	{"Dip 2",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"      },
};

STDINPUTINFO(Blockout);

static struct BurnInputInfo BlckoutjInputList[] = {
	{"Coin 1",		BIT_DIGITAL,	DrvJoy3 + 1,	"p1 coin"  },
	{"Coin 2",		BIT_DIGITAL,	DrvJoy3 + 2,	"p2 coin"  },
	{"Coin 3",		BIT_DIGITAL,	DrvJoy3 + 3,	"p3 coin"  },

	{"P1 Start",		BIT_DIGITAL,	DrvJoy1 + 7,	"p1 start" },
	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 up"    },
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 3,	"p1 down"  },
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 left"  },
	{"P1 Right",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 right" },
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},
	{"P1 Button 4",		BIT_DIGITAL,	DrvJoy5 + 6,	"p1 fire 4"},

	{"P2 Start",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 start" },
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 up"    },
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 3,	"p2 down"  },
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 left"  },
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 right" },
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},
	{"P2 Button 4",		BIT_DIGITAL,	DrvJoy5 + 7,	"p2 fire 4"},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"    },
	{"Dip 1",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"      },
	{"Dip 2",		BIT_DIPSWITCH,	DrvDips + 1,	"dip"      },
};

STDINPUTINFO(Blckoutj);

static struct BurnDIPInfo BlockoutDIPList[]=
{
	// Default Values
	{0x16, 0xff, 0xff, 0xff, NULL			},
	{0x17, 0xff, 0xff, 0x3f, NULL			},

	{0   , 0xfe, 0   ,    4, "Coinage"		},
	{0x16, 0x01, 0x03, 0x00, "3 Coins 1 Credits "	},
	{0x16, 0x01, 0x03, 0x01, "2 Coins 1 Credits "	},
	{0x16, 0x01, 0x03, 0x03, "1 Coin 1 Credits "	},
	{0x16, 0x01, 0x03, 0x02, "1 Coin 2 Credits "	},

	{0   , 0xfe, 0   ,    2, "1 Coin to Continue"	},
	{0x16, 0x01, 0x10, 0x10, "Off"			},
	{0x16, 0x01, 0x10, 0x00, "On"			},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x16, 0x01, 0x20, 0x00, "Off"			},
	{0x16, 0x01, 0x20, 0x20, "On"			},

	{0   , 0xfe, 0   ,    2, "Difficulty"		},
	{0x17, 0x01, 0x03, 0x02, "Easy"			},
	{0x17, 0x01, 0x03, 0x03, "Normal"		},
	{0x17, 0x01, 0x03, 0x01, "Hard"			},
	{0x17, 0x01, 0x03, 0x00, "Very Hard"		},

	{0   , 0xfe, 0   ,    2, "Rotate Buttons"	},
	{0x17, 0x01, 0x04, 0x00, "2"			},
	{0x17, 0x01, 0x04, 0x04, "3"			},
};

STDDIPINFO(Blockout);

static struct BurnDIPInfo AgressDIPList[]=
{
	// Default Values
	{0x16, 0xff, 0xff, 0xff, NULL			},
	{0x17, 0xff, 0xff, 0x3f, NULL			},

	{0   , 0xfe, 0   ,    4, "Coinage"		},
	{0x16, 0x01, 0x03, 0x00, "3 Coins 1 Credits "	},
	{0x16, 0x01, 0x03, 0x01, "2 Coins 1 Credits "	},
	{0x16, 0x01, 0x03, 0x03, "1 Coin 1 Credits "	},
	{0x16, 0x01, 0x03, 0x02, "1 Coin 2 Credits "	},

	{0   , 0xfe, 0   ,    2, "Opening Cut"		},
	{0x16, 0x01, 0x04, 0x00, "Off"			},
	{0x16, 0x01, 0x04, 0x04, "On"			},

	{0   , 0xfe, 0   ,    2, "Demo Sounds"		},
	{0x16, 0x01, 0x20, 0x00, "Off"			},
	{0x16, 0x01, 0x20, 0x20, "On"			},

	{0   , 0xfe, 0   ,    2, "Difficulty"		},
	{0x17, 0x01, 0x03, 0x02, "Easy"			},
	{0x17, 0x01, 0x03, 0x03, "Normal"		},
	{0x17, 0x01, 0x03, 0x01, "Hard"			},
	{0x17, 0x01, 0x03, 0x00, "Very Hard"		},

	{0   , 0xfe, 0   ,    2, "Players"		},
	{0x17, 0x01, 0x04, 0x04, "1"			},
	{0x17, 0x01, 0x04, 0x00, "2"			},
};

STDDIPINFO(Agress);

static void palette_write(int offset)
{
	unsigned short rgb = *((unsigned short*)(DrvPalRAM + offset));

	int bit0,bit1,bit2,bit3;
	int r,g,b;

	bit0 = (rgb >> 0) & 0x01;
	bit1 = (rgb >> 1) & 0x01;
	bit2 = (rgb >> 2) & 0x01;
	bit3 = (rgb >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	bit0 = (rgb >> 4) & 0x01;
	bit1 = (rgb >> 5) & 0x01;
	bit2 = (rgb >> 6) & 0x01;
	bit3 = (rgb >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	bit0 = (rgb >> 8) & 0x01;
	bit1 = (rgb >> 9) & 0x01;
	bit2 = (rgb >> 10) & 0x01;
	bit3 = (rgb >> 11) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	DrvPalette[offset/2] = BurnHighCol(r, g, b, 0);
}

static void update_pixels(int offset)
{
	int x = (offset & 0xff);
	int y = (offset >> 8) & 0xff;
	if (x >= 320 || y < 8 || y > 247) return;

	unsigned short *src = (unsigned short*)DrvVidRAM0 + ((y << 8) | x);
	unsigned short *dst = DrvTmpBmp + (y-8) * 320 + x * 2;

	int front = src[0x00000];
	int back  = src[0x10000];

	if (front >> 8)   dst[0] = front >> 8;
	else              dst[0] = (back >> 8) | 0x100;

	if (front & 0xff) dst[1] = front & 0xff;
	else              dst[1] = (back & 0xff) | 0x100;
}

void __fastcall blockout_write_byte(unsigned int address, unsigned char data)
{
	if (address >= 0x180000 && address <= 0x1bffff) {
		address = (address & 0x3ffff) ^ 1;
		DrvVidRAM0[address] = data;
		update_pixels(address>>1);
		return;
	}

	switch (address ^ 1)
	{
		case 0x100014:
			*soundlatch = data & 0xff;
			ZetNmi();
		return;
	}

	return;
}

void __fastcall blockout_write_word(unsigned int address, unsigned short data)
{
	if (address >= 0x280200 && address <= 0x2805ff) {
		*((unsigned short*)(DrvPalRAM + (address - 0x280200))) = data;
		palette_write(address & 0x3fe);
		return;
	}

	if (address >= 0x180000 && address <= 0x1bffff) {
		address &= 0x3fffe;
		*((unsigned short*)(DrvVidRAM0 + address)) = data;
		update_pixels(address>>1);
		return;
	}

	switch (address)
	{
		case 0x100014:
			*soundlatch = data & 0xff;
			ZetNmi();
		return;

		case 0x280002: // front color
			*((unsigned short*)(DrvPalRAM + 0x400)) = data;
			palette_write(0x400);
		return;
	}

	return;
}

unsigned char __fastcall blockout_read_byte(unsigned int address)
{
	switch (address^1)
	{
		case 0x100000:
			return DrvInputs[0];

		case 0x100002:
			return DrvInputs[1];

		case 0x100004:
			return DrvInputs[2] & 0x0e;

		case 0x100006:
			return DrvDips[0];

		case 0x100008:
			return ((DrvDips[1] & 0x3f) | (DrvInputs[4] & 0xc0));
	}

	return 0;
}

unsigned short __fastcall blockout_read_word(unsigned int address)
{
	switch (address)
	{
		case 0x100000:
			return DrvInputs[0];

		case 0x100002:
			return DrvInputs[1];

		case 0x100004:
			return DrvInputs[2] & 0x0e;

		case 0x100006:
			return DrvDips[0];

		case 0x100008:
			return ((DrvDips[1] & 0x3f) | (DrvInputs[4] & 0xc0));
	}

	return 0;
}

void __fastcall blockout_sound_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0x8800:
			BurnYM2151SelectRegister(data);
		return;

		case 0x8801:
			BurnYM2151WriteRegister(data);
		return;

		case 0x9800:
			MSM6295Command(0, data);
		return;
	}
}

unsigned char __fastcall blockout_sound_read(unsigned short address)
{
	switch (address)
	{
		case 0x8800:
		case 0x8801:
			return BurnYM2151ReadStatus(address & 0x01);

		case 0x9800:
			return MSM6295ReadStatus(0);

		case 0xa000:
			return *soundlatch;
	}

	return 0;
}

void BlockoutYM2151IrqHandler(int Irq)
{
	if (Irq) {
		ZetSetIRQLine(0xff, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	SekOpen(0);
	SekReset();
	SekClose();

	ZetOpen(0);
	ZetReset();
	ZetClose();

	MSM6295Reset(0);
	BurnYM2151Reset();

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	Drv68KROM	= Next; Next += 0x040000;
	DrvZ80ROM	= Next; Next += 0x008000;

	MSM6295ROM	= Next;
	DrvSndROM	= Next; Next += 0x040000;

	DrvPalette	= (unsigned int*)Next; Next += 0x0201 * sizeof(int);

	AllRam		= Next;

	Drv68KRAM0	= Next; Next += 0x00c000;
	Drv68KRAM1	= Next; Next += 0x00c000;
	Drv68KRAM2	= Next; Next += 0x018000;
	DrvVidRAM0	= Next; Next += 0x040000;
	DrvVidRAM1	= Next; Next += 0x008000;
	DrvPalRAM	= Next; Next += 0x000800;

	DrvZ80RAM	= Next; Next += 0x000800;

	soundlatch	= Next; Next += 0x000001;

	DrvTmpBmp	= (unsigned short*)Next; Next += (320 * 240) * sizeof(short);

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

static int DrvInit()
{
	BurnSetRefreshRate(58.0);

	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(Drv68KROM + 0x000001, 0, 2)) return 1;
		if (BurnLoadRom(Drv68KROM + 0x000000, 1, 2)) return 1;

		if (BurnLoadRom(DrvZ80ROM + 0x000000, 2, 1)) return 1;

		if (BurnLoadRom(DrvSndROM + 0x000000, 3, 1)) return 1;
	}

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68KROM,		0x000000, 0x03ffff, SM_ROM);
	SekMapMemory(DrvVidRAM0,	0x180000, 0x1bffff, SM_ROM);
	SekMapMemory(Drv68KRAM0,	0x1d4000, 0x1dffff, SM_RAM);
	SekMapMemory(Drv68KRAM1,	0x1f4000, 0x1fffff, SM_RAM);
	SekMapMemory(DrvVidRAM1,	0x200000, 0x207fff, SM_RAM);
	SekMapMemory(Drv68KRAM2,	0x208000, 0x21ffff, SM_RAM);
	SekMapMemory(DrvPalRAM,		0x280200, 0x2805ff, SM_ROM); // maps 280000 - 2707ff
	SekSetWriteByteHandler(0,	blockout_write_byte);
	SekSetWriteWordHandler(0,	blockout_write_word);
	SekSetReadByteHandler(0,	blockout_read_byte);
	SekSetReadWordHandler(0,	blockout_read_word);	
	SekClose();

	ZetInit(1);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM);
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80RAM);
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80RAM);
	ZetSetWriteHandler(blockout_sound_write);
	ZetSetReadHandler(blockout_sound_read);
	ZetMemEnd();
	ZetClose();

	MSM6295Init(0, 1056000 / 132, 100.0, 1);

	BurnYM2151Init(3579545, 60.0);
	BurnYM2151SetIrqHandler(&BlockoutYM2151IrqHandler);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	GenericTilesExit();

	MSM6295Exit(0);
	BurnYM2151Exit();

	SekExit();
	ZetExit();

	free (AllMem);
	AllMem = NULL;

	MSM6295ROM = NULL;

	return 0;
}

static int DrvDraw()
{
	if (DrvRecalc) {
		for (int i = 0; i < 0x402; i+=2) {
			palette_write(i);
		}
	}

	memcpy (pTransDraw, DrvTmpBmp, 320 * 240 * 2);

	{
		unsigned short *vram = (unsigned short*)DrvVidRAM1;

		for (int y = 0; y < 240;y++)
		{
			for (int x = 0; x < 320; x+=8)
			{
				int d = vram[((y + 8) << 6) + (x >> 3)];

				if (d) {
					for (int v = 0x80, c = 0; v > 0; v >>= 1, c++)
						if (d & v) pTransDraw[(y * 320) + x + c] = 512;
				}
			}
		}
	}

	BurnTransferCopy(DrvPalette);

	return 0;
}


static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		memset (DrvInputs, 0xff, 10); 
		for (int i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
			DrvInputs[4] ^= (DrvJoy5[i] & 1) << i;
		}
	}

	int nSegment;
	int nInterleave = 10;
	int nSoundBufferPos = 0;
	int nTotalCycles[2] = { 10000000 / 58, 3579545 / 58 };

	SekOpen(0);
	ZetOpen(0);

	for (int i = 0; i < nInterleave; i++)
	{

		nSegment = nTotalCycles[0] / nInterleave;

		SekRun(nSegment);

		nSegment = nTotalCycles[1] / nInterleave;

		ZetRun(nSegment);
		nSegment = nBurnSoundLen / nInterleave;

		if (pBurnSoundOut) {
			short *pSoundBuf = pBurnSoundOut + nSoundBufferPos * 2;
			BurnYM2151Render(pSoundBuf, nSegment);
			MSM6295Render(0, pSoundBuf, nSegment);
			nSoundBufferPos += nSegment;
		}

		if (i == (nInterleave / 2)-1) SekSetIRQLine(6, SEK_IRQSTATUS_AUTO);
	}

	if (pBurnSoundOut) {
		short *pSoundBuf = pBurnSoundOut + nSoundBufferPos * 2;
		nSegment = nBurnSoundLen - nSoundBufferPos;
		if (nSegment) {
			BurnYM2151Render(pSoundBuf, nSegment);
			MSM6295Render(0, pSoundBuf, nSegment);
		}
	}

	SekSetIRQLine(5, SEK_IRQSTATUS_AUTO);

	ZetClose();
	SekClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin = 0x029698;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd-AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SekScan(nAction);
		ZetScan(nAction);

		BurnYM2151Scan(nAction);
		MSM6295Scan(0, nAction);
	}

	return 0;
}


// Block Out (set 1)

static struct BurnRomInfo blockoutRomDesc[] = {
	{ "bo29a0-2.bin",	0x20000, 0xb0103427, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "bo29a1-2.bin",	0x20000, 0x5984d5a2, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "bo29e3-0.bin",	0x08000, 0x3ea01f78, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "bo29e2-0.bin",	0x20000, 0x15c5a99d, 3 | BRF_SND },           //  3 Samples

	{ "mb7114h.25",		0x00100, 0xb25bbda7, 0 | BRF_OPT },           //  4 Unused Prom
};

STD_ROM_PICK(blockout);
STD_ROM_FN(blockout);

struct BurnDriver BurnDrvBlockout = {
	"blockout", NULL, NULL, "1989",
	"Block Out (set 1)\0", NULL, "Technos + California Dreams", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, blockoutRomInfo, blockoutRomName, BlockoutInputInfo, BlockoutDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};


// Block Out (set 2)

static struct BurnRomInfo blckout2RomDesc[] = {
	{ "29a0",		0x20000, 0x605f931e, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "29a1",		0x20000, 0x38f07000, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "bo29e3-0.bin",	0x08000, 0x3ea01f78, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "bo29e2-0.bin",	0x20000, 0x15c5a99d, 3 | BRF_SND },           //  3 Samples

	{ "mb7114h.25",		0x00100, 0xb25bbda7, 0 | BRF_OPT },           //  4 Unused Prom
};

STD_ROM_PICK(blckout2);
STD_ROM_FN(blckout2);

struct BurnDriver BurnDrvBlckout2 = {
	"blckout2", "blockout", NULL, "1989",
	"Block Out (set 2)\0", NULL, "Technos + California Dreams", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, blckout2RomInfo, blckout2RomName, BlockoutInputInfo, BlockoutDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};


// Block Out (Japan)

static struct BurnRomInfo blckoutjRomDesc[] = {
	{ "2.bin",		0x20000, 0xe16cf065, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "1.bin",		0x20000, 0x950b28a3, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "bo29e3-0.bin",	0x08000, 0x3ea01f78, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "bo29e2-0.bin",	0x20000, 0x15c5a99d, 3 | BRF_SND },           //  3 Samples

	{ "mb7114h.25",		0x00100, 0xb25bbda7, 0 | BRF_OPT },           //  4 Unused Prom
};

STD_ROM_PICK(blckoutj);
STD_ROM_FN(blckoutj);

struct BurnDriver BurnDrvBlckoutj = {
	"blckoutj", "blockout", NULL, "1989",
	"Block Out (Japan)\0", NULL, "Technos + California Dreams", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, blckoutjRomInfo, blckoutjRomName, BlckoutjInputInfo, BlockoutDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};


// Agress

static struct BurnRomInfo agressRomDesc[] = {
	{ "palco1.81",		0x20000, 0x3acc917a, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "palco2.91",		0x20000, 0xabfd5bcc, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "palco3.73",		0x08000, 0x2a21c97d, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "palco4.78",		0x20000, 0x9dfd9cfe, 3 | BRF_SND },           //  3 Samples

	{ "82s129pr.25",	0x00100, 0xb25bbda7, 0 | BRF_OPT },           //  4 Unused Prom
};

STD_ROM_PICK(agress);
STD_ROM_FN(agress);

struct BurnDriver BurnDrvAgress = {
	"agress", NULL, NULL, "1991",
	"Agress\0", NULL, "Palco", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NULL, agressRomInfo, agressRomName, BlockoutInputInfo, AgressDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};


// Agress (English bootleg)

static struct BurnRomInfo agressbRomDesc[] = {
	{ "palco1.ic81",		0x20000, 0xa1875175, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "palco2.ic91",		0x20000, 0xab3182c3, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "palco3.ic73",		0x08000, 0x2a21c97d, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 Code

	{ "palco4.ic78",		0x20000, 0x9dfd9cfe, 3 | BRF_SND },           //  3 Samples

	{ "prom29-mb7114h.ic25",	0x00100, 0xb25bbda7, 0 | BRF_OPT },           //  4 Unused Prom
};

STD_ROM_PICK(agressb);
STD_ROM_FN(agressb);

struct BurnDriver BurnDrvAgressb = {
	"agressb", "agress", NULL, "2003",
	"Agress (English bootleg)\0", NULL, "Palco", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NULL, agressbRomInfo, agressbRomName, BlockoutInputInfo, AgressDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	320, 240, 4, 3
};
