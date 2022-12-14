#include "tiles_generic.h"
#include "burn_ym2151.h"
//#include "msm5205.h"

// To do:
//	implement OKI MSM5205 audio (2x)

static unsigned char *Mem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *Drv68kRom;
static unsigned char *DrvZ80Rom0;
static unsigned char *DrvZ80Rom1;
static unsigned char *Drv68kPalRam;
static unsigned char *Drv68kVidRam;
static unsigned char *Drv68kRam;
static unsigned char *DrvZ80Ram0;
static unsigned char *Gfx0;
static unsigned char *Gfx1;
static unsigned char *Gfx2;
static unsigned char *Gfx3;
static unsigned char *Gfx4;
static unsigned char *Gfx1Trans;
static unsigned char *Gfx3Trans;
static unsigned int *Palette;
static unsigned int *DrvPalette;
static unsigned char DrvRecalc;

static unsigned char ALIGN_DATA DrvJoy1[16];
static unsigned char ALIGN_DATA DrvJoy2[16];
static unsigned char ALIGN_DATA DrvJoy3[16];
static unsigned char ALIGN_DATA DrvJoy4[16];
static unsigned char ALIGN_DATA DrvJoy5[16];
static unsigned char ALIGN_DATA DrvDips[ 4];
static unsigned char DrvReset;
static unsigned short ALIGN_DATA DrvInputs[8];

static int version;

static int sf_fg_scroll_x;
static int sf_bg_scroll_x;

static int soundlatch;
static int flipscreen;

static int sf_active;
static int sound2_bank;

static struct BurnInputInfo SfInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy2 + 1,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 0,	"p1 start" },
	{"P1 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p1 up",   },
	{"P1 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p1 down", },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy3 + 1, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy3 + 0, 	"p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 0,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 1,	"p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy4 + 2,	"p1 fire 3"},
	{"P1 Button 4"  , BIT_DIGITAL  , DrvJoy4 + 8,	"p1 fire 4"},
	{"P1 Button 5"  , BIT_DIGITAL  , DrvJoy4 + 9,	"p1 fire 5"},
	{"P1 Button 6"  , BIT_DIGITAL  , DrvJoy4 + 10,	"p1 fire 6"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 1,	"p2 start" },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy3 + 11,  "p2 up",   },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy3 + 10,  "p2 down", },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy3 + 9, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy3 + 8, 	"p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy5 + 0,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy5 + 1,	"p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy5 + 2,	"p2 fire 3"},
	{"P2 Button 4"  , BIT_DIGITAL  , DrvJoy5 + 8,	"p2 fire 4"},
	{"P2 Button 5"  , BIT_DIGITAL  , DrvJoy5 + 9,	"p2 fire 5"},
	{"P2 Button 6"  , BIT_DIGITAL  , DrvJoy5 + 10,	"p2 fire 6"},

	{"Service"      , BIT_DIGITAL  , DrvJoy1 + 2,	"service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvDips + 2,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvDips + 3,	"dip"	   },
};

STDINPUTINFO(Sf);

static struct BurnInputInfo SfjpInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy2 + 1,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 0,	"p1 start" },
	{"P1 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p1 up",   },
	{"P1 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p1 down", },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy3 + 1, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy3 + 0, 	"p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy3 + 8,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy3 + 9,	"p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy3 + 10,	"p1 fire 3"},
	{"P1 Button 4"  , BIT_DIGITAL  , DrvJoy3 + 12,	"p1 fire 4"},
	{"P1 Button 5"  , BIT_DIGITAL  , DrvJoy3 + 13,	"p1 fire 5"},
	{"P1 Button 6"  , BIT_DIGITAL  , DrvJoy3 + 14,	"p1 fire 6"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 1,	"p2 start" },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy4 + 3,   "p1 up",   },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy4 + 2,   "p2 down", },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy4 + 1, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy4 + 0, 	"p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy4 + 8,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy4 + 9,	"p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy4 + 10,	"p2 fire 3"},
	{"P2 Button 4"  , BIT_DIGITAL  , DrvJoy4 + 12,	"p2 fire 4"},
	{"P2 Button 5"  , BIT_DIGITAL  , DrvJoy4 + 13,	"p2 fire 5"},
	{"P2 Button 6"  , BIT_DIGITAL  , DrvJoy4 + 14,	"p2 fire 6"},

	{"Service"      , BIT_DIGITAL  , DrvJoy1 + 2,	"service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvDips + 2,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvDips + 3,	"dip"	   },
};

STDINPUTINFO(Sfjp);

static struct BurnInputInfo SfusInputList[] = {
	{"Coin 1"       , BIT_DIGITAL  , DrvJoy2 + 0,	"p1 coin"  },
	{"Coin 2"       , BIT_DIGITAL  , DrvJoy2 + 1,	"p2 coin"  },

	{"P1 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 0,	"p1 start" },
	{"P1 Up",	  BIT_DIGITAL,   DrvJoy3 + 3,   "p1 up",   },
	{"P1 Down",	  BIT_DIGITAL,   DrvJoy3 + 2,   "p1 down", },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy3 + 1, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy3 + 0, 	"p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy3 + 4,	"p1 fire 1"},
	{"P1 Button 2"  , BIT_DIGITAL  , DrvJoy3 + 5,	"p1 fire 2"},
	{"P1 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 9,	"p1 fire 3"},
	{"P1 Button 4"  , BIT_DIGITAL  , DrvJoy3 + 6,	"p1 fire 4"},
	{"P1 Button 5"  , BIT_DIGITAL  , DrvJoy3 + 7,	"p1 fire 5"},
	{"P1 Button 6"  , BIT_DIGITAL  , DrvJoy2 + 2,	"p1 fire 6"},

	{"P2 Start"  ,    BIT_DIGITAL  , DrvJoy1 + 1,	"p2 start" },
	{"P2 Up",	  BIT_DIGITAL,   DrvJoy3 + 11,  "p1 up",   },
	{"P2 Down",	  BIT_DIGITAL,   DrvJoy3 + 10,  "p2 down", },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy3 + 9, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy3 + 8, 	"p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy3 + 12,	"p2 fire 1"},
	{"P2 Button 2"  , BIT_DIGITAL  , DrvJoy3 + 13,	"p2 fire 2"},
	{"P2 Button 3"  , BIT_DIGITAL  , DrvJoy2 + 10,	"p2 fire 3"},
	{"P2 Button 4"  , BIT_DIGITAL  , DrvJoy3 + 14,	"p2 fire 4"},
	{"P2 Button 5"  , BIT_DIGITAL  , DrvJoy3 + 15,	"p2 fire 5"},
	{"P2 Button 6"  , BIT_DIGITAL  , DrvJoy2 + 8,	"p2 fire 6"},

	{"Service"      , BIT_DIGITAL  , DrvJoy1 + 2,	"service"  },

	{"Reset",	  BIT_DIGITAL  , &DrvReset,	"reset"    },
	{"Dip 1",	  BIT_DIPSWITCH, DrvDips + 0,	"dip"	   },
	{"Dip 2",	  BIT_DIPSWITCH, DrvDips + 1,	"dip"	   },
	{"Dip 3",	  BIT_DIPSWITCH, DrvDips + 2,	"dip"	   },
	{"Dip 4",	  BIT_DIPSWITCH, DrvDips + 3,	"dip"	   },
};

STDINPUTINFO(Sfus);

static struct BurnDIPInfo SfDIPList[]=
{
	{0x1a, 0xff, 0xff, 0xff, NULL				},
	{0x1b, 0xff, 0xff, 0xff, NULL				},
	{0x1c, 0xff, 0xff, 0xf8, NULL				},
	{0x1d, 0xff, 0xff, 0x7f, NULL				},

	{0   , 0xfe, 0   , 2   , "Attract Music"		},
	{0x1a, 0x01, 0x02, 0x00, "Off"				},
	{0x1a, 0x01, 0x02, 0x02, "On"				},

	{0   , 0xfe, 0   , 2   , "Speed"			},
	{0x1a, 0x01, 0x10, 0x00, "Slow"				},
	{0x1a, 0x01, 0x10, 0x10, "Normal"			},

	{0   , 0xfe, 0   , 2   , "Demo Sounds"			},
	{0x1a, 0x01, 0x20, 0x20, "Off"				},
	{0x1a, 0x01, 0x20, 0x00, "On"				},

	{0   , 0xfe, 0   , 2   , "Freeze"			},
	{0x1a, 0x01, 0x40, 0x40, "Off"				},
	{0x1a, 0x01, 0x40, 0x00, "On"				},

	{0   , 0xfe, 0   , 8   , "Coin A"			},
	{0x1b, 0x01, 0x07, 0x00, "4 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x01, "3 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x02, "2 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x07, "1 Coin  1 Credit"		},
	{0x1b, 0x01, 0x07, 0x06, "1 Coin  2 Credits"		},
	{0x1b, 0x01, 0x07, 0x05, "1 Coin  3 Credits"		},
	{0x1b, 0x01, 0x07, 0x04, "1 Coin  4 Credits"		},
	{0x1b, 0x01, 0x07, 0x03, "1 Coin  6 Credits"		},

	{0   , 0xfe, 0   , 8   , "Coin B"			},
	{0x1b, 0x01, 0x38, 0x00, "4 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x08, "3 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x10, "2 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x38, "1 Coin  1 Credit"		},
	{0x1b, 0x01, 0x38, 0x30, "1 Coin  2 Credits"		},
	{0x1b, 0x01, 0x38, 0x28, "1 Coin  3 Credits"		},
	{0x1b, 0x01, 0x38, 0x20, "1 Coin  4 Credits"		},
	{0x1b, 0x01, 0x38, 0x18, "1 Coin  6 Credits"		},

	{0   , 0xfe, 0   , 6   , "Buy-in max stage"		},
	{0x1c, 0x01, 0x38, 0x38, "5th"				},
	{0x1c, 0x01, 0x38, 0x30, "4th"				},
	{0x1c, 0x01, 0x38, 0x28, "3rd"				},
	{0x1c, 0x01, 0x38, 0x20, "2nd"				},
	{0x1c, 0x01, 0x38, 0x18, "1st"				},
	{0x1c, 0x01, 0x38, 0x08, "No buy-in"			},

	{0   , 0xfe, 0   , 2   , "Number of start countries"	},
	{0x1c, 0x01, 0x40, 0x40, "4"				},
	{0x1c, 0x01, 0x40, 0x00, "2"				},

	{0   , 0xfe, 0   , 6   , "Continuation max stage"	},
	{0x1d, 0x01, 0x07, 0x07, "5th"				},
	{0x1d, 0x01, 0x07, 0x06, "4th"				},
	{0x1d, 0x01, 0x07, 0x05, "3rd"				},
	{0x1d, 0x01, 0x07, 0x04, "2nd"				},
	{0x1d, 0x01, 0x07, 0x03, "1st"				},
	{0x1d, 0x01, 0x07, 0x02, "No continuation"		},

	{0   , 0xfe, 0   , 4   , "Round time"			},
	{0x1d, 0x01, 0x18, 0x18, "100"				},
	{0x1d, 0x01, 0x18, 0x10, "150"				},
	{0x1d, 0x01, 0x18, 0x08, "200"				},
	{0x1d, 0x01, 0x18, 0x00, "250"				},

	{0   , 0xfe, 0   , 4   , "Difficulty"			},
	{0x1d, 0x01, 0x60, 0x60, "Normal"			},
	{0x1d, 0x01, 0x60, 0x40, "Easy"				},
	{0x1d, 0x01, 0x60, 0x20, "Difficult"			},
	{0x1d, 0x01, 0x60, 0x00, "Very difficult"		},
};

STDDIPINFO(Sf);

static struct BurnDIPInfo SfusDIPList[]=
{
	{0x1a, 0xff, 0xff, 0xff, NULL				},
	{0x1b, 0xff, 0xff, 0xff, NULL				},
	{0x1c, 0xff, 0xff, 0xf8, NULL				},
	{0x1d, 0xff, 0xff, 0x7f, NULL				},

	{0   , 0xfe, 0   , 2   , "Flip Screen"			},
	{0x1a, 0x01, 0x01, 0x01, "Off"				},
	{0x1a, 0x01, 0x01, 0x00, "On"				},

	{0   , 0xfe, 0   , 2   , "Attract Music"		},
	{0x1a, 0x01, 0x02, 0x00, "Off"				},
	{0x1a, 0x01, 0x02, 0x02, "On"				},

	{0   , 0xfe, 0   , 2   , "Speed"			},
	{0x1a, 0x01, 0x10, 0x00, "Slow"				},
	{0x1a, 0x01, 0x10, 0x10, "Normal"			},

	{0   , 0xfe, 0   , 2   , "Demo Sounds"			},
	{0x1a, 0x01, 0x20, 0x20, "Off"				},
	{0x1a, 0x01, 0x20, 0x00, "On"				},

	{0   , 0xfe, 0   , 2   , "Freeze"			},
	{0x1a, 0x01, 0x40, 0x40, "Off"				},
	{0x1a, 0x01, 0x40, 0x00, "On"				},

	{0   , 0xfe, 0   , 8   , "Coin A"			},
	{0x1b, 0x01, 0x07, 0x00, "4 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x01, "3 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x02, "2 Coins 1 Credit"		},
	{0x1b, 0x01, 0x07, 0x07, "1 Coin  1 Credit"		},
	{0x1b, 0x01, 0x07, 0x06, "1 Coin  2 Credits"		},
	{0x1b, 0x01, 0x07, 0x05, "1 Coin  3 Credits"		},
	{0x1b, 0x01, 0x07, 0x04, "1 Coin  4 Credits"		},
	{0x1b, 0x01, 0x07, 0x03, "1 Coin  6 Credits"		},

	{0   , 0xfe, 0   , 8   , "Coin B"			},
	{0x1b, 0x01, 0x38, 0x00, "4 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x08, "3 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x10, "2 Coins 1 Credit"		},
	{0x1b, 0x01, 0x38, 0x38, "1 Coin  1 Credit"		},
	{0x1b, 0x01, 0x38, 0x30, "1 Coin  2 Credits"		},
	{0x1b, 0x01, 0x38, 0x28, "1 Coin  3 Credits"		},
	{0x1b, 0x01, 0x38, 0x20, "1 Coin  4 Credits"		},
	{0x1b, 0x01, 0x38, 0x18, "1 Coin  6 Credits"		},

	{0   , 0xfe, 0   , 6   , "Buy-in max stage"		},
	{0x1c, 0x01, 0x38, 0x38, "5th"				},
	{0x1c, 0x01, 0x38, 0x30, "4th"				},
	{0x1c, 0x01, 0x38, 0x28, "3rd"				},
	{0x1c, 0x01, 0x38, 0x20, "2nd"				},
	{0x1c, 0x01, 0x38, 0x18, "1st"				},
	{0x1c, 0x01, 0x38, 0x08, "No buy-in"			},

	{0   , 0xfe, 0   , 2   , "Number of start countries"	},
	{0x1c, 0x01, 0x40, 0x00, "4"				},
	{0x1c, 0x01, 0x40, 0x40, "2"				},

	{0   , 0xfe, 0   , 6   , "Continuation max stage"	},
	{0x1d, 0x01, 0x07, 0x07, "5th"				},
	{0x1d, 0x01, 0x07, 0x06, "4th"				},
	{0x1d, 0x01, 0x07, 0x05, "3rd"				},
	{0x1d, 0x01, 0x07, 0x04, "2nd"				},
	{0x1d, 0x01, 0x07, 0x03, "1st"				},
	{0x1d, 0x01, 0x07, 0x02, "No continuation"		},

	{0   , 0xfe, 0   , 4   , "Round time"			},
	{0x1d, 0x01, 0x18, 0x18, "100"				},
	{0x1d, 0x01, 0x18, 0x10, "150"				},
	{0x1d, 0x01, 0x18, 0x08, "200"				},
	{0x1d, 0x01, 0x18, 0x00, "250"				},

	{0   , 0xfe, 0   , 4   , "Difficulty"			},
	{0x1d, 0x01, 0x60, 0x60, "Normal"			},
	{0x1d, 0x01, 0x60, 0x40, "Easy"				},
	{0x1d, 0x01, 0x60, 0x20, "Difficult"			},
	{0x1d, 0x01, 0x60, 0x00, "Very difficult"		},
};

STDDIPINFO(Sfus);

static void protection_w()
{
	static const int maplist[4][10] = {
		{ 1, 0, 3, 2, 4, 5, 6, 7, 8, 9 },
		{ 4, 5, 6, 7, 1, 0, 3, 2, 8, 9 },
		{ 3, 2, 1, 0, 6, 7, 4, 5, 8, 9 },
		{ 6, 7, 4, 5, 3, 2, 1, 0, 8, 9 }
	};
	int map;

	map = maplist[SekReadByte(0xffc006)][(SekReadByte(0xffc003)<<1) + (SekReadWord(0xffc004)>>8)];

	switch (SekReadByte(0xffc684))
	{
		case 1:
		{
			int base;

			base = 0x1b6e8+0x300e*map;

			SekWriteLong(0xffc01c, 0x16bfc+0x270*map);
			SekWriteLong(0xffc020, base+0x80);
			SekWriteLong(0xffc024, base);
			SekWriteLong(0xffc028, base+0x86);
			SekWriteLong(0xffc02c, base+0x8e);
			SekWriteLong(0xffc030, base+0x20e);
			SekWriteLong(0xffc034, base+0x30e);
			SekWriteLong(0xffc038, base+0x38e);
			SekWriteLong(0xffc03c, base+0x40e);
			SekWriteLong(0xffc040, base+0x80e);
			SekWriteLong(0xffc044, base+0xc0e);
			SekWriteLong(0xffc048, base+0x180e);
			SekWriteLong(0xffc04c, base+0x240e);
			SekWriteLong(0xffc050, 0x19548+0x60*map);
			SekWriteLong(0xffc054, 0x19578+0x60*map);
			break;
		}

		case 2:
		{
			static const int delta1[10] = {
				0x1f80, 0x1c80, 0x2700, 0x2400, 0x2b80, 0x2e80, 0x3300, 0x3600, 0x3a80, 0x3d80
			};
			static const int delta2[10] = {
				0x2180, 0x1800, 0x3480, 0x2b00, 0x3e00, 0x4780, 0x5100, 0x5a80, 0x6400, 0x6d80
			};

			int d1 = delta1[map] + 0xc0;
			int d2 = delta2[map];

			SekWriteWord(0xffc680, d1);
			SekWriteWord(0xffc682, d2);
			SekWriteWord(0xffc00c, 0xc0);
			SekWriteWord(0xffc00e, 0);

			sf_fg_scroll_x = d1;
			sf_bg_scroll_x = d2;

			break;
		}

		case 4:
		{
			int pos = SekReadByte(0xffc010);
			pos = (pos+1) & 3;
			SekWriteByte(0xffc010, pos);

			if(!pos) {
				int d1 = SekReadWord(0xffc682);
				int off = SekReadWord(0xffc00e);
				if(off!=512) {
					off++;
					d1++;
				} else {
					off = 0;
					d1 -= 512;
				}
				SekWriteWord(0xffc682, d1);
				SekWriteWord(0xffc00e, off);

				sf_bg_scroll_x = d1;
			}
			break;
		}
	}
}

inline static void write_palette(unsigned short data, int offset)
{
	int r = (data >> 8) & 0x0f;
	int g = (data >> 4) & 0x0f;
	int b = (data >> 0) & 0x0f;

	r |= r << 4;
	g |= g << 4;
	b |= b << 4;

	Palette[offset] = (r << 16) | (g << 8) | b;
	DrvPalette[offset] = BurnHighCol(r, g, b, 0);
}

void __fastcall sf_write_word(unsigned int address, unsigned short data)
{
	if ((address & 0xfff800) == 0xb00000) {
		unsigned short *pal = (unsigned short*)(Drv68kPalRam + (address & 0x7ff));

		*pal = data;

		write_palette(*pal, (address >> 1) & 0x3ff);

		return;
	}

	switch (address)
	{
		case 0xc00010:
		return;

		case 0xc00014:
			sf_fg_scroll_x = data;
		return;

		case 0xc00018:
			sf_bg_scroll_x = data;
		return;

		case 0xc0001c:
		{
			soundlatch = data & 0xff;

			ZetOpen(0);
			ZetNmi();
			ZetClose();
		}
		return;

		case 0xc0001e:
			if (version == 3) {
				protection_w();
			}
		return;
	}
}

void __fastcall sf_write_byte(unsigned int address, unsigned char data)
{
	switch (address)
	{
		case 0xc0001b:
		{
			sf_active = data & 0xff;
			flipscreen = data & 0x04;
		}
		return;
	}
}

unsigned short __fastcall sf_read_word(unsigned int address)
{
	switch (address)
	{
		case 0xc00000:
			return DrvInputs[3];

		case 0xc00002:
			return DrvInputs[4];

		case 0xc00004:
			return DrvInputs[5];

		case 0xc00006:
			return DrvInputs[6];

		case 0xc00008:
			return DrvInputs[0];

		case 0xc0000a:
			return DrvInputs[1];

		case 0xc0000c:
			return DrvInputs[2];

		case 0xc0000e:
			return 0xffff;
	}

	return 0;
}

unsigned char __fastcall sf_read_byte(unsigned int address)
{
	switch (address)
	{
		case 0xc00000:
		case 0xc00001:
			return DrvInputs[3] >> ((~address & 1) << 3);

		case 0xc00002:
		case 0xc00003:
			return DrvInputs[4] >> ((~address & 1) << 3);

		case 0xc00004:
		case 0xc00005:
			return DrvInputs[5] >> ((~address & 1) << 3);

		case 0xc00006:
		case 0xc00007:
			return DrvInputs[6] >> ((~address & 1) << 3);

		case 0xc00008:
		case 0xc00009:
			return DrvInputs[0] >> ((~address & 1) << 3);

		case 0xc0000a:
		case 0xc0000b:
			return DrvInputs[1] >> ((~address & 1) << 3);

		case 0xc0000c:
		case 0xc0000d:
			return DrvInputs[2] >> ((~address & 1) << 3);

		case 0xc0000e:
		case 0xc0000f:
			return 0xff;
	}

	return 0;
}

void __fastcall sf_sound_write(unsigned short address, unsigned char data)
{
	switch (address)
	{
		case 0xe000:
			BurnYM2151SelectRegister(data);
		return;

		case 0xe001:
			BurnYM2151WriteRegister(data);
		return;
	}
}

unsigned char __fastcall sf_sound_read(unsigned short address)
{
	switch (address)
	{
		case 0xc800:
			return soundlatch;

		case 0xe000:
		case 0xe001:
			return BurnYM2151ReadStatus(address & 0x01);
	}

	return 0;
}


static void sound2_bank_w(int data)
{
	sound2_bank = (data + 1) << 15;

	ZetMapArea(0x8000, 0xffff, 0, DrvZ80Rom1 + sound2_bank);
	ZetMapArea(0x8000, 0xffff, 2, DrvZ80Rom1 + sound2_bank);
}

void __fastcall sf_sound2_out(unsigned short address, unsigned char data)
{
	switch (address & 0xff)
	{
		case 0x00:
		case 0x01:
	//		msm5205_w(address & 1, data);
		break;

		case 0x02:
			sound2_bank_w(data);
		break;
	}
}

unsigned char __fastcall sf_sound2_in(unsigned short address)
{
	switch (address & 0xff)
	{
		case 0x01:
			return soundlatch;
	}

	return 0;
}



static int DrvGfxDecode()
{
#ifdef BUILD_PSP
	extern void ui_update_progress2(float size, const char * txt);
	#define UPDATE_PROGRESS(i) ui_update_progress2(1.0 / 6, i ? NULL : "decoding graphics data..." )
#else
	#define UPDATE_PROGRESS(i)
#endif
	static int Gfx0Planes[4]   = { 4, 0, 0x200004, 0x200000 };
	static int Gfx1Planes[4]   = { 4, 0, 0x400004, 0x400000 };
	static int Gfx2Planes[4]   = { 4, 0, 0x700004, 0x700000 };
	static int Gfx3Planes[2]   = { 4, 0 };

	static int GfxXOffsets[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 256, 257, 258, 259, 264, 265, 266, 267 };
	static int GfxYOffsets[16] = { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
				       0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0 };

	unsigned char *tmp = (unsigned char*)malloc(0x1C0000);
	if (tmp == NULL) {
		return 1;
	}

	memset(tmp, 0, 0x1C0000);
	memcpy (tmp, Gfx0, 0x080000);

	GfxDecode(0x1000, 4, 16, 16, Gfx0Planes, GfxXOffsets, GfxYOffsets, 0x200, tmp, Gfx0);
	UPDATE_PROGRESS(0);

	memset(tmp, 0, 0x1C0000);
	memcpy (tmp, Gfx1, 0x100000);

	GfxDecode(0x2000, 4, 16, 16, Gfx1Planes, GfxXOffsets, GfxYOffsets, 0x200, tmp, Gfx1);
	UPDATE_PROGRESS(1);

	memset(tmp, 0, 0x1C0000);
	memcpy (tmp, Gfx2, 0x1c0000);

	GfxDecode(0x3800, 4, 16, 16, Gfx2Planes, GfxXOffsets, GfxYOffsets, 0x200, tmp, Gfx2);
	UPDATE_PROGRESS(1);

	memset(tmp, 0, 0x1C0000);
	memcpy (tmp, Gfx3, 0x004000);

	GfxDecode(0x0400, 2,  8,  8, Gfx3Planes, GfxXOffsets, GfxYOffsets, 0x080, tmp, Gfx3);
	UPDATE_PROGRESS(1);

	memset (Gfx1Trans, 1, 0x2000);
	for (int i = 0; i < 0x2000 * 0x100; i++) {
		if (Gfx1[i] != 15) Gfx1Trans[i/0x100] = 0;
	}
	UPDATE_PROGRESS(1);

	memset (Gfx3Trans, 1, 0x400);
	for (int i = 0; i < 0x400 * 0x40; i++) {
		if (Gfx3[i] != 3) Gfx3Trans[i/0x40] = 0;
	}
	UPDATE_PROGRESS(1);

	free (tmp);

	return 0;
}

static int DrvDoReset()
{
	DrvReset = 0;

	memset (AllRam, 0, RamEnd - AllRam);

	sf_fg_scroll_x = 0;
	sf_bg_scroll_x = 0;

	soundlatch = 0;
	flipscreen = 0;

	sf_active = 0;

	sound2_bank = 0;

	SekOpen(0);
	SekReset();
	SekClose();

	for (int i = 0; i < 2; i++) {
		ZetOpen(i);
		if (i) sound2_bank_w(0);
		ZetReset();
		ZetClose();

	//	if (pBurnSoundOut) MSM5205Reset(i);
	}

	BurnYM2151Reset();

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = Mem;

	Drv68kRom	= Next; Next += 0x060000;
	DrvZ80Rom0	= Next; Next += 0x008000;
	DrvZ80Rom1	= Next; Next += 0x040000;

	Gfx0		= Next; Next += 0x100000;
	Gfx1		= Next; Next += 0x200000;
	Gfx1Trans	= Next; Next += 0x002000;
	Gfx2		= Next; Next += 0x380000;
	Gfx3		= Next; Next += 0x010000;
	Gfx3Trans	= Next; Next += 0x000400;
	Gfx4		= Next; Next += 0x040000;

	DrvPalette	= (unsigned int*)Next; Next += 0x00401 * sizeof(unsigned int);

	AllRam          = Next;

	Drv68kPalRam    = Next; Next += 0x000800;
	Drv68kVidRam    = Next; Next += 0x001000;
	Drv68kRam       = Next; Next += 0x008000;

	DrvZ80Ram0	= Next; Next += 0x000800;

	Palette		= (unsigned int*)Next; Next += 0x00401 * sizeof(unsigned int);

	RamEnd          = Next;

	MemEnd		= Next;

	return 0;
}

void sfYM2151IrqHandler(int Irq)
{
	if (Irq) {
		ZetSetIRQLine(0xff, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int DrvInit(int initver)
{
	int nLen;

	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	MemIndex();

//	MSM5205ROM = DrvZ80Rom1; // ?

	if (initver == 4)
	{
		if (BurnLoadRom(Drv68kRom + 1, 0, 2)) return 1;
		if (BurnLoadRom(Drv68kRom + 0, 1, 2)) return 1;

		if (BurnLoadRom(DrvZ80Rom0, 2, 1)) return 1;
		if (BurnLoadRom(DrvZ80Rom1, 3, 1)) return 1;

		for (int i = 0; i <  4; i++) {
			if (BurnLoadRom(Gfx0 + i * 0x20000, i +  4, 1)) return 1;
		}

		for (int i = 0; i <  8; i++) {
			if (BurnLoadRom(Gfx1 + i * 0x20000, i +  8, 1)) return 1;
		}

		for (int i = 0; i < 6; i++) {
			if (BurnLoadRom(Gfx2 + 0x00000 + i * 0x10000, i + 16, 1)) return 1;
			if (BurnLoadRom(Gfx2 + 0xe0000 + i * 0x10000, i + 22, 1)) return 1;
		}

		if (BurnLoadRom(Gfx3, 28, 1)) return 1;

		for (int i = 0; i <  4; i++) {
			if (BurnLoadRom(Gfx4 + i * 0x10000, i + 29, 1)) return 1;
		}
	} else {
		for (int i = 0; i < 6; i+=2) {
			if (BurnLoadRom(Drv68kRom + 1 + i * 0x10000, i +  0, 2)) return 1;
			if (BurnLoadRom(Drv68kRom + 0 + i * 0x10000, i +  1, 2)) return 1;
		}

		if (BurnLoadRom(DrvZ80Rom0, 6, 1)) return 1;
		if (BurnLoadRom(DrvZ80Rom1 + 0x00000, 7, 1)) return 1;
		if (BurnLoadRom(DrvZ80Rom1 + 0x20000, 8, 1)) return 1;

		for (int i = 0; i <  4; i++) {
			if (BurnLoadRom(Gfx0 + i * 0x20000, i +  9, 1)) return 1;
		}

		for (int i = 0; i <  8; i++) {
			if (BurnLoadRom(Gfx1 + i * 0x20000, i + 13, 1)) return 1;
		}

		for (int i = 0; i < 14; i++) {
			if (BurnLoadRom(Gfx2 + i * 0x20000, i + 21, 1)) return 1;
		}

		if (BurnLoadRom(Gfx3, 35, 1)) return 1;

		for (int i = 0; i <  4; i++) {
			if (BurnLoadRom(Gfx4 + i * 0x10000, i + 36, 1)) return 1;
		}
	}

	if (DrvGfxDecode()) return 1;

	SekInit(0, 0x68000);
	SekOpen(0);
	SekMapMemory(Drv68kRom,		0x000000, 0x04ffff, SM_ROM);
	SekMapMemory(Drv68kVidRam,	0x800000, 0x800fff, SM_RAM);
	SekMapMemory(Drv68kRam,		0xff8000, 0xffffff, SM_RAM);
	SekSetWriteByteHandler(0, sf_write_byte);
	SekSetWriteWordHandler(0, sf_write_word);
	SekSetReadByteHandler(0, sf_read_byte);
	SekSetReadWordHandler(0, sf_read_word);
	SekClose();

	ZetInit(2);
	ZetOpen(0);
	ZetMapArea(0x0000, 0x7fff, 0, DrvZ80Rom0);
	ZetMapArea(0x0000, 0x7fff, 2, DrvZ80Rom0);
	ZetMapArea(0xc000, 0xc7ff, 0, DrvZ80Ram0);
	ZetMapArea(0xc000, 0xc7ff, 1, DrvZ80Ram0);
	ZetMapArea(0xc000, 0xc7ff, 2, DrvZ80Ram0);
	ZetSetWriteHandler(sf_sound_write);
	ZetSetReadHandler(sf_sound_read);
	ZetMemEnd();
	ZetClose();

	ZetOpen(1);
	ZetMapArea(0x0000, 0xffff, 0, DrvZ80Rom1);
	ZetMapArea(0x0000, 0xffff, 2, DrvZ80Rom1);
	ZetSetOutHandler(sf_sound2_out);
	ZetSetInHandler(sf_sound2_in);
	ZetMemEnd();
	ZetClose();

	BurnYM2151Init(3579545, 60.0);
	BurnYM2151SetIrqHandler(&sfYM2151IrqHandler);

//	MSM5205Init(0, 384000 / 132, 100, 1);
//	MSM5205Init(1, 384000 / 132, 100, 1);

	GenericTilesInit();

	DrvDoReset();

	version = initver;

	Palette[0x400] = 0xff00ff;

	return 0;
}

static int DrvExit()
{
	BurnYM2151Exit();
	SekExit();
	ZetExit();

//	MSM5205Exit(0);
//	MSM5205Exit(1);

	version = 0;

	GenericTilesExit();

	free (Mem);

	Mem = NULL;

	sf_fg_scroll_x = 0;
	sf_bg_scroll_x = 0;

	soundlatch = 0;
	flipscreen = 0;

	sf_active = 0;
	sound2_bank = 0;

	return 0;
}

static void draw_background()
{
	unsigned char *base = Gfx4 + ((sf_bg_scroll_x >> 4) << 5);

	for (int offs = 0; offs < 0x200; offs++, base += 2)
	{
		int sx = (offs & 0x1f0) - (sf_bg_scroll_x & 0x0f);
		int sy = (offs << 4) & 0xf0;

		if (sx < 49 || sx >= 448 || sy < 16 || sy > 239) continue;

		int attr  = base[0x10000];
		int color = base[0x00000];
		int code  = (base[0x10001] << 8) | base[1];

		int flipx = attr & 1;
		int flipy = attr & 2;

		if (flipscreen) {
			flipx = !flipx;
			flipy = !flipy;
			sx = ((sx + (sf_bg_scroll_x & 0x0f)) ^ 0x1f0) + (sf_bg_scroll_x & 0x0f);
			sy ^= 0xf0;
		}

		sx -= 64;
		sy -= 16;

		if (flipy) {
			if (flipx) {
				Render16x16Tile_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 0, Gfx0);
			} else {
				Render16x16Tile_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 0, Gfx0);
			}
		} else {
			if (flipx) {
				Render16x16Tile_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 0, Gfx0);
			} else {
				Render16x16Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, Gfx0);
			}
		}
	}
}

static void draw_foreground()
{
	unsigned char *base = 0x20000 + Gfx4 + ((sf_fg_scroll_x >> 4) << 5);

	for (int offs = 0; offs < 0x200; offs++, base += 2)
	{
		int sx = (offs & 0x1f0) - (sf_fg_scroll_x & 0x0f);
		int sy = (offs << 4) & 0xf0;

		if (sx < 49 || sx >= 448 || sy < 16 || sy > 239) continue;

		int code  = (base[0x10001] << 8) | base[1];
		if (Gfx1Trans[code]) continue;

		int attr  = base[0x10000];
		int color = base[0x00000];

		int flipx = attr & 1;
		int flipy = attr & 2;

		if (flipscreen) {
			flipx = !flipx;
			flipy = !flipy;
			sx = ((sx + (sf_fg_scroll_x & 0x0f)) ^ 0x1f0) + (sf_fg_scroll_x & 0x0f);
			sy ^= 0xf0;
		}

		sx -= 64;
		sy -= 16;

		if (flipy) {
			if (flipx) {
				Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0x100, Gfx1);
			} else {
				Render16x16Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0x100, Gfx1);
			}
		} else {
			if (flipx) {
				Render16x16Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0x100, Gfx1);
			} else {
				Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 15, 0x100, Gfx1);
			}
		}
	}
}

static void draw_characters()
{
	unsigned short *fg_ram = (unsigned short*)(Drv68kVidRam);

	for (int offs = 0x100 / 2; offs < 0x0f00 / 2; offs++)
	{
		int sx = (offs & 0x3f) << 3;
		int sy = (offs >> 6) << 3;

		if (sx < 64 || sx > 447) continue;

		int attr = fg_ram[offs];
		int code = attr & 0x3ff;
		if (Gfx3Trans[code]) continue;

		int color = attr >> 12;

		int flipx = attr & 0x400;
		int flipy = attr & 0x800;

		if (flipscreen) {
			flipx = !flipx;
			flipy = !flipy;
			sx ^= 0x1f8;
			sy ^= 0x0f8;
		}

		sx -= 64;
		sy -= 16;

		if (flipy) {
			if (flipx) {
				Render8x8Tile_Mask_FlipXY(pTransDraw, code, sx, sy, color, 2, 3, 0x300, Gfx3);
			} else {
				Render8x8Tile_Mask_FlipY(pTransDraw, code, sx, sy, color, 2, 3, 0x300, Gfx3);
			}
		} else {
			if (flipx) {
				Render8x8Tile_Mask_FlipX(pTransDraw, code, sx, sy, color, 2, 3, 0x300, Gfx3);
			} else {
				Render8x8Tile_Mask(pTransDraw, code, sx, sy, color, 2, 3, 0x300, Gfx3);
			}
		}
	}
}

inline static int sf_invert(int nb)
{
	static const int delta[4] = {0x00, 0x18, 0x18, 0x00};
	return nb ^ delta[(nb >> 3) & 3];
}

static void draw_sprites()
{
	unsigned short *sf_objectram = (unsigned short*)(Drv68kRam + 0x6000);

	for (int offs = 0x1000-0x20;offs >= 0;offs -= 0x20)
	{
		int c = sf_objectram[offs];
		int attr = sf_objectram[offs+1];
		int sy = sf_objectram[offs+2];
		int sx = sf_objectram[offs+3];
		int color = attr & 0x000f;
		int flipx = attr & 0x0100;
		int flipy = attr & 0x0200;

		if (attr & 0x400)	// large sprite
		{
			int c1,c2,c3,c4,t;

			if (flipscreen)
			{
				sx = 480 - sx;
				sy = 224 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			if (sy > 239 || sx < 33 || sx > 447) continue;

			sx -= 64;
			sy -= 16;

			c1 = c;
			c2 = c+1;
			c3 = c+16;
			c4 = c+17;

			if (flipx)
			{
				t = c1; c1 = c2; c2 = t;
				t = c3; c3 = c4; c4 = t;
			}
			if (flipy)
			{
				t = c1; c1 = c3; c3 = t;
				t = c2; c2 = c4; c4 = t;
			}

			if (flipy) {
				if (flipx) {
					Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, sf_invert(c1), sx     , sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, sf_invert(c2), sx + 16, sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, sf_invert(c3), sx     , sy + 16, color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, sf_invert(c4), sx + 16, sy + 16, color, 4, 15, 0x200, Gfx2);
				} else {
					Render16x16Tile_Mask_FlipY_Clip(pTransDraw, sf_invert(c1), sx     , sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipY_Clip(pTransDraw, sf_invert(c2), sx + 16, sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipY_Clip(pTransDraw, sf_invert(c3), sx     , sy + 16, color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipY_Clip(pTransDraw, sf_invert(c4), sx + 16, sy + 16, color, 4, 15, 0x200, Gfx2);
				}
			} else {
				if (flipx) {
					Render16x16Tile_Mask_FlipX_Clip(pTransDraw, sf_invert(c1), sx     , sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipX_Clip(pTransDraw, sf_invert(c2), sx + 16, sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipX_Clip(pTransDraw, sf_invert(c3), sx     , sy + 16, color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_FlipX_Clip(pTransDraw, sf_invert(c4), sx + 16, sy + 16, color, 4, 15, 0x200, Gfx2);
				} else {
					Render16x16Tile_Mask_Clip(pTransDraw, sf_invert(c1), sx     , sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_Clip(pTransDraw, sf_invert(c2), sx + 16, sy     , color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_Clip(pTransDraw, sf_invert(c3), sx     , sy + 16, color, 4, 15, 0x200, Gfx2);
					Render16x16Tile_Mask_Clip(pTransDraw, sf_invert(c4), sx + 16, sy + 16, color, 4, 15, 0x200, Gfx2);
				}
			}
		}
		else
		{
			if (flipscreen)
			{
				sx = 496 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			if (sy < 1 || sy > 239 || sx < 49 || sx > 447) continue;

			sx -= 64;
			sy -= 16;

			if (flipy) {
				if (flipx) {
					Render16x16Tile_Mask_FlipXY_Clip(pTransDraw, sf_invert(c), sx, sy, color, 4, 15, 0x200, Gfx2);
				} else {
					Render16x16Tile_Mask_FlipY_Clip(pTransDraw, sf_invert(c), sx, sy, color, 4, 15, 0x200, Gfx2);
				}
			} else {
				if (flipx) {
					Render16x16Tile_Mask_FlipX_Clip(pTransDraw, sf_invert(c), sx, sy, color, 4, 15, 0x200, Gfx2);
				} else {
					Render16x16Tile_Mask_Clip(pTransDraw, sf_invert(c), sx, sy, color, 4, 15, 0x200, Gfx2);
				}
			}
		}
	}
}


static int DrvDraw()
{
	if (DrvRecalc) {
		for (int i = 0; i < 0x401; i++) {
			int rgb = Palette[i];
			DrvPalette[i] = BurnHighCol(rgb >> 16, rgb >> 8, rgb, 0);
		}
	}

	if (nBurnLayer & 8) {
		if (sf_active & 0x20)
			draw_background();
		else
			memset (pTransDraw, 0, nScreenWidth * nScreenHeight * sizeof(short));
	} else {
		for (int i = 0; i < nScreenWidth * nScreenHeight; i++)
			pTransDraw[i] = 0x0400; // point to magenta
	}

	if (sf_active & 0x40 && nBurnLayer & 4)
		draw_foreground();

	if (sf_active & 0x80 && nBurnLayer & 1)
		draw_sprites();

	if (sf_active & 0x08 && nBurnLayer & 2)
		draw_characters();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static void DrvMakeInputs()
{
	DrvInputs[0] = (DrvDips[0] << 8) | DrvDips[1];
	DrvInputs[1] = (DrvDips[2] << 4) | DrvDips[3] | 0xf800;

	DrvInputs[2] = 0xff7f;
	DrvInputs[3] = 0xffff;
	DrvInputs[4] = 0xffff;
	DrvInputs[5] = 0xffff;
	DrvInputs[6] = 0xffff;

	for (int i = 0; i < 16; i++) {
		DrvInputs[2] ^= (DrvJoy1[i] & 1) << i;
		DrvInputs[3] ^= (DrvJoy2[i] & 1) << i;
		DrvInputs[4] ^= (DrvJoy3[i] & 1) << i;
		DrvInputs[5] ^= (DrvJoy4[i] & 1) << i;
	}

	// clear opposites
	{
		if (!(DrvInputs[4] & 0x0003)) DrvInputs[4] |= 0x0003; // p1 left/right
		if (!(DrvInputs[4] & 0x000c)) DrvInputs[4] |= 0x000c; // p1 up/down
		if (!(DrvInputs[4] & 0x0300)) DrvInputs[4] |= 0x0300; // p2 left/right
		if (!(DrvInputs[4] & 0x0c00)) DrvInputs[4] |= 0x0c00; // p2 up/down
	}

	// World & prototype -- simulate analogue buttons
	if (version == 1 || version == 4)
	{
		static const int scale[8] = { 0x00, 0x60, 0xe0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe };

		int t = (DrvJoy4[0] & 1) | ((DrvJoy4[1] & 1) << 1) | ((DrvJoy4[2] & 1) << 2);
			DrvInputs[5] = scale[t];
		    t = (DrvJoy5[0] & 1) | ((DrvJoy5[1] & 1) << 1) | ((DrvJoy5[2] & 1) << 2);
			DrvInputs[5] |= scale[t] << 8;

		    t = (DrvJoy4[8] & 1) | ((DrvJoy4[9] & 1) << 1) | ((DrvJoy4[10] & 1) << 2);
			DrvInputs[6]  = scale[t];

		    t = (DrvJoy5[8] & 1) | ((DrvJoy5[9] & 1) << 1) | ((DrvJoy5[10] & 1) << 2);
			DrvInputs[6] |= scale[t] << 8;
	}
}

static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	DrvMakeInputs();

	int nSoundBufferPos = 0;
	int nInterleave = 132; // for sound cpu 2 irq (actually ~132.5)
	int nCycles[3];

//	nCycles[0] = (int)((long long)8000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
//	nCycles[1] = (int)((long long)3579545 * nBurnCPUSpeedAdjust / (0x0100 * 60));
//	nCycles[2] = (int)((long long)3579545 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCycles[0] = (int)((long long)8000000 / 60);
	nCycles[1] = (int)((long long)3579545 / 60);
	nCycles[2] = (int)((long long)3579545 / 60);

	SekOpen(0);
	for (int i = 0; i < nInterleave; i++) {
		int nCyclesSegment;

		nCyclesSegment = nCycles[0] / (nInterleave - i);
		nCycles[0] -= SekRun(nCyclesSegment);

		ZetOpen(0);
		nCyclesSegment = nCycles[1] / (nInterleave - i);
		nCycles[1] -= ZetRun(nCyclesSegment);

		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen / nInterleave;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
		//	MSM5205Render(0,pSoundBuf, nSegmentLength);
		//	MSM5205Render(1,pSoundBuf, nSegmentLength);
		}

		ZetClose();

		ZetOpen(1);
		nCyclesSegment = nCycles[2] / (nInterleave - i);
		nCycles[2] -= ZetRun(nCyclesSegment);
		ZetRaiseIrq(0);
		ZetClose();
	}

	SekSetIRQLine((version == 4) ? 6 : 1, SEK_IRQSTATUS_AUTO);
	SekClose();

	if (pBurnSoundOut) {
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);

		if (nSegmentLength) {
			ZetOpen(0);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			ZetClose();
		//	MSM5205Render(0,pSoundBuf, nSegmentLength);
		//	MSM5205Render(1,pSoundBuf, nSegmentLength);
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
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd - AllRam;
		ba.szName = "All RAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SekScan(nAction);
		ZetScan(nAction);

	//	MSM5205Scan(0, nAction);
		BurnYM2151Scan(nAction);

		SCAN_VAR(sf_fg_scroll_x);
		SCAN_VAR(sf_bg_scroll_x);
		SCAN_VAR(soundlatch);		
		SCAN_VAR(flipscreen);
		SCAN_VAR(sf_active);
		SCAN_VAR(sound2_bank);
	}

	if (nAction & ACB_WRITE) {
		ZetOpen(1);
		sound2_bank_w(sound2_bank);
		ZetClose();
	}

	return 0;
}


// Street Fighter (World)

static struct BurnRomInfo sfRomDesc[] = {
	{ "sfe-19",     0x10000, 0x8346c3ca, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "sfe-22",     0x10000, 0x3a4bfaa8, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "sfe-20",     0x10000, 0xb40e67ee, 1 | BRF_PRG | BRF_ESS }, //  2
	{ "sfe-23",     0x10000, 0x477c3d5b, 1 | BRF_PRG | BRF_ESS }, //  3
	{ "sfe-21",     0x10000, 0x2547192b, 1 | BRF_PRG | BRF_ESS }, //  4
	{ "sfe-24",     0x10000, 0x79680f4e, 1 | BRF_PRG | BRF_ESS }, //  5

	{ "sf-02.bin",  0x08000, 0x4a9ac534, 2 | BRF_PRG | BRF_ESS }, //  6 Z80 #0 Code

	{ "sfu-00",     0x20000, 0xa7cce903, 3 | BRF_PRG | BRF_ESS }, //  7 Z80 #1 Code
	{ "sf-01.bin",  0x20000, 0x86e0f0d5, 3 | BRF_PRG | BRF_ESS }, //  8

	{ "sf-39.bin",  0x20000, 0xcee3d292, 4 | BRF_GRA },           //  9 Background Tiles
	{ "sf-38.bin",  0x20000, 0x2ea99676, 4 | BRF_GRA },           // 10
	{ "sf-41.bin",  0x20000, 0xe0280495, 4 | BRF_GRA },           // 11
	{ "sf-40.bin",  0x20000, 0xc70b30de, 4 | BRF_GRA },           // 12

	{ "sf-25.bin",  0x20000, 0x7f23042e, 5 | BRF_GRA },           // 13 More tiles?
	{ "sf-28.bin",  0x20000, 0x92f8b91c, 5 | BRF_GRA },           // 14
	{ "sf-30.bin",  0x20000, 0xb1399856, 5 | BRF_GRA },           // 15
	{ "sf-34.bin",  0x20000, 0x96b6ae2e, 5 | BRF_GRA },           // 16
	{ "sf-26.bin",  0x20000, 0x54ede9f5, 5 | BRF_GRA },           // 17
	{ "sf-29.bin",  0x20000, 0xf0649a67, 5 | BRF_GRA },           // 18
	{ "sf-31.bin",  0x20000, 0x8f4dd71a, 5 | BRF_GRA },           // 19
	{ "sf-35.bin",  0x20000, 0x70c00fb4, 5 | BRF_GRA },           // 20

	{ "sf-15.bin",  0x20000, 0xfc0113db, 6 | BRF_GRA },           // 21 Sprites
	{ "sf-16.bin",  0x20000, 0x82e4a6d3, 6 | BRF_GRA },           // 22
	{ "sf-11.bin",  0x20000, 0xe112df1b, 6 | BRF_GRA },           // 23
	{ "sf-12.bin",  0x20000, 0x42d52299, 6 | BRF_GRA },           // 24
	{ "sf-07.bin",  0x20000, 0x49f340d9, 6 | BRF_GRA },           // 25
	{ "sf-08.bin",  0x20000, 0x95ece9b1, 6 | BRF_GRA },           // 26
	{ "sf-03.bin",  0x20000, 0x5ca05781, 6 | BRF_GRA },           // 27
	{ "sf-17.bin",  0x20000, 0x69fac48e, 6 | BRF_GRA },           // 28
	{ "sf-18.bin",  0x20000, 0x71cfd18d, 6 | BRF_GRA },           // 29
	{ "sf-13.bin",  0x20000, 0xfa2eb24b, 6 | BRF_GRA },           // 30
	{ "sf-14.bin",  0x20000, 0xad955c95, 6 | BRF_GRA },           // 31
	{ "sf-09.bin",  0x20000, 0x41b73a31, 6 | BRF_GRA },           // 32
	{ "sf-10.bin",  0x20000, 0x91c41c50, 6 | BRF_GRA },           // 33
	{ "sf-05.bin",  0x20000, 0x538c7cbe, 6 | BRF_GRA },           // 34

	{ "sf-27.bin",  0x04000, 0x2b09b36d, 7 | BRF_GRA },           // 35 Characters

	{ "sf-37.bin",  0x10000, 0x23d09d3d, 8 | BRF_GRA },           // 36 Tilemaps
	{ "sf-36.bin",  0x10000, 0xea16df6c, 8 | BRF_GRA },           // 37
	{ "sf-32.bin",  0x10000, 0x72df2bd9, 8 | BRF_GRA },           // 38
	{ "sf-33.bin",  0x10000, 0x3e99d3d5, 8 | BRF_GRA },           // 39

	{ "mb7114h.12k", 0x0100, 0x75af3553, 0 | BRF_OPT },           // 40 Proms
	{ "mb7114h.11h", 0x0100, 0xc0e56586, 0 | BRF_OPT },           // 41
	{ "mb7114h.12j", 0x0100, 0x4c734b64, 0 | BRF_OPT },           // 42
	{ "mmi-7603.13h",0x0020, 0x06bcda53, 0 | BRF_OPT },           // 43
};

STD_ROM_PICK(sf);
STD_ROM_FN(sf);

static int SfInit()
{
	return DrvInit(1);
}

struct BurnDriver BurnDrvsf = {
	"sf", NULL, NULL, "1987",
	"Street Fighter (World)\0", NULL, "Capcom", "Misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_VSFIGHT, FBF_SF,
	NULL, sfRomInfo, sfRomName, SfInputInfo, SfDIPInfo,
	SfInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, 
	&DrvRecalc, 384, 224, 4, 3
};



// Street Fighter (US)

static struct BurnRomInfo sfusRomDesc[] = {
	{ "sfd-19",     0x10000, 0xfaaf6255, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "sfd-22",     0x10000, 0xe1fe3519, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "sfd-20",     0x10000, 0x44b915bd, 1 | BRF_PRG | BRF_ESS }, //  2
	{ "sfd-23",     0x10000, 0x79c43ff8, 1 | BRF_PRG | BRF_ESS }, //  3
	{ "sfd-21",     0x10000, 0xe8db799b, 1 | BRF_PRG | BRF_ESS }, //  4
	{ "sfd-24",     0x10000, 0x466a3440, 1 | BRF_PRG | BRF_ESS }, //  5

	{ "sf-02.bin",  0x08000, 0x4a9ac534, 2 | BRF_PRG | BRF_ESS }, //  6 Z80 #0 Code

	{ "sfu-00",     0x20000, 0xa7cce903, 3 | BRF_PRG | BRF_ESS }, //  7 Z80 #1 Code
	{ "sf-01.bin",  0x20000, 0x86e0f0d5, 3 | BRF_PRG | BRF_ESS }, //  8

	{ "sf-39.bin",  0x20000, 0xcee3d292, 4 | BRF_GRA },           //  9 Background Tiles
	{ "sf-38.bin",  0x20000, 0x2ea99676, 4 | BRF_GRA },           // 10
	{ "sf-41.bin",  0x20000, 0xe0280495, 4 | BRF_GRA },           // 11
	{ "sf-40.bin",  0x20000, 0xc70b30de, 4 | BRF_GRA },           // 12

	{ "sf-25.bin",  0x20000, 0x7f23042e, 5 | BRF_GRA },           // 13 Foreground Tiles
	{ "sf-28.bin",  0x20000, 0x92f8b91c, 5 | BRF_GRA },           // 14
	{ "sf-30.bin",  0x20000, 0xb1399856, 5 | BRF_GRA },           // 15
	{ "sf-34.bin",  0x20000, 0x96b6ae2e, 5 | BRF_GRA },           // 16
	{ "sf-26.bin",  0x20000, 0x54ede9f5, 5 | BRF_GRA },           // 17
	{ "sf-29.bin",  0x20000, 0xf0649a67, 5 | BRF_GRA },           // 18
	{ "sf-31.bin",  0x20000, 0x8f4dd71a, 5 | BRF_GRA },           // 19
	{ "sf-35.bin",  0x20000, 0x70c00fb4, 5 | BRF_GRA },           // 20

	{ "sf-15.bin",  0x20000, 0xfc0113db, 6 | BRF_GRA },           // 21 Sprites
	{ "sf-16.bin",  0x20000, 0x82e4a6d3, 6 | BRF_GRA },           // 22
	{ "sf-11.bin",  0x20000, 0xe112df1b, 6 | BRF_GRA },           // 23
	{ "sf-12.bin",  0x20000, 0x42d52299, 6 | BRF_GRA },           // 24
	{ "sf-07.bin",  0x20000, 0x49f340d9, 6 | BRF_GRA },           // 25
	{ "sf-08.bin",  0x20000, 0x95ece9b1, 6 | BRF_GRA },           // 26
	{ "sf-03.bin",  0x20000, 0x5ca05781, 6 | BRF_GRA },           // 27
	{ "sf-17.bin",  0x20000, 0x69fac48e, 6 | BRF_GRA },           // 28
	{ "sf-18.bin",  0x20000, 0x71cfd18d, 6 | BRF_GRA },           // 29
	{ "sf-13.bin",  0x20000, 0xfa2eb24b, 6 | BRF_GRA },           // 30
	{ "sf-14.bin",  0x20000, 0xad955c95, 6 | BRF_GRA },           // 31
	{ "sf-09.bin",  0x20000, 0x41b73a31, 6 | BRF_GRA },           // 32
	{ "sf-10.bin",  0x20000, 0x91c41c50, 6 | BRF_GRA },           // 33
	{ "sf-05.bin",  0x20000, 0x538c7cbe, 6 | BRF_GRA },           // 34

	{ "sf-27.bin",  0x04000, 0x2b09b36d, 7 | BRF_GRA },           // 35 Characters

	{ "sf-37.bin",  0x10000, 0x23d09d3d, 8 | BRF_GRA },           // 36 Tilemaps
	{ "sf-36.bin",  0x10000, 0xea16df6c, 8 | BRF_GRA },           // 37
	{ "sf-32.bin",  0x10000, 0x72df2bd9, 8 | BRF_GRA },           // 38
	{ "sf-33.bin",  0x10000, 0x3e99d3d5, 8 | BRF_GRA },           // 39

	{ "mb7114h.12k", 0x0100, 0x75af3553, 0 | BRF_OPT },           // 40 Proms
	{ "mb7114h.11h", 0x0100, 0xc0e56586, 0 | BRF_OPT },           // 41
	{ "mb7114h.12j", 0x0100, 0x4c734b64, 0 | BRF_OPT },           // 42
	{ "mmi-7603.13h",0x0020, 0x06bcda53, 0 | BRF_OPT },           // 43
};

STD_ROM_PICK(sfus);
STD_ROM_FN(sfus);

static int SfusInit()
{
	return DrvInit(2);
}

struct BurnDriver BurnDrvsfus = {
	"sfus", "sf", NULL, "1987",
	"Street Fighter (US)\0", NULL, "Capcom", "Misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_VSFIGHT, FBF_SF,
	NULL, sfusRomInfo, sfusRomName, SfusInputInfo, SfusDIPInfo,
	SfusInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, 
	&DrvRecalc, 384, 224, 4, 3
};


// Street Fighter (Japan)

static struct BurnRomInfo sfjpRomDesc[] = {
	{ "sf-19.bin",  0x10000, 0x116027d7, 1 | BRF_PRG | BRF_ESS }, //  0 68k Code
	{ "sf-22.bin",  0x10000, 0xd3cbd09e, 1 | BRF_PRG | BRF_ESS }, //  1
	{ "sf-20.bin",  0x10000, 0xfe07e83f, 1 | BRF_PRG | BRF_ESS }, //  2
	{ "sf-23.bin",  0x10000, 0x1e435d33, 1 | BRF_PRG | BRF_ESS }, //  3
	{ "sf-21.bin",  0x10000, 0xe086bc4c, 1 | BRF_PRG | BRF_ESS }, //  4
	{ "sf-24.bin",  0x10000, 0x13a6696b, 1 | BRF_PRG | BRF_ESS }, //  5

	{ "sf-02.bin",  0x08000, 0x4a9ac534, 2 | BRF_PRG | BRF_ESS }, //  6 Z80 #0 Code

	{ "sf-00.bin",  0x20000, 0x4b733845, 3 | BRF_PRG | BRF_ESS }, //  7 Z80 #1 Code
	{ "sf-01.bin",  0x20000, 0x86e0f0d5, 3 | BRF_PRG | BRF_ESS }, //  8

	{ "sf-39.bin",  0x20000, 0xcee3d292, 4 | BRF_GRA },           //  9 Background Tiles
	{ "sf-38.bin",  0x20000, 0x2ea99676, 4 | BRF_GRA },           // 10
	{ "sf-41.bin",  0x20000, 0xe0280495, 4 | BRF_GRA },           // 11
	{ "sf-40.bin",  0x20000, 0xc70b30de, 4 | BRF_GRA },           // 12

	{ "sf-25.bin",  0x20000, 0x7f23042e, 5 | BRF_GRA },           // 13 Foreground Tiles
	{ "sf-28.bin",  0x20000, 0x92f8b91c, 5 | BRF_GRA },           // 14
	{ "sf-30.bin",  0x20000, 0xb1399856, 5 | BRF_GRA },           // 15
	{ "sf-34.bin",  0x20000, 0x96b6ae2e, 5 | BRF_GRA },           // 16
	{ "sf-26.bin",  0x20000, 0x54ede9f5, 5 | BRF_GRA },           // 17
	{ "sf-29.bin",  0x20000, 0xf0649a67, 5 | BRF_GRA },           // 18
	{ "sf-31.bin",  0x20000, 0x8f4dd71a, 5 | BRF_GRA },           // 19
	{ "sf-35.bin",  0x20000, 0x70c00fb4, 5 | BRF_GRA },           // 20

	{ "sf-15.bin",  0x20000, 0xfc0113db, 6 | BRF_GRA },           // 21 Sprites
	{ "sf-16.bin",  0x20000, 0x82e4a6d3, 6 | BRF_GRA },           // 22
	{ "sf-11.bin",  0x20000, 0xe112df1b, 6 | BRF_GRA },           // 23
	{ "sf-12.bin",  0x20000, 0x42d52299, 6 | BRF_GRA },           // 24
	{ "sf-07.bin",  0x20000, 0x49f340d9, 6 | BRF_GRA },           // 25
	{ "sf-08.bin",  0x20000, 0x95ece9b1, 6 | BRF_GRA },           // 26
	{ "sf-03.bin",  0x20000, 0x5ca05781, 6 | BRF_GRA },           // 27
	{ "sf-17.bin",  0x20000, 0x69fac48e, 6 | BRF_GRA },           // 28
	{ "sf-18.bin",  0x20000, 0x71cfd18d, 6 | BRF_GRA },           // 29
	{ "sf-13.bin",  0x20000, 0xfa2eb24b, 6 | BRF_GRA },           // 30
	{ "sf-14.bin",  0x20000, 0xad955c95, 6 | BRF_GRA },           // 31
	{ "sf-09.bin",  0x20000, 0x41b73a31, 6 | BRF_GRA },           // 32
	{ "sf-10.bin",  0x20000, 0x91c41c50, 6 | BRF_GRA },           // 33
	{ "sf-05.bin",  0x20000, 0x538c7cbe, 6 | BRF_GRA },           // 34

	{ "sf-27.bin",  0x04000, 0x2b09b36d, 7 | BRF_GRA },           // 35 Characters

	{ "sf-37.bin",  0x10000, 0x23d09d3d, 8 | BRF_GRA },           // 36 Tilemaps
	{ "sf-36.bin",  0x10000, 0xea16df6c, 8 | BRF_GRA },           // 37
	{ "sf-32.bin",  0x10000, 0x72df2bd9, 8 | BRF_GRA },           // 38
	{ "sf-33.bin",  0x10000, 0x3e99d3d5, 8 | BRF_GRA },           // 39

	{ "sfb05.bin",   0x0100, 0x864199ad, 0 | BRF_OPT },           // 40 Proms
	{ "sfb00.bin",   0x0100, 0xbd3f8c5d, 0 | BRF_OPT },           // 41
	{ "mb7114h.12j", 0x0100, 0x4c734b64, 0 | BRF_OPT },           // 42
	{ "mmi-7603.13h",0x0020, 0x06bcda53, 0 | BRF_OPT },           // 43
};

STD_ROM_PICK(sfjp);
STD_ROM_FN(sfjp);

static int SfjpInit()
{
	return DrvInit(3);
}

struct BurnDriver BurnDrvsfjp = {
	"sfjp", "sf", NULL, "1987",
	"Street Fighter (Japan)\0", NULL, "Capcom", "Misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_VSFIGHT, FBF_SF,
	NULL, sfjpRomInfo, sfjpRomName, SfjpInputInfo, SfusDIPInfo,
	SfjpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, 
	&DrvRecalc, 384, 224, 4, 3
};


// Street Fighter (Prototype)

static struct BurnRomInfo sfpRomDesc[] = {
	{ "prg8.2a",     0x20000, 0xd48d06a3, 1 | BRF_PRG | BRF_ESS }, //  0 68K Code
	{ "prg0.2c",     0x20000, 0xe8606c1a, 1 | BRF_PRG | BRF_ESS }, //  1

	{ "sound.9j",    0x08000, 0x43cd32ae, 2 | BRF_PRG | BRF_ESS }, //  2 Z80 #0 Code

	{ "voice.1g",    0x10000, 0x3f23c180, 3 | BRF_PRG | BRF_ESS }, //  3 Z80 #1 Code

	{ "bkchr.2k",    0x20000, 0xe4d47aca, 4 | BRF_GRA },           //  4 Background Tiles
	{ "bkchr.1k",    0x20000, 0x5a1cbc1b, 4 | BRF_GRA },           //  5
	{ "bkchr.4k",    0x20000, 0xc351bd48, 4 | BRF_GRA },           //  6
	{ "bkchr.3k",    0x20000, 0x6bb2b050, 4 | BRF_GRA },           //  7

	{ "mchr.1d",     0x20000, 0xab06a60b, 5 | BRF_GRA },           //  8 Foreground Tiles
	{ "mchr.1e",     0x20000, 0xd221387d, 5 | BRF_GRA },           //  9
	{ "mchr.1g",     0x20000, 0x1e4c1712, 5 | BRF_GRA },           // 10
	{ "mchr.1h",     0x20000, 0xa381f529, 5 | BRF_GRA },           // 11
	{ "mchr.2d",     0x20000, 0xe52303c4, 5 | BRF_GRA },           // 12
	{ "mchr.2e",     0x20000, 0x23b9a6a1, 5 | BRF_GRA },           // 13
	{ "mchr.2g",     0x20000, 0x1283ac09, 5 | BRF_GRA },           // 14
	{ "mchr.2h",     0x20000, 0xcc6bf05c, 5 | BRF_GRA },           // 15

	{ "b1m.bin",     0x10000, 0x64758232, 6 | BRF_GRA },           // 16 Sprites
	{ "b2m.bin",     0x10000, 0xd958f5ad, 6 | BRF_GRA },           // 17
	{ "b1k.bin",     0x10000, 0xe766f5fe, 6 | BRF_GRA },           // 18
	{ "b2k.bin",     0x10000, 0xe71572d3, 6 | BRF_GRA },           // 19
	{ "b1h.bin",     0x10000, 0x8494f38c, 6 | BRF_GRA },           // 20
	{ "b2h.bin",     0x10000, 0x1fc5f049, 6 | BRF_GRA },           // 21
	{ "b3m.bin",     0x10000, 0xd136802e, 6 | BRF_GRA },           // 22
	{ "b4m.bin",     0x10000, 0xb4fa85d3, 6 | BRF_GRA },           // 23
	{ "b3k.bin",     0x10000, 0x40e11cc8, 6 | BRF_GRA },           // 24
	{ "b4k.bin",     0x10000, 0x5ca9716e, 6 | BRF_GRA },           // 25
	{ "b3h.bin",     0x10000, 0x8c3d9173, 6 | BRF_GRA },           // 26
	{ "b4h.bin",     0x10000, 0xa2df66f8, 6 | BRF_GRA },           // 27

	{ "vram.4d",     0x04000, 0xbfadfb32, 7 | BRF_GRA },           // 28 Characters

	{ "bks1j10.5h",  0x10000, 0x4934aacd, 8 | BRF_GRA },           // 29 Tilemaps
	{ "bks1j18.3h",  0x10000, 0x551ffc88, 8 | BRF_GRA },           // 30
	{ "ms1j10.3g",   0x10000, 0xf92958b8, 8 | BRF_GRA },           // 31
	{ "ms1j18.5g",   0x10000, 0x89e35dc1, 8 | BRF_GRA },           // 32

	{ "sfb05.bin",    0x0100, 0x864199ad, 0 | BRF_OPT },           // 33 Proms
	{ "sfb00.bin",    0x0100, 0xbd3f8c5d, 0 | BRF_OPT },           // 34
	{ "mb7114h.12j",  0x0100, 0x4c734b64, 0 | BRF_OPT },           // 35
	{ "mmi-7603.13h", 0x0020, 0x06bcda53, 0 | BRF_OPT },           // 36
};

STD_ROM_PICK(sfp);
STD_ROM_FN(sfp);

static int SfpInit()
{
	return DrvInit(4);
}

struct BurnDriver BurnDrvsfp = {
	"sfp", "sf", NULL, "1987",
	"Street Fighter (Prototype)\0", NULL, "Capcom", "Misc",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_PROTOTYPE, 2, HARDWARE_MISC_PRE90S, GBF_VSFIGHT, FBF_SF,
	NULL, sfpRomInfo, sfpRomName, SfInputInfo, SfDIPInfo,
	SfpInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, 
	&DrvRecalc, 384, 224, 4, 3
};

