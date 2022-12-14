/*
 * Aero Fighters driver for FB Alpha 0.2.96.71
 * Port by OopsWare. 2007
 * http://oopsware.googlepages.com
 * http://oopsware.ys168.com
 *
 * 6.13.2007
 *   Add driver Spinal Breakers (spinlbrk)
 *
 * 6.12.2007
 *   Add driver Karate Blazers (karatblz)
 *
 * 6.11.2007
 *   Add driver Turbo Force (turbofrc)
 *
 * 6.10.2007
 *   Add BurnHighCol support, and add BDF_16BIT_ONLY into driver.   thanks to KEV
 *
 */

#include "burnint.h"
#include "burn_ym2610.h"

#define	USE_BURN_HIGHCOL	1

static unsigned char ALIGN_DATA DrvButton[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvButton2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy3[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy4[8] = {0, 0, 0, 0, 0, 0, 0, 0};

static unsigned char ALIGN_DATA DrvInput[10] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char DrvReset = 0;

static unsigned char *Mem = NULL, *MemEnd = NULL;
static unsigned char *RamStart, *RamEnd;
static unsigned char *Rom01;
static unsigned char *RomZ80;
static unsigned char *RomBg;
static unsigned char *RomSpr1;
static unsigned char *RomSpr2;
static unsigned char *RomSnd1;
static unsigned char *RomSnd2;

static unsigned char *RamPal;
static unsigned short *RamRaster;
static unsigned short *RamBg1V, *RamBg2V;
static unsigned short *RamSpr1;
static unsigned short *RamSpr2;
static unsigned short *RamSpr3;
static unsigned char *Ram01;
static unsigned char *RamZ80;

//static int nCyclesDone[2];
static int nCyclesTotal[2];
//static int nCyclesSegment;

static unsigned char ALIGN_DATA RamGfxBank[8];

static unsigned int *RamCurPal;
static unsigned char *DeRomBg;
static unsigned char *DeRomSpr1;
static unsigned char *DeRomSpr2;

static unsigned int RamSpr1SizeMask;
static unsigned int RamSpr2SizeMask;
static unsigned int RomSpr1SizeMask;
static unsigned int RomSpr2SizeMask;

static int pending_command = 0;
static int RomSndSize1, RomSndSize2;

static unsigned short bg1scrollx, bg2scrollx;
static unsigned short bg1scrolly, bg2scrolly;

static int nAerofgtZ80Bank;
static unsigned char nSoundlatch;

// Pal routines

#if USE_BURN_HIGHCOL

inline static unsigned int CalcCol(unsigned short nColour)
{
	int r, g, b;

	r = (nColour & 0x001F) << 3;	// Red
	r |= r >> 5;
	g = (nColour & 0x03E0) >> 2;  	// Green
	g |= g >> 5;
	b = (nColour & 0x7C00) >> 7;	// Blue
	b |= b >> 5;

	return BurnHighCol(b, g, r, 0);
}

#else

inline static unsigned int CalcCol(unsigned short nColour)
{
	return (nColour & 0x001F) | ((nColour & 0x7FE0) << 1);
}

#endif

int aerofgtPalUpdate()
{
	int i;
	unsigned short* ps;
	unsigned int* pd;

	for (i = 0, ps = (unsigned short*)RamPal, pd = RamCurPal; i < 1024; i++, ps++, pd++) {
		*pd = CalcCol(*ps);
	}
	return 0;
}

static struct BurnInputInfo aerofgtInputList[] = {
	{"P1 Coin",      BIT_DIGITAL,    DrvButton + 0,  "p1 coin"   },
	{"P1 Start",     BIT_DIGITAL,    DrvButton + 2,  "p1 start"  },

	{"P1 Up",        BIT_DIGITAL,    DrvJoy1 + 0,    "p1 up"     },
	{"P1 Down",      BIT_DIGITAL,    DrvJoy1 + 1,    "p1 down"   },
	{"P1 Left",      BIT_DIGITAL,    DrvJoy1 + 2,    "p1 left"   },
	{"P1 Right",     BIT_DIGITAL,    DrvJoy1 + 3,    "p1 right"  },
	{"P1 Button 1",  BIT_DIGITAL,    DrvJoy1 + 4,    "p1 fire 1" },
	{"P1 Button 2",  BIT_DIGITAL,    DrvJoy1 + 5,    "p1 fire 2" },

	{"P2 Coin",      BIT_DIGITAL,    DrvButton + 1,  "p2 coin"   },
	{"P2 Start",     BIT_DIGITAL,    DrvButton + 3,  "p2 start"  },

	{"P2 Up",        BIT_DIGITAL,    DrvJoy2 + 0,    "p2 up"     },
	{"P2 Down",      BIT_DIGITAL,    DrvJoy2 + 1,    "p2 down"   },
	{"P2 Left",      BIT_DIGITAL,    DrvJoy2 + 2,    "p2 left"   },
	{"P2 Right",     BIT_DIGITAL,    DrvJoy2 + 3,    "p2 right"  },
	{"P2 Button 1",  BIT_DIGITAL,    DrvJoy2 + 4,    "p2 fire 1" },
	{"P2 Button 2",  BIT_DIGITAL,    DrvJoy2 + 5,    "p2 fire 2" },
	
	{"Reset",        BIT_DIGITAL,    &DrvReset,      "reset"     },
	{"Service",      BIT_DIGITAL,    DrvButton + 6,  "service"   },
	{"Dip A",        BIT_DIPSWITCH,  DrvInput + 3,   "dip"       },
	{"Dip B",        BIT_DIPSWITCH,  DrvInput + 4,   "dip"       },
	{"Dip C",        BIT_DIPSWITCH,  DrvInput + 5,   "dip"       },
};

STDINPUTINFO(aerofgt);

static struct BurnDIPInfo sonicwiDIPList[] = {
	// Defaults
	{0x12, 0xFF, 0xFF, 0x00, NULL},
	{0x13, 0xFF, 0xFF, 0x02, NULL},

	// DIP 1
	{0,    0xFE, 0,    2,    "Coin slot"},
	{0x12, 0x82, 0x01, 0x00, "Same"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x01, 0x01, "Individual"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0,    0xFE, 0,    9,    "Coin 1"},
	{0x12, 0x82, 0x0E, 0x00, "1 coin 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x02, "2 coins 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x04, "3 coins 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x06, "1 coin 2 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x08, "1 coin 3 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x0A, "1 coin 4 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x0C, "1 coin 5 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x0E, 0x0E, "1 coin 6 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x01, 0xFF, 0xFF, "Free play"},
	{0,    0xFE, 0,    9,    "Coin 2"},
	{0x12, 0x82, 0x70, 0x00, "1 coin 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x10, "2 coins 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x20, "3 coins 1 credit"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x30, "1 coin 2 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x40, "1 coin 3 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x50, "1 coin 4 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x60, "1 coin 5 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x70, 0x70, "1 coin 6 credits"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x01, 0xFF, 0xFF, "Free play"},
	{0,    0xFE, 0,    2,    "2 coins to start, 1 to continue"},
	{0x12, 0x82, 0x80, 0x00, "Off"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},
	{0x12, 0x82, 0x80, 0x80, "On"},
	{0x12, 0x00, 0xFF, 0xFF, NULL},

	// DIP 2
	{0,    0xFE, 0,    2,    "Screen flip"},
	{0x13, 0x01, 0x01, 0x00, "Off"},
	{0x13, 0x01, 0x01, 0x01, "On"},
	{0,    0xFE, 0,    2,    "Demo sound"},
	{0x13, 0x01, 0x02, 0x00, "Off"},
	{0x13, 0x01, 0x02, 0x02, "On"},
	{0,    0xFE, 0,    4,    "Difficulty"},
	{0x13, 0x01, 0x0C, 0x00, "Normal"},
	{0x13, 0x01, 0x0C, 0x04, "Easy"},
	{0x13, 0x01, 0x0C, 0x08, "Hard"},
	{0x13, 0x01, 0x0C, 0x0C, "Hardest"},
	{0,    0xFE, 0,    4,    "Lives"},
	{0x13, 0x01, 0x30, 0x00, "3"},
	{0x13, 0x01, 0x30, 0x10, "1"},
	{0x13, 0x01, 0x30, 0x20, "2"},
	{0x13, 0x01, 0x30, 0x30, "4"},
	{0,    0xFE, 0,    2,    "Bonus life"},
	{0x13, 0x01, 0x40, 0x00, "200000"},
	{0x13, 0x01, 0x40, 0x40, "300000"},
	{0,    0xFE, 0,    2,    "Service mode"},
	{0x13, 0x01, 0x80, 0x00, "Off"},
	{0x13, 0x01, 0x80, 0x80, "On"},
};

static struct BurnDIPInfo aerofgtDIPList[] = {
	// Defaults
	{0x14, 0xFF, 0xFF, 0x03, NULL},

	// DIP 3
	{0,    0xFE, 0,    5,    "Region"},
	{0x14, 0x01, 0x0F, 0x00, "USA / Canada"},
	{0x14, 0x01, 0x0F, 0x01, "Korea"},
	{0x14, 0x01, 0x0F, 0x02, "Hong Kong"},
	{0x14, 0x01, 0x0F, 0x03, "World"},
	{0x14, 0x01, 0x0F, 0x04, "Taiwan"},
};

static struct BurnDIPInfo aerofgtbDIPList[] = {
	// Defaults
	{0x14, 0xFF, 0xFF, 0x00, NULL},

	// DIP 3
	{0,    0xFE, 0,    4,    "Region"},
	{0x14, 0x01, 0x0F, 0x00, "USA / Canada"},
	{0x14, 0x01, 0x0F, 0x01, "Korea"},
	{0x14, 0x01, 0x0F, 0x02, "Hong Kong"},
	{0x14, 0x01, 0x0F, 0x03, "Taiwan"},
	{0x14, 0x01, 0x0F, 0x04, "Taiwan"},
};

STDDIPINFO(sonicwi);
STDDIPINFOEXT(aerofgt, sonicwi, aerofgt);
STDDIPINFOEXT(aerofgtb, sonicwi, aerofgtb);

// Rom information
static struct BurnRomInfo aerofgtRomDesc[] = {
	{ "1.u4",         0x080000, 0x6fdff0a2, BRF_ESS | BRF_PRG }, // 68000 code swapped

	{ "538a54.124",   0x080000, 0x4d2c4df2, BRF_GRA },           // graphics
	{ "1538a54.124",  0x080000, 0x286d109e, BRF_GRA },
	
	{ "538a53.u9",    0x100000, 0x630d8e0b, BRF_GRA },           // 
	{ "534g8f.u18",   0x080000, 0x76ce0926, BRF_GRA },

	{ "2.153",        0x020000, 0xa1ef64ec, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "it-19-01",     0x040000, 0x6d42723d, BRF_SND },           // samples
	{ "it-19-06",     0x100000, 0xcdbbdb1d, BRF_SND },
};

STD_ROM_PICK(aerofgt);
STD_ROM_FN(aerofgt);

static int MemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01 		= Next; Next += 0x080000;			// 68000 ROM
	RomZ80		= Next; Next += 0x030000;			// Z80 ROM
	RomBg		= Next; Next += 0x200040;			// Background, 1M 8x8x4bit decode to 2M + 64Byte safe 
	DeRomBg		= 	   RomBg +  0x000040;
	RomSpr1		= Next; Next += 0x200000;			// Sprite 1	 , 1M 16x16x4bit decode to 2M + 256Byte safe 
	RomSpr2		= Next; Next += 0x100100;			// Sprite 2
	
	DeRomSpr1	= RomSpr1    +  0x000100;
	DeRomSpr2	= RomSpr2    += 0x000100;
	
	RomSnd1		= Next; Next += 0x040000;			// ADPCM data
	RomSndSize1 = 0x040000;
	RomSnd2		= Next; Next += 0x100000;			// ADPCM data
	RomSndSize2 = 0x100000;
	RamStart	= Next;
	
	RamPal		= Next; Next += 0x000800;			// 1024 of X1R5G5B5 Palette
	RamRaster	= (unsigned short *)Next; Next += 0x001000;	// 0x1b0000~0x1b07ff, Raster
															// 0x1b0800~0x1b0801, NOP
															// 0x1b0ff0~0x1b0fff, stack area during boot
	RamBg1V		= (unsigned short *)Next; Next += 0x002000;	// BG1 Video Ram
	RamBg2V		= (unsigned short *)Next; Next += 0x002000;	// BG2 Video Ram
	RamSpr1		= (unsigned short *)Next; Next += 0x008000;	// Sprite 1 Ram
	RamSpr2		= (unsigned short *)Next; Next += 0x002000;	// Sprite 2 Ram
	Ram01		= Next; Next += 0x010000;					// Work Ram
	RamZ80		= Next; Next += 0x000800;					// Z80 Ram 2K

	RamEnd		= Next;

	RamCurPal	= (unsigned int *)Next; Next += 0x000800 * sizeof(unsigned int);	// 1024 colors

	MemEnd		= Next;
	return 0;
}

static void DecodeBg()
{
	for (int c=0x8000-1; c>=0; c--) {
		for (int y=7; y>=0; y--) {
			DeRomBg[(c * 64) + (y * 8) + 7] = RomBg[0x00002 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 6] = RomBg[0x00002 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 5] = RomBg[0x00003 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 4] = RomBg[0x00003 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 3] = RomBg[0x00000 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 2] = RomBg[0x00000 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 1] = RomBg[0x00001 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 0] = RomBg[0x00001 + (y * 4) + (c * 32)] >> 4;
		}
	}
}

static void DecodeSpr(unsigned char *d, unsigned char *s, int cnt)
{
	for (int c=cnt-1; c>=0; c--)
		for (int y=15; y>=0; y--) {
			d[(c * 256) + (y * 16) + 15] = s[0x00006 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 14] = s[0x00006 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 13] = s[0x00007 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 12] = s[0x00007 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 11] = s[0x00004 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 10] = s[0x00004 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  9] = s[0x00005 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  8] = s[0x00005 + (y * 8) + (c * 128)] >> 4;

			d[(c * 256) + (y * 16) +  7] = s[0x00002 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  6] = s[0x00002 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  5] = s[0x00003 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  4] = s[0x00003 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  3] = s[0x00000 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  2] = s[0x00000 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  1] = s[0x00001 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  0] = s[0x00001 + (y * 8) + (c * 128)] >> 4;
		}
}

static int LoadRoms()
{
	// Load 68000 ROM
	if (BurnLoadRom(Rom01, 0, 1)) {
		return 1;
	}
	
	// Load Graphic
	BurnLoadRom(RomBg,   1, 1);
	BurnLoadRom(RomBg+0x80000,  2, 1);
	DecodeBg();
	
	BurnLoadRom(RomSpr1+0x000000, 3, 1);
	BurnLoadRom(RomSpr1+0x100000, 4, 1);
	DecodeSpr(DeRomSpr1, RomSpr1, 8192+4096);
	
	// Load Z80 ROM
	if (BurnLoadRom(RomZ80+0x10000, 5, 1)) return 1;
	memcpy(RomZ80, RomZ80+0x10000, 0x10000);

	BurnLoadRom(RomSnd1,  6, 1);
	BurnLoadRom(RomSnd2,  7, 1);
	
	return 0;
}

unsigned char __fastcall aerofgtReadByte(unsigned int sekAddress)
{
	switch (sekAddress) {
		case 0xFFFFA1:
			return ~DrvInput[0];
		case 0xFFFFA3:
			return ~DrvInput[1];
		case 0xFFFFA5:
			return ~DrvInput[2];
		case 0xFFFFA7:
			return ~DrvInput[3];
		case 0xFFFFA9:
			return ~DrvInput[4];
		case 0xFFFFAD:
			//printf("read pending_command %d addr %08x\n", pending_command, sekAddress);
			return pending_command;
		case 0xFFFFAF:
			return ~DrvInput[5];
//		default:
//			printf("Attempt to read byte value of location %x\n", sekAddress);
	}
	return 0;
}

/*
unsigned short __fastcall aerofgtReadWord(unsigned int sekAddress)
{
	switch (sekAddress) {


		default:
			printf("Attempt to read word value of location %x\n", sekAddress);
	}
	return 0;
}
*/

static void SynchroniseZ80()
{
	int nCycles = ((long long)SekTotalCycles() * nCyclesTotal[1] / nCyclesTotal[0]);
	if (nCycles <= ZetTotalCycles()) return;
	
	BurnTimerUpdate(nCycles);
}

static void SoundCommand(unsigned char nCommand)
{
	SynchroniseZ80();
	
//	bprintf(PRINT_NORMAL, _T("  - Sound command sent (0x%02X).\n"), nCommand);
	nSoundlatch = nCommand;
	ZetNmi();
}

void __fastcall aerofgtWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	if (( sekAddress & 0xFF0000 ) == 0x1A0000) {
		sekAddress &= 0xFFFF;
		if (sekAddress < 0x800) {
			RamPal[sekAddress^1] = byteValue;
			// palette byte write at boot self-test only ?!
			// if (sekAddress & 1)
			//	RamCurPal[sekAddress>>1] = CalcCol( *((unsigned short *)&RamPal[sekAddress]) );
		}
		return;	
	}

	switch (sekAddress) {
		case 0xFFFFC1:
			pending_command = 1;
			SoundCommand(byteValue);
			break;
/*			
		case 0xFFFFB9:
		case 0xFFFFBB:
		case 0xFFFFBD:
		case 0xFFFFBF:
			break;
		case 0xFFFFAD:
			// NOP
			break;
		default:
			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
*/
	}
}

void __fastcall aerofgtWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	if (( sekAddress & 0xFF0000 ) == 0x1A0000) {
		sekAddress &= 0xFFFF;
		if (sekAddress < 0x800)
			*((unsigned short *)&RamPal[sekAddress]) = wordValue;
			RamCurPal[sekAddress>>1] = CalcCol( wordValue );
		return;	
	}

	switch (sekAddress) {
		case 0xFFFF80:
			RamGfxBank[0] = wordValue >> 0x08;
			RamGfxBank[1] = wordValue & 0xFF;
			break;
		case 0xFFFF82:
			RamGfxBank[2] = wordValue >> 0x08;
			RamGfxBank[3] = wordValue & 0xFF;
			break;
		case 0xFFFF84:
			RamGfxBank[4] = wordValue >> 0x08;
			RamGfxBank[5] = wordValue & 0xFF;
			break;
		case 0xFFFF86:
			RamGfxBank[6] = wordValue >> 0x08;
			RamGfxBank[7] = wordValue & 0xFF;
			break;
		case 0xFFFF88:
			bg1scrolly = wordValue;
			break;
		case 0xFFFF90:
			bg2scrolly = wordValue;
			break;
/*
		case 0xFFFF40:
		case 0xFFFF42:
		case 0xFFFF44:
		case 0xFFFF48:
		case 0xFFFF4A:
		case 0xFFFF4C:
		case 0xFFFF50:
		case 0xFFFF52:
		case 0xFFFF60:
			break;
		default:
			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
*/
	}
}

static void aerofgtFMIRQHandler(int, int nStatus)
{
//	bprintf(PRINT_NORMAL, _T("  - IRQ -> %i.\n"), nStatus);
	if (nStatus) {
		ZetSetIRQLine(0xFF, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int aerofgtSynchroniseStream(int nSoundRate)
{
	return (long long)ZetTotalCycles() * nSoundRate / 4000000;
}

static float aerofgtGetTime()
{
	return (float)ZetTotalCycles() / 4000000.0;
}

static int aerofgtSynchroniseStream1(int nSoundRate)
{
	return (long long)ZetTotalCycles() * nSoundRate / 5000000;
}

static float aerofgtGetTime1()
{
	return (float)ZetTotalCycles() / 5000000.0;
}

/*
static unsigned char __fastcall aerofgtZ80Read(unsigned short a)
{
	switch (a) {

		default:
			printf("Z80 Attempt to read word value of location %04x\n", a);
	}
	return 0;
}

static void __fastcall aerofgtZ80Write(unsigned short a,unsigned char v)
{
	switch (a) {

		default:
			printf("Attempt to write word value %x to location %x\n", v, a);
	}
}
*/

static void aerofgtSndBankSwitch(unsigned char v)
{
/*
	UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;
	memory_set_bankptr(1,rom + (data & 0x03) * 0x8000);
*/
	v &= 0x03;
	if (v != nAerofgtZ80Bank) {
		unsigned char* nStartAddress = RomZ80 + 0x10000 + (v << 15);
		ZetMapArea(0x8000, 0xFFFF, 0, nStartAddress);
		ZetMapArea(0x8000, 0xFFFF, 2, nStartAddress);
		nAerofgtZ80Bank = v;
	}
}

unsigned char __fastcall aerofgtZ80PortRead(unsigned short p)
{
	switch (p & 0xFF) {
		case 0x00:
			return BurnYM2610Read(0);
		case 0x02:
			return BurnYM2610Read(2);
		case 0x0C:
			return nSoundlatch;
//		default:
//			printf("Z80 Attempt to read port %04x\n", p);
	}
	return 0;
}

void __fastcall aerofgtZ80PortWrite(unsigned short p, unsigned char v)
{
	switch (p & 0x0FF) {
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
			BurnYM2610Write(p & 3, v);
			break;
		case 0x04:
			aerofgtSndBankSwitch(v);
			break;
		case 0x08:
			pending_command = 0;
			break;
//		default:
//			printf("Z80 Attempt to write %02x to port %04x\n", v, p);
	}
}


static int DrvDoReset()
{
	nAerofgtZ80Bank = -1;
	aerofgtSndBankSwitch(0);
	
	SekOpen(0);
	//nIRQPending = 0;
    SekSetIRQLine(0, SEK_IRQSTATUS_NONE);
	SekReset();
	SekClose();

	ZetReset();
	BurnYM2610Reset();

	memset(RamGfxBank, 0 , sizeof(RamGfxBank));

	return 0;
}

static int aerofgtInit()
{
	Mem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) {
		return 1;
	}
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();	
	
	// Load the roms into memory
	
	if (LoadRoms()) {
		return 1;
	}

	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,			0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(RamPal,		0x1A0000, 0x1A07FF, SM_ROM);	// Palette
		SekMapMemory((unsigned char *)RamRaster,
									0x1B0000, 0x1B0FFF, SM_RAM);	// Raster / MRA_NOP / MRA_BANK7
		SekMapMemory((unsigned char *)RamBg1V,		
									0x1B2000, 0x1B3FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamBg2V,		
									0x1B4000, 0x1B5FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamSpr1,
									0x1C0000, 0x1C7FFF, SM_RAM);
		SekMapMemory((unsigned char *)RamSpr2,
									0x1D0000, 0x1D1FFF, SM_RAM);
		SekMapMemory(Ram01,			0xFEF000, 0xFFEFFF, SM_RAM);	// 64K Work RAM

//		SekSetReadWordHandler(0, aerofgtReadWord);
		SekSetReadByteHandler(0, aerofgtReadByte);
		SekSetWriteWordHandler(0, aerofgtWriteWord);
		SekSetWriteByteHandler(0, aerofgtWriteByte);

		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0x77FF, 0, RomZ80);
		ZetMapArea(0x0000, 0x77FF, 2, RomZ80);
		
		ZetMapArea(0x7800, 0x7FFF, 0, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 1, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 2, RamZ80);
		
		ZetMemEnd();
		
		//ZetSetReadHandler(aerofgtZ80Read);
		//ZetSetWriteHandler(aerofgtZ80Write);
		ZetSetInHandler(aerofgtZ80PortRead);
		ZetSetOutHandler(aerofgtZ80PortWrite);
		
		ZetClose();
	}
	
	BurnYM2610Init(8000000, 25.0, 100.0, RomSnd2, &RomSndSize2, RomSnd1, &RomSndSize1, &aerofgtFMIRQHandler, aerofgtSynchroniseStream1, aerofgtGetTime1, 0);
	BurnTimerAttachZet(5000000);
	
	DrvDoReset();												// Reset machine
	return 0;
}

static int DrvExit()
{
	BurnYM2610Exit();
	
	ZetExit();
	SekExit();
	
	free(Mem);
	Mem = NULL;
	return 0;
}

#ifndef BUILD_PSP
#define X_SIZE	320
#else
#define X_SIZE	512
#endif

static void drawgfxzoom(int bank,unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,int scalex,int scaley)
{
	if (!scalex || !scaley) return;
			
	unsigned short * p = (unsigned short *) pBurnDraw;
	unsigned char * q;
	unsigned int * pal;
	
	if (bank) {
		q = DeRomSpr2 + (code) * 256;
		pal = RamCurPal + 768;
	} else {
		q = DeRomSpr1 + (code) * 256;
		pal = RamCurPal + 512;
	}

	p += sy * X_SIZE + sx;
		
	if (sx < 0 || sx >= 304 || sy < 0 || sy >= 208 ) {
		
		if ((sx <= -16) || (sx >= 320) || (sy <= -16) || (sy >= 224))
			return;
			
		if (flipy) {
		
			p += X_SIZE * 15;
			
			if (flipx) {
			
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}	
			
			} else {
	
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}		
			}
			
		} else {
			
			if (flipx) {
			
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}		
			
			} else {
	
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}	
	
			}
			
		}
		
		return;
	}

	if (flipy) {
		
		p += X_SIZE * 15;
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}	
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}		
		}
		
	} else {
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}		
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}	

		}
		
	}

}

static void aerofgt_drawsprites(int priority)
{
	priority <<= 12;
	int offs = 0;
	
	while (offs < 0x0400 && (RamSpr2[offs] & 0x8000) == 0) {
		int attr_start = (RamSpr2[offs] & 0x03ff) * 4;
		
		/* is the way I handle priority correct? Or should I just check bit 13? */
		if ((RamSpr2[attr_start + 2] & 0x3000) == priority) {
			int map_start;
			int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color;

			ox = RamSpr2[attr_start + 1] & 0x01ff;
			xsize = (RamSpr2[attr_start + 1] & 0x0e00) >> 9;
			zoomx = (RamSpr2[attr_start + 1] & 0xf000) >> 12;
			oy = RamSpr2[attr_start + 0] & 0x01ff;
			ysize = (RamSpr2[attr_start + 0] & 0x0e00) >> 9;
			zoomy = (RamSpr2[attr_start + 0] & 0xf000) >> 12;
			flipx = RamSpr2[attr_start + 2] & 0x4000;
			flipy = RamSpr2[attr_start + 2] & 0x8000;
			color = (RamSpr2[attr_start + 2] & 0x0f00) >> 4;
			map_start = RamSpr2[attr_start + 3] & 0x3fff;

			ox += (xsize*zoomx+2)/4;
			oy += (ysize*zoomy+2)/4;
			
			zoomx = 32 - zoomx;
			zoomy = 32 - zoomy;
		
			for (y = 0;y <= ysize;y++) {
				int sx,sy;

				if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16;
				else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16;

				for (x = 0;x <= xsize;x++) {
					if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16;
					else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16;

					//if (map_start < 0x2000)
					int code = RamSpr1[map_start] & 0x1fff;
					
					drawgfxzoom(map_start&0x2000,code,color,flipx,flipy,sx,sy,zoomx<<11, zoomy<<11);

					map_start++;
				}
			}
		}
		offs++;
	}
}


/*
 * aerofgt Background Tile 
 *   16-bit tile code in RamBg1V and RamBg2V
 * 0xE000 shr 13 is Color
 * 0x1800 shr 11 is Gfx Bank
 * 0x07FF shr  0 or Tile element index (low part)
 * 
 */

static void TileBackground_1(unsigned short *bg, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - RamRaster[0x0000] + 18;
		if (x <= -192) x += 512;
		
		y = my * 8 - (bg1scrolly & 0x1FF);
		if (y <= (224-512)) y += 512;
		
		if ( x<=-8 || x>=320 || y<=-8 || y>= 224 ) 
			continue;
		else
		if ( x >=0 && x < 312 && y >= 0 && y < 216) {
			
			unsigned char *d = DeRomBg + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
 				p[0] = pal[ d[0] | c ];
 				p[1] = pal[ d[1] | c ];
 				p[2] = pal[ d[2] | c ];
 				p[3] = pal[ d[3] | c ];
 				p[4] = pal[ d[4] | c ];
 				p[5] = pal[ d[5] | c ];
 				p[6] = pal[ d[6] | c ];
 				p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = DeRomBg + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<224 ) {
	 				if ((x + 0) >= 0 && (x + 0)<320) p[0] = pal[ d[0] | c ];
	 				if ((x + 1) >= 0 && (x + 1)<320) p[1] = pal[ d[1] | c ];
	 				if ((x + 2) >= 0 && (x + 2)<320) p[2] = pal[ d[2] | c ];
	 				if ((x + 3) >= 0 && (x + 3)<320) p[3] = pal[ d[3] | c ];
	 				if ((x + 4) >= 0 && (x + 4)<320) p[4] = pal[ d[4] | c ];
	 				if ((x + 5) >= 0 && (x + 5)<320) p[5] = pal[ d[5] | c ];
	 				if ((x + 6) >= 0 && (x + 6)<320) p[6] = pal[ d[6] | c ];
	 				if ((x + 7) >= 0 && (x + 7)<320) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void TileBackground_2(unsigned short *bg, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - RamRaster[0x0200] + 20;
		if (x <= -192) x += 512;
		
		y = my * 8 - (bg2scrolly & 0x1FF);
		if (y <= (224-512)) y += 512;
		
		if ( x<=-8 || x>=320 || y<=-8 || y>= 224 ) 
			continue;
		else
		if ( x >=0 && x < 312 && y >= 0 && y < 216) {
			
			unsigned char *d = DeRomBg + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				
 				if (d[0] != 15) p[0] = pal[ d[0] | c ];
 				if (d[1] != 15) p[1] = pal[ d[1] | c ];
 				if (d[2] != 15) p[2] = pal[ d[2] | c ];
 				if (d[3] != 15) p[3] = pal[ d[3] | c ];
 				if (d[4] != 15) p[4] = pal[ d[4] | c ];
 				if (d[5] != 15) p[5] = pal[ d[5] | c ];
 				if (d[6] != 15) p[6] = pal[ d[6] | c ];
 				if (d[7] != 15) p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = DeRomBg + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<224 ) {
	 				if (d[0] != 15 && (x + 0) >= 0 && (x + 0)<320) p[0] = pal[ d[0] | c ];
	 				if (d[1] != 15 && (x + 1) >= 0 && (x + 1)<320) p[1] = pal[ d[1] | c ];
	 				if (d[2] != 15 && (x + 2) >= 0 && (x + 2)<320) p[2] = pal[ d[2] | c ];
	 				if (d[3] != 15 && (x + 3) >= 0 && (x + 3)<320) p[3] = pal[ d[3] | c ];
	 				if (d[4] != 15 && (x + 4) >= 0 && (x + 4)<320) p[4] = pal[ d[4] | c ];
	 				if (d[5] != 15 && (x + 5) >= 0 && (x + 5)<320) p[5] = pal[ d[5] | c ];
	 				if (d[6] != 15 && (x + 6) >= 0 && (x + 6)<320) p[6] = pal[ d[6] | c ];
	 				if (d[7] != 15 && (x + 7) >= 0 && (x + 7)<320) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static int DrvDraw()
{
	// update palette
	aerofgtPalUpdate();

 	// background 1
 	TileBackground_1(RamBg1V, RamCurPal);

 	aerofgt_drawsprites(0);
	aerofgt_drawsprites(1);
	
 	// background 2
 	TileBackground_2(RamBg2V, RamCurPal + 256);
 	
	aerofgt_drawsprites(2);
	aerofgt_drawsprites(3);

	return 0;
}

#undef X_SIZE

static int DrvFrame()
{
	if (DrvReset) {														// Reset machine
		DrvDoReset();
	}
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;												// Joy1
	DrvInput[1] = 0x0000;												// Joy2
	DrvInput[2] = 0x0000;												// Buttons
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvButton[i] & 1) << i;
	}

	SekNewFrame();
	ZetNewFrame();
	
//	nCyclesTotal[0] = (int)((long long)10000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)10000000 / 60);
	nCyclesTotal[1] = (int)((long long)5000000  / 60);

//	SekSetCyclesScanline(nCyclesTotal[0] / 262);
	
	SekOpen(0);
	ZetOpen(0);
	
	SekRun(nCyclesTotal[0]);
	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
	
	BurnTimerEndFrame(nCyclesTotal[1]);
	BurnYM2610Update(pBurnSoundOut, nBurnSoundLen);

	ZetClose();
	SekClose();
	
	if (pBurnDraw) DrvDraw();
	
	return 0;
}



static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin =  0x029671;
	}

	if (nAction & ACB_MEMORY_RAM) {								// Scan all memory, devices & variables
		memset(&ba, 0, sizeof(ba));
    	ba.Data	  = RamStart;
		ba.nLen	  = RamEnd-RamStart;
		ba.szName = "All Ram";
		BurnAcb(&ba);
		
		if (nAction & ACB_WRITE) {
			// update palette while loaded
			unsigned short* ps = (unsigned short*) RamPal;
			unsigned int* pd = RamCurPal;
			for (int i=0; i<1024; i++, ps++, pd++)
				*pd = CalcCol(*ps);
		}
	}

	if (nAction & ACB_DRIVER_DATA) {

		SekScan(nAction);										// Scan 68000 state

		ZetOpen(0);
		ZetScan(nAction);										// Scan Z80 state
		ZetClose();
		
		BurnYM2610Scan(nAction, pnMin);
		SCAN_VAR(nSoundlatch);
		SCAN_VAR(nAerofgtZ80Bank);
		
		SCAN_VAR(RamGfxBank);
		SCAN_VAR(RamCurPal);
		SCAN_VAR(DrvInput);

		if (nAction & ACB_WRITE) {
			int nBank = nAerofgtZ80Bank;
			nAerofgtZ80Bank = -1;
			aerofgtSndBankSwitch(nBank);
		}
	}
	return 0;
}

struct BurnDriver BurnDrvAerofgt = {
	"aerofgt", NULL, NULL, "1992",
	"Aero Fighters\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_VERSHOOT, FBF_SONICWI,
	NULL, aerofgtRomInfo, aerofgtRomName, aerofgtInputInfo, aerofgtDIPInfo,
	aerofgtInit,DrvExit,DrvFrame,DrvDraw,DrvScan,
	0, NULL, NULL, NULL, NULL, 224,320,3,4
};

// ------------------------------------------------------

static struct BurnInputInfo turbofrcInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvButton + 0,	"p1 coin"},
	{"P1 Start",	BIT_DIGITAL,	DrvButton + 2,	"p1 start"},
	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 left"},
	{"P1 Right",	BIT_DIGITAL,	DrvJoy1 + 3,	"p1 right"},
	{"P1 Button 1",	BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},

	{"P2 Coin",		BIT_DIGITAL,	DrvButton + 1,	"p2 coin"},
	{"P2 Start",	BIT_DIGITAL,	DrvButton + 3,	"p2 start"},
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 left"},
	{"P2 Right",	BIT_DIGITAL,	DrvJoy2 + 3,	"p2 right"},
	{"P2 Button 1",	BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	
	{"P3 Coin",		BIT_DIGITAL,	DrvButton + 7,	"p3 coin"},
	{"P3 Start",	BIT_DIGITAL,	DrvJoy3 + 7,	"p3 start"},
	{"P3 Up",		BIT_DIGITAL,	DrvJoy3 + 0,	"p3 up"},
	{"P3 Down",		BIT_DIGITAL,	DrvJoy3 + 1,	"p3 down"},
	{"P3 Left",		BIT_DIGITAL,	DrvJoy3 + 2,	"p3 left"},
	{"P3 Right",	BIT_DIGITAL,	DrvJoy3 + 3,	"p3 right"},
	{"P3 Button 1",	BIT_DIGITAL,	DrvJoy3 + 4,	"p3 fire 1"},
	
	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"},
	{"Dip A",		BIT_DIPSWITCH,	DrvInput + 4,	"dip"},
	{"Dip B",		BIT_DIPSWITCH,	DrvInput + 5,	"dip"},
};

STDINPUTINFO(turbofrc);

static struct BurnDIPInfo turbofrcDIPList[] = {

	// Defaults
	{0x16,	0xFF, 0xFF,	0x70, NULL},
	{0x17,	0xFF, 0xFF,	0x06, NULL},

	// DIP 1
	{0,		0xFE, 0,	8,	  "Coinage"},
	{0x16,	0x82, 0x07, 0x00, "1 coin 1 credit"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x01, "2 coins 1 credit"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x02, "3 coins 1 credit"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x03, "4 coins 1 credit"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x04, "1 coin 2 credits"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x05, "1 coin 3 credits"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x06, "1 coin 5 credits"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0x16,	0x82, 0x07, 0x07, "1 coin 6 credits"},
	{0x16,	0x00, 0x08, 0x08, NULL},
	{0,		0xFE, 0,	2,	  "2 coins to start, 1 to continue"},
	{0x16,	0x01, 0x08,	0x00, "Off"},
	{0x16,	0x01, 0x08,	0x08, "On"},
	{0,		0xFE, 0,	2,	  "Coin slot"},
	{0x16,	0x01, 0x10,	0x00, "Same"},
	{0x16,	0x01, 0x10,	0x10, "Individual"},
	{0,		0xFE, 0,	2,	  "Play mode"},
	{0x16,	0x01, 0x20,	0x00, "2 players"},
	{0x16,	0x01, 0x20,	0x20, "3 players"},
	{0,		0xFE, 0,	2,	  "Demo sounds"},
	{0x16,	0x01, 0x40,	0x00, "Off"},
	{0x16,	0x01, 0x40,	0x40, "On"},
	{0,		0xFE, 0,	2,	  "Service mode"},
	{0x16,	0x01, 0x80,	0x00, "Off"},
	{0x16,	0x01, 0x80,	0x80, "On"},

	// DIP 2
	{0,		0xFE, 0,	2,	  "Screen flip"},
	{0x17,	0x01, 0x01, 0x00, "Off"},
	{0x17,	0x01, 0x01, 0x01, "On"},
	{0,		0xFE, 0,	8,	  "Difficulty"},
	{0x17,	0x01, 0x0E, 0x00, "1 (Easiest)"},
	{0x17,	0x01, 0x0E, 0x02, "2"},
	{0x17,	0x01, 0x0E, 0x04, "3"},
	{0x17,	0x01, 0x0E, 0x06, "4 (Normal)"},
	{0x17,	0x01, 0x0E, 0x08, "5"},
	{0x17,	0x01, 0x0E, 0x0A, "6"},
	{0x17,	0x01, 0x0E, 0x0C, "7"},
	{0x17,	0x01, 0x0E, 0x0E, "8 (Hardest)"},
	{0,		0xFE, 0,	2,	  "Lives"},
	{0x17,	0x01, 0x10, 0x10, "2"},
	{0x17,	0x01, 0x10, 0x00, "3"},
	{0,		0xFE, 0,	2,	  "Bonus life"},
	{0x17,	0x01, 0x20, 0x00, "200000"},
	{0x17,	0x01, 0x20, 0x20, "300000"},
};

STDDIPINFO(turbofrc);

static struct BurnRomInfo turbofrcRomDesc[] = {
	{ "tfrc2.bin",	  0x040000, 0x721300ee, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "tfrc1.bin",    0x040000, 0x6cd5312b, BRF_ESS | BRF_PRG },
	{ "tfrc3.bin",    0x040000, 0x63f50557, BRF_ESS | BRF_PRG },

	{ "tfrcu94.bin",  0x080000, 0xbaa53978, BRF_GRA },			 // gfx1
	{ "tfrcu95.bin",  0x020000, 0x71a6c573, BRF_GRA },
	
	{ "tfrcu105.bin", 0x080000, 0x4de4e59e, BRF_GRA },			 // gfx2
	{ "tfrcu106.bin", 0x020000, 0xc6479eb5, BRF_GRA },
	
	{ "tfrcu116.bin", 0x080000, 0xdf210f3b, BRF_GRA },			 // gfx3
	{ "tfrcu118.bin", 0x040000, 0xf61d1d79, BRF_GRA },
	{ "tfrcu117.bin", 0x080000, 0xf70812fd, BRF_GRA },
	{ "tfrcu119.bin", 0x040000, 0x474ea716, BRF_GRA },

	{ "tfrcu134.bin", 0x080000, 0x487330a2, BRF_GRA },			 // gfx4
	{ "tfrcu135.bin", 0x080000, 0x3a7e5b6d, BRF_GRA },

	{ "tfrcu166.bin", 0x020000, 0x2ca14a65, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "tfrcu180.bin", 0x020000, 0x39c7c7d5, BRF_SND },			 // samples
	{ "tfrcu179.bin", 0x100000, 0x60ca0333, BRF_SND },	
};

STD_ROM_PICK(turbofrc);
STD_ROM_FN(turbofrc);

unsigned char __fastcall turbofrcReadByte(unsigned int sekAddress)
{
	sekAddress &= 0x0FFFFF;
	switch (sekAddress) {
		case 0x0FF000:
			return ~DrvInput[3];
		case 0x0FF001:
			return ~DrvInput[0];
		case 0x0FF002:
			return 0xFF;
		case 0x0FF003:
			return ~DrvInput[1];
		case 0x0FF004:
			return ~DrvInput[5];
		case 0x0FF005:
			return ~DrvInput[4];
		case 0x0FF007:
			return pending_command;
		case 0x0FF009:
			return ~DrvInput[2];
//		default:
//			printf("Attempt to read byte value of location %x\n", sekAddress);
	}
	return 0;
}

/*
unsigned short __fastcall turbofrcReadWord(unsigned int sekAddress)
{
//	sekAddress &= 0x0FFFFF;
//	switch (sekAddress) {


//		default:
//			printf("Attempt to read word value of location %x\n", sekAddress);
//	}
	return 0;
}
*/

void __fastcall turbofrcWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	if (( sekAddress & 0x0FF000 ) == 0x0FE000) {
		sekAddress &= 0x07FF;
		RamPal[sekAddress^1] = byteValue;
		// palette byte write at boot self-test only ?!
		//if (sekAddress & 1)
		//	RamCurPal[sekAddress>>1] = CalcCol( *((unsigned short *)&RamPal[sekAddress & 0xFFE]) );
		return;	
	}
	
	sekAddress &= 0x0FFFFF;
	
	switch (sekAddress) {
	
		case 0x0FF00E:
			pending_command = 1;
			SoundCommand(byteValue);
			break;	
//		default:
//			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
	}
}

void __fastcall turbofrcWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	if (( sekAddress & 0x0FF000 ) == 0x0FE000) {
		sekAddress &= 0x07FE;
		*((unsigned short *)&RamPal[sekAddress]) = wordValue;
		RamCurPal[sekAddress>>1] = CalcCol( wordValue );
		return;	
	}

	sekAddress &= 0x0FFFFF;

	switch (sekAddress) {
		case 0x0FF002:
			bg1scrolly = wordValue;
			break;
		case 0x0FF004:
			bg2scrollx = wordValue;
			break;
		case 0x0FF006:
			bg2scrolly = wordValue;
			break;
			
		case 0x0FF008:
			RamGfxBank[0] = (wordValue >>  0) & 0x0f;
			RamGfxBank[1] = (wordValue >>  4) & 0x0f;
			RamGfxBank[2] = (wordValue >>  8) & 0x0f;
			RamGfxBank[3] = (wordValue >> 12) & 0x0f;
			break;
		case 0x0FF00A:
			RamGfxBank[4] = (wordValue >>  0) & 0x0f;
			RamGfxBank[5] = (wordValue >>  4) & 0x0f;
			RamGfxBank[6] = (wordValue >>  8) & 0x0f;
			RamGfxBank[7] = (wordValue >> 12) & 0x0f;
			break;
		case 0x0FF00C:
			// NOP
			break;
//		default:
//			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
	}
}

unsigned char __fastcall turbofrcZ80PortRead(unsigned short p)
{
	switch (p & 0xFF) {
		case 0x14:
			return nSoundlatch;
		case 0x18:
			return BurnYM2610Read(0);
		case 0x1A:
			return BurnYM2610Read(2);
//		default:
//			printf("Z80 Attempt to read port %04x\n", p);
	}
	return 0;
}

void __fastcall turbofrcZ80PortWrite(unsigned short p, unsigned char v)
{
	switch (p & 0x0FF) {
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B:
			BurnYM2610Write(p & 3, v);
			break;
		case 0x00:
			aerofgtSndBankSwitch(v);
			break;
		case 0x14:
			pending_command = 0;
			break;
//		default:
//			printf("Z80 Attempt to write %02x to port %04x\n", v, p);
	}
}

static int turbofrcMemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01 		= Next; Next += 0x0C0000;			// 68000 ROM
	RomZ80		= Next; Next += 0x030000;			// Z80 ROM
	
	RomBg		= Next; Next += 0x280040;			// 
	DeRomBg		= 	   RomBg +  0x000040;
	
	RomSpr1		= Next; Next += 0x400000;			// Sprite 1
	RomSpr2		= Next; Next += 0x200100;			// Sprite 2
	
	DeRomSpr1	= RomSpr1    +  0x000100;
	DeRomSpr2	= RomSpr2    += 0x000100;
	
	RomSnd1		= Next; Next += 0x020000;			// ADPCM data
	RomSndSize1 = 0x020000;
	RomSnd2		= Next; Next += 0x100000;			// ADPCM data
	RomSndSize2 = 0x100000;
	
	RamStart	= Next;

	RamBg1V		= (unsigned short *)Next; Next += 0x002000;	// BG1 Video Ram
	RamBg2V		= (unsigned short *)Next; Next += 0x002000;	// BG2 Video Ram
	RamSpr1		= (unsigned short *)Next; Next += 0x004000;	// Sprite 1 Ram
	RamSpr2		= (unsigned short *)Next; Next += 0x004000;	// Sprite 2 Ram
	RamSpr3		= (unsigned short *)Next; Next += 0x000800;	// Sprite 3 Ram
	RamRaster	= (unsigned short *)Next; Next += 0x001000;	// Raster Ram
	
	RamSpr1SizeMask = 0x1FFF;
	RamSpr2SizeMask = 0x1FFF;
	RomSpr1SizeMask = 0x3FFF;
	RomSpr2SizeMask = 0x1FFF;
	
	Ram01		= Next; Next += 0x014000;					// Work Ram
	
	RamPal		= Next; Next += 0x000800;					// 1024 of X1R5G5B5 Palette
	RamZ80		= Next; Next += 0x000800;					// Z80 Ram 2K

	RamEnd		= Next;

	RamCurPal	= (unsigned int *)Next; Next += 0x000800 * sizeof(unsigned int);

	MemEnd		= Next;
	return 0;
}

static void pspikesDecodeBg(int cnt)
{
	for (int c=cnt-1; c>=0; c--) {
		for (int y=7; y>=0; y--) {
			DeRomBg[(c * 64) + (y * 8) + 7] = RomBg[0x00003 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 6] = RomBg[0x00003 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 5] = RomBg[0x00002 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 4] = RomBg[0x00002 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 3] = RomBg[0x00001 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 2] = RomBg[0x00001 + (y * 4) + (c * 32)] & 0x0f;
			DeRomBg[(c * 64) + (y * 8) + 1] = RomBg[0x00000 + (y * 4) + (c * 32)] >> 4;
			DeRomBg[(c * 64) + (y * 8) + 0] = RomBg[0x00000 + (y * 4) + (c * 32)] & 0x0f;
		}
	}
}

static void pspikesDecodeSpr(unsigned char *d, unsigned char *s, int cnt)
{
	for (int c=cnt-1; c>=0; c--)
		for (int y=15; y>=0; y--) {
			d[(c * 256) + (y * 16) + 15] = s[0x00007 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 14] = s[0x00007 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 13] = s[0x00005 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 12] = s[0x00005 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 11] = s[0x00006 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 10] = s[0x00006 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  9] = s[0x00004 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  8] = s[0x00004 + (y * 8) + (c * 128)] & 0x0f;

			d[(c * 256) + (y * 16) +  7] = s[0x00003 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  6] = s[0x00003 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  5] = s[0x00001 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  4] = s[0x00001 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  3] = s[0x00002 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  2] = s[0x00002 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  1] = s[0x00000 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  0] = s[0x00000 + (y * 8) + (c * 128)] & 0x0f;
		}
}


static int turbofrcInit()
{
	Mem = NULL;
	turbofrcMemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	turbofrcMemIndex();	
	
	// Load 68000 ROM
	if (BurnLoadRom(Rom01+0x00000, 0, 1)) return 1;
	if (BurnLoadRom(Rom01+0x40000, 1, 1)) return 1;
	if (BurnLoadRom(Rom01+0x80000, 2, 1)) return 1;
	
	// Load Graphic
	BurnLoadRom(RomBg+0x000000, 3, 1);
	BurnLoadRom(RomBg+0x080000, 4, 1);
	BurnLoadRom(RomBg+0x0A0000, 5, 1);
	BurnLoadRom(RomBg+0x120000, 6, 1);
	pspikesDecodeBg(0x14000);
		
	BurnLoadRom(RomSpr1+0x000000,  7, 2);
	BurnLoadRom(RomSpr1+0x000001,  9, 2);
	BurnLoadRom(RomSpr1+0x100000,  8, 2);
	BurnLoadRom(RomSpr1+0x100001, 10, 2);
	BurnLoadRom(RomSpr1+0x200000, 11, 2);
	BurnLoadRom(RomSpr1+0x200001, 12, 2);
	pspikesDecodeSpr(DeRomSpr1, RomSpr1, 0x6000);
	
	// Load Z80 ROM
	if (BurnLoadRom(RomZ80+0x10000, 13, 1)) return 1;
	memcpy(RomZ80, RomZ80+0x10000, 0x10000);

	BurnLoadRom(RomSnd1, 14, 1);
	BurnLoadRom(RomSnd2, 15, 1);
	
	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,			0x000000, 0x0BFFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,			0x0C0000, 0x0CFFFF, SM_RAM);	// 64K Work RAM
		SekMapMemory((unsigned char *)RamBg1V,		
									0x0D0000, 0x0D1FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamBg2V,		
									0x0D2000, 0x0D3FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamSpr1,
									0x0E0000, 0x0E3FFF, SM_RAM);
		SekMapMemory((unsigned char *)RamSpr2,
									0x0E4000, 0x0E7FFF, SM_RAM);
		SekMapMemory(Ram01+0x10000,	0x0F8000, 0x0FBFFF, SM_RAM);	// Work RAM
		SekMapMemory(Ram01+0x10000,	0xFF8000, 0xFFBFFF, SM_RAM);	// Work RAM
		SekMapMemory((unsigned char *)RamSpr3,
									0x0FC000, 0x0FC7FF, SM_RAM);
		SekMapMemory((unsigned char *)RamSpr3,
									0xFFC000, 0xFFC7FF, SM_RAM);
		SekMapMemory((unsigned char *)RamRaster,
									0x0FD000, 0x0FDFFF, SM_RAM);
		SekMapMemory((unsigned char *)RamRaster,
									0xFFD000, 0xFFDFFF, SM_RAM);
		SekMapMemory(RamPal,		0x0FE000, 0x0FE7FF, SM_ROM);	// Palette

//		SekSetReadWordHandler(0, turbofrcReadWord);
		SekSetReadByteHandler(0, turbofrcReadByte);
		SekSetWriteWordHandler(0, turbofrcWriteWord);
		SekSetWriteByteHandler(0, turbofrcWriteByte);

		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0x77FF, 0, RomZ80);
		ZetMapArea(0x0000, 0x77FF, 2, RomZ80);
		
		ZetMapArea(0x7800, 0x7FFF, 0, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 1, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 2, RamZ80);
		
		ZetMemEnd();
		
		ZetSetInHandler(turbofrcZ80PortRead);
		ZetSetOutHandler(turbofrcZ80PortWrite);
		
		ZetClose();
	}
	
	BurnYM2610Init(8000000, 25.0, 100.0, RomSnd2, &RomSndSize2, RomSnd1, &RomSndSize1, &aerofgtFMIRQHandler, aerofgtSynchroniseStream1, aerofgtGetTime1, 0);
	BurnTimerAttachZet(5000000);
	
	DrvDoReset();	
	
	return 0;
}

#ifndef BUILD_PSP
#define X_SIZE	352
#else
#define X_SIZE	512
#endif


static void turbofrcTileBackground_1(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - (RamRaster[7] & 0x1FF) - 11;
		if (x <= (352-512)) x += 512;
		
		y = my * 8 - (bg1scrolly & 0x1FF) - 2;
		if (y <= (240-512)) y += 512;
		
		if ( x<=-8 || x>=352 || y<=-8 || y>= 240 ) 
			continue;

		else
		if ( x >=0 && x < (352-8) && y >= 0 && y < (240-8)) {
			
			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
 				p[0] = pal[ d[0] | c ];
 				p[1] = pal[ d[1] | c ];
 				p[2] = pal[ d[2] | c ];
 				p[3] = pal[ d[3] | c ];
 				p[4] = pal[ d[4] | c ];
 				p[5] = pal[ d[5] | c ];
 				p[6] = pal[ d[6] | c ];
 				p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<240 ) {
	 				if ((x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if ((x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if ((x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if ((x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if ((x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if ((x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if ((x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if ((x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void turbofrcTileBackground_2(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;

	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - (bg2scrollx & 0x1FF) + 7;
		if (x <= (352-512)) x += 512;
		
		y = my * 8 - (bg2scrolly & 0x1FF) - 2;
		if (y <= (240-512)) y += 512;
		
		if ( x<=-8 || x>=352 || y<=-8 || y>= 240 ) 
			continue;
		else
		if ( x >=0 && x < (352-8) && y >= 0 && y < (240-8)) {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				
 				if (d[0] != 15) p[0] = pal[ d[0] | c ];
 				if (d[1] != 15) p[1] = pal[ d[1] | c ];
 				if (d[2] != 15) p[2] = pal[ d[2] | c ];
 				if (d[3] != 15) p[3] = pal[ d[3] | c ];
 				if (d[4] != 15) p[4] = pal[ d[4] | c ];
 				if (d[5] != 15) p[5] = pal[ d[5] | c ];
 				if (d[6] != 15) p[6] = pal[ d[6] | c ];
 				if (d[7] != 15) p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<240 ) {
	 				if (d[0] != 15 && (x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if (d[1] != 15 && (x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if (d[2] != 15 && (x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if (d[3] != 15 && (x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if (d[4] != 15 && (x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if (d[5] != 15 && (x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if (d[6] != 15 && (x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if (d[7] != 15 && (x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void pdrawgfxzoom(int bank,unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,int scalex,int scaley)
{
	if (!scalex || !scaley) return;
			
	unsigned short * p = (unsigned short *) pBurnDraw;
	unsigned char * q;
	unsigned int * pal;
	
	if (bank) {
		//if (code > RomSpr2SizeMask)
			code &= RomSpr2SizeMask;
		q = DeRomSpr2 + (code) * 256;
		pal = RamCurPal + 768;
	} else {
		//if (code > RomSpr1SizeMask)
			code &= RomSpr1SizeMask;
		q = DeRomSpr1 + (code) * 256;
		pal = RamCurPal + 512;
	}

	p += sy * X_SIZE + sx;
		
	if (sx < 0 || sx >= (352-16) || sy < 0 || sy >= (240-16) ) {
		
		if ((sx <= -16) || (sx >= 352) || (sy <= -16) || (sy >= 240))
			return;
			
		if (flipy) {
		
			p += X_SIZE * 15;
			
			if (flipx) {
			
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<240)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<352)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<352)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<352)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<352)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<352)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<352)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<352)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<352)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<352)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<352)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<352)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<352)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<352)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<352)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<352)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<352)) p[ 0] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}	
			
			} else {
	
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<240)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<352)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<352)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<352)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<352)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<352)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<352)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<352)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<352)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<352)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<352)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<352)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<352)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<352)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<352)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<352)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<352)) p[15] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}		
			}
			
		} else {
			
			if (flipx) {
			
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<240)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<352)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<352)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<352)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<352)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<352)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<352)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<352)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<352)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<352)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<352)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<352)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<352)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<352)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<352)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<352)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<352)) p[ 0] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}		
			
			} else {
	
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<240)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<352)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<352)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<352)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<352)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<352)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<352)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<352)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<352)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<352)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<352)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<352)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<352)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<352)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<352)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<352)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<352)) p[15] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}	
	
			}
			
		}
		
		return;
	}

	if (flipy) {
		
		p += X_SIZE * 15;
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}	
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}		
		}
		
	} else {
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}		
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}	

		}
		
	}

}

static void turbofrc_drawsprites(int chip,int chip_disabled_pri)
{
	int attr_start,base,first;

	base = chip * 0x0200;
	first = 4 * RamSpr3[0x1fe + base];

	//for (attr_start = base + 0x0200-8;attr_start >= first + base;attr_start -= 4) {
	for (attr_start = first + base; attr_start <= base + 0x0200-8; attr_start += 4) {
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color,pri;
// some other drivers still use this wrong table, they have to be upgraded
//      int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };

		if (!(RamSpr3[attr_start + 2] & 0x0080)) continue;
		pri = RamSpr3[attr_start + 2] & 0x0010;
		if ( chip_disabled_pri & !pri) continue;
		if (!chip_disabled_pri & (pri>>4)) continue;
		ox = RamSpr3[attr_start + 1] & 0x01ff;
		xsize = (RamSpr3[attr_start + 2] & 0x0700) >> 8;
		zoomx = (RamSpr3[attr_start + 1] & 0xf000) >> 12;
		oy = RamSpr3[attr_start + 0] & 0x01ff;
		ysize = (RamSpr3[attr_start + 2] & 0x7000) >> 12;
		zoomy = (RamSpr3[attr_start + 0] & 0xf000) >> 12;
		flipx = RamSpr3[attr_start + 2] & 0x0800;
		flipy = RamSpr3[attr_start + 2] & 0x8000;
		color = (RamSpr3[attr_start + 2] & 0x000f) << 4;	// + 16 * spritepalettebank;

		map_start = RamSpr3[attr_start + 3];

// aerofgt has this adjustment, but doing it here would break turbo force title screen
//      ox += (xsize*zoomx+2)/4;
//      oy += (ysize*zoomy+2)/4;

		zoomx = 32 - zoomx;
		zoomy = 32 - zoomy;

		for (y = 0;y <= ysize;y++) {
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16;
			else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16;

			for (x = 0;x <= xsize;x++) {
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16 - 8;
				else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16 - 8;

				if (chip == 0)	code = RamSpr1[map_start & RamSpr1SizeMask];
				else			code = RamSpr2[map_start & RamSpr2SizeMask];

				pdrawgfxzoom(chip,code,color,flipx,flipy,sx,sy,zoomx << 11, zoomy << 11);

				map_start++;
			}

			if (xsize == 2) map_start += 1;
			if (xsize == 4) map_start += 3;
			if (xsize == 5) map_start += 2;
			if (xsize == 6) map_start += 1;
		}
	}
}

static int turbofrcDraw()
{
	// update palette
	aerofgtPalUpdate();

	turbofrcTileBackground_1(RamBg1V, DeRomBg, RamCurPal);
 	turbofrcTileBackground_2(RamBg2V, DeRomBg + 0x140000, RamCurPal + 256);

/* 
	// we use the priority buffer so sprites are drawn front to back 
	turbofrc_drawsprites(0,-1); //enemy
	turbofrc_drawsprites(0, 0); //enemy
	turbofrc_drawsprites(1,-1); //ship
	turbofrc_drawsprites(1, 0); //intro
*/
	// in MAME it use a pri-buf control render to draw sprites from front to back
	// i'm not use it, is right ???
	
	turbofrc_drawsprites(0, 0); 
 	turbofrc_drawsprites(0,-1); 
	turbofrc_drawsprites(1, 0); 
	turbofrc_drawsprites(1,-1); 

	return 0;
}

static int turbofrcFrame()
{
	if (DrvReset) DrvDoReset();
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;												// Joy1
	DrvInput[1] = 0x0000;												// Joy2
	DrvInput[2] = 0x0000;												// Joy3
	DrvInput[3] = 0x0000;												// Buttons
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvJoy3[i] & 1) << i;
		DrvInput[3] |= (DrvButton[i] & 1) << i;
	}
	
	SekNewFrame();
	ZetNewFrame();
	
//	nCyclesTotal[0] = (int)((long long)10000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)10000000 / 60);
	nCyclesTotal[1] = (int)((long long)5000000  / 60);

	SekOpen(0);
	ZetOpen(0);
	
	SekRun(nCyclesTotal[0]);
	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
	
	BurnTimerEndFrame(nCyclesTotal[1]);
	BurnYM2610Update(pBurnSoundOut, nBurnSoundLen);

	ZetClose();
	SekClose();
	
	if (pBurnDraw) turbofrcDraw();
	
	return 0;
}

struct BurnDriver BurnDrvTurbofrc = {
	"turbofrc", NULL, NULL, "1991",
	"Turbo Force\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 3, HARDWARE_MISC_POST90S, GBF_VERSHOOT, 0,
	NULL, turbofrcRomInfo, turbofrcRomName, turbofrcInputInfo, turbofrcDIPInfo,
	turbofrcInit,DrvExit,turbofrcFrame,turbofrcDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,240,352,3,4
};


// ------------------------------------------------------------

static struct BurnInputInfo karatblzInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvButton + 0,	"p1 coin"},
	{"P1 Start",	BIT_DIGITAL,	DrvButton + 2,	"p1 start"},

	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 left"},
	{"P1 Right",	BIT_DIGITAL,	DrvJoy1 + 3,	"p1 right"},
	{"P1 Button 1",	BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",	BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",	BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},
	{"P1 Button 4",	BIT_DIGITAL,	DrvJoy1 + 7,	"p1 fire 4"},

	{"P2 Coin",		BIT_DIGITAL,	DrvButton + 1,	"p2 coin"},
	{"P2 Start",	BIT_DIGITAL,	DrvButton + 3,	"p2 start"},

	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 left"},
	{"P2 Right",	BIT_DIGITAL,	DrvJoy2 + 3,	"p2 right"},
	{"P2 Button 1",	BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",	BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",	BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},
	{"P2 Button 4",	BIT_DIGITAL,	DrvJoy2 + 7,	"p2 fire 4"},
	
	{"P3 Coin",		BIT_DIGITAL,	DrvButton2 + 0,	"p3 coin"},
	{"P3 Start",	BIT_DIGITAL,	DrvButton2 + 2,	"p3 start"},

	{"P3 Up",		BIT_DIGITAL,	DrvJoy3 + 0,	"p3 up"},
	{"P3 Down",		BIT_DIGITAL,	DrvJoy3 + 1,	"p3 down"},
	{"P3 Left",		BIT_DIGITAL,	DrvJoy3 + 2,	"p3 left"},
	{"P3 Right",	BIT_DIGITAL,	DrvJoy3 + 3,	"p3 right"},
	{"P3 Button 1",	BIT_DIGITAL,	DrvJoy3 + 4,	"p3 fire 1"},
	{"P3 Button 2",	BIT_DIGITAL,	DrvJoy3 + 5,	"p3 fire 2"},
	{"P3 Button 3",	BIT_DIGITAL,	DrvJoy3 + 6,	"p3 fire 3"},
	{"P3 Button 4",	BIT_DIGITAL,	DrvJoy3 + 7,	"p3 fire 4"},

	{"P4 Coin",		BIT_DIGITAL,	DrvButton2 + 1,	"p4 coin"},
	{"P4 Start",	BIT_DIGITAL,	DrvButton2 + 3,	"p4 start"},

	{"P4 Up",		BIT_DIGITAL,	DrvJoy4 + 0,	"p4 up"},
	{"P4 Down",		BIT_DIGITAL,	DrvJoy4 + 1,	"p4 down"},
	{"P4 Left",		BIT_DIGITAL,	DrvJoy4 + 2,	"p4 left"},
	{"P4 Right",	BIT_DIGITAL,	DrvJoy4 + 3,	"p4 right"},
	{"P4 Button 1",	BIT_DIGITAL,	DrvJoy4 + 4,	"p4 fire 1"},
	{"P4 Button 2",	BIT_DIGITAL,	DrvJoy4 + 5,	"p4 fire 2"},
	{"P4 Button 3",	BIT_DIGITAL,	DrvJoy4 + 6,	"p4 fire 3"},
	{"P4 Button 4",	BIT_DIGITAL,	DrvJoy4 + 7,	"p4 fire 4"},
	
	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"},
	{"Service 1",	BIT_DIGITAL,	DrvButton + 6,	"service"},
	{"Service 2",	BIT_DIGITAL,	DrvButton2 + 6,	""},
	{"Dip A",		BIT_DIPSWITCH,	DrvInput + 6,	"dip"},
	{"Dip B",		BIT_DIPSWITCH,	DrvInput + 7,	"dip"},
};

STDINPUTINFO(karatblz);

static struct BurnDIPInfo karatblzDIPList[] = {

	// Defaults
	{0x2B,	0xFF, 0xFF,	0x00, NULL},
	{0x2C,	0xFF, 0xFF,	0x40, NULL},

	// DIP 1
	{0,		0xFE, 0,	8,	  "Coinage"},
	{0x2B,	0x82, 0x07,	0x00, "1 coin 1 credit"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x01, "2 coins 1 credit"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x02, "3 coins 1 credit"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x03, "4 coins 1 credit"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x04, "1 coin 2 credits"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x05, "1 coin 3 credits"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x06, "1 coin 5 credits"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0x2B,	0x82, 0x07,	0x07, "1 coin 6 credits"},
	{0x2B,	0x00, 0x08, 0x08, NULL},
	{0,		0xFE, 0,	2,	  "2 coins to start, 1 to continue"},
	{0x2B,	0x01, 0x08,	0x00, "Off"},
	{0x2B,	0x01, 0x08,	0x08, "On"},
	{0,		0xFE, 0,	2,	  "Lives"},
	{0x2B,	0x01, 0x10,	0x00, "2"},
	{0x2B,	0x01, 0x10,	0x10, "1"},
	{0,		0xFE, 0,	4,	  "Cabinet"},
	{0x2B,	0x01, 0x60,	0x00, "2 players"},
	{0x2B,	0x01, 0x60,	0x20, "3 players"},
	{0x2B,	0x01, 0x60,	0x40, "4 players"},
	{0x2B,	0x01, 0x60,	0x60, "4 players (Team)"},
	{0,		0xFE, 0,	2,	  "Coin slot"},
	{0x2B,	0x01, 0x80,	0x00, "Same"},
	{0x2B,	0x01, 0x80,	0x80, "Individual"},

	// DIP 2
	{0,		0xFE, 0,	2,	  "Service mode"},
	{0x2C,	0x01, 0x01,	0x00, "Off"},
	{0x2C,	0x01, 0x01,	0x01, "On"},
	{0,		0xFE, 0,	4,	  "Number of enemies"},
	{0x2C,	0x01, 0x06,	0x00, "Normal"},
	{0x2C,	0x01, 0x06,	0x02, "Easy"},
	{0x2C,	0x01, 0x06,	0x04, "Hard"},
	{0x2C,	0x01, 0x06,	0x06, "Hardest"},
	{0,		0xFE, 0,	4,	  "Strength of enemies"},
	{0x2C,	0x01, 0x18,	0x00, "Normal"},
	{0x2C,	0x01, 0x18,	0x08, "Easy"},
	{0x2C,	0x01, 0x18,	0x10, "Hard"},
	{0x2C,	0x01, 0x18,	0x18, "Hardest"},
	{0,		0xFE, 0,	2,	  "Freeze"},
	{0x2C,	0x01, 0x20, 0x00, "Off"},
	{0x2C,	0x01, 0x20, 0x20, "On"},
	{0,		0xFE, 0,	2,	  "Demo sound"},
	{0x2C,	0x01, 0x40, 0x00, "Off"},
	{0x2C,	0x01, 0x40, 0x40, "On"},
	{0,		0xFE, 0,	2,	  "Flip screen"},
	{0x2C,	0x01, 0x80, 0x00, "Off"},
	{0x2C,	0x01, 0x80, 0x80, "On"},
};

STDDIPINFO(karatblz);

static struct BurnRomInfo karatblzRomDesc[] = {
	{ "rom2v3",    	  0x040000, 0x01f772e1, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "1.u15",    	  0x040000, 0xd16ee21b, BRF_ESS | BRF_PRG },

	{ "gha.u55",   	  0x080000, 0x3e0cea91, BRF_GRA },			 // gfx1
	{ "gh9.u61",  	  0x080000, 0x5d1676bd, BRF_GRA },			 // gfx2
	
	{ "u42",          0x100000, 0x65f0da84, BRF_GRA },			 // gfx3
	{ "3.u44",        0x020000, 0x34bdead2, BRF_GRA },
	{ "u43",          0x100000, 0x7b349e5d, BRF_GRA },			
	{ "4.u45",        0x020000, 0xbe4d487d, BRF_GRA },
	
	{ "u59.ghb",      0x080000, 0x158c9cde, BRF_GRA },			 // gfx4
	{ "ghd.u60",      0x080000, 0x73180ae3, BRF_GRA },

	{ "5.u92",    	  0x020000, 0x97d67510, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "u105.gh8",     0x080000, 0x7a68cb1b, BRF_SND },			 // samples
	{ "u104",         0x100000, 0x5795e884, BRF_SND },	
};

STD_ROM_PICK(karatblz);
STD_ROM_FN(karatblz);

static struct BurnRomInfo karatbluRomDesc[] = {
	{ "2.u14",    	  0x040000, 0x202e6220, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "1.u15",    	  0x040000, 0xd16ee21b, BRF_ESS | BRF_PRG },

	{ "gha.u55",   	  0x080000, 0x3e0cea91, BRF_GRA },			 // gfx1
	{ "gh9.u61",  	  0x080000, 0x5d1676bd, BRF_GRA },			 // gfx2
	
	{ "u42",          0x100000, 0x65f0da84, BRF_GRA },			 // gfx3
	{ "3.u44",        0x020000, 0x34bdead2, BRF_GRA },
	{ "u43",          0x100000, 0x7b349e5d, BRF_GRA },			
	{ "4.u45",        0x020000, 0xbe4d487d, BRF_GRA },
	
	{ "u59.ghb",      0x080000, 0x158c9cde, BRF_GRA },			 // gfx4
	{ "ghd.u60",      0x080000, 0x73180ae3, BRF_GRA },

	{ "5.u92",    	  0x020000, 0x97d67510, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "u105.gh8",     0x080000, 0x7a68cb1b, BRF_SND },			 // samples
	{ "u104",         0x100000, 0x5795e884, BRF_SND },	
};

STD_ROM_PICK(karatblu);
STD_ROM_FN(karatblu);

static struct BurnRomInfo karatbljRomDesc[] = {
	{ "2tecmo.u14",   0x040000, 0x57e52654, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "1.u15",    	  0x040000, 0xd16ee21b, BRF_ESS | BRF_PRG },

	{ "gha.u55",   	  0x080000, 0x3e0cea91, BRF_GRA },			 // gfx1
	{ "gh9.u61",  	  0x080000, 0x5d1676bd, BRF_GRA },			 // gfx2
	
	{ "u42",          0x100000, 0x65f0da84, BRF_GRA },			 // gfx3
	{ "3.u44",        0x020000, 0x34bdead2, BRF_GRA },
	{ "u43",          0x100000, 0x7b349e5d, BRF_GRA },			
	{ "4.u45",        0x020000, 0xbe4d487d, BRF_GRA },
	
	{ "u59.ghb",      0x080000, 0x158c9cde, BRF_GRA },			 // gfx4
	{ "ghd.u60",      0x080000, 0x73180ae3, BRF_GRA },

	{ "5.u92",    	  0x020000, 0x97d67510, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "u105.gh8",     0x080000, 0x7a68cb1b, BRF_SND },			 // samples
	{ "u104",         0x100000, 0x5795e884, BRF_SND },	
};

STD_ROM_PICK(karatblj);
STD_ROM_FN(karatblj);

unsigned char __fastcall karatblzReadByte(unsigned int sekAddress)
{
	sekAddress &= 0x0FFFFF;
	
	switch (sekAddress) {
		case 0x0FF000:
			return ~DrvInput[4];
		case 0x0FF001:
			return ~DrvInput[0];
		case 0x0FF002:
			return 0xFF;
		case 0x0FF003:
			return ~DrvInput[1];
		case 0x0FF004:
			return ~DrvInput[5];
		case 0x0FF005:
			return ~DrvInput[2];
		case 0x0FF006:
			return 0xFF;
		case 0x0FF007:
			return ~DrvInput[3];
		case 0x0FF008:
			return ~DrvInput[7];
		case 0x0FF009:
			return ~DrvInput[6];
		case 0x0FF00B:
			return pending_command;

//		default:
//			printf("Attempt to read byte value of location %x\n", sekAddress);
	}
	return 0;
}

/*
unsigned short __fastcall karatblzReadWord(unsigned int sekAddress)
{
	sekAddress &= 0x0FFFFF;
	
	switch (sekAddress) {


		default:
			printf("Attempt to read word value of location %x\n", sekAddress);
	}
	return 0;
}
*/

void __fastcall karatblzWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	sekAddress &= 0x0FFFFF;
	
	switch (sekAddress) {
		case 0x0FF000:
		case 0x0FF401:
		case 0x0FF403:
			
			break;
		
		case 0x0FF002:
			//if (ACCESSING_MSB) {
			//	setbank(bg1_tilemap,0,(data & 0x0100) >> 8);
			//	setbank(bg2_tilemap,1,(data & 0x0800) >> 11);	
			//}
			RamGfxBank[0] = (byteValue & 0x1);
			RamGfxBank[1] = (byteValue & 0x8) >> 3;
			break;
		case 0x0FF007:
			pending_command = 1;
			SoundCommand(byteValue);
			break;	
//		default:
//			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
	}
}

void __fastcall karatblzWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	if (( sekAddress & 0x0FF000 ) == 0x0FE000) {
		sekAddress &= 0x07FF;
		*((unsigned short *)&RamPal[sekAddress]) = wordValue;
		RamCurPal[sekAddress>>1] = CalcCol( wordValue );
		return;	
	}

	sekAddress &= 0x0FFFFF;


	switch (sekAddress) {
		case 0x0ff008:
			bg1scrollx = wordValue;
			break;
		case 0x0ff00A:
			bg1scrolly = wordValue;
			break;	
		case 0x0ff00C:
			bg2scrollx = wordValue;
			break;
		case 0x0ff00E:
			bg2scrolly = wordValue;
			break;	
//		default:
//			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
	}
}

static int karatblzMemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01 		= Next; Next += 0x080000;			// 68000 ROM
	RomZ80		= Next; Next += 0x030000;			// Z80 ROM
	
	RomBg		= Next; Next += 0x200040;			// Background, 1M 8x8x4bit decode to 2M + 64Byte safe 
	DeRomBg		= 	   RomBg +  0x000040;
	
	RomSpr1		= Next; Next += 0x800000;			// Sprite 1	 , 1M 16x16x4bit decode to 2M + 256Byte safe 
	RomSpr2		= Next; Next += 0x200100;			// Sprite 2
	
	DeRomSpr1	= RomSpr1    +  0x000100;
	DeRomSpr2	= RomSpr2    += 0x000100;
	
	RomSnd1		= Next; Next += 0x080000;			// ADPCM data
	RomSndSize1 = 0x080000;
	RomSnd2		= Next; Next += 0x100000;			// ADPCM data
	RomSndSize2 = 0x100000;
	
	RamStart	= Next;

	RamBg1V		= (unsigned short *)Next; Next += 0x002000;	// BG1 Video Ram
	RamBg2V		= (unsigned short *)Next; Next += 0x002000;	// BG2 Video Ram
	RamSpr1		= (unsigned short *)Next; Next += 0x010000;	// Sprite 1 Ram
	RamSpr2		= (unsigned short *)Next; Next += 0x010000;	// Sprite 2 Ram
	RamSpr3		= (unsigned short *)Next; Next += 0x000800;	// Sprite 3 Ram
	Ram01		= Next; Next += 0x014000;					// Work Ram 1 + Work Ram 1
	RamPal		= Next; Next += 0x000800;					// 1024 of X1R5G5B5 Palette
	RamZ80		= Next; Next += 0x000800;					// Z80 Ram 2K

	RamSpr1SizeMask = 0x7FFF;
	RamSpr2SizeMask = 0x7FFF;
	RomSpr1SizeMask = 0x7FFF;
	RomSpr2SizeMask = 0x1FFF;

	RamEnd		= Next;

	RamCurPal	= (unsigned int *)Next; Next += 0x000800 * sizeof(unsigned int);

	MemEnd		= Next;
	return 0;
}

static int karatblzInit()
{
	Mem = NULL;
	karatblzMemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	karatblzMemIndex();	
	
	// Load 68000 ROM
	if (BurnLoadRom(Rom01+0x00000, 0, 1)) return 1;
	if (BurnLoadRom(Rom01+0x40000, 1, 1)) return 1;
	
	// Load Graphic
	BurnLoadRom(RomBg+0x00000, 2, 1);
	BurnLoadRom(RomBg+0x80000, 3, 1);
	pspikesDecodeBg(0x10000);
	
	BurnLoadRom(RomSpr1+0x000000, 4, 2);
	BurnLoadRom(RomSpr1+0x000001, 6, 2);
	BurnLoadRom(RomSpr1+0x200000, 5, 2);
	BurnLoadRom(RomSpr1+0x200001, 7, 2);
	BurnLoadRom(RomSpr1+0x400000, 8, 2);
	BurnLoadRom(RomSpr1+0x400001, 9, 2);
	pspikesDecodeSpr(DeRomSpr1, RomSpr1, 0xA000);
	
	// Load Z80 ROM
	if (BurnLoadRom(RomZ80+0x10000, 10, 1)) return 1;
	memcpy(RomZ80, RomZ80+0x10000, 0x10000);

	BurnLoadRom(RomSnd1, 11, 1);
	BurnLoadRom(RomSnd2, 12, 1);
	
	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,			0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory((unsigned char *)RamBg1V,		
									0x080000, 0x081FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamBg2V,		
									0x082000, 0x083FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamSpr1,
									0x0A0000, 0x0AFFFF, SM_RAM);
		SekMapMemory((unsigned char *)RamSpr2,
									0x0B0000, 0x0BFFFF, SM_RAM);
		SekMapMemory(Ram01,			0x0C0000, 0x0CFFFF, SM_RAM);	// 64K Work RAM
		SekMapMemory(Ram01+0x10000,	0x0F8000, 0x0FBFFF, SM_RAM);	// Work RAM
		SekMapMemory(Ram01+0x10000,	0xFF8000, 0xFFBFFF, SM_RAM);	// Work RAM
		SekMapMemory((unsigned char *)RamSpr3,
									0x0FC000, 0x0FC7FF, SM_RAM);
		SekMapMemory(RamPal,		0x0FE000, 0x0FE7FF, SM_ROM);	// Palette

//		SekSetReadWordHandler(0, karatblzReadWord);
		SekSetReadByteHandler(0, karatblzReadByte);
		SekSetWriteWordHandler(0, karatblzWriteWord);
		SekSetWriteByteHandler(0, karatblzWriteByte);

		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0x77FF, 0, RomZ80);
		ZetMapArea(0x0000, 0x77FF, 2, RomZ80);
		
		ZetMapArea(0x7800, 0x7FFF, 0, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 1, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 2, RamZ80);
		
		ZetMemEnd();
		
		ZetSetInHandler(turbofrcZ80PortRead);
		ZetSetOutHandler(turbofrcZ80PortWrite);
		
		ZetClose();
	}
	
	BurnYM2610Init(8000000, 25.0, 100.0, RomSnd2, &RomSndSize2, RomSnd1, &RomSndSize1, &aerofgtFMIRQHandler, aerofgtSynchroniseStream, aerofgtGetTime, 0);
	BurnTimerAttachZet(4000000);
	
	DrvDoReset();	
	
	return 0;
}

static void karatblzTileBackground_1(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}
		
		x = mx * 8 - (((signed short)bg1scrollx + 8)& 0x1FF);
		if (x <= (352-512)) x += 512;
		
		y = my * 8 - (bg1scrolly & 0x1FF);
		if (y <= (240-512)) y += 512;
		
		if ( x<=-8 || x>=352 || y<=-8 || y>= 240 ) 
			continue;

		else
		if ( x >=0 && x < (352-8) && y >= 0 && y < (240-8)) {
			
			unsigned char *d = BgGfx + ( (bg[offs] & 0x1FFF) + ( RamGfxBank[0] << 13 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
 				p[0] = pal[ d[0] | c ];
 				p[1] = pal[ d[1] | c ];
 				p[2] = pal[ d[2] | c ];
 				p[3] = pal[ d[3] | c ];
 				p[4] = pal[ d[4] | c ];
 				p[5] = pal[ d[5] | c ];
 				p[6] = pal[ d[6] | c ];
 				p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x1FFF) + ( RamGfxBank[0] << 13 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<240 ) {
	 				if ((x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if ((x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if ((x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if ((x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if ((x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if ((x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if ((x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if ((x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void karatblzTileBackground_2(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;

	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - ( ((signed short)bg2scrollx + 4) & 0x1FF);
		if (x <= (352-512)) x += 512;
		
		y = my * 8 - (bg2scrolly & 0x1FF);
		if (y <= (240-512)) y += 512;
		
		if ( x<=-8 || x>=352 || y<=-8 || y>= 240 ) 
			continue;
		else
		if ( x >=0 && x < (352-8) && y >= 0 && y < (240-8)) {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x1FFF) + ( RamGfxBank[1] << 13 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				
 				if (d[0] != 15) p[0] = pal[ d[0] | c ];
 				if (d[1] != 15) p[1] = pal[ d[1] | c ];
 				if (d[2] != 15) p[2] = pal[ d[2] | c ];
 				if (d[3] != 15) p[3] = pal[ d[3] | c ];
 				if (d[4] != 15) p[4] = pal[ d[4] | c ];
 				if (d[5] != 15) p[5] = pal[ d[5] | c ];
 				if (d[6] != 15) p[6] = pal[ d[6] | c ];
 				if (d[7] != 15) p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x1FFF) + ( RamGfxBank[1] << 13 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<240 ) {
	 				if (d[0] != 15 && (x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if (d[1] != 15 && (x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if (d[2] != 15 && (x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if (d[3] != 15 && (x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if (d[4] != 15 && (x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if (d[5] != 15 && (x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if (d[6] != 15 && (x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if (d[7] != 15 && (x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static int karatblzDraw()
{
	// update palette
	aerofgtPalUpdate();

	karatblzTileBackground_1(RamBg1V, DeRomBg, RamCurPal);
 	karatblzTileBackground_2(RamBg2V, DeRomBg + 0x100000, RamCurPal + 256);

/* 	
	turbofrc_drawsprites(1,-1); 
	turbofrc_drawsprites(1, 0); 
 	turbofrc_drawsprites(0,-1); 
	turbofrc_drawsprites(0, 0); 
*/

	turbofrc_drawsprites(0, 0); 
 	turbofrc_drawsprites(0,-1); 
	turbofrc_drawsprites(1, 0); 
	turbofrc_drawsprites(1,-1); 

	return 0;
}

static int karatblzFrame()
{
	if (DrvReset) DrvDoReset();
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;												// Joy1
	DrvInput[1] = 0x0000;												// Joy2
	DrvInput[2] = 0x0000;												// Joy3
	DrvInput[3] = 0x0000;												// Joy4
	DrvInput[4] = 0x0000;												// Buttons1
	DrvInput[5] = 0x0000;												// Buttons2
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvJoy3[i] & 1) << i;
		DrvInput[3] |= (DrvJoy4[i] & 1) << i;
		DrvInput[4] |= (DrvButton[i] & 1) << i;
		DrvInput[5] |= (DrvButton2[i] & 1) << i;
	}
	
	SekNewFrame();
	ZetNewFrame();
	
//	nCyclesTotal[0] = (int)((long long)10000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)10000000 / 60);
	nCyclesTotal[1] = (int)((long long)4000000  / 60);

	SekOpen(0);
	ZetOpen(0);
	
	SekRun(nCyclesTotal[0]);
	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
	
	BurnTimerEndFrame(nCyclesTotal[1]);
	BurnYM2610Update(pBurnSoundOut, nBurnSoundLen);

	ZetClose();
	SekClose();
	
	if (pBurnDraw) karatblzDraw();
	
	return 0;
}

struct BurnDriver BurnDrvKaratblz = {
	"karatblz", NULL, NULL, "1991",
	"Karate Blazers (World?)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 4, HARDWARE_MISC_POST90S, GBF_SCRFIGHT, 0,
	NULL, karatblzRomInfo, karatblzRomName, karatblzInputInfo, karatblzDIPInfo,
	karatblzInit,DrvExit,karatblzFrame,karatblzDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

struct BurnDriver BurnDrvKaratblu = {
	"karatblu", "karatblz", NULL, "1991",
	"Karate Blazers (US)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 4, HARDWARE_MISC_POST90S, GBF_SCRFIGHT, 0,
	NULL, karatbluRomInfo, karatbluRomName, karatblzInputInfo, karatblzDIPInfo,
	karatblzInit,DrvExit,karatblzFrame,karatblzDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

struct BurnDriver BurnDrvKaratblj = {
	"karatblj", "karatblz", NULL, "1991",
	"Karate Blazers (Japan)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 4, HARDWARE_MISC_POST90S, GBF_SCRFIGHT, 0,
	NULL, karatbljRomInfo, karatbljRomName, karatblzInputInfo, karatblzDIPInfo,
	karatblzInit,DrvExit,karatblzFrame,karatblzDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

// -----------------------------------------------------------

static struct BurnInputInfo spinlbrkInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvButton + 0,	"p1 coin"},
	{"P1 Start",	BIT_DIGITAL,	DrvButton + 2,	"p1 start"},

	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 left"},
	{"P1 Right",	BIT_DIGITAL,	DrvJoy1 + 3,	"p1 right"},
	{"P1 Button 1",	BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",	BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",	BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},

	{"P2 Coin",		BIT_DIGITAL,	DrvButton + 1,	"p2 coin"},
	{"P2 Start",	BIT_DIGITAL,	DrvButton + 3,	"p2 start"},

	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 left"},
	{"P2 Right",	BIT_DIGITAL,	DrvJoy2 + 3,	"p2 right"},
	{"P2 Button 1",	BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",	BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",	BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},
	
	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"},
	{"Service",		BIT_DIGITAL,	DrvButton + 6,	"service"},
	{"Dip A",		BIT_DIPSWITCH,	DrvInput + 3,	"dip"},
	{"Dip B",		BIT_DIPSWITCH,	DrvInput + 4,	"dip"},
};

STDINPUTINFO(spinlbrk);

static struct BurnDIPInfo spinlbrkDIPList[] = {

	// Defaults
	{0x14,	0xFF, 0xFF,	0x00, NULL},
	{0x15,	0xFF, 0xFF,	0x00, NULL},

	// DIP 1
	// Coin slot Individual condition
	{0,		0xFE, 0,	16,	  "Coin A"},
	{0x14,	0x02, 0x0F, 0x00, "1 coin 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x01, "1 coin 2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x02, "1 coin 3 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x03, "1 coin 4 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x04, "1 coin 5 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x05, "1 coin 6 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x06, "2 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x07, "3 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x08, "4 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x09, "5 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0A, "2 coins 3 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0B, "2-2-1-1 coins 1-1-1-1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0C, "2-2 coins 1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0D, "1-1-1-1-1 coin 1-1-1-1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0E, "1-1-1-1 coin 1-1-1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0x0F, 0x0F, "1-1 coin 1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0,		0xFE, 0,	16,	  "Coin B"},
	{0x14,	0x02, 0xF0, 0x00, "1 coin 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x10, "1 coin 2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x20, "1 coin 3 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x30, "1 coin 4 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x40, "1 coin 5 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x50, "1 coin 6 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x60, "2 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x70, "3 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x80, "4 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0x90, "5 coins 1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xA0, "2 coins 3 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xB0, "2-2-1-1 coins 1-1-1-1 health pack"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xC0, "2-2 coins 1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xD0, "1-1-1-1-1 coin 1-1-1-1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xE0, "1-1-1-1 coin 1-1-1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},
	{0x14,	0x02, 0xF0, 0xF0, "1-1 coin 1-2 health packs"},
	{0x15,	0x00, 0x04, 0x00, NULL},

	// Coin slot Same condition
	{0,		0xFE, 0,	16,	  "Coin A"},
	{0x14,	0x02, 0x0F, 0x00, "1 coin 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x01, "1 coin 2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x02, "1 coin 3 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x03, "1 coin 4 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x04, "1 coin 5 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x05, "1 coin 6 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x06, "2 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x07, "3 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x08, "4 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x09, "5 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0A, "2 coins 3 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0B, "2-2-1-1 coins 1-1-1-1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0C, "2-2 coins 1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0D, "1-1-1-1-1 coin 1-1-1-1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0E, "1-1-1-1 coin 1-1-1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0x0F, 0x0F, "1-1 coin 1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0,		0xFE, 0,	16,	  "Coin B"},
	{0x14,	0x02, 0xF0, 0x00, "1 coin 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x10, "1 coin 2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x20, "1 coin 3 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x30, "1 coin 4 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x40, "1 coin 5 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x50, "1 coin 6 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x60, "2 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x70, "3 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x80, "4 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0x90, "5 coins 1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xA0, "2 coins 3 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xB0, "2-2-1-1 coins 1-1-1-1 credit"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xC0, "2-2 coins 1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xD0, "1-1-1-1-1 coin 1-1-1-1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xE0, "1-1-1-1 coin 1-1-1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},
	{0x14,	0x02, 0xF0, 0xF0, "1-1 coin 1-2 credits"},
	{0x15,	0x00, 0x04, 0x04, NULL},

	// DIP 2
	{0,		0xFE, 0,	4,	  "Difficulty"},
	{0x15,	0x01, 0x03, 0x00, "Normal"},
	{0x15,	0x01, 0x03, 0x01, "Easy"},
	{0x15,	0x01, 0x03, 0x02, "Hard"},
	{0x15,	0x01, 0x03, 0x03, "Hardest"},
	{0,		0xFE, 0,	2,	  "Coin slot"},
	{0x15,	0x01, 0x04, 0x04, "Same"},
	{0x15,	0x01, 0x04, 0x00, "Individual"},
	{0,		0xFE, 0,	2,	  "Flip screen"},
	{0x15,	0x01, 0x08, 0x00, "Off"},
	{0x15,	0x01, 0x08, 0x08, "On"},
	{0,		0xFE, 0,	2,	  "Lever type"},
	{0x15,	0x01, 0x10, 0x00, "Digital"},
	{0x15,	0x01, 0x10, 0x10, "Analog"},
	{0,		0xFE, 0,	2,	  "Service mode"},
	{0x15,	0x01, 0x20, 0x00, "Off"},
	{0x15,	0x01, 0x20, 0x20, "On"},
};

static struct BurnDIPInfo spinlbrk_DIPList[] = {

	{0,		0xFE, 0,	2,	  "Health pack"},
	{0x15,	0x01, 0x40, 0x00, "32 hitpoints"},
	{0x15,	0x01, 0x40, 0x40, "40 hitpoints"},
	{0,		0xFE, 0,	2,	  "Life restoration"},
	{0x15,	0x01, 0x80, 0x00, "10 points"},
	{0x15,	0x01, 0x80, 0x80, "5 points"},
};

static struct BurnDIPInfo spinlbruDIPList[] = {

	{0,		0xFE, 0,	2,	  "Health pack"},
	{0x15,	0x01, 0x40, 0x00, "20 hitpoints"},
	{0x15,	0x01, 0x40, 0x40, "32 hitpoints"},
	{0,		0xFE, 0,	2,	  "Life restoration"},
	{0x15,	0x01, 0x80, 0x00, "10 points"},
	{0x15,	0x01, 0x80, 0x80, "5 points"},
};

static struct BurnDIPInfo spinlbrjDIPList[] = {

	// Defaults
	{0x14,	0xFF, 0xFF,	0x00, NULL},
	{0x15,	0xFF, 0xFF,	0x00, NULL},

	// DIP 1
	{0,		0xFE, 0,	16,	  "Coin A"},
	{0x14,	0x01, 0x0F, 0x00, "1 coin 1 credit"},
	{0x14,	0x01, 0x0F, 0x01, "1 coin 2 credits"},
	{0x14,	0x01, 0x0F, 0x02, "1 coin 3 credits"},
	{0x14,	0x01, 0x0F, 0x03, "1 coin 4 credits"},
	{0x14,	0x01, 0x0F, 0x04, "1 coin 5 credits"},
	{0x14,	0x01, 0x0F, 0x05, "1 coin 6 credits"},
	{0x14,	0x01, 0x0F, 0x06, "2 coins 1 credit"},
	{0x14,	0x01, 0x0F, 0x07, "3 coins 1 credit"},
	{0x14,	0x01, 0x0F, 0x08, "4 coins 1 credit"},
	{0x14,	0x01, 0x0F, 0x09, "5 coins 1 credit"},
	{0x14,	0x01, 0x0F, 0x0A, "2 coins 3 credits"},
	{0x14,	0x01, 0x0F, 0x0B, "2-2-1-1 coins 1-1-1-1 credit"},
	{0x14,	0x01, 0x0F, 0x0C, "2-2 coins 1-2 credits"},
	{0x14,	0x01, 0x0F, 0x0D, "1-1-1-1-1 coin 1-1-1-1-2 credits"},
	{0x14,	0x01, 0x0F, 0x0E, "1-1-1-1 coin 1-1-1-2 credits"},
	{0x14,	0x01, 0x0F, 0x0F, "1-1 coin 1-2 credits"},
	{0,		0xFE, 0,	16,	  "Coin B"},
	{0x14,	0x01, 0xF0, 0x00, "1 coin 1 credit"},
	{0x14,	0x01, 0xF0, 0x10, "1 coin 2 credits"},
	{0x14,	0x01, 0xF0, 0x20, "1 coin 3 credits"},
	{0x14,	0x01, 0xF0, 0x30, "1 coin 4 credits"},
	{0x14,	0x01, 0xF0, 0x40, "1 coin 5 credits"},
	{0x14,	0x01, 0xF0, 0x50, "1 coin 6 credits"},
	{0x14,	0x01, 0xF0, 0x60, "2 coins 1 credit"},
	{0x14,	0x01, 0xF0, 0x70, "3 coins 1 credit"},
	{0x14,	0x01, 0xF0, 0x80, "4 coins 1 credit"},
	{0x14,	0x01, 0xF0, 0x90, "5 coins 1 credit"},
	{0x14,	0x01, 0xF0, 0xA0, "2 coins 3 credits"},
	{0x14,	0x01, 0xF0, 0xB0, "2-2-1-1 coins 1-1-1-1 credit"},
	{0x14,	0x01, 0xF0, 0xC0, "2-2 coins 1-2 credits"},
	{0x14,	0x01, 0xF0, 0xD0, "1-1-1-1-1 coin 1-1-1-1-2 credits"},
	{0x14,	0x01, 0xF0, 0xE0, "1-1-1-1 coin 1-1-1-2 credits"},
	{0x14,	0x01, 0xF0, 0xF0, "1-1 coin 1-2 credits"},

	// DIP 2
	{0,		0xFE, 0,	4,	  "Difficulty"},
	{0x15,	0x01, 0x03, 0x00, "Normal"},
	{0x15,	0x01, 0x03, 0x01, "Easy"},
	{0x15,	0x01, 0x03, 0x02, "Hard"},
	{0x15,	0x01, 0x03, 0x03, "Hardest"},
	{0,		0xFE, 0,	2,	  "Continue"},
	{0x15,	0x01, 0x04, 0x00, "Unlimited"},
	{0x15,	0x01, 0x04, 0x04, "6 times"},
	{0,		0xFE, 0,	2,	  "Flip screen"},
	{0x15,	0x01, 0x08, 0x00, "Off"},
	{0x15,	0x01, 0x08, 0x08, "On"},
	{0,		0xFE, 0,	2,	  "Lever type"},
	{0x15,	0x01, 0x10, 0x00, "Digital"},
	{0x15,	0x01, 0x10, 0x10, "Analog"},
	{0,		0xFE, 0,	2,	  "Service mode"},
	{0x15,	0x01, 0x20, 0x00, "Off"},
	{0x15,	0x01, 0x20, 0x20, "On"},
	{0,		0xFE, 0,	2,	  "Health pack"},
	{0x15,	0x01, 0x40, 0x00, "32 hitpoints"},
	{0x15,	0x01, 0x40, 0x40, "40 hitpoints"},
	{0,		0xFE, 0,	2,	  "Life restoration"},
	{0x15,	0x01, 0x80, 0x00, "10 points"},
	{0x15,	0x01, 0x80, 0x80, "5 points"},
};

STDDIPINFOEXT(spinlbrk, spinlbrk, spinlbrk_);
STDDIPINFOEXT(spinlbru, spinlbrk, spinlbru);
STDDIPINFO(spinlbrj);

static struct BurnRomInfo spinlbrkRomDesc[] = {
	{ "ic98",    	  0x010000, 0x36c2bf70, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "ic104",    	  0x010000, 0x34a7e158, BRF_ESS | BRF_PRG },
	{ "ic93",    	  0x010000, 0x726f4683, BRF_ESS | BRF_PRG },
	{ "ic94",    	  0x010000, 0xc4385e03, BRF_ESS | BRF_PRG },

	{ "ic15",         0x080000, 0xe318cf3a, BRF_GRA },			 // gfx 1
	{ "ic9",          0x080000, 0xe071f674, BRF_GRA },

	{ "ic17",         0x080000, 0xa63d5a55, BRF_GRA },			 // gfx 2
	{ "ic11",         0x080000, 0x7dcc913d, BRF_GRA },
	{ "ic16",         0x080000, 0x0d84af7f, BRF_GRA },
	
	{ "ic12",         0x080000, 0xd63fac4e, BRF_GRA },			 // gfx 3
	{ "ic18",         0x080000, 0x5a60444b, BRF_GRA },
	
	{ "ic14",         0x080000, 0x1befd0f3, BRF_GRA },			 // gfx 4
	{ "ic20",         0x080000, 0xc2f84a61, BRF_GRA },
	{ "ic35",         0x080000, 0xeba8e1a3, BRF_GRA },
	{ "ic40",         0x080000, 0x5ef5aa7e, BRF_GRA },
	
	{ "ic19",         0x010000, 0xdb24eeaa, BRF_GRA },			 // gfx 5, hardcoded sprite maps
	{ "ic13",         0x010000, 0x97025bf4, BRF_GRA },

	{ "ic117",    	  0x008000, 0x625ada41, BRF_ESS | BRF_PRG }, // Sound CPU
	{ "ic118",    	  0x010000, 0x1025f024, BRF_ESS | BRF_PRG },
	
	{ "ic166",	      0x080000, 0x6e0d063a, BRF_SND },			 // samples
	{ "ic163",   	  0x080000, 0xe6621dfb, BRF_SND },	
	
	{ "epl16p8bp.ic100",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic127",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic133",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic99",    263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic114",    279, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic95",     279, 0x00000000, BRF_OPT | BRF_NODUMP },
};

STD_ROM_PICK(spinlbrk);
STD_ROM_FN(spinlbrk);

static struct BurnRomInfo spinlbruRomDesc[] = {
	{ "ic98.u5",      0x010000, 0x3a0f7667, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "ic104.u6",     0x010000, 0xa0e0af31, BRF_ESS | BRF_PRG },
	{ "ic93.u4",      0x010000, 0x0cf73029, BRF_ESS | BRF_PRG },
	{ "ic94.u3",      0x010000, 0x5cf7c426, BRF_ESS | BRF_PRG },

	{ "ic15",         0x080000, 0xe318cf3a, BRF_GRA },			 // gfx 1
	{ "ic9",          0x080000, 0xe071f674, BRF_GRA },

	{ "ic17",         0x080000, 0xa63d5a55, BRF_GRA },			 // gfx 2
	{ "ic11",         0x080000, 0x7dcc913d, BRF_GRA },
	{ "ic16",         0x080000, 0x0d84af7f, BRF_GRA },
	
	{ "ic12",         0x080000, 0xd63fac4e, BRF_GRA },			 // gfx 3
	{ "ic18",         0x080000, 0x5a60444b, BRF_GRA },
	
	{ "ic14",         0x080000, 0x1befd0f3, BRF_GRA },			 // gfx 4
	{ "ic20",         0x080000, 0xc2f84a61, BRF_GRA },
	{ "ic35",         0x080000, 0xeba8e1a3, BRF_GRA },
	{ "ic40",         0x080000, 0x5ef5aa7e, BRF_GRA },
	
	{ "ic19",         0x010000, 0xdb24eeaa, BRF_GRA },			 // gfx 5, hardcoded sprite maps
	{ "ic13",         0x010000, 0x97025bf4, BRF_GRA },

	{ "ic117",    	  0x008000, 0x625ada41, BRF_ESS | BRF_PRG }, // Sound CPU
	{ "ic118",    	  0x010000, 0x1025f024, BRF_ESS | BRF_PRG },
	
	{ "ic166",	      0x080000, 0x6e0d063a, BRF_SND },			 // samples
	{ "ic163",   	  0x080000, 0xe6621dfb, BRF_SND },	
	
	{ "epl16p8bp.ic100",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic127",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic133",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic99",    263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic114",    279, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic95",     279, 0x00000000, BRF_OPT | BRF_NODUMP },		
};

STD_ROM_PICK(spinlbru);
STD_ROM_FN(spinlbru);

static struct BurnRomInfo spinlbrjRomDesc[] = {
	{ "j5",			  0x010000, 0x6a3d690e, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "j6",			  0x010000, 0x869593fa, BRF_ESS | BRF_PRG },
	{ "j4",			  0x010000, 0x33e33912, BRF_ESS | BRF_PRG },
	{ "j3",     	  0x010000, 0x16ca61d0, BRF_ESS | BRF_PRG },

	{ "ic15",         0x080000, 0xe318cf3a, BRF_GRA },			 // gfx 1
	{ "ic9",          0x080000, 0xe071f674, BRF_GRA },

	{ "ic17",         0x080000, 0xa63d5a55, BRF_GRA },			 // gfx 2
	{ "ic11",         0x080000, 0x7dcc913d, BRF_GRA },
	{ "ic16",         0x080000, 0x0d84af7f, BRF_GRA },
	
	{ "ic12",         0x080000, 0xd63fac4e, BRF_GRA },			 // gfx 3
	{ "ic18",         0x080000, 0x5a60444b, BRF_GRA },
	
	{ "ic14",         0x080000, 0x1befd0f3, BRF_GRA },			 // gfx 4
	{ "ic20",         0x080000, 0xc2f84a61, BRF_GRA },
	{ "ic35",         0x080000, 0xeba8e1a3, BRF_GRA },
	{ "ic40",         0x080000, 0x5ef5aa7e, BRF_GRA },
	
	{ "ic19",         0x010000, 0xdb24eeaa, BRF_GRA },			 // gfx 5, hardcoded sprite maps
	{ "ic13",         0x010000, 0x97025bf4, BRF_GRA },

	{ "ic117",    	  0x008000, 0x625ada41, BRF_ESS | BRF_PRG }, // Sound CPU
	{ "ic118",    	  0x010000, 0x1025f024, BRF_ESS | BRF_PRG },
	
	{ "ic166",	      0x080000, 0x6e0d063a, BRF_SND },			 // samples
	{ "ic163",   	  0x080000, 0xe6621dfb, BRF_SND },	
	
	{ "epl16p8bp.ic100",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic127",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic133",   263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "epl16p8bp.ic99",    263, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic114",    279, 0x00000000, BRF_OPT | BRF_NODUMP },	
	{ "gal16v8a.ic95",     279, 0x00000000, BRF_OPT | BRF_NODUMP },	
};

STD_ROM_PICK(spinlbrj);
STD_ROM_FN(spinlbrj);

unsigned short __fastcall spinlbrkReadWord(unsigned int sekAddress)
{
	switch (sekAddress) {
		case 0xFFF000:
			return ~(DrvInput[0] | (DrvInput[2] << 8));
		case 0xFFF002:
			return ~(DrvInput[1]);
		case 0xFFF004:
			return ~(DrvInput[3] | (DrvInput[4] << 8));

//		default:
//			printf("Attempt to read word value of location %x\n", sekAddress);
	}
	return 0;
}


void __fastcall spinlbrkWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	switch (sekAddress) {
		case 0xFFF401:
		case 0xFFF403:
			// NOP
			break;
			
		case 0xFFF007:
			pending_command = 1;
			SoundCommand(byteValue);
			break;	

//		default:
//			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
	}
}

void __fastcall spinlbrkWriteWord(unsigned int sekAddress, unsigned short wordValue)
{

	if (( sekAddress & 0xFFF000 ) == 0xFFE000) {
		sekAddress &= 0x07FF;
		*((unsigned short *)&RamPal[sekAddress]) = wordValue;
		RamCurPal[sekAddress>>1] = CalcCol( wordValue );
		return;	
	}

	switch (sekAddress) {
		case 0xFFF000:
			RamGfxBank[0] = (wordValue & 0x07);
			RamGfxBank[1] = (wordValue & 0x38) >> 3;
			break;
		case 0xFFF002:
			bg2scrollx = wordValue;
			break;
		case 0xFFF008:
			// NOP
			break;

//		default:
//			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
	}
}

static int spinlbrkMemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01 		= Next; Next += 0x040000;			// 68000 ROM
	RomZ80		= Next; Next += 0x030000;			// Z80 ROM
	
	RomBg		= Next; Next += 0x500040;			// Background, 2.5M 8x8x4bit
	DeRomBg		= 	   RomBg +  0x000040;
	
	RomSpr1		= Next; Next += 0x200000;			// Sprite 1
	RomSpr2		= Next; Next += 0x400100;			// Sprite 2
	
	DeRomSpr1	= RomSpr1    +  0x000100;
	DeRomSpr2	= RomSpr2    += 0x000100;
	
	RomSnd2		= Next; Next += 0x100000;			// ADPCM data
	RomSnd1		= RomSnd2;

	RomSndSize1 = 0x100000;
	RomSndSize2 = 0x100000;
	
	RamSpr2		= (unsigned short *)Next; Next += 0x020000;	// Sprite 2 Ram
	RamSpr1		= (unsigned short *)Next; Next += 0x004000;	// Sprite 1 Ram
		
	RamStart	= Next;

	RamBg1V		= (unsigned short *)Next; Next += 0x001000;	// BG1 Video Ram
	RamBg2V		= (unsigned short *)Next; Next += 0x002000;	// BG2 Video Ram
	Ram01		= Next; Next += 0x004000;					// Work Ram
	RamSpr3		= (unsigned short *)Next; Next += 0x000800;	// Sprite 3 Ram
	RamRaster	= (unsigned short *)Next; Next += 0x000200;	// Raster
	RamPal		= Next; Next += 0x000800;					// 1024 of X1R5G5B5 Palette
	RamZ80		= Next; Next += 0x000800;					// Z80 Ram 2K

	RamSpr1SizeMask = 0x1FFF;
	RamSpr2SizeMask = 0xFFFF;
	
	RomSpr1SizeMask = 0x1FFF;
	RomSpr2SizeMask = 0x3FFF;

	RamEnd		= Next;

	RamCurPal	= (unsigned int *)Next; Next += 0x000800 * sizeof(unsigned int);

	MemEnd		= Next;
	return 0;
}

static int spinlbrkInit()
{
	Mem = NULL;
	spinlbrkMemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	spinlbrkMemIndex();	
	
	// Load 68000 ROM
	if (BurnLoadRom(Rom01+0x00001, 0, 2)) return 1;
	if (BurnLoadRom(Rom01+0x00000, 1, 2)) return 1;
	if (BurnLoadRom(Rom01+0x20001, 2, 2)) return 1;
	if (BurnLoadRom(Rom01+0x20000, 3, 2)) return 1;
	
	// Load Graphic
	BurnLoadRom(RomBg+0x000000, 4, 1);
	BurnLoadRom(RomBg+0x080000, 5, 1);
	BurnLoadRom(RomBg+0x100000, 6, 1);
	BurnLoadRom(RomBg+0x180000, 7, 1);
	BurnLoadRom(RomBg+0x200000, 8, 1);
	pspikesDecodeBg(0x14000);
	
	BurnLoadRom(RomSpr1+0x000000,  9, 2);
	BurnLoadRom(RomSpr1+0x000001, 10, 2);
	BurnLoadRom(RomSpr1+0x100000, 11, 2);
	BurnLoadRom(RomSpr1+0x100001, 13, 2);
	BurnLoadRom(RomSpr1+0x200000, 12, 2);
	BurnLoadRom(RomSpr1+0x200001, 14, 2);
	pspikesDecodeSpr(DeRomSpr1, RomSpr1, 0x6000);
	
	BurnLoadRom((unsigned char *)RamSpr2+0x000001, 15, 2);
	BurnLoadRom((unsigned char *)RamSpr2+0x000000, 16, 2);
	
	// Load Z80 ROM
	if (BurnLoadRom(RomZ80+0x00000, 17, 1)) return 1;
	if (BurnLoadRom(RomZ80+0x08000, 17, 1)) return 1;
	if (BurnLoadRom(RomZ80+0x10000, 18, 1)) return 1;
	
	BurnLoadRom(RomSnd2+0x00000, 19, 1);
	BurnLoadRom(RomSnd2+0x80000, 20, 1);
	
	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,			0x000000, 0x04FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory((unsigned char *)RamBg1V,		
									0x080000, 0x080FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamBg2V,		
									0x082000, 0x083FFF, SM_RAM);
		SekMapMemory(Ram01,			0xFF8000, 0xFFBFFF, SM_RAM);	// Work RAM
		SekMapMemory((unsigned char *)RamSpr3,
									0xFFC000, 0xFFC7FF, SM_RAM);
		SekMapMemory((unsigned char *)RamRaster,
									0xFFD000, 0xFFD1FF, SM_RAM);
		SekMapMemory(RamPal,		0xFFE000, 0xFFE7FF, SM_ROM);	// Palette

		SekSetReadWordHandler(0, spinlbrkReadWord);
//		SekSetReadByteHandler(0, spinlbrkReadByte);
		SekSetWriteWordHandler(0, spinlbrkWriteWord);
		SekSetWriteByteHandler(0, spinlbrkWriteByte);

		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0x77FF, 0, RomZ80);
		ZetMapArea(0x0000, 0x77FF, 2, RomZ80);
		
		ZetMapArea(0x7800, 0x7FFF, 0, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 1, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 2, RamZ80);
		
		ZetMemEnd();
		
		ZetSetInHandler(turbofrcZ80PortRead);
		ZetSetOutHandler(turbofrcZ80PortWrite);
		
		ZetClose();
	}
	
	BurnYM2610Init(8000000, 25.0, 100.0, RomSnd2, &RomSndSize2, RomSnd1, &RomSndSize1, &aerofgtFMIRQHandler, aerofgtSynchroniseStream1, aerofgtGetTime1, 0);
	BurnTimerAttachZet(5000000);
	
	bg2scrollx = 0;	// 
	
	for (unsigned short i=0; i<0x2000;i++)
		RamSpr1[i] = i;
	
	DrvDoReset();	
	
	return 0;
}

static void spinlbrkTileBackground_1(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}
		
		x = mx * 8 - (RamRaster[my*8] & 0x1FF); // + 8
		if (x <= (352-512)) x += 512;
		
		y = my * 8;
		
		if ( x<=-8 || x>=352 || y<=-8 || y>= 240 ) 
			continue;

		else
		if ( x >=0 && x < (352-8) && y >= 0 && y < (240-8)) {
			
			unsigned char *d = BgGfx + ( (bg[offs] & 0x0FFF) + ( RamGfxBank[0] << 12 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xF000) >> 8;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
 				p[0] = pal[ d[0] | c ];
 				p[1] = pal[ d[1] | c ];
 				p[2] = pal[ d[2] | c ];
 				p[3] = pal[ d[3] | c ];
 				p[4] = pal[ d[4] | c ];
 				p[5] = pal[ d[5] | c ];
 				p[6] = pal[ d[6] | c ];
 				p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x0FFF) + ( RamGfxBank[0] << 12 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xF000) >> 8;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<240 ) {
	 				if ((x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if ((x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if ((x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if ((x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if ((x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if ((x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if ((x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if ((x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

#undef X_SIZE

static int spinlbrkDraw()
{
	// update palette
	aerofgtPalUpdate();

	spinlbrkTileBackground_1(RamBg1V, DeRomBg, RamCurPal);
	karatblzTileBackground_2(RamBg2V, DeRomBg + 0x200000, RamCurPal + 256);

	turbofrc_drawsprites(1,-1);	// enemy(near far)
	turbofrc_drawsprites(1, 0);	// enemy(near) fense
 	turbofrc_drawsprites(0, 0); // avatar , post , bullet
	turbofrc_drawsprites(0,-1); 

	return 0;
}

static int spinlbrkFrame()
{
	if (DrvReset) DrvDoReset();
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;												// Joy1
	DrvInput[1] = 0x0000;												// Joy2
	DrvInput[2] = 0x0000;												// Buttons
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvButton[i] & 1) << i;
	}

	SekNewFrame();
	ZetNewFrame();
	
//	nCyclesTotal[0] = (int)((long long)10000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)10000000 / 60);
	nCyclesTotal[1] = (int)((long long)5000000  / 60);

	SekOpen(0);
	ZetOpen(0);
	
	SekRun(nCyclesTotal[0]);
	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
	
	BurnTimerEndFrame(nCyclesTotal[1]);
	BurnYM2610Update(pBurnSoundOut, nBurnSoundLen);

	ZetClose();
	SekClose();
	
	if (pBurnDraw) spinlbrkDraw();
	
	return 0;
}

struct BurnDriver BurnDrvSpinlbrk = {
	"spinlbrk", NULL, NULL, "1990",
	"Spinal Breakers (World)\0", NULL, "V-System Co.", "V-System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, spinlbrkRomInfo, spinlbrkRomName, spinlbrkInputInfo, spinlbrkDIPInfo,
	spinlbrkInit,DrvExit,spinlbrkFrame,spinlbrkDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

struct BurnDriver BurnDrvSpinlbru = {
	"spinlbru", "spinlbrk", NULL, "1990",
	"Spinal Breakers (US)\0", NULL, "V-System Co.", "V-System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, spinlbruRomInfo, spinlbruRomName, spinlbrkInputInfo, spinlbruDIPInfo,
	spinlbrkInit,DrvExit,spinlbrkFrame,spinlbrkDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

struct BurnDriver BurnDrvSpinlbrj = {
	"spinlbrj", "spinlbrk", NULL, "1990",
	"Spinal Breakers (Japan)\0", NULL, "V-System Co.", "V-System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_SHOOT, 0,
	NULL, spinlbrjRomInfo, spinlbrjRomName, spinlbrkInputInfo, spinlbrjDIPInfo,
	spinlbrkInit,DrvExit,spinlbrkFrame,spinlbrkDraw,DrvScan,
	0, NULL, NULL, NULL, NULL,352,240,4,3
};

// -------------------------------------------------------


static struct BurnRomInfo aerofgtbRomDesc[] = {
	{ "v2",    	      0x040000, 0x5c9de9f0, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "v1",    	      0x040000, 0x89c1dcf4, BRF_ESS | BRF_PRG }, // 68000 code swapped

	{ "it-19-03",     0x080000, 0x85eba1a4, BRF_GRA },			 // graphics
	{ "it-19-02",     0x080000, 0x4f57f8ba, BRF_GRA },
	
	{ "it-19-04",     0x080000, 0x3b329c1f, BRF_GRA },			 // 
	{ "it-19-05",     0x080000, 0x02b525af, BRF_GRA },			 //
	
	{ "g27",          0x040000, 0x4d89cbc8, BRF_GRA },
	{ "g26",          0x040000, 0x8072c1d2, BRF_GRA },

	{ "v3",    	      0x020000, 0xcbb18cf4, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "it-19-01",     0x040000, 0x6d42723d, BRF_SND },			 // samples
	{ "it-19-06",     0x100000, 0xcdbbdb1d, BRF_SND },	
};

STD_ROM_PICK(aerofgtb);
STD_ROM_FN(aerofgtb);

static struct BurnRomInfo aerofgtcRomDesc[] = {
	{ "v2.149",       0x040000, 0xf187aec6, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "v1.111",       0x040000, 0x9e684b19, BRF_ESS | BRF_PRG }, // 68000 code swapped

	{ "it-19-03",     0x080000, 0x85eba1a4, BRF_GRA },			 // graphics
	{ "it-19-02",     0x080000, 0x4f57f8ba, BRF_GRA },
	
	{ "it-19-04",     0x080000, 0x3b329c1f, BRF_GRA },			 // 
	{ "it-19-05",     0x080000, 0x02b525af, BRF_GRA },			 //
	
	{ "g27",          0x040000, 0x4d89cbc8, BRF_GRA },
	{ "g26",          0x040000, 0x8072c1d2, BRF_GRA },

	{ "2.153",        0x020000, 0xa1ef64ec, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "it-19-01",     0x040000, 0x6d42723d, BRF_SND },			 // samples
	{ "it-19-06",     0x100000, 0xcdbbdb1d, BRF_SND },	
};

STD_ROM_PICK(aerofgtc);
STD_ROM_FN(aerofgtc);

static struct BurnRomInfo sonicwiRomDesc[] = {
	{ "2.149",    	  0x040000, 0x3d1b96ba, BRF_ESS | BRF_PRG }, // 68000 code swapped
	{ "1.111",    	  0x040000, 0xa3d09f94, BRF_ESS | BRF_PRG }, // 68000 code swapped

	{ "it-19-03",     0x080000, 0x85eba1a4, BRF_GRA },			 // graphics
	{ "it-19-02",     0x080000, 0x4f57f8ba, BRF_GRA },
	
	{ "it-19-04",     0x080000, 0x3b329c1f, BRF_GRA },			 // 
	{ "it-19-05",     0x080000, 0x02b525af, BRF_GRA },			 //
	
	{ "g27",          0x040000, 0x4d89cbc8, BRF_GRA },
	{ "g26",          0x040000, 0x8072c1d2, BRF_GRA },

	{ "2.153",        0x020000, 0xa1ef64ec, BRF_ESS | BRF_PRG }, // Sound CPU
	
	{ "it-19-01",     0x040000, 0x6d42723d, BRF_SND },			 // samples
	{ "it-19-06",     0x100000, 0xcdbbdb1d, BRF_SND },	
};

STD_ROM_PICK(sonicwi);
STD_ROM_FN(sonicwi);

unsigned char __fastcall aerofgtbReadByte(unsigned int sekAddress)
{
	switch (sekAddress) {
		case 0x0FE000:
			return ~DrvInput[2];
		case 0x0FE001:
			return ~DrvInput[0];
		case 0x0FE002:
			return 0xFF;
		case 0x0FE003:
			return ~DrvInput[1];
		case 0x0FE004:
			return ~DrvInput[4];			
		case 0x0FE005:
			return ~DrvInput[3];
		case 0x0FE007:
			return pending_command;
		case 0x0FE009:
			return ~DrvInput[5];
			
		default:
			printf("Attempt to read byte value of location %x\n", sekAddress);
	}
	return 0;
}

unsigned short __fastcall aerofgtbReadWord(unsigned int sekAddress)
{
//	switch (sekAddress) {
//		default:
//			printf("Attempt to read word value of location %x\n", sekAddress);
//	}
	return 0;
}

void __fastcall aerofgtbWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	if (( sekAddress & 0x0FF000 ) == 0x0FD000) {
		sekAddress &= 0x07FF;
		RamPal[sekAddress^1] = byteValue;
		// palette byte write at boot self-test only ?!
		//if (sekAddress & 1)
		//	RamCurPal[sekAddress>>1] = CalcCol( *((unsigned short *)&RamPal[sekAddress & 0xFFE]) );
		return;	
	}
	
	switch (sekAddress) {
		case 0x0FE001:
		case 0x0FE401:
		case 0x0FE403:
			// NOP
			break;
			
		case 0x0FE00E:
			pending_command = 1;
			SoundCommand(byteValue);
			break;	

		default:
			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
	}
}

void __fastcall aerofgtbWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	if (( sekAddress & 0x0FF000 ) == 0x0FD000) {
		sekAddress &= 0x07FE;
		*((unsigned short *)&RamPal[sekAddress]) = wordValue;
		RamCurPal[sekAddress>>1] = CalcCol( wordValue );
		return;	
	}

	switch (sekAddress) {
		case 0x0FE002:
			bg1scrolly = wordValue;
			break;
		case 0x0FE004:
			bg2scrollx = wordValue;
			break;
		case 0x0FE006:
			bg2scrolly = wordValue;
			break;
		case 0x0FE008:
			RamGfxBank[0] = (wordValue >>  0) & 0x0f;
			RamGfxBank[1] = (wordValue >>  4) & 0x0f;
			RamGfxBank[2] = (wordValue >>  8) & 0x0f;
			RamGfxBank[3] = (wordValue >> 12) & 0x0f;
			break;
		case 0x0FE00A:
			RamGfxBank[4] = (wordValue >>  0) & 0x0f;
			RamGfxBank[5] = (wordValue >>  4) & 0x0f;
			RamGfxBank[6] = (wordValue >>  8) & 0x0f;
			RamGfxBank[7] = (wordValue >> 12) & 0x0f;
			break;
		case 0x0FE00C:
			// NOP
			break;

		default:
			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
	}
}

static int aerofgtbMemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01 		= Next; Next += 0x080000;			// 68000 ROM
	RomZ80		= Next; Next += 0x030000;			// Z80 ROM
	RomBg		= Next; Next += 0x200040;			// Background, 1M 8x8x4bit decode to 2M + 64Byte safe 
	DeRomBg		= 	   RomBg +  0x000040;
	RomSpr1		= Next; Next += 0x200000;			// Sprite 1	 , 1M 16x16x4bit decode to 2M + 256Byte safe 
	RomSpr2		= Next; Next += 0x100100;			// Sprite 2
	
	DeRomSpr1	= RomSpr1    +  0x000100;
	DeRomSpr2	= RomSpr2    += 0x000100;
	
	RomSnd1		= Next; Next += 0x040000;			// ADPCM data
	RomSndSize1 = 0x040000;
	RomSnd2		= Next; Next += 0x100000;			// ADPCM data
	RomSndSize2 = 0x100000;

	RamStart	= Next;
	
	Ram01		= Next; Next += 0x014000;					// Work Ram 
	RamBg1V		= (unsigned short *)Next; Next += 0x002000;	// BG1 Video Ram
	RamBg2V		= (unsigned short *)Next; Next += 0x002000;	// BG2 Video Ram
	RamSpr1		= (unsigned short *)Next; Next += 0x004000;	// Sprite 1 Ram
	RamSpr2		= (unsigned short *)Next; Next += 0x004000;	// Sprite 2 Ram
	RamSpr3		= (unsigned short *)Next; Next += 0x000800;	// Sprite 3 Ram
	RamPal		= Next; Next += 0x000800;					// 1024 of X1R5G5B5 Palette
	RamRaster	= (unsigned short *)Next; Next += 0x001000;	// Raster

	RamSpr1SizeMask = 0x1FFF;
	RamSpr2SizeMask = 0x1FFF;
	RomSpr1SizeMask = 0x1FFF;
	RomSpr2SizeMask = 0x0FFF;
	
	RamZ80		= Next; Next += 0x000800;					// Z80 Ram 2K

	RamEnd		= Next;

	RamCurPal	= (unsigned int *)Next; Next += 0x000800 * sizeof(unsigned int);	// 1024 colors

	MemEnd		= Next;
	return 0;
}

static void aerofgtbDecodeSpr(unsigned char *d, unsigned char *s, int cnt)
{
	for (int c=cnt-1; c>=0; c--)
		for (int y=15; y>=0; y--) {
			d[(c * 256) + (y * 16) + 15] = s[0x00005 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 14] = s[0x00005 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 13] = s[0x00007 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 12] = s[0x00007 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) + 11] = s[0x00004 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) + 10] = s[0x00004 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  9] = s[0x00006 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  8] = s[0x00006 + (y * 8) + (c * 128)] & 0x0f;

			d[(c * 256) + (y * 16) +  7] = s[0x00001 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  6] = s[0x00001 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  5] = s[0x00003 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  4] = s[0x00003 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  3] = s[0x00000 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  2] = s[0x00000 + (y * 8) + (c * 128)] & 0x0f;
			d[(c * 256) + (y * 16) +  1] = s[0x00002 + (y * 8) + (c * 128)] >> 4;
			d[(c * 256) + (y * 16) +  0] = s[0x00002 + (y * 8) + (c * 128)] & 0x0f;
		}
}

static int aerofgtbInit()
{
	Mem = NULL;
	aerofgtbMemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) {
		return 1;
	}
	memset(Mem, 0, nLen);										// blank all memory
	aerofgtbMemIndex();	
	
	// Load the roms into memory
	// Load 68000 ROM
	if (BurnLoadRom(Rom01 + 0x00001, 0, 2)) return 1;
	if (BurnLoadRom(Rom01 + 0x00000, 1, 2)) return 1;
	
	// Load Graphic
	BurnLoadRom(RomBg+0x00000,  2, 1);
	BurnLoadRom(RomBg+0x80000,  3, 1);
	pspikesDecodeBg(0x8000);
		
	BurnLoadRom(RomSpr1+0x000000, 4, 2);
	BurnLoadRom(RomSpr1+0x000001, 5, 2);
	BurnLoadRom(RomSpr1+0x100000, 6, 2);
	BurnLoadRom(RomSpr1+0x100001, 7, 2);
	aerofgtbDecodeSpr(DeRomSpr1, RomSpr1, 0x3000);
	
	// Load Z80 ROM
	if (BurnLoadRom(RomZ80+0x10000, 8, 1)) return 1;
	memcpy(RomZ80, RomZ80+0x10000, 0x10000);

	BurnLoadRom(RomSnd1,  9, 1);
	BurnLoadRom(RomSnd2, 10, 1);

	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,			0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,			0x0C0000, 0x0CFFFF, SM_RAM);	// 64K Work RAM
		SekMapMemory((unsigned char *)RamBg1V,		
									0x0D0000, 0x0D1FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamBg2V,		
									0x0D2000, 0x0D3FFF, SM_RAM);	
		SekMapMemory((unsigned char *)RamSpr1,
									0x0E0000, 0x0E3FFF, SM_RAM);
		SekMapMemory((unsigned char *)RamSpr2,
									0x0E4000, 0x0E7FFF, SM_RAM);
		SekMapMemory(Ram01+0x10000,	0x0F8000, 0x0FBFFF, SM_RAM);	// Work RAM
		SekMapMemory((unsigned char *)RamSpr3,
									0x0FC000, 0x0FC7FF, SM_RAM);
		SekMapMemory(RamPal,		0x0FD000, 0x0FD7FF, SM_ROM);	// Palette
		SekMapMemory((unsigned char *)RamRaster,
									0x0FF000, 0x0FFFFF, SM_RAM);	// Raster 

		SekSetReadWordHandler(0, aerofgtbReadWord);
		SekSetReadByteHandler(0, aerofgtbReadByte);
		SekSetWriteWordHandler(0, aerofgtbWriteWord);
		SekSetWriteByteHandler(0, aerofgtbWriteByte);

		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0x77FF, 0, RomZ80);
		ZetMapArea(0x0000, 0x77FF, 2, RomZ80);
		
		ZetMapArea(0x7800, 0x7FFF, 0, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 1, RamZ80);
		ZetMapArea(0x7800, 0x7FFF, 2, RamZ80);
		
		ZetMemEnd();
		
		//ZetSetReadHandler(aerofgtZ80Read);
		//ZetSetWriteHandler(aerofgtZ80Write);
		ZetSetInHandler(aerofgtZ80PortRead);
		ZetSetOutHandler(aerofgtZ80PortWrite);
		
		ZetClose();
	}
	
	BurnYM2610Init(8000000, 25.0, 100.0, RomSnd2, &RomSndSize2, RomSnd1, &RomSndSize1, &aerofgtFMIRQHandler, aerofgtSynchroniseStream, aerofgtGetTime, 0);
	BurnTimerAttachZet(4000000);
	
	DrvDoReset();												// Reset machine
	return 0;
}

#ifndef BUILD_PSP
#define X_SIZE	320
#else
#define X_SIZE	512
#endif

static void aerofgtbTileBackground_1(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;
	
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - (RamRaster[7] & 0x1FF);
		if (x <= (320-512)) x += 512;
		
		y = my * 8 - (((signed short)bg1scrolly + 2) & 0x1FF);
		if (y <= (224-512)) y += 512;
		
		if ( x<=-8 || x>=320 || y<=-8 || y>= 224 ) 
			continue;

		else
		if ( x >=0 && x < (320-8) && y >= 0 && y < (224-8)) {
			
			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
 				p[0] = pal[ d[0] | c ];
 				p[1] = pal[ d[1] | c ];
 				p[2] = pal[ d[2] | c ];
 				p[3] = pal[ d[3] | c ];
 				p[4] = pal[ d[4] | c ];
 				p[5] = pal[ d[5] | c ];
 				p[6] = pal[ d[6] | c ];
 				p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11)] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<224 ) {
	 				if ((x + 0) >= 0 && (x + 0)<320) p[0] = pal[ d[0] | c ];
	 				if ((x + 1) >= 0 && (x + 1)<320) p[1] = pal[ d[1] | c ];
	 				if ((x + 2) >= 0 && (x + 2)<320) p[2] = pal[ d[2] | c ];
	 				if ((x + 3) >= 0 && (x + 3)<320) p[3] = pal[ d[3] | c ];
	 				if ((x + 4) >= 0 && (x + 4)<320) p[4] = pal[ d[4] | c ];
	 				if ((x + 5) >= 0 && (x + 5)<320) p[5] = pal[ d[5] | c ];
	 				if ((x + 6) >= 0 && (x + 6)<320) p[6] = pal[ d[6] | c ];
	 				if ((x + 7) >= 0 && (x + 7)<320) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void aerofgtbTileBackground_2(unsigned short *bg, unsigned char *BgGfx, unsigned int *pal)
{
	int offs, mx, my, x, y;
	//printf(" %5d%5d%5d%5d\n", (signed short)RamRaster[7],(signed short)bg1scrolly, (signed short)bg2scrollx,(signed short)bg2scrolly);
	mx = -1;
	my = 0;
	for (offs = 0; offs < 64*64; offs++) {
		mx++;
		if (mx == 64) {
			mx = 0;
			my++;
		}

		x = mx * 8 - (((signed short)bg2scrollx + 5) & 0x1FF);
		if (x <= (320-512)) x += 512;
		
		y = my * 8 - (((signed short)bg2scrolly + 2) & 0x1FF);
		if (y <= (224-512)) y += 512;
		
		if ( x<=-8 || x>=320 || y<=-8 || y>= 224 ) 
			continue;
		else
		if ( x >=0 && x < (320-8) && y >= 0 && y < (224-8)) {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				
 				if (d[0] != 15) p[0] = pal[ d[0] | c ];
 				if (d[1] != 15) p[1] = pal[ d[1] | c ];
 				if (d[2] != 15) p[2] = pal[ d[2] | c ];
 				if (d[3] != 15) p[3] = pal[ d[3] | c ];
 				if (d[4] != 15) p[4] = pal[ d[4] | c ];
 				if (d[5] != 15) p[5] = pal[ d[5] | c ];
 				if (d[6] != 15) p[6] = pal[ d[6] | c ];
 				if (d[7] != 15) p[7] = pal[ d[7] | c ];
 				
 				d += 8;
 				p += X_SIZE;
 			}
		} else {

			unsigned char *d = BgGfx + ( (bg[offs] & 0x07FF) + ( RamGfxBank[((bg[offs] & 0x1800) >> 11) + 4] << 11 ) ) * 64;
 			unsigned short c = (bg[offs] & 0xE000) >> 9;
 			unsigned short * p = (unsigned short *) pBurnDraw + y * X_SIZE + x;
			
			for (int k=0;k<8;k++) {
				if ( (y+k)>=0 && (y+k)<224 ) {
	 				if (d[0] != 15 && (x + 0) >= 0 && (x + 0)<352) p[0] = pal[ d[0] | c ];
	 				if (d[1] != 15 && (x + 1) >= 0 && (x + 1)<352) p[1] = pal[ d[1] | c ];
	 				if (d[2] != 15 && (x + 2) >= 0 && (x + 2)<352) p[2] = pal[ d[2] | c ];
	 				if (d[3] != 15 && (x + 3) >= 0 && (x + 3)<352) p[3] = pal[ d[3] | c ];
	 				if (d[4] != 15 && (x + 4) >= 0 && (x + 4)<352) p[4] = pal[ d[4] | c ];
	 				if (d[5] != 15 && (x + 5) >= 0 && (x + 5)<352) p[5] = pal[ d[5] | c ];
	 				if (d[6] != 15 && (x + 6) >= 0 && (x + 6)<352) p[6] = pal[ d[6] | c ];
	 				if (d[7] != 15 && (x + 7) >= 0 && (x + 7)<352) p[7] = pal[ d[7] | c ];
	 			}
 				d += 8;
 				p += X_SIZE;
 			}
			
		}

	}
}

static void aerofgtb_pdrawgfxzoom(int bank,unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,int scalex,int scaley)
{
	if (!scalex || !scaley) return;
			
	unsigned short * p = (unsigned short *) pBurnDraw;
	unsigned char * q;
	unsigned int * pal;
	
	if (bank) {
		//if (code > RomSpr2SizeMask)
			code &= RomSpr2SizeMask;
		q = DeRomSpr2 + (code) * 256;
		pal = RamCurPal + 768;
	} else {
		//if (code > RomSpr1SizeMask)
			code &= RomSpr1SizeMask;
		q = DeRomSpr1 + (code) * 256;
		pal = RamCurPal + 512;
	}

	p += sy * X_SIZE + sx;
		
	if (sx < 0 || sx >= (320-16) || sy < 0 || sy >= (224-16) ) {
		
		if ((sx <= -16) || (sx >= 320) || (sy <= -16) || (sy >= 224))
			return;
			
		if (flipy) {
		
			p += X_SIZE * 15;
			
			if (flipx) {
			
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}	
			
			} else {
	
				for (int i=15;i>=0;i--) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[15] | color];
					}
					p -= X_SIZE;
					q += 16;
				}		
			}
			
		} else {
			
			if (flipx) {
			
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}		
			
			} else {
	
				for (int i=0;i<16;i++) {
					if (((sy+i)>=0) && ((sy+i)<224)) {
						if (q[ 0] != 15 && ((sx +  0) >= 0) && ((sx +  0)<320)) p[ 0] = pal[ q[ 0] | color];
						if (q[ 1] != 15 && ((sx +  1) >= 0) && ((sx +  1)<320)) p[ 1] = pal[ q[ 1] | color];
						if (q[ 2] != 15 && ((sx +  2) >= 0) && ((sx +  2)<320)) p[ 2] = pal[ q[ 2] | color];
						if (q[ 3] != 15 && ((sx +  3) >= 0) && ((sx +  3)<320)) p[ 3] = pal[ q[ 3] | color];
						if (q[ 4] != 15 && ((sx +  4) >= 0) && ((sx +  4)<320)) p[ 4] = pal[ q[ 4] | color];
						if (q[ 5] != 15 && ((sx +  5) >= 0) && ((sx +  5)<320)) p[ 5] = pal[ q[ 5] | color];
						if (q[ 6] != 15 && ((sx +  6) >= 0) && ((sx +  6)<320)) p[ 6] = pal[ q[ 6] | color];
						if (q[ 7] != 15 && ((sx +  7) >= 0) && ((sx +  7)<320)) p[ 7] = pal[ q[ 7] | color];
						
						if (q[ 8] != 15 && ((sx +  8) >= 0) && ((sx +  8)<320)) p[ 8] = pal[ q[ 8] | color];
						if (q[ 9] != 15 && ((sx +  9) >= 0) && ((sx +  9)<320)) p[ 9] = pal[ q[ 9] | color];
						if (q[10] != 15 && ((sx + 10) >= 0) && ((sx + 10)<320)) p[10] = pal[ q[10] | color];
						if (q[11] != 15 && ((sx + 11) >= 0) && ((sx + 11)<320)) p[11] = pal[ q[11] | color];
						if (q[12] != 15 && ((sx + 12) >= 0) && ((sx + 12)<320)) p[12] = pal[ q[12] | color];
						if (q[13] != 15 && ((sx + 13) >= 0) && ((sx + 13)<320)) p[13] = pal[ q[13] | color];
						if (q[14] != 15 && ((sx + 14) >= 0) && ((sx + 14)<320)) p[14] = pal[ q[14] | color];
						if (q[15] != 15 && ((sx + 15) >= 0) && ((sx + 15)<320)) p[15] = pal[ q[15] | color];
					}
					p += X_SIZE;
					q += 16;
				}	
	
			}
			
		}
		
		return;
	}

	if (flipy) {
		
		p += X_SIZE * 15;
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}	
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p -= X_SIZE;
				q += 16;
			}		
		}
		
	} else {
		
		if (flipx) {
		
			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[15] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[14] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[13] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[12] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[11] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[10] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 9] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 8] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 7] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 6] = pal[ q[ 9] | color];
				if (q[10] != 15) p[ 5] = pal[ q[10] | color];
				if (q[11] != 15) p[ 4] = pal[ q[11] | color];
				if (q[12] != 15) p[ 3] = pal[ q[12] | color];
				if (q[13] != 15) p[ 2] = pal[ q[13] | color];
				if (q[14] != 15) p[ 1] = pal[ q[14] | color];
				if (q[15] != 15) p[ 0] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}		
		
		} else {

			for (int i=0;i<16;i++) {
				if (q[ 0] != 15) p[ 0] = pal[ q[ 0] | color];
				if (q[ 1] != 15) p[ 1] = pal[ q[ 1] | color];
				if (q[ 2] != 15) p[ 2] = pal[ q[ 2] | color];
				if (q[ 3] != 15) p[ 3] = pal[ q[ 3] | color];
				if (q[ 4] != 15) p[ 4] = pal[ q[ 4] | color];
				if (q[ 5] != 15) p[ 5] = pal[ q[ 5] | color];
				if (q[ 6] != 15) p[ 6] = pal[ q[ 6] | color];
				if (q[ 7] != 15) p[ 7] = pal[ q[ 7] | color];
				
				if (q[ 8] != 15) p[ 8] = pal[ q[ 8] | color];
				if (q[ 9] != 15) p[ 9] = pal[ q[ 9] | color];
				if (q[10] != 15) p[10] = pal[ q[10] | color];
				if (q[11] != 15) p[11] = pal[ q[11] | color];
				if (q[12] != 15) p[12] = pal[ q[12] | color];
				if (q[13] != 15) p[13] = pal[ q[13] | color];
				if (q[14] != 15) p[14] = pal[ q[14] | color];
				if (q[15] != 15) p[15] = pal[ q[15] | color];
	
				p += X_SIZE;
				q += 16;
			}	

		}
		
	}

}

#undef X_SIZE

static void aerofgtb_drawsprites(int chip,int chip_disabled_pri)
{
	int attr_start,base,first;

	base = chip * 0x0200;
	first = 4 * RamSpr3[0x1fe + base];

	//for (attr_start = base + 0x0200-8;attr_start >= first + base;attr_start -= 4) {
	for (attr_start = first + base; attr_start <= base + 0x0200-8; attr_start += 4) {
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color,pri;
// some other drivers still use this wrong table, they have to be upgraded
//      int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };

		if (!(RamSpr3[attr_start + 2] & 0x0080)) continue;
		pri = RamSpr3[attr_start + 2] & 0x0010;
		if ( chip_disabled_pri & !pri) continue;
		if (!chip_disabled_pri & (pri>>4)) continue;
		ox = RamSpr3[attr_start + 1] & 0x01ff;
		xsize = (RamSpr3[attr_start + 2] & 0x0700) >> 8;
		zoomx = (RamSpr3[attr_start + 1] & 0xf000) >> 12;
		oy = RamSpr3[attr_start + 0] & 0x01ff;
		ysize = (RamSpr3[attr_start + 2] & 0x7000) >> 12;
		zoomy = (RamSpr3[attr_start + 0] & 0xf000) >> 12;
		flipx = RamSpr3[attr_start + 2] & 0x0800;
		flipy = RamSpr3[attr_start + 2] & 0x8000;
		color = (RamSpr3[attr_start + 2] & 0x000f) << 4;	// + 16 * spritepalettebank;

		map_start = RamSpr3[attr_start + 3];

// aerofgt has this adjustment, but doing it here would break turbo force title screen
//      ox += (xsize*zoomx+2)/4;
//      oy += (ysize*zoomy+2)/4;

		zoomx = 32 - zoomx;
		zoomy = 32 - zoomy;

		for (y = 0;y <= ysize;y++) {
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y)/2 + 16) & 0x1ff) - 16 - 1;
			else sy = ((oy + zoomy * y / 2 + 16) & 0x1ff) - 16 - 1;

			for (x = 0;x <= xsize;x++) {
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) / 2 + 16) & 0x1ff) - 16 - 8;
				else sx = ((ox + zoomx * x / 2 + 16) & 0x1ff) - 16 - 8;

				if (chip == 0)	code = RamSpr1[map_start & RamSpr1SizeMask];
				else			code = RamSpr2[map_start & RamSpr2SizeMask];

				aerofgtb_pdrawgfxzoom(chip,code,color,flipx,flipy,sx,sy,zoomx << 11, zoomy << 11);

				map_start++;
			}

			if (xsize == 2) map_start += 1;
			if (xsize == 4) map_start += 3;
			if (xsize == 5) map_start += 2;
			if (xsize == 6) map_start += 1;
		}
	}
}

static int aerofgtbDraw()
{
	// update palette
	aerofgtPalUpdate();

	aerofgtbTileBackground_1(RamBg1V, DeRomBg, RamCurPal);
	aerofgtbTileBackground_2(RamBg2V, DeRomBg + 0x100000, RamCurPal + 256);
	
	aerofgtb_drawsprites(0, 0); 
 	aerofgtb_drawsprites(0,-1); 
	aerofgtb_drawsprites(1, 0); 
	aerofgtb_drawsprites(1,-1);
	
	return 0;
}

static int aerofgtbFrame()
{
	if (DrvReset) DrvDoReset();
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;												// Joy1
	DrvInput[1] = 0x0000;												// Joy2
	DrvInput[2] = 0x0000;												// Buttons
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvButton[i] & 1) << i;
	}
	
	SekNewFrame();
	ZetNewFrame();
	
//	nCyclesTotal[0] = (int)((long long)10000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)10000000 / 60);
	nCyclesTotal[1] = (int)((long long)4000000  / 60);

	SekOpen(0);
	ZetOpen(0);
	
	SekRun(nCyclesTotal[0]);
	SekSetIRQLine(1, SEK_IRQSTATUS_AUTO);
	
	BurnTimerEndFrame(nCyclesTotal[1]);
	BurnYM2610Update(pBurnSoundOut, nBurnSoundLen);

	ZetClose();
	SekClose();
	
	if (pBurnDraw) aerofgtbDraw();
	
	return 0;
}

struct BurnDriver BurnDrvAerofgtb = {
	"aerofgtb", "aerofgt", NULL, "1992",
	"Aero Fighters (Turbo Force hardware set 1)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_VERSHOOT, FBF_SONICWI,
	NULL, aerofgtbRomInfo, aerofgtbRomName, aerofgtInputInfo, aerofgtDIPInfo,
	aerofgtbInit,DrvExit,aerofgtbFrame,aerofgtbDraw,DrvScan,
	0, NULL, NULL, NULL, NULL, 224,320,3,4
};

struct BurnDriver BurnDrvAerofgtc = {
	"aerofgtc", "aerofgt", NULL, "1992",
	"Aero Fighters (Turbo Force hardware set 2)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_VERSHOOT, FBF_SONICWI,
	NULL, aerofgtcRomInfo, aerofgtcRomName, aerofgtInputInfo, aerofgtDIPInfo,
	aerofgtbInit,DrvExit,aerofgtbFrame,aerofgtbDraw,DrvScan,
	0, NULL, NULL, NULL, NULL, 224,320,3,4
};

struct BurnDriver BurnDrvSonicwi = {
	"sonicwi", "aerofgt", NULL, "1992",
	"Sonic Wings (Japan)\0", NULL, "Video System Co.", "Video System",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_MISC_POST90S, GBF_VERSHOOT, FBF_SONICWI,
	NULL, sonicwiRomInfo, sonicwiRomName, aerofgtInputInfo, aerofgtDIPInfo,
	aerofgtbInit,DrvExit,aerofgtbFrame,aerofgtbDraw,DrvScan,
	0, NULL, NULL, NULL, NULL, 224,320,3,4
};

