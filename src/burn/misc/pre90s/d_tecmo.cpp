// FB Alpha Tecmo (Rygar, Silkworm, and Gemini Wing) driver Module
// Based on MAME driver by Nicola Salmoria

#include "tiles_generic.h"
#include "burn_ym3812.h"
#include "msm5205.h"

static unsigned char ALIGN_DATA DrvJoy1[8];
static unsigned char ALIGN_DATA DrvJoy2[8];
static unsigned char ALIGN_DATA DrvJoy3[8];
static unsigned char ALIGN_DATA DrvJoy4[8];
static unsigned char ALIGN_DATA DrvJoy5[8];
static unsigned char ALIGN_DATA DrvJoy6[8];
static unsigned char ALIGN_DATA DrvJoy11[8];
static unsigned char DrvReset;
static unsigned char ALIGN_DATA DrvInputs[11];

static unsigned char *AllMem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *DrvZ80ROM0;
static unsigned char *DrvZ80ROM1;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvGfxROM2;
static unsigned char *DrvGfxROM3;
static unsigned char *DrvSndROM;
static unsigned char *DrvZ80RAM0;
static unsigned char *DrvZ80RAM1;
static unsigned char *DrvPalRAM;
static unsigned char *DrvTextRAM;
static unsigned char *DrvSprRAM;
static unsigned char *DrvForeRAM;
static unsigned char *DrvBackRAM;
static unsigned short *DrvBgScroll;
static unsigned short *DrvFgScroll;
static unsigned int *DrvPalette;
static unsigned int *Palette;
static unsigned char DrvRecalc;

static int tecmo_video_type;

static unsigned int adpcm_pos;
static unsigned int adpcm_end;
static unsigned int adpcm_vol;

static unsigned int DrvZ80Bank;
static unsigned char soundlatch;
static unsigned char flipscreen;
static unsigned char DrvEnableNmi;

static struct BurnInputInfo RygarInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy5 + 3,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy5 + 2,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy5 + 1,	"p1 start" },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 0, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 1, 	"p1 right" },
	{"P1 Down",	  BIT_DIGITAL  , DrvJoy1 + 2,   "p1 down", },
	{"P1 Up",	  BIT_DIGITAL  , DrvJoy1 + 3,   "p1 up",   },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 1,	"p1 fire 2"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy5 + 0,	"p2 start" },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy3 + 0, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy3 + 1, 	"p2 right" },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p2 down", },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p2 up",   },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 0,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 1,	"p2 fire 2"},

	{"Service",	  BIT_DIGITAL  , DrvJoy2 + 2,   "diag"     },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvInputs + 6,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvInputs + 7,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvInputs + 8,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvInputs + 9,	"dip"	   },
};

STDINPUTINFO(Rygar);

static struct BurnInputInfo GeminiInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy6 + 2,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy6 + 3,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy6 + 0,	"p1 start" },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 0, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 1, 	"p1 right" },
	{"P1 Down",	  BIT_DIGITAL  , DrvJoy1 + 2,   "p1 down", },
	{"P1 Up",	  BIT_DIGITAL  , DrvJoy1 + 3,   "p1 up",   },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 1,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 fire 2"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy6 + 1,	"p2 start" },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy3 + 0, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy3 + 1, 	"p2 right" },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p2 down", },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p2 up",   },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 1,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 0,	"p2 fire 2"},

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvInputs + 6,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvInputs + 7,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvInputs + 8,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvInputs + 9,	"dip"	   },
};

STDINPUTINFO(Gemini);

static struct BurnInputInfo SilkwormInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy11 + 2,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy11 + 3,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy11 + 0,	"p1 start" },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 0, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 1, 	"p1 right" },
	{"P1 Down",	  BIT_DIGITAL  , DrvJoy1 + 2,   "p1 down", },
	{"P1 Up",	  BIT_DIGITAL  , DrvJoy1 + 3,   "p1 up",   },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 1,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 2,	"p1 fire 3"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy11 + 1,	"p2 start" },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy3 + 0, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy3 + 1, 	"p2 right" },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p2 down", },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p2 up",   },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 1,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 0,	"p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy4 + 2,	"p2 fire 3"},

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvInputs + 6,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvInputs + 7,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvInputs + 8,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvInputs + 9,	"dip"	   },

};

STDINPUTINFO(Silkworm);

static struct BurnDIPInfo RygarDIPList[]=
{
	{0x12, 0xff, 0xff, 0x00, NULL },
	{0x13, 0xff, 0xff, 0x00, NULL },
	{0x14, 0xff, 0xff, 0x00, NULL },
	{0x15, 0xff, 0xff, 0x00, NULL },

	{0x12, 0xfe, 0,       4, "Coin A" },
	{0x12, 0x01, 0x03, 0x01, "2C 1C" },
	{0x12, 0x01, 0x03, 0x00, "1C 1C" },
	{0x12, 0x01, 0x03, 0x02, "1C 2C" },
	{0x12, 0x01, 0x03, 0x03, "1C 3C" },

	{0x12, 0xfe, 0,       4, "Coin B" },
	{0x12, 0x01, 0x0C, 0x04, "2C 1C" },
	{0x12, 0x01, 0x0C, 0x00, "1C 1C" },
	{0x12, 0x01, 0x0C, 0x08, "1C 2C" },
	{0x12, 0x01, 0x0C, 0x0C, "1C 3C" },

	{0x13, 0xfe, 0,       4, "Lives" },
	{0x13, 0x01, 0x03, 0x03, "2" },
	{0x13, 0x01, 0x03, 0x00, "3" },
	{0x13, 0x01, 0x03, 0x01, "4" },
	{0x13, 0x01, 0x03, 0x02, "5" },

	{0x13, 0xfe, 0,       2, "Cabinet" },
	{0x13, 0x01, 0x04, 0x04, "Upright" },
	{0x13, 0x01, 0x04, 0x00, "Cocktail" },

	{0x14, 0xfe, 0,       4, "Bonus Life" },
	{0x14, 0x01, 0x03, 0x00, "50000 200000 500000" },
	{0x14, 0x01, 0x03, 0x01, "100000 300000 600000" },
	{0x14, 0x01, 0x03, 0x02, "200000 500000" },
	{0x14, 0x01, 0x03, 0x03, "100000" },

	{0x15, 0xfe, 0,       4, "Difficulty" },
	{0x15, 0x01, 0x03, 0x00, "Easy" },
	{0x15, 0x01, 0x03, 0x01, "Normal" },
	{0x15, 0x01, 0x03, 0x02, "Hard" },
	{0x15, 0x01, 0x03, 0x03, "Hardest" },

	{0x15, 0xfe, 0,       2, "2P Can Start Anytime" },
	{0x15, 0x01, 0x04, 0x00, "No" },
	{0x15, 0x01, 0x04, 0x04, "Yes" },

	{0x15, 0xfe, 0,       2, "Allow Continue" },
	{0x15, 0x01, 0x08, 0x00, "No" },
	{0x15, 0x01, 0x08, 0x08, "Yes" },
};

STDDIPINFO(Rygar);

static struct BurnDIPInfo GeminiDIPList[]=
{
	{0x11, 0xff, 0xff, 0x00, NULL },
	{0x12, 0xff, 0xff, 0x00, NULL },
	{0x13, 0xff, 0xff, 0x00, NULL },
	{0x14, 0xff, 0xff, 0x00, NULL },

	{0x11, 0xfe, 0,       8, "Coin A" },
	{0x11, 0x01, 0x07, 0x06, "2C 1C" },
	{0x11, 0x01, 0x07, 0x00, "1C 1C" },
	{0x11, 0x01, 0x07, 0x07, "2C 3C" },
	{0x11, 0x01, 0x07, 0x01, "1C 2C" },
	{0x11, 0x01, 0x07, 0x02, "1C 3C" },
	{0x11, 0x01, 0x07, 0x03, "1C 4C" },
	{0x11, 0x01, 0x07, 0x04, "1C 5C" },
	{0x11, 0x01, 0x07, 0x05, "1C 6C" },

	{0x11, 0xfe, 0,       2, "Final Round Continuation" },
	{0x11, 0x01, 0x08, 0x00, "Round 6" },
	{0x11, 0x01, 0x08, 0x08, "Round 7" },

	{0x12, 0xfe, 0,       8, "Coin B" },
	{0x12, 0x01, 0x07, 0x06, "2C 1C" },
	{0x12, 0x01, 0x07, 0x00, "1C 1C" },
	{0x12, 0x01, 0x07, 0x07, "2C 3C" },
	{0x12, 0x01, 0x07, 0x01, "1C 2C" },
	{0x12, 0x01, 0x07, 0x02, "1C 3C" },
	{0x12, 0x01, 0x07, 0x03, "1C 4C" },
	{0x12, 0x01, 0x07, 0x04, "1C 5C" },
	{0x12, 0x01, 0x07, 0x05, "1C 6C" },

	{0x12, 0xfe, 0,       2, "Buy in During Final Round" },
	{0x12, 0x01, 0x08, 0x00, "No" },
	{0x12, 0x01, 0x08, 0x08, "Yes" },

	{0x13, 0xfe, 0,       4, "Lives" },
	{0x13, 0x01, 0x03, 0x03, "2" },
	{0x13, 0x01, 0x03, 0x00, "3" },
	{0x13, 0x01, 0x03, 0x01, "4" },
	{0x13, 0x01, 0x03, 0x02, "5" },

	{0x13, 0xfe, 0,       4, "Difficulty" },
	{0x13, 0x01, 0x0c, 0x00, "Easy" },
	{0x13, 0x01, 0x0c, 0x04, "Normal" },
	{0x13, 0x01, 0x0c, 0x08, "Hard" },
	{0x13, 0x01, 0x0c, 0x0c, "Hardest" },

	{0x14, 0xfe, 0,       8, "Bonus Life" },
	{0x14, 0x01, 0x07, 0x00, "50000 200000" },
	{0x14, 0x01, 0x07, 0x01, "50000 300000" },
	{0x14, 0x01, 0x07, 0x02, "100000 500000" },
	{0x14, 0x01, 0x07, 0x03, "50000" },
	{0x14, 0x01, 0x07, 0x04, "100000" },
	{0x14, 0x01, 0x07, 0x05, "200000" },
	{0x14, 0x01, 0x07, 0x06, "300000" },
	{0x14, 0x01, 0x07, 0x07, "None" },

	{0x14, 0xfe, 0,       2, "Demo Sounds" },
	{0x14, 0x01, 0x08, 0x08, "Off" },
	{0x14, 0x01, 0x08, 0x00, "On" },
};

STDDIPINFO(Gemini);

static struct BurnDIPInfo SilkwormDIPList[]=
{
	{0x13, 0xff, 0xff, 0x00, NULL },
	{0x14, 0xff, 0xff, 0x00, NULL },
	{0x15, 0xff, 0xff, 0x00, NULL },
	{0x16, 0xff, 0xff, 0x00, NULL },

	{0x13, 0xfe, 0,      4, "Coin A" },
	{0x13, 0x01, 0x3, 0x01, "2C 1C" },
	{0x13, 0x01, 0x3, 0x00, "1C 1C" },
	{0x13, 0x01, 0x3, 0x02, "1C 2C" },
	{0x13, 0x01, 0x3, 0x03, "1C 3C" },

	{0x13, 0xfe, 0,      4, "Coin B" },
	{0x13, 0x01, 0xC, 0x04, "2C 1C" },
	{0x13, 0x01, 0xC, 0x00, "1C 1C" },
	{0x13, 0x01, 0xC, 0x08, "1C 2C" },
	{0x13, 0x01, 0xC, 0x0C, "1C 3C" },

	{0x14, 0xfe, 0,      4 , "Lives" },
	{0x14, 0x01, 0x3, 0x03, "2" },
	{0x14, 0x01, 0x3, 0x00, "3" },
	{0x14, 0x01, 0x3, 0x01, "4" },
	{0x14, 0x01, 0x3, 0x02, "5" },

	{0x14, 0xfe, 0,      2 , "Demo Sounds" },
	{0x14, 0x01, 0x8, 0x00, "Off" },
	{0x14, 0x01, 0x8, 0x08, "On" },

	{0x15, 0xfe, 0,      8, "Bonus Life" },
	{0x15, 0x01, 0x7, 0x00, "50000 200000 500000" },
	{0x15, 0x01, 0x7, 0x01, "100000 300000 800000" },
	{0x15, 0x01, 0x7, 0x02, "50000 200000" },
	{0x15, 0x01, 0x7, 0x03, "100000 300000" },
	{0x15, 0x01, 0x7, 0x04, "50000" },
	{0x15, 0x01, 0x7, 0x05, "100000" },
	{0x15, 0x01, 0x7, 0x06, "200000" },
	{0x15, 0x01, 0x7, 0x07, "None" },

	{0x16, 0xfe, 0,      6, "Difficulty" },
	{0x16, 0x01, 0x7, 0x00, "0" },
	{0x16, 0x01, 0x7, 0x01, "1" },
	{0x16, 0x01, 0x7, 0x02, "2" },
	{0x16, 0x01, 0x7, 0x03, "3" },
	{0x16, 0x01, 0x7, 0x04, "4" },
	{0x16, 0x01, 0x7, 0x05, "5" },

	{0x16, 0xfe, 0,      2, "Allow Continue" },
	{0x16, 0x01, 0x8, 0x08, "No" },
	{0x16, 0x01, 0x8, 0x00, "Yes" },
};

STDDIPINFO(Silkworm);

unsigned char __fastcall rygar_main_read(unsigned short address)
{
	switch (address)
	{
		case 0xf800:
		case 0xf801:
		case 0xf802:
		case 0xf803:
		case 0xf804:
		case 0xf805:
		case 0xf806:
		case 0xf807:
		case 0xf808:
		case 0xf809:
			return DrvInputs[address & 0x0f];

		case 0xf80f:
			return DrvInputs[10];
	}

	return 0;
}

static void bankswitch_w(int data)
{
	DrvZ80Bank = 0x10000 + ((data & 0xf8) << 8);

	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80ROM0 + DrvZ80Bank);
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80ROM0 + DrvZ80Bank);
}

static void palette_write(int offset)
{
	unsigned short data;
	unsigned char r,g,b;

	data = *((unsigned short*)(DrvPalRAM + (offset & ~1)));
	data = (data << 8) | (data >> 8);

	r = (data >> 4) & 0x0f;
	g = (data >> 0) & 0x0f;
	b = (data >> 8) & 0x0f;

	r |= r << 4;
	g |= g << 4;
	b |= b << 4;

	Palette[offset >> 1] = (r << 16) | (g << 8) | b;
	DrvPalette[offset >> 1] = BurnHighCol(r, g, b, 0);
}

void __fastcall rygar_main_write(unsigned short address, unsigned char data)
{
	if ((address & 0xf000) == 0xe000) {
		DrvPalRAM[address & 0x7ff] = data;
		palette_write(address & 0x7ff);
		return;
	}

	switch (address)
	{
		case 0xf800:
			DrvFgScroll[0] = (DrvFgScroll[0] & 0xff00) | data;
		return;

		case 0xf801:
			DrvFgScroll[0] = (DrvFgScroll[0] & 0x00ff) | (data << 8);
		return;

		case 0xf802:
			DrvFgScroll[1] = data;
		return;

		case 0xf803:
			DrvBgScroll[0] = (DrvBgScroll[0] & 0xff00) | data;
		return;

		case 0xf804:
			DrvBgScroll[0] = (DrvBgScroll[0] & 0x00ff) | (data << 8);
		return;

		case 0xf805:
			DrvBgScroll[1] = data;
		return;

		case 0xf806:
			soundlatch = data;
			DrvEnableNmi = 1;
		return;

		case 0xf807:
			flipscreen = data & 1;
		return;

		case 0xf808:
			bankswitch_w(data);
		return;

		case 0xf80b:
			// watchdog reset
		return;
	}

	return;
}

unsigned char __fastcall rygar_sound_read(unsigned short address)
{
	switch (address)
	{
		case 0xc000:
			return soundlatch;
	}

	return 0;
}

void __fastcall rygar_sound_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0x8000:
			BurnYM3812Write(0, data);
		return;

		case 0x8001:
			BurnYM3812Write(1, data);
		return;

		case 0xc000:
			adpcm_pos = data << 8;
			MSM5205Reset(0);
		return;

		case 0xd000:
			adpcm_end = (data + 1) << 8;
			MSM5205Play(adpcm_pos, adpcm_end, 0);
		return;

		case 0xe000:
			adpcm_vol = (data & 0x0f) * 100 / 15;
		return;

		case 0xf000:
		return;
	}

	return;
}

void __fastcall tecmo_sound_write(unsigned short address, unsigned char data)
{
	if ((address & 0xff80) == 0) {
		DrvZ80ROM0[address] = data;
		return;
	}

	switch (address)
	{
		case 0xa000:
			BurnYM3812Write(0, data);
		return;

		case 0xa001:
			BurnYM3812Write(1, data);
		return;

		case 0xc000:
			adpcm_pos = data << 8;
			MSM5205Reset(0);
		return;

		case 0xc400:
			adpcm_end = (data + 1) << 8;
			MSM5205Play(adpcm_pos, adpcm_end, 0);
		return;

		case 0xc800:
			adpcm_vol = (data & 0x0f) * 100 / 15;
		return;

		case 0xcc00:
		return;
	}

	return;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	DrvZ80ROM0	= Next; Next += 0x20000;
	DrvZ80ROM1	= Next; Next += 0x08000;

	MSM5205ROM	= Next;
	DrvSndROM	= Next; Next += 0x08000;

	DrvGfxROM0	= Next; Next += 0x10000;
	DrvGfxROM1	= Next; Next += 0x80000;
	DrvGfxROM2	= Next; Next += 0x80000;
	DrvGfxROM3	= Next; Next += 0x80000;

	AllRam		= Next;

	DrvZ80RAM0	= Next; Next += 0x01000;
	DrvZ80RAM1	= Next; Next += 0x00800;

	DrvPalRAM	= Next; Next += 0x00800;
	DrvTextRAM	= Next; Next += 0x00800;
	DrvBackRAM	= Next; Next += 0x00400;
	DrvForeRAM	= Next; Next += 0x00400;
	DrvSprRAM	= Next; Next += 0x00800;

	DrvBgScroll	= (unsigned short*)Next; Next += 0x00002 * sizeof(unsigned short);
	DrvFgScroll	= (unsigned short*)Next; Next += 0x00002 * sizeof(unsigned short);

	Palette		= (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);
	DrvPalette	= (unsigned int*)Next; Next += 0x00400 * sizeof(unsigned int);

	RamEnd		= Next;
	MemEnd		= Next;

	return 0;
}

static int DrvGfxDecode()
{
#ifdef BUILD_PSP
	extern void ui_update_progress2(float size, const char * txt);
	#define UPDATE_PROGRESS(i) ui_update_progress2(1.0 / 4, i ? NULL : "decoding graphics data..." )
#else
	#define UPDATE_PROGRESS(i)
#endif
	unsigned char *tmp = (unsigned char*)malloc(0x40000);
	if (tmp == NULL) {
		return 1;
	}

	static int Planes[4] = {
		0x000, 0x001, 0x002, 0x003
	};

	static int XOffs[16] = {
		0x000, 0x004, 0x008, 0x00c, 0x010, 0x014, 0x018, 0x01c,
		0x100, 0x104, 0x108, 0x10c, 0x110, 0x114, 0x118, 0x11c
	};

	static int YOffs[16] = {
		0x000, 0x020, 0x040, 0x060, 0x080, 0x0a0, 0x0c0, 0x0e0,
		0x200, 0x220, 0x240, 0x260, 0x280, 0x2a0, 0x2c0, 0x2e0
	};

	memset(tmp, 0, 0x40000);
	memcpy (tmp, DrvGfxROM0, 0x08000);

	GfxDecode(0x0400, 4,  8,  8, Planes, XOffs, YOffs, 0x100, tmp, DrvGfxROM0);
	UPDATE_PROGRESS(0);

	memcpy (tmp, DrvGfxROM1, 0x40000);

	GfxDecode(0x2000, 4,  8,  8, Planes, XOffs, YOffs, 0x100, tmp, DrvGfxROM1);
	UPDATE_PROGRESS(1);

	memcpy (tmp, DrvGfxROM2, 0x40000);

	GfxDecode(0x0800, 4, 16, 16, Planes, XOffs, YOffs, 0x400, tmp, DrvGfxROM2);
	UPDATE_PROGRESS(1);

	memcpy (tmp, DrvGfxROM3, 0x40000);

	GfxDecode(0x0800, 4, 16, 16, Planes, XOffs, YOffs, 0x400, tmp, DrvGfxROM3);
	UPDATE_PROGRESS(1);

	free (tmp);

	return 0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	ZetOpen(0);
	ZetReset();
	bankswitch_w(0);
	ZetClose();

	ZetOpen(1);
	ZetReset();
	ZetClose();

	MSM5205Reset(0);
	BurnYM3812Reset();

	if (tecmo_video_type) {
		memset (DrvZ80ROM1 + 0x2000, 0, 0x80);
	}

	soundlatch = 0;
	flipscreen = 0;

	adpcm_pos = 0;
	adpcm_end = 0;
	adpcm_vol = 0;

	return 0;
}

static void TecmoFMIRQHandler(int, int nStatus)
{
	if (nStatus) {
		ZetSetIRQLine(0xFF, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int TecmoSynchroniseStream(int nSoundRate)
{
	return (long long)ZetTotalCycles() * nSoundRate / 4000000;
}

static int RygarInit()
{
	tecmo_video_type = 0;

	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	ZetInit(2);
	ZetOpen(0);
	ZetMapArea(0x0000, 0xbfff, 0, DrvZ80ROM0);
	ZetMapArea(0x0000, 0xbfff, 2, DrvZ80ROM0);
	ZetMapArea(0xc000, 0xcfff, 0, DrvZ80RAM0);
	ZetMapArea(0xc000, 0xcfff, 1, DrvZ80RAM0);
	ZetMapArea(0xc000, 0xcfff, 2, DrvZ80RAM0);
	ZetMapArea(0xd000, 0xd7ff, 0, DrvTextRAM);
	ZetMapArea(0xd000, 0xd7ff, 1, DrvTextRAM);
	ZetMapArea(0xd800, 0xdbff, 0, DrvForeRAM);
	ZetMapArea(0xd800, 0xdbff, 1, DrvForeRAM);
	ZetMapArea(0xdc00, 0xdfff, 0, DrvBackRAM);
	ZetMapArea(0xdc00, 0xdfff, 1, DrvBackRAM);
	ZetMapArea(0xe000, 0xe7ff, 0, DrvSprRAM);
	ZetMapArea(0xe000, 0xe7ff, 1, DrvSprRAM);
	ZetMapArea(0xe800, 0xefff, 0, DrvPalRAM);
	ZetSetWriteHandler(rygar_main_write);
	ZetSetReadHandler(rygar_main_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea(0x0000, 0x3fff, 0, DrvZ80ROM1);
	ZetMapArea(0x0000, 0x3fff, 2, DrvZ80ROM1);
	ZetMapArea(0x4000, 0x47ff, 0, DrvZ80RAM1);
	ZetMapArea(0x4000, 0x47ff, 1, DrvZ80RAM1);
	ZetMapArea(0x4000, 0x47ff, 2, DrvZ80RAM1);
	ZetSetWriteHandler(rygar_sound_write);
	ZetSetReadHandler(rygar_sound_read);
	ZetMemEnd();
	ZetClose();

	{
		for (int i = 0; i < 3; i++) {
			if (BurnLoadRom(DrvZ80ROM0 + i * 0x8000, i +  0, 1)) return 1;
		}

		if (BurnLoadRom(DrvZ80ROM1,	3, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0,	4, 1)) return 1;

		for (int i = 0; i < 4; i++) {
			if (BurnLoadRom(DrvGfxROM1 + i * 0x8000, i +  5, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM2 + i * 0x8000, i +  9, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM3 + i * 0x8000, i + 13, 1)) return 1;
		}

		if (BurnLoadRom(DrvSndROM,	17, 1)) return 1;

		DrvGfxDecode();
	}

	BurnYM3812Init(4000000, &TecmoFMIRQHandler, &TecmoSynchroniseStream, 0);
	BurnTimerAttachZet(4000000);

	MSM5205Init(0, 8000, 100, 1);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}


static int SilkwormInit()
{
	tecmo_video_type = 1;

	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	ZetInit(2);
	ZetOpen(0);
	ZetMapArea(0x0000, 0xbfff, 0, DrvZ80ROM0);
	ZetMapArea(0x0000, 0xbfff, 2, DrvZ80ROM0);
	ZetMapArea(0xc000, 0xc3ff, 0, DrvBackRAM);
	ZetMapArea(0xc000, 0xc3ff, 1, DrvBackRAM);
	ZetMapArea(0xc400, 0xc7ff, 0, DrvForeRAM);
	ZetMapArea(0xc400, 0xc7ff, 1, DrvForeRAM);
	ZetMapArea(0xc800, 0xcfff, 0, DrvTextRAM);
	ZetMapArea(0xc800, 0xcfff, 1, DrvTextRAM);
	ZetMapArea(0xd000, 0xdfff, 0, DrvZ80RAM0);
	ZetMapArea(0xd000, 0xdfff, 1, DrvZ80RAM0);
	ZetMapArea(0xd000, 0xdfff, 2, DrvZ80RAM0);
	ZetMapArea(0xe000, 0xe7ff, 0, DrvSprRAM);
	ZetMapArea(0xe000, 0xe7ff, 1, DrvSprRAM);
	ZetMapArea(0xe800, 0xefff, 0, DrvPalRAM);
	ZetSetWriteHandler(rygar_main_write);
	ZetSetReadHandler(rygar_main_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM1);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM1);
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80RAM1);
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80RAM1);
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80RAM1);
	ZetSetWriteHandler(tecmo_sound_write);
	ZetSetReadHandler(rygar_sound_read);
	ZetMemEnd();
	ZetClose();

	{
		for (int i = 0; i < 2; i++) {
			if (BurnLoadRom(DrvZ80ROM0 + i * 0x10000, i +  0, 1)) return 1;
		}

		if (BurnLoadRom(DrvZ80ROM1,	2, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0,	3, 1)) return 1;

		for (int i = 0; i < 4; i++) {
			if (BurnLoadRom(DrvGfxROM1 + i * 0x10000, i +  4, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM2 + i * 0x10000, i +  8, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM3 + i * 0x10000, i + 12, 1)) return 1;
		}

		if (BurnLoadRom(DrvSndROM,	16, 1)) return 1;

		DrvGfxDecode();
	}

	BurnYM3812Init(4000000, &TecmoFMIRQHandler, &TecmoSynchroniseStream, 0);
	BurnTimerAttachZet(4000000);

	MSM5205Init(0, 8000, 100, 1);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int GeminiInit()
{
	tecmo_video_type = 2;

	AllMem = NULL;
	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	ZetInit(2);
	ZetOpen(0);
	ZetMapArea(0x0000, 0xbfff, 0, DrvZ80ROM0);
	ZetMapArea(0x0000, 0xbfff, 2, DrvZ80ROM0);
	ZetMapArea(0xc000, 0xcfff, 0, DrvZ80RAM0);
	ZetMapArea(0xc000, 0xcfff, 1, DrvZ80RAM0);
	ZetMapArea(0xc000, 0xcfff, 2, DrvZ80RAM0);
	ZetMapArea(0xd000, 0xd7ff, 0, DrvTextRAM);
	ZetMapArea(0xd000, 0xd7ff, 1, DrvTextRAM);
	ZetMapArea(0xd800, 0xdbff, 0, DrvForeRAM);
	ZetMapArea(0xd800, 0xdbff, 1, DrvForeRAM);
	ZetMapArea(0xdc00, 0xdfff, 0, DrvBackRAM);
	ZetMapArea(0xdc00, 0xdfff, 1, DrvBackRAM);
	ZetMapArea(0xe000, 0xe7ff, 0, DrvPalRAM);
	ZetMapArea(0xe800, 0xefff, 0, DrvSprRAM);
	ZetMapArea(0xe800, 0xefff, 1, DrvSprRAM);
	ZetSetWriteHandler(rygar_main_write);
	ZetSetReadHandler(rygar_main_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80ROM1);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80ROM1);
	ZetMapArea(0x8000, 0x87ff, 0, DrvZ80RAM1);
	ZetMapArea(0x8000, 0x87ff, 1, DrvZ80RAM1);
	ZetMapArea(0x8000, 0x87ff, 2, DrvZ80RAM1);
	ZetSetWriteHandler(tecmo_sound_write);
	ZetSetReadHandler(rygar_sound_read);
	ZetMemEnd();
	ZetClose();

	{
		for (int i = 0; i < 2; i++) {
			if (BurnLoadRom(DrvZ80ROM0 + i * 0x10000, i +  0, 1)) return 1;
		}

		if (BurnLoadRom(DrvZ80ROM1,	2, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0,	3, 1)) return 1;

		for (int i = 0; i < 4; i++) {
			if (BurnLoadRom(DrvGfxROM1 + i * 0x10000, i +  4, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM2 + i * 0x10000, i +  8, 1)) return 1;
			if (BurnLoadRom(DrvGfxROM3 + i * 0x10000, i + 12, 1)) return 1;
		}

		if (BurnLoadRom(DrvSndROM,	16, 1)) return 1;

		DrvGfxDecode();
	}

	BurnYM3812Init(4000000, &TecmoFMIRQHandler, &TecmoSynchroniseStream, 0);
	BurnTimerAttachZet(4000000);

	MSM5205Init(0, 8000, 100, 1);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	MSM5205Exit(0);
	BurnYM3812Exit();

	GenericTilesExit();

	ZetExit();

	free (AllMem);
	AllMem = NULL;

	tecmo_video_type = 0;

	return 0;
}

static void draw_sprites(int priority)
{
	int offs;
	static const unsigned char layout[8][8] =
	{
		{0,1,4,5,16,17,20,21},
		{2,3,6,7,18,19,22,23},
		{8,9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	for (offs = 0; offs < 0x800; offs += 8)
	{
		int flags = DrvSprRAM[offs+3];
		if (priority != (flags >> 6)) continue;

		int bank = DrvSprRAM[offs+0];

		if (bank & 4)
		{
			int which = DrvSprRAM[offs+1];
			int code, xpos, ypos, flipx, flipy, x, y;
			int size = DrvSprRAM[offs + 2] & 3;

			if (tecmo_video_type)
				code = which + ((bank & 0xf8) << 5);
			else
				code = which + ((bank & 0xf0) << 4);

			code &= ~((1 << (size << 1)) - 1);
			size = 1 << size;

			xpos = DrvSprRAM[offs + 5] - ((flags & 0x10) << 4);
			ypos = DrvSprRAM[offs + 4] - ((flags & 0x20) << 3);
			flipx = bank & 1;
			flipy = bank & 2;

			for (y = 0; y < size; y++)
			{
				for (x = 0; x < size; x++)
				{
					int sx = xpos + ((flipx ? (size - 1 - x) : x) << 3);
					int sy = ypos + ((flipy ? (size - 1 - y) : y) << 3);
					    sy -= 16;

					if (sy < -7 || sx < -7 || sx > 255 || sy > 223) continue;

					if (flipy) {
						if (flipx) {
							Render8x8Tile_Mask_FlipXY_Clip(pTransDraw, code + layout[y][x], sx, sy, flags & 0x0f, 4, 0, 0, DrvGfxROM1);
						} else {
							Render8x8Tile_Mask_FlipY_Clip(pTransDraw, code + layout[y][x], sx, sy, flags & 0x0f, 4, 0, 0, DrvGfxROM1);
						}
					} else {
						if (flipx) {
							Render8x8Tile_Mask_FlipX_Clip(pTransDraw, code + layout[y][x], sx, sy, flags & 0x0f, 4, 0, 0, DrvGfxROM1);
						} else {
							Render8x8Tile_Mask_Clip(pTransDraw, code + layout[y][x], sx, sy, flags & 0x0f, 4, 0, 0, DrvGfxROM1);
						}
					}
				}
			}
		}
	}
}
 
static int Draw_16x16_Tiles(unsigned char *vidram, unsigned char *gfx_base, int paloffs, unsigned short *scroll)
{
	for (int offs = 0; offs < 32 * 16; offs++)
	{
		int sx = (offs & 0x1f) << 4;
		int sy = (offs >> 5) << 4;

		    sx -= scroll[0] & 0x1ff;

		    if (flipscreen) {
			sx += 48 + 256;
		    } else {
			sx -= 48;
		    }

		if (sx <   -15) sx += 0x200;
		if (sx >   511) sx -= 0x200;
		    sy -= scroll[1];
		if (sy <   -15) sy += 0x100;

		unsigned char color = vidram[0x200 | offs];
		int code  = vidram[offs];

		if (tecmo_video_type == 2) {
			color = (color << 4) | (color >> 4);
		}

		    code  |= ((color & 7) << 8);
		    color >>= 4;

		if (sx < -15 || sy < 1 || sx > 255 || sy > 239) continue;

		if (sx < 0 || sy < 0 || sx > 240 || sy > 224) {
			Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy-16, color, 4, 0, paloffs, gfx_base);
		} else {
			Render16x16Tile_Mask(pTransDraw, code, sx, sy-16, color, 4, 0, paloffs, gfx_base);
		}
	}

	return 0;
}

static int DrvDraw()
{
	if (DrvRecalc) {
		for (int i = 0; i < 0x400; i++) {
			unsigned int col = Palette[i];
			DrvPalette[i] = BurnHighCol(col>>16, col>>8, col, 0);
		}
	}

	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		pTransDraw[i] = 0x100;
	}
 
	draw_sprites(3);

	Draw_16x16_Tiles(DrvBackRAM, DrvGfxROM3, 0x300, DrvBgScroll);

	draw_sprites(2);

	Draw_16x16_Tiles(DrvForeRAM, DrvGfxROM2, 0x200, DrvFgScroll);

	draw_sprites(1);
	draw_sprites(0);

	{
		for (int offs = 0; offs < 0x400; offs++)
		{
			int sx = (offs & 0x1f) << 3;
			int sy = (offs >> 5) << 3;

			int color = DrvTextRAM[offs | 0x400];

			int code = DrvTextRAM[offs] | ((color & 3) << 8);

			color >>= 4;

			if (sy < 16 || sy > 239) continue;

			Render8x8Tile_Mask(pTransDraw, code, sx, sy-16, color, 4, 0, 0x100, DrvGfxROM0);
		}
	}

	if (flipscreen) {
		int nSize = (nScreenWidth * nScreenHeight) - 1;
		for (int i = 0; i < nSize >> 1; i++) {
			int n = pTransDraw[i];
			pTransDraw[i] = pTransDraw[nSize - i];
			pTransDraw[nSize - i] = n;
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
		memset (DrvInputs, 0, 6);
		DrvInputs[10] = 0;

		for (int i = 0; i < 8; i++) {
			DrvInputs[ 0] ^= DrvJoy1[i] << i;
			DrvInputs[ 1] ^= DrvJoy2[i] << i;
			DrvInputs[ 2] ^= DrvJoy3[i] << i;
			DrvInputs[ 3] ^= DrvJoy4[i] << i;
			DrvInputs[ 4] ^= DrvJoy5[i] << i;
			DrvInputs[ 5] ^= DrvJoy6[i] << i;
			DrvInputs[10] ^= DrvJoy11[i] << i;
		}
	}

	ZetNewFrame();

	int nSegment;
	int nInterleave = 10;
	int nTotalCycles[2] = { 6000000 / 60, 4000000 / 60 };
	int nCyclesDone[2] = { 0, 0 };

	for (int i = 0; i < nInterleave; i++)
	{
		nSegment = (nTotalCycles[0] - nCyclesDone[0]) / (nInterleave - i);

		ZetOpen(0);
		nCyclesDone[0] += ZetRun(nSegment);
		if (i == (nInterleave-1)) ZetRaiseIrq(0);
		ZetClose();

		nSegment = (nTotalCycles[1] - nCyclesDone[1]) / (nInterleave - i);

		ZetOpen(1);
		if (DrvEnableNmi) {
			ZetNmi();
			DrvEnableNmi = 0;
		}
		nCyclesDone[1] += nSegment;
		BurnTimerUpdate(nSegment);
		ZetClose();
	}

	ZetOpen(1);
	BurnTimerEndFrame(nTotalCycles[1]);
	BurnYM3812Update(pBurnSoundOut, nBurnSoundLen);
	MSM5205Render(0, pBurnSoundOut, nBurnSoundLen);
	ZetClose();

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {
		*pnMin = 0x029622;
	}

	if (nAction & ACB_VOLATILE) {		
		memset(&ba, 0, sizeof(ba));

		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);

		ba.Data   = DrvZ80ROM1 + 0x2000;
		ba.nLen	  = 0x80;
		ba.szName = "Sound Z80 RAM";
		BurnAcb(&ba);

		ZetScan(nAction);
		BurnYM3812Scan(nAction, pnMin);
		MSM5205Scan(0, nAction);

		SCAN_VAR(flipscreen);
		SCAN_VAR(soundlatch);
		SCAN_VAR(DrvZ80Bank);

		SCAN_VAR(adpcm_pos);
		SCAN_VAR(adpcm_end);
		SCAN_VAR(adpcm_vol);
	}

	ZetOpen(0);
	ZetMapArea(0xf000, 0xf7ff, 0, DrvZ80ROM0 + DrvZ80Bank);
	ZetMapArea(0xf000, 0xf7ff, 2, DrvZ80ROM0 + DrvZ80Bank);
	ZetClose();

	return 0;
}


// Rygar (US set 1)

static struct BurnRomInfo rygarRomDesc[] = {
	{ "5.5p",      	0x08000, 0x062cd55d, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "cpu_5m.bin",	0x04000, 0x7ac5191b, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "cpu_5j.bin",	0x08000, 0xed76d606, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "cpu_4h.bin",	0x02000, 0xe4a2fa87, 2 | BRF_PRG | BRF_ESS }, //  3 - Z80 Code

	{ "cpu_8k.bin",	0x08000, 0x4d482fb6, 3 | BRF_GRA },	      //  4 - Characters

	{ "vid_6k.bin",	0x08000, 0xaba6db9e, 4 | BRF_GRA },	      //  5 - Sprites
	{ "vid_6j.bin",	0x08000, 0xae1f2ed6, 4 | BRF_GRA },	      //  6
	{ "vid_6h.bin",	0x08000, 0x46d9e7df, 4 | BRF_GRA },	      //  7
	{ "vid_6g.bin",	0x08000, 0x45839c9a, 4 | BRF_GRA },	      //  8

	{ "vid_6p.bin",	0x08000, 0x9eae5f8e, 5 | BRF_GRA },	      //  9 - Foreground Tiles
	{ "vid_6o.bin",	0x08000, 0x5a10a396, 5 | BRF_GRA },	      // 10
	{ "vid_6n.bin",	0x08000, 0x7b12cf3f, 5 | BRF_GRA },	      // 11
	{ "vid_6l.bin",	0x08000, 0x3cea7eaa, 5 | BRF_GRA },	      // 12

	{ "vid_6f.bin",	0x08000, 0x9840edd8, 6 | BRF_GRA },	      // 13 - Background Tiles
	{ "vid_6e.bin",	0x08000, 0xff65e074, 6 | BRF_GRA },	      // 14 
	{ "vid_6c.bin",	0x08000, 0x89868c85, 6 | BRF_GRA },	      // 15 
	{ "vid_6b.bin",	0x08000, 0x35389a7b, 6 | BRF_GRA },	      // 16 

	{ "cpu_1f.bin",	0x04000, 0x3cc98c5a, 7 | BRF_SND },	      // 17 - Samples
};

STD_ROM_PICK(rygar);
STD_ROM_FN(rygar);

struct BurnDriver BurnDrvRygar = {
	"rygar", NULL, NULL, "1986",
	"Rygar (US set 1)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, rygarRomInfo, rygarRomName, RygarInputInfo, RygarDIPInfo,
	RygarInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Rygar (US set 2)

static struct BurnRomInfo rygar2RomDesc[] = {
	{ "5p.bin",     0x08000, 0x151ffc0b, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "cpu_5m.bin",	0x04000, 0x7ac5191b, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "cpu_5j.bin",	0x08000, 0xed76d606, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "cpu_4h.bin",	0x02000, 0xe4a2fa87, 2 | BRF_PRG | BRF_ESS }, //  3 - Z80 Code

	{ "cpu_8k.bin",	0x08000, 0x4d482fb6, 3 | BRF_GRA },	      //  4 - Characters

	{ "vid_6k.bin",	0x08000, 0xaba6db9e, 4 | BRF_GRA },	      //  5 - Sprites
	{ "vid_6j.bin",	0x08000, 0xae1f2ed6, 4 | BRF_GRA },	      //  6
	{ "vid_6h.bin",	0x08000, 0x46d9e7df, 4 | BRF_GRA },	      //  7
	{ "vid_6g.bin",	0x08000, 0x45839c9a, 4 | BRF_GRA },	      //  8

	{ "vid_6p.bin",	0x08000, 0x9eae5f8e, 5 | BRF_GRA },	      //  9 - Foreground Tiles
	{ "vid_6o.bin",	0x08000, 0x5a10a396, 5 | BRF_GRA },	      // 10
	{ "vid_6n.bin",	0x08000, 0x7b12cf3f, 5 | BRF_GRA },	      // 11
	{ "vid_6l.bin",	0x08000, 0x3cea7eaa, 5 | BRF_GRA },	      // 12

	{ "vid_6f.bin",	0x08000, 0x9840edd8, 6 | BRF_GRA },	      // 13 - Background Tiles
	{ "vid_6e.bin",	0x08000, 0xff65e074, 6 | BRF_GRA },	      // 14 
	{ "vid_6c.bin",	0x08000, 0x89868c85, 6 | BRF_GRA },	      // 15 
	{ "vid_6b.bin",	0x08000, 0x35389a7b, 6 | BRF_GRA },	      // 16 

	{ "cpu_1f.bin",	0x04000, 0x3cc98c5a, 7 | BRF_SND },	      // 17 - Samples
};

STD_ROM_PICK(rygar2);
STD_ROM_FN(rygar2);

struct BurnDriver BurnDrvRygar2 = {
	"rygar2", "rygar", NULL, "1986",
	"Rygar (US set 2)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, rygar2RomInfo, rygar2RomName, RygarInputInfo, RygarDIPInfo,
	RygarInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Rygar (US set 3, old version)

static struct BurnRomInfo rygar3RomDesc[] = {
	{ "cpu_5p.bin", 0x08000, 0xe79c054a, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "cpu_5m.bin",	0x04000, 0x7ac5191b, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "cpu_5j.bin",	0x08000, 0xed76d606, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "cpu_4h.bin",	0x02000, 0xe4a2fa87, 2 | BRF_PRG | BRF_ESS }, //  3 - Z80 Code

	{ "cpu_8k.bin",	0x08000, 0x4d482fb6, 3 | BRF_GRA },	      //  4 - Characters

	{ "vid_6k.bin",	0x08000, 0xaba6db9e, 4 | BRF_GRA },	      //  5 - Sprites
	{ "vid_6j.bin",	0x08000, 0xae1f2ed6, 4 | BRF_GRA },	      //  6
	{ "vid_6h.bin",	0x08000, 0x46d9e7df, 4 | BRF_GRA },	      //  7
	{ "vid_6g.bin",	0x08000, 0x45839c9a, 4 | BRF_GRA },	      //  8

	{ "vid_6p.bin",	0x08000, 0x9eae5f8e, 5 | BRF_GRA },	      //  9 - Foreground Tiles
	{ "vid_6o.bin",	0x08000, 0x5a10a396, 5 | BRF_GRA },	      // 10
	{ "vid_6n.bin",	0x08000, 0x7b12cf3f, 5 | BRF_GRA },	      // 11
	{ "vid_6l.bin",	0x08000, 0x3cea7eaa, 5 | BRF_GRA },	      // 12

	{ "vid_6f.bin",	0x08000, 0x9840edd8, 6 | BRF_GRA },	      // 13 - Background Tiles
	{ "vid_6e.bin",	0x08000, 0xff65e074, 6 | BRF_GRA },	      // 14 
	{ "vid_6c.bin",	0x08000, 0x89868c85, 6 | BRF_GRA },	      // 15 
	{ "vid_6b.bin",	0x08000, 0x35389a7b, 6 | BRF_GRA },	      // 16 

	{ "cpu_1f.bin",	0x04000, 0x3cc98c5a, 7 | BRF_SND },	      // 17 - Samples
};

STD_ROM_PICK(rygar3);
STD_ROM_FN(rygar3);

struct BurnDriver BurnDrvRygar3 = {
	"rygar3", "rygar", NULL, "1986",
	"Rygar (US set 3, old version)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, rygar3RomInfo, rygar3RomName, RygarInputInfo, RygarDIPInfo,
	RygarInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Argus no Senshi (Japan)

static struct BurnRomInfo rygarjRomDesc[] = {
	{ "cpuj_5p.bin",0x08000, 0xb39698ba, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "cpuj_5m.bin",0x04000, 0x3f180979, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "cpuj_5j.bin",0x08000, 0x69e44e8f, 1 | BRF_PRG | BRF_ESS }, //  2

	{ "cpu_4h.bin",	0x02000, 0xe4a2fa87, 2 | BRF_PRG | BRF_ESS }, //  3 - Z80 Code

	{ "cpuj_8k.bin",0x08000, 0x45047707, 3 | BRF_GRA },	      //  4 - Characters

	{ "vid_6k.bin",	0x08000, 0xaba6db9e, 4 | BRF_GRA },	      //  5 - Sprites
	{ "vid_6j.bin",	0x08000, 0xae1f2ed6, 4 | BRF_GRA },	      //  6
	{ "vid_6h.bin",	0x08000, 0x46d9e7df, 4 | BRF_GRA },	      //  7
	{ "vid_6g.bin",	0x08000, 0x45839c9a, 4 | BRF_GRA },	      //  8

	{ "vid_6p.bin",	0x08000, 0x9eae5f8e, 5 | BRF_GRA },	      //  9 - Foreground Tiles
	{ "vid_6o.bin",	0x08000, 0x5a10a396, 5 | BRF_GRA },	      // 10
	{ "vid_6n.bin",	0x08000, 0x7b12cf3f, 5 | BRF_GRA },	      // 11
	{ "vid_6l.bin",	0x08000, 0x3cea7eaa, 5 | BRF_GRA },	      // 12

	{ "vid_6f.bin",	0x08000, 0x9840edd8, 6 | BRF_GRA },	      // 13 - Background Tiles
	{ "vid_6e.bin",	0x08000, 0xff65e074, 6 | BRF_GRA },	      // 14 
	{ "vid_6c.bin",	0x08000, 0x89868c85, 6 | BRF_GRA },	      // 15 
	{ "vid_6b.bin",	0x08000, 0x35389a7b, 6 | BRF_GRA },	      // 16 

	{ "cpu_1f.bin",	0x04000, 0x3cc98c5a, 7 | BRF_SND },	      // 17 - Samples
};

STD_ROM_PICK(rygarj);
STD_ROM_FN(rygarj);

struct BurnDriver BurnDrvRygarj = {
	"rygarj", "rygar", NULL, "1986",
	"Argus no Senshi (Japan)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, rygarjRomInfo, rygarjRomName, RygarInputInfo, RygarDIPInfo,
	RygarInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Silk Worm (set 1)

static struct BurnRomInfo silkwormRomDesc[] = {
	{ "silkworm.4",		0x10000, 0xa5277cce, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "silkworm.5",		0x10000, 0xa6c7bb51, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "silkworm.3",		0x08000, 0xb589f587, 2 | BRF_PRG | BRF_ESS }, //  2 - Z80 Code

	{ "silkworm.2",		0x08000, 0xe80a1cd9, 3 | BRF_GRA },	      //  3 - Characters

	{ "silkworm.6",		0x10000, 0x1138d159, 4 | BRF_GRA },	      //  4 - Sprites
	{ "silkworm.7",		0x10000, 0xd96214f7, 4 | BRF_GRA },	      //  5
	{ "silkworm.8",		0x10000, 0x0494b38e, 4 | BRF_GRA },	      //  6
	{ "silkworm.9",		0x10000, 0x8ce3cdf5, 4 | BRF_GRA },	      //  7

	{ "silkworm.10",	0x10000, 0x8c7138bb, 5 | BRF_GRA },	      //  8 - Foreground Tiles
	{ "silkworm.11",	0x10000, 0x6c03c476, 5 | BRF_GRA },	      //  9
	{ "silkworm.12",	0x10000, 0xbb0f568f, 5 | BRF_GRA },	      // 10
	{ "silkworm.13",	0x10000, 0x773ad0a4, 5 | BRF_GRA },	      // 11

	{ "silkworm.14",	0x10000, 0x409df64b, 6 | BRF_GRA },	      // 12 - Background Tiles
	{ "silkworm.15",	0x10000, 0x6e4052c9, 6 | BRF_GRA },	      // 13
	{ "silkworm.16",	0x10000, 0x9292ed63, 6 | BRF_GRA },	      // 14
	{ "silkworm.17",	0x10000, 0x3fa4563d, 6 | BRF_GRA },	      // 15

	{ "silkworm.1",		0x08000, 0x5b553644, 7 | BRF_SND },	      // 16 - Samples
};

STD_ROM_PICK(silkworm);
STD_ROM_FN(silkworm);

struct BurnDriver BurnDrvSilkworm = {
	"silkworm", NULL, NULL, "1988",
	"Silk Worm (set 1)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_HORSHOOT, 0,
	NULL, silkwormRomInfo, silkwormRomName, SilkwormInputInfo, SilkwormDIPInfo,
	SilkwormInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Silk Worm (set 1)

static struct BurnRomInfo silkwrm2RomDesc[] = {
	{ "r4",			0x10000, 0x6df3df22, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "silkworm.5",		0x10000, 0xa6c7bb51, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "r3",			0x08000, 0xb79848d0, 2 | BRF_PRG | BRF_ESS }, //  2 - Z80 Code

	{ "silkworm.2",		0x08000, 0xe80a1cd9, 3 | BRF_GRA },	      //  3 - Characters

	{ "silkworm.6",		0x10000, 0x1138d159, 4 | BRF_GRA },	      //  4 - Sprites
	{ "silkworm.7",		0x10000, 0xd96214f7, 4 | BRF_GRA },	      //  5
	{ "silkworm.8",		0x10000, 0x0494b38e, 4 | BRF_GRA },	      //  6
	{ "silkworm.9",		0x10000, 0x8ce3cdf5, 4 | BRF_GRA },	      //  7

	{ "silkworm.10",	0x10000, 0x8c7138bb, 5 | BRF_GRA },	      //  8 - Foreground Tiles
	{ "silkworm.11",	0x10000, 0x6c03c476, 5 | BRF_GRA },	      //  9
	{ "silkworm.12",	0x10000, 0xbb0f568f, 5 | BRF_GRA },	      // 10
	{ "silkworm.13",	0x10000, 0x773ad0a4, 5 | BRF_GRA },	      // 11

	{ "silkworm.14",	0x10000, 0x409df64b, 6 | BRF_GRA },	      // 12 - Background Tiles
	{ "silkworm.15",	0x10000, 0x6e4052c9, 6 | BRF_GRA },	      // 13
	{ "silkworm.16",	0x10000, 0x9292ed63, 6 | BRF_GRA },	      // 14
	{ "silkworm.17",	0x10000, 0x3fa4563d, 6 | BRF_GRA },	      // 15

	{ "silkworm.1",		0x08000, 0x5b553644, 7 | BRF_SND },	      // 16 - Samples
};

STD_ROM_PICK(silkwrm2);
STD_ROM_FN(silkwrm2);

struct BurnDriver BurnDrvSilkwrm2 = {
	"silkwrm2", "silkworm", NULL, "1988",
	"Silk Worm (set 2)\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_HORSHOOT, 0,
	NULL, silkwrm2RomInfo, silkwrm2RomName, SilkwormInputInfo, SilkwormDIPInfo,
	SilkwormInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	256, 224, 4, 3
};


// Gemini Wing

static struct BurnRomInfo geminiRomDesc[] = {
	{ "gw04-5s.rom",	0x10000, 0xff9de855, 1 | BRF_PRG | BRF_ESS }, //  0 - Z80 Code
	{ "gw05-6s.rom",	0x10000, 0x5a6947a9, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "gw03-5h.rom",	0x08000, 0x9bc79596, 2 | BRF_PRG | BRF_ESS }, //  2 - Z80 Code

	{ "gw02-3h.rom",	0x08000, 0x7acc8d35, 3 | BRF_GRA },	      //  3 - Characters

	{ "gw06-1c.rom",	0x10000, 0x4ea51631, 4 | BRF_GRA },	      //  4 - Sprites
	{ "gw07-1d.rom",	0x10000, 0xda42637e, 4 | BRF_GRA },	      //  5
	{ "gw08-1f.rom",	0x10000, 0x0b4e8d70, 4 | BRF_GRA },	      //  6
	{ "gw09-1h.rom",	0x10000, 0xb65c5e4c, 4 | BRF_GRA },	      //  7

	{ "gw10-1n.rom",	0x10000, 0x5e84cd4f, 5 | BRF_GRA },	      //  8 - Foreground Tiles
	{ "gw11-2na.rom",	0x10000, 0x08b458e1, 5 | BRF_GRA },	      //  9
	{ "gw12-2nb.rom",	0x10000, 0x229c9714, 5 | BRF_GRA },	      // 10
	{ "gw13-3n.rom",	0x10000, 0xc5dfaf47, 5 | BRF_GRA },	      // 11

	{ "gw14-1r.rom",	0x10000, 0x9c10e5b5, 6 | BRF_GRA },	      // 12 - Background Tiles
	{ "gw15-2ra.rom",	0x10000, 0x4cd18cfa, 6 | BRF_GRA },	      // 13
	{ "gw16-2rb.rom",	0x10000, 0xf911c7be, 6 | BRF_GRA },	      // 14
	{ "gw17-3r.rom",	0x10000, 0x79a9ce25, 6 | BRF_GRA },	      // 15

	{ "gw01-6a.rom",	0x08000, 0xd78afa05, 7 | BRF_SND },	      // 16 - Samples
};

STD_ROM_PICK(gemini);
STD_ROM_FN(gemini);

struct BurnDriver BurnDrvGemini = {
	"gemini", NULL, NULL, "1987",
	"Gemini Wing\0", NULL, "Tecmo", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_PRE90S, GBF_VERSHOOT, 0,
	NULL, geminiRomInfo, geminiRomName, GeminiInputInfo, GeminiDIPInfo,
	GeminiInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &DrvRecalc,
	224, 256, 3, 4
};
