// ------------------------------------------------------------------------------------
// Capcom Play System III Drivers for FB Alpha (2007 - 2008).
// ------------------------------------------------------------------------------------
//
//	v1	[ OopsWare ]
//     - Original drivers release.
//
//	v2  [ CaptainCPS-X ]
//     - Verified drivers.
//     - Updated DIPs.
//     - Updated some Inits.
//     - Added some Comments.
//
//  v3 [ BisonSAS ]
//     - Added default game regions DIPs.
//     - Added unicode titles for "jojo" and "jojoba".
//     - Changed the redeartn BIOS to "warzard_euro.29f400.u2".
//     - Added "HARDWARE_CAPCOM_CPS3_NO_CD" flag for NOCD sets.
//
//  v4 [ CaptainCPS-X ]
//     - Updated comments & organized structures of code.
//     - Revised code for compatibility with FB Alpha Enhanced.
//
//	More info: http://neosource.1emu.net/forums/index.php
//
// ------------------------------------------------------------------------------------

#include "cps3.h"

static struct BurnInputInfo cps3InputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	Cps3But2 +  8,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	Cps3But2 + 12,	"p1 start"	},

	{"P1 Up",			BIT_DIGITAL,	Cps3But1 +  0,	"p1 up"		},
	{"P1 Down",			BIT_DIGITAL,	Cps3But1 +  1,	"p1 down"	},
	{"P1 Left",			BIT_DIGITAL,	Cps3But1 +  2,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	Cps3But1 +  3,	"p1 right"	},
	{"P1 Weak Punch",	BIT_DIGITAL,	Cps3But1 +  4,	"p1 fire 1"	},
	{"P1 Medium Punch",	BIT_DIGITAL,	Cps3But1 +  5,	"p1 fire 2"	},
	{"P1 Strong Punch",	BIT_DIGITAL,	Cps3But1 +  6,	"p1 fire 3"	},
	{"P1 Weak Kick",	BIT_DIGITAL,	Cps3But3 +  3,	"p1 fire 4"	},
	{"P1 Medium Kick",	BIT_DIGITAL,	Cps3But3 +  2,	"p1 fire 5"	},
	{"P1 Strong Kick",	BIT_DIGITAL,	Cps3But3 +  1,	"p1 fire 6"	},

	{"P2 Coin",			BIT_DIGITAL,	Cps3But2 +  9,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	Cps3But2 + 13,	"p2 start"	},

	{"P2 Up",			BIT_DIGITAL,	Cps3But1 +  8,	"p2 up"		},
	{"P2 Down",			BIT_DIGITAL,	Cps3But1 +  9,	"p2 down"	},
	{"P2 Left",			BIT_DIGITAL,	Cps3But1 + 10,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	Cps3But1 + 11,	"p2 right"	},
	{"P2 Weak Punch",	BIT_DIGITAL,	Cps3But1 + 12,	"p2 fire 1"	},
	{"P2 Medium Punch",	BIT_DIGITAL,	Cps3But1 + 13,	"p2 fire 2"	},
	{"P2 Strong Punch",	BIT_DIGITAL,	Cps3But1 + 14,	"p2 fire 3"	},
	{"P2 Weak Kick",	BIT_DIGITAL,	Cps3But3 +  4,	"p2 fire 4"	},
	{"P2 Medium Kick",	BIT_DIGITAL,	Cps3But3 +  5,	"p2 fire 5"	},
	{"P2 Strong Kick",	BIT_DIGITAL,	Cps3But2 + 10,	"p2 fire 6"	},

	{"Reset",			BIT_DIGITAL,	&cps3_reset,	"reset"		},
	{"Diagnostic",		BIT_DIGITAL,	Cps3But2 +  1,	"diag"		},
	{"Service",			BIT_DIGITAL,	Cps3But2 +  0,	"service"	},
	{"Region",			BIT_DIPSWITCH,	&cps3_dip,		"dip"		},
};

STDINPUTINFO(cps3);

// ------------------------------------------------------------------------------------

static struct BurnDIPInfo regionDIPList[] = {

	// Region
	{0,		0xFD, 0,	7,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x08, "Asia"},
	{0x1B,	0x01, 0x0F,	0x00, "XXXXXX"},

//	{0,		0xFE, 0,	2,		"NO CD"},
//	{0x1B,	0x01, 0x10, 0x00,	"No"},
//	{0x1B,	0x01, 0x10, 0x10,	"Yes"},
};

STDDIPINFO(region);

static struct BurnDIPInfo jojoDIPList[] = {

	// Region
	{0,		0xFD, 0,	7,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x00, "XXXXXX"},
	
	{0,		0xFD, 0,	8,	  "Version"},
	{0x1B,	0x01, 0x70,	0x00, "Normal"},
	{0x1B,	0x01, 0x70,	0x10, "Character Check Version"},
	{0x1B,	0x01, 0x70,	0x20, "Publicity Version"},
	{0x1B,	0x01, 0x70,	0x30, "Location Test Version"},
	{0x1B,	0x01, 0x70,	0x40, "Show Version"},
	{0x1B,	0x01, 0x70,	0x50, "Debug Version"},
	{0x1B,	0x01, 0x70,	0x60, "Sample Version"},
	{0x1B,	0x01, 0x70,	0x70, "Development Version"},
//	{0x1B,	0x01, 0xF0,	0x80, "Inspection Version"},
};

STDDIPINFO(jojo);

static struct BurnDIPInfo jojobaDIPList[] = {

	// Region
	{0,		0xFD, 0,	8,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x08, "Korea"}, // fake region?
	{0x1B,	0x01, 0x0F,	0x00, "XXXXXX"},
	
	{0,		0xFD, 0,	8,	  "Version"},
	{0x1B,	0x01, 0x70,	0x00, "Normal"},
	{0x1B,	0x01, 0x70,	0x10, "Character Check Version"},
	{0x1B,	0x01, 0x70,	0x20, "Publicity Version"},
	{0x1B,	0x01, 0x70,	0x30, "Location Test Version"},
	{0x1B,	0x01, 0x70,	0x40, "Show Version"},
	{0x1B,	0x01, 0x70,	0x50, "Debug Version"},
	{0x1B,	0x01, 0x70,	0x60, "Sample Version"},
	{0x1B,	0x01, 0x70,	0x70, "Development Version"},
//	{0x1B,	0x01, 0xF0,	0x80, "Inspection Version"},
};

STDDIPINFO(jojoba);

static struct BurnDIPInfo redearthDIPList[] = {

	// Region
	{0,		0xFD, 0,	7,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x08, "Asia"},
	{0x1B,	0x01, 0x0F,	0x00, "Japan"},

	{0,		0xFD, 0,	5,	  "Version"},
	{0x1B,	0x01, 0x70,	0x00, "Normal"},
	{0x1B,	0x01, 0x70,	0x10, "Character Check Version"},
	{0x1B,	0x01, 0x70,	0x20, "Publicity Version"},
	{0x1B,	0x01, 0x70,	0x30, "Location Test Version"},
	{0x1B,	0x01, 0x70,	0x40, "Show Version"},
	{0x1B,	0x01, 0x70,	0x50, "unknown-1"},
	{0x1B,	0x01, 0x70,	0x60, "unknown-2"},
};

STDDIPINFO(redearth);

static struct BurnDIPInfo redeartnDIPList[] = {

	// Region
	{0,		0xFD, 0,	7,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x08, "Asia"},
	{0x1B,	0x01, 0x0F,	0x00, "Japan"},

	{0,		0xFD, 0,	5,	  "Version"},
	{0x1B,	0x01, 0x70,	0x60, "Normal"},
	{0x1B,	0x01, 0x70,	0x10, "Character Check Version"},
	{0x1B,	0x01, 0x70,	0x20, "Publicity Version"},
	{0x1B,	0x01, 0x70,	0x30, "Location Test Version"},
	{0x1B,	0x01, 0x70,	0x40, "Show Version"},
	{0x1B,	0x01, 0x70,	0x50, "unknown"},
};

STDDIPINFO(redeartn);

static struct BurnDIPInfo sfiiiDIPList[] = {

	// Region
	{0,		0xFD, 0,	7,	  "Region"},
	{0x1B,	0x01, 0x0F,	0x01, "Japan"},
	{0x1B,	0x01, 0x0F,	0x02, "Asia"},
	{0x1B,	0x01, 0x0F,	0x03, "Euro"},
	{0x1B,	0x01, 0x0F,	0x04, "USA"},
	{0x1B,	0x01, 0x0F,	0x05, "Hispanic"},
	{0x1B,	0x01, 0x0F,	0x06, "Brazil"},
	{0x1B,	0x01, 0x0F,	0x07, "Oceania"},
	{0x1B,	0x01, 0x0F,	0x08, "Asia"},
	{0x1B,	0x01, 0x0F,	0x00, "XXXXXX"},
	
	{0,		0xFD, 0,	2,	  "Fake Widescreen DIP"},
	{0x1B,	0x01, 0x80,	0x80, "Widescreen"},
	{0x1B,	0x01, 0x80,	0x00, "Normal"},
};

STDDIPINFO(sfiii);

static struct BurnDIPInfo japanRegionDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x01, NULL},
};

static struct BurnDIPInfo asiaRegionDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x02, NULL},
};

static struct BurnDIPInfo euroRegionDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x03, NULL},
};

static struct BurnDIPInfo usaRegionDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x04, NULL},
};

static struct BurnDIPInfo euroRedeartnDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x63, NULL},
};

static struct BurnDIPInfo euroRedearthDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x03, NULL},
};

static struct BurnDIPInfo japanWarzardDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x01, NULL},
};

static struct BurnDIPInfo japanJojoDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x01, NULL},
};

static struct BurnDIPInfo asiaJojoDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x02, NULL},
};

static struct BurnDIPInfo japanJojobaDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x01, NULL},
};

static struct BurnDIPInfo euroJojobaDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x03, NULL},
};

static struct BurnDIPInfo japanSfiiiDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x01, NULL},
};

static struct BurnDIPInfo asiaSfiiiDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x02, NULL},
};

static struct BurnDIPInfo usaSfiiiDIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x04, NULL},
};

static struct BurnDIPInfo japanSfiii3DIPList[] = {

	// Defaults
	{0x1B,	0xFF, 0xFF,	0x11, NULL},
};

STDDIPINFOEXT(japan, region, japanRegion);
STDDIPINFOEXT(asia,  region, asiaRegion);
STDDIPINFOEXT(euro,  region, euroRegion);
STDDIPINFOEXT(usa,   region, usaRegion);
STDDIPINFOEXT(jojoJapan, jojo, japanJojo);
STDDIPINFOEXT(jojoAsia,  jojo, asiaJojo);
STDDIPINFOEXT(jojobaJapan, jojo, japanJojoba);
STDDIPINFOEXT(jojobaEuro,  jojo, euroJojoba);
STDDIPINFOEXT(redeartnEuro, redeartn, euroRedeartn);
STDDIPINFOEXT(redearthEuro, redearth, euroRedearth);
STDDIPINFOEXT(warzardJapan, redearth, japanWarzard);
STDDIPINFOEXT(sfiiijapan, sfiii, japanSfiii);
STDDIPINFOEXT(sfiiiasia,  sfiii, asiaSfiii);
STDDIPINFOEXT(sfiiiusa,   sfiii, usaSfiii);
STDDIPINFOEXT(sfiii3japan, region, japanSfiii3);

// -----------------------------------------------
// Street Fighter III: New Generation (USA 970204)
// -----------------------------------------------
static struct BurnRomInfo sfiiiRomDesc[] = {

	{ "sfiii_usa.29f400.u2",
					  0x080000, 0xfb172a8e, BRF_ESS | BRF_BIOS },	// SH-2 Bios
					  
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0xe896dc27, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x98c2d07c, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x7115a396, BRF_GRA },
	{ "40",			  0x800000, 0x839f0972, BRF_GRA },
	{ "41",			  0x800000, 0x8a8b252c, BRF_GRA },
	{ "50",			  0x400000, 0x58933dc2, BRF_GRA },
#endif

//	{ "sf3000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(sfiii);
STD_ROM_FN(sfiii);

// -------------------------------------------------
// Street Fighter III: New Generation (Japan 970204)
// -------------------------------------------------
static struct BurnRomInfo sfiiijRomDesc[] = {

	{ "sfiii_japan.29f400.u2",
					  0x080000, 0x74205250, BRF_ESS | BRF_BIOS },	// SH-2 Bios
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0xe896dc27, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x98c2d07c, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x7115a396, BRF_GRA },
	{ "40",			  0x800000, 0x839f0972, BRF_GRA },
	{ "41",			  0x800000, 0x8a8b252c, BRF_GRA },
	{ "50",			  0x400000, 0x58933dc2, BRF_GRA },
#endif
	
//	{ "sf3000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(sfiiij);
STD_ROM_FN(sfiiij);

// --------------------------------------------------------
// Street Fighter III: New Generation (Asia 970204, NO CD)
// --------------------------------------------------------
static struct BurnRomInfo sfiiinRomDesc[] = {

	{ "sfiii_asia_nocd.29f400.u2",
					  0x080000, 0x73e32463, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0xe896dc27, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x98c2d07c, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x7115a396, BRF_GRA },
	{ "40",			  0x800000, 0x839f0972, BRF_GRA },
	{ "41",			  0x800000, 0x8a8b252c, BRF_GRA },
	{ "50",			  0x400000, 0x58933dc2, BRF_GRA },
};

STD_ROM_PICK(sfiiin);
STD_ROM_FN(sfiiin);

// ----------------------------------------------------------------
// Street Fighter III 3rd Strike: Fight for the Future (USA 990608)
// ----------------------------------------------------------------
static struct BurnRomInfo sfiii3RomDesc[] = {

	{ "sfiii3_usa.29f400.u2",
					  0x080000, 0xecc545c1, BRF_ESS | BRF_BIOS },	// SH-2 Bios
					  
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0xba7f76b2, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x5ca8faba, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0xb37cf960, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x450ec982, BRF_GRA },
	{ "40",			  0x800000, 0x632c965f, BRF_GRA },
	{ "41",			  0x800000, 0x7a4c5f33, BRF_GRA },
	{ "50",			  0x800000, 0x8562358e, BRF_GRA },
	{ "51",			  0x800000, 0x7baf234b, BRF_GRA },
	{ "60",			  0x800000, 0xbc9487b7, BRF_GRA },
	{ "61",			  0x800000, 0xb813a1b1, BRF_GRA },
#endif

//	{ "33s000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(sfiii3);
STD_ROM_FN(sfiii3);

// ----------------------------------------------------------------
// Street Fighter III 3rd Strike: Fight for the Future (USA 990512)
// ----------------------------------------------------------------
static struct BurnRomInfo sfiii3aRomDesc[] = {

	{ "sfiii3_usa.29f400.u2",	  
					  0x080000, 0xecc545c1, BRF_ESS | BRF_BIOS },	// SH-2 Bios

#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0x77233d39, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x5ca8faba, BRF_ESS | BRF_PRG }, 

	{ "30",			  0x800000, 0xb37cf960, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x450ec982, BRF_GRA },
	{ "40",			  0x800000, 0x632c965f, BRF_GRA },
	{ "41",			  0x800000, 0x7a4c5f33, BRF_GRA },
	{ "50",			  0x800000, 0x8562358e, BRF_GRA },
	{ "51",			  0x800000, 0x7baf234b, BRF_GRA },
	{ "60",			  0x800000, 0xbc9487b7, BRF_GRA },
	{ "61",			  0x800000, 0xb813a1b1, BRF_GRA },
#endif

//	{ "cap-33s-2",	  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(sfiii3a);
STD_ROM_FN(sfiii3a);

// -------------------------------------------------------------------------
// Street Fighter III 3rd Strike: Fight for the Future (Japan 990608, NO CD)
// -------------------------------------------------------------------------
static struct BurnRomInfo sfiii3anRomDesc[] = {

	{ "sfiii3_japan_nocd.29f400.u2",
					  0x080000, 0x1edc6366, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0x77233d39, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x5ca8faba, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0xb37cf960, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x450ec982, BRF_GRA },
	{ "40",			  0x800000, 0x632c965f, BRF_GRA },
	{ "41",			  0x800000, 0x7a4c5f33, BRF_GRA },
	{ "50",			  0x800000, 0x8562358e, BRF_GRA },
	{ "51",			  0x800000, 0x7baf234b, BRF_GRA },
	{ "60",			  0x800000, 0xbc9487b7, BRF_GRA },
	{ "61",			  0x800000, 0xb813a1b1, BRF_GRA },
};

STD_ROM_PICK(sfiii3an);
STD_ROM_FN(sfiii3an);

// -------------------------------------------------------------------------
// Street Fighter III 3rd Strike: Fight for the Future (Japan 990512, NO CD)
// -------------------------------------------------------------------------
static struct BurnRomInfo sfiii3nRomDesc[] = {

	{ "sfiii3_japan_nocd.29f400.u2",
					  0x080000, 0x1edc6366, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0xba7f76b2, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x5ca8faba, BRF_ESS | BRF_PRG }, 

	{ "30",			  0x800000, 0xb37cf960, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x450ec982, BRF_GRA },
	{ "40",			  0x800000, 0x632c965f, BRF_GRA },
	{ "41",			  0x800000, 0x7a4c5f33, BRF_GRA },
	{ "50",			  0x800000, 0x8562358e, BRF_GRA },
	{ "51",			  0x800000, 0x7baf234b, BRF_GRA },
	{ "60",			  0x800000, 0xbc9487b7, BRF_GRA },
	{ "61",			  0x800000, 0xb813a1b1, BRF_GRA },
};

STD_ROM_PICK(sfiii3n);
STD_ROM_FN(sfiii3n);

// -------------------------------------------------------
// JoJo no Kimyouna Bouken / JoJo's Venture (Japan 990108)
// -------------------------------------------------------
static struct BurnRomInfo jojoRomDesc[] = {

	{ "jojo_japan.29f400.u2",
					  0x080000, 0x02778f60, BRF_ESS | BRF_BIOS },	// SH-2 Bios
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0xbc612872, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x0e1daddf, BRF_ESS | BRF_PRG },
	
	{ "30",			  0x800000, 0x1d99181b, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x6889fbda, BRF_GRA },
	{ "40",			  0x800000, 0x8069f9de, BRF_GRA },
	{ "41",			  0x800000, 0x9c426823, BRF_GRA },
	{ "50",			  0x400000, 0x1c749cc7, BRF_GRA },
#endif

//	{ "jjk000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(jojo);
STD_ROM_FN(jojo);

// -------------------------------------------------------
// JoJo no Kimyouna Bouken / JoJo's Venture (Japan 981202)
// -------------------------------------------------------
static struct BurnRomInfo jojoaRomDesc[] = {

	{ "jojo_japan.29f400.u2",
					  0x080000, 0x02778f60, BRF_ESS | BRF_BIOS },	// SH-2 Bios

#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0xe40dc123, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x0571e37c, BRF_ESS | BRF_PRG },
	
	{ "30",			  0x800000, 0x1d99181b, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x6889fbda, BRF_GRA },
	{ "40",			  0x800000, 0x8069f9de, BRF_GRA },
	{ "41",			  0x800000, 0x9c426823, BRF_GRA },
	{ "50",			  0x400000, 0x1c749cc7, BRF_GRA },
#endif

//	{ "cap-jjk-160",  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(jojoa);
STD_ROM_FN(jojoa);

// -------------------------------------------------------------
// JoJo's Venture / JoJo no Kimyouna Bouken (Asia 990108, NO CD)
// -------------------------------------------------------------
static struct BurnRomInfo jojonRomDesc[] = {

	{ "jojo_asia_nocd.29f400.u2",
					  0x080000, 0x05b4f953, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0xbc612872, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x0e1daddf, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0x1d99181b, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x6889fbda, BRF_GRA },
	{ "40",			  0x800000, 0x8069f9de, BRF_GRA },
	{ "41",			  0x800000, 0x9c426823, BRF_GRA },
	{ "50",			  0x400000, 0x1c749cc7, BRF_GRA },
};

STD_ROM_PICK(jojon);
STD_ROM_FN(jojon);

// -------------------------------------------------------------
// JoJo's Venture / JoJo no Kimyouna Bouken (Asia 981202, NO CD)
// -------------------------------------------------------------
static struct BurnRomInfo jojoanRomDesc[] = {

	{ "jojo_asia_nocd.29f400.u2",
					  0x080000, 0x05b4f953, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0xe40dc123, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x0571e37c, BRF_ESS | BRF_PRG },
	
	{ "30",			  0x800000, 0x1d99181b, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x6889fbda, BRF_GRA },
	{ "40",			  0x800000, 0x8069f9de, BRF_GRA },
	{ "41",			  0x800000, 0x9c426823, BRF_GRA },
	{ "50",			  0x400000, 0x1c749cc7, BRF_GRA },
};

STD_ROM_PICK(jojoan);
STD_ROM_FN(jojoan);

// ---------------------------------------------------------------------------------
// JoJo no Kimyouna Bouken: Miraie no Isan / JoJo's Bizarre Adventure (Japan 990913)
// ---------------------------------------------------------------------------------
static struct BurnRomInfo jojobaRomDesc[] = {

	{ "jojoba_japan.29f400.u2",
					  0x080000, 0x3085478c, BRF_ESS | BRF_BIOS },	// SH-2 Bios
					  
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0x6e2490f6, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x1293892b, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0xd25c5005, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x51bb3dba, BRF_GRA },
	{ "40",			  0x800000, 0x94dc26d4, BRF_GRA },
	{ "41",			  0x800000, 0x1c53ee62, BRF_GRA },
	{ "50",			  0x800000, 0x36e416ed, BRF_GRA },
	{ "51",			  0x800000, 0xeedf19ca, BRF_GRA },
#endif

//	{ "jjm000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(jojoba);
STD_ROM_FN(jojoba);

// ----------------------------------------------------------------------------------------
// JoJo no Kimyouna Bouken: Miraie no Isan / JoJo's Bizarre Adventure (Japan 990913, NO CD)
// ----------------------------------------------------------------------------------------
static struct BurnRomInfo jojobanRomDesc[] = {

	{ "jojoba_japan_nocd.29f400.u2",
					  0x080000, 0x4dab19f5, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0x6e2490f6, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x1293892b, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0xd25c5005, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x51bb3dba, BRF_GRA },
	{ "40",			  0x800000, 0x94dc26d4, BRF_GRA },
	{ "41",			  0x800000, 0x1c53ee62, BRF_GRA },
	{ "50",			  0x800000, 0x36e416ed, BRF_GRA },
	{ "51",			  0x800000, 0xeedf19ca, BRF_GRA },
};

STD_ROM_PICK(jojoban);
STD_ROM_FN(jojoban);

// ---------------------------------------------------------------------------------------
// JoJo's Bizarre Adventure / JoJo no Kimyouna Bouken: Miraie no Isan (Euro 990913, NO CD)
// ---------------------------------------------------------------------------------------
static struct BurnRomInfo jojobaneRomDesc[] = {

	{ "jojoba_euro_nocd.29f400.u2",
					  0x080000, 0x1ee2d679, BRF_ESS | BRF_BIOS },	// SH-2 Bios

	{ "10",			  0x800000, 0x6e2490f6, BRF_ESS | BRF_PRG },	// SH-2 Code
	{ "20",			  0x800000, 0x1293892b, BRF_ESS | BRF_PRG },

	{ "30",			  0x800000, 0xd25c5005, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x51bb3dba, BRF_GRA },
	{ "40",			  0x800000, 0x94dc26d4, BRF_GRA },
	{ "41",			  0x800000, 0x1c53ee62, BRF_GRA },
	{ "50",			  0x800000, 0x36e416ed, BRF_GRA },
	{ "51",			  0x800000, 0xeedf19ca, BRF_GRA },
};

STD_ROM_PICK(jojobane);
STD_ROM_FN(jojobane);

// ----------------------------------
// Red Earth / War-Zard (Euro 961121)
// ----------------------------------
static struct BurnRomInfo redearthRomDesc[] = {

	{ "warzard_euro.29f400.u2",
					  0x080000, 0x02e0f336, BRF_ESS | BRF_BIOS },	// SH-2 Bios

#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0x68188016, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x074cab4d, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x14e2cad4, BRF_GRA },
	{ "40",			  0x800000, 0x72d98890, BRF_GRA },
	{ "41",			  0x800000, 0x88ccb33c, BRF_GRA },
	{ "50",			  0x400000, 0x2f5b44bd, BRF_GRA },
#endif

//	{ "wzd000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(redearth);
STD_ROM_FN(redearth);

// -----------------------------------
// War-Zard / Red Earth (Japan 961121)
// -----------------------------------
static struct BurnRomInfo warzardRomDesc[] = {

	{ "warzard_japan.29f400.u2",
					  0x080000, 0xf8e2f0c6, BRF_ESS | BRF_BIOS },	// SH-2 Bios
					  
#if !defined (ROM_VERIFY)
	{ "10",			  0x800000, 0x68188016, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x074cab4d, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x14e2cad4, BRF_GRA },
	{ "40",			  0x800000, 0x72d98890, BRF_GRA },
	{ "41",			  0x800000, 0x88ccb33c, BRF_GRA },
	{ "50",			  0x400000, 0x2f5b44bd, BRF_GRA },
#endif

//	{ "wzd000",		  0x000000, 0x00000000, BRF_ESS | BRF_CHD },	// CD-ROM
};

STD_ROM_PICK(warzard);
STD_ROM_FN(warzard);

// -----------------------------------------
// Red Earth / War-Zard (Euro 961121, NO CD)
// -----------------------------------------
static struct BurnRomInfo redeartnRomDesc[] = {

#if !defined (ROM_VERIFY)
	{ "warzard_euro.29f400.u2",0x080000, 0x02e0f336, BRF_ESS | BRF_BIOS },	// SH-2 Bios
#else
	{ "redearth_nocd.bios",   0x080000, 0x00000000, BRF_ESS | BRF_BIOS | BRF_NODUMP },	// SH-2 Bios
#endif

	{ "10",			  0x800000, 0x68188016, BRF_ESS | BRF_PRG },	// SH-2 Code

	{ "30",			  0x800000, 0x074cab4d, BRF_GRA },				// cd content region
	{ "31",			  0x800000, 0x14e2cad4, BRF_GRA },
	{ "40",			  0x800000, 0x72d98890, BRF_GRA },
	{ "41",			  0x800000, 0x88ccb33c, BRF_GRA },
	{ "50",			  0x400000, 0x2f5b44bd, BRF_GRA },
};

STD_ROM_PICK(redeartn);
STD_ROM_FN(redeartn);

// ------------------------------------------------------------------------------------

static int sfiiiInit()
{
	cps3_key1 = 0xb5fe053e;
	cps3_key2 = 0xfc03925a;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x000166b4;
	cps3_game_test_hack = 0x063cdff4;

	cps3_speedup_ram_address  = 0x0200cc6c;
	cps3_speedup_code_address = 0x06000884;

	cps3_region_address = 0x0001fec8;
	cps3_ncd_address    = 0x0001fecf;

	return cps3Init();
}

static int sfiii3Init()
{
	cps3_key1 = 0xa55432b4;
	cps3_key2 = 0x0c129981;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x00011c44;
	cps3_game_test_hack = 0x0613ab48;

	cps3_speedup_ram_address  = 0x0200d794;
	cps3_speedup_code_address = 0x06000884;

	cps3_region_address = 0x0001fec8;
	cps3_ncd_address    = 0x0001fecf;

	return cps3Init();
}

static int jojoInit()
{
	cps3_key1 = 0x02203ee3;
	cps3_key2 = 0x01301972;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x00011c2c;
	cps3_game_test_hack = 0x06172568;

	cps3_speedup_ram_address  = 0x020223d8;
	cps3_speedup_code_address = 0x0600065c;

	cps3_region_address = 0x0001fec8;
	cps3_ncd_address    = 0x0001fecf;

	return cps3Init();
}

static int jojobaInit()
{
	cps3_key1 = 0x23323ee3;
	cps3_key2 = 0x03021972;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x00011c90;
	cps3_game_test_hack = 0x061c45bc;

	cps3_speedup_ram_address  = 0x020267dc;
	cps3_speedup_code_address = 0x0600065c;

	cps3_region_address = 0x0001fec8;
	cps3_ncd_address    = 0x0001fecf;

	return cps3Init();
}

static int jojoaltInit()
{
	cps3_key1 = 0x02203ee3;
	cps3_key2 = 0x01301972;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x00011c2c;
	cps3_game_test_hack = 0x06172568;

	cps3_speedup_ram_address  = 0x020223c0;
	cps3_speedup_code_address = 0x0600065c;

	cps3_region_address = 0x0001fec8;
	cps3_ncd_address    = 0x0001fecf;

	return cps3Init();
}

static int redearthInit()
{
	cps3_key1 = 0x9e300ab1;
	cps3_key2 = 0xa175b82c;
	cps3_isSpecial = 0;

	cps3_bios_test_hack = 0x00016530;
	cps3_game_test_hack = 0x060105f0;

	cps3_speedup_ram_address  = 0x0202136c;
	cps3_speedup_code_address = 0x0600194e;

	cps3_region_address = 0x0001fed8;
	cps3_ncd_address    = 0x00000000;

	return cps3Init();
}


struct BurnDriver BurnDrvSfiii = {
	"sfiii", NULL, NULL, "1997",
	"Street Fighter III: New Generation (USA 970204)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, FBF_SF,
	NULL, sfiiiRomInfo, sfiiiRomName, cps3InputInfo, sfiiiusaDIPInfo,
	sfiiiInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvSfiiij = {
	"sfiiij", "sfiii", NULL, "1997",
	"Street Fighter III: New Generation (Japan 970204)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_CLONE | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, FBF_SF,
	NULL, sfiiijRomInfo, sfiiijRomName, cps3InputInfo, sfiiijapanDIPInfo,
	sfiiiInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvSfiiin = {
	"sfiiin", "sfiii", NULL, "1997",
	"Street Fighter III: New Generation (Asia 970204, NO CD)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, FBF_SF,
	NULL, sfiiinRomInfo, sfiiinRomName, cps3InputInfo, sfiiiasiaDIPInfo,
	sfiiiInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvSfiii3 = {
	"sfiii3", NULL, NULL, "1999",
	"Street Fighter III 3rd Strike: Fight for the Future (USA 990608)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_CLONE | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, FBF_SF,
	NULL, sfiii3RomInfo, sfiii3RomName, cps3InputInfo, usaDIPInfo,
	sfiii3Init, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};
		
struct BurnDriver BurnDrvSfiii3a = {
	"sfiii3a", "sfiii3", NULL, "1999",
	"Street Fighter III 3rd Strike: Fight for the Future (USA 990512)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, FBF_SF,
	NULL, sfiii3aRomInfo, sfiii3aRomName, cps3InputInfo, usaDIPInfo,
	sfiii3Init, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvSfiii3n = {
	"sfiii3n", "sfiii3", NULL, "1999",
	"Street Fighter III 3rd Strike: Fight for the Future (Japan 990608, NO CD)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, FBF_SF,
	NULL, sfiii3nRomInfo, sfiii3nRomName, cps3InputInfo, sfiii3japanDIPInfo,
	sfiii3Init, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvSfiii3an = {
	"sfiii3an", "sfiii3", NULL, "1999",
	"Street Fighter III 3rd Strike: Fight for the Future (Japan 990512, NO CD)\0", NULL, "Capcom", "CPS-3",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, FBF_SF,
	NULL, sfiii3anRomInfo, sfiii3anRomName, cps3InputInfo, sfiii3japanDIPInfo,
	sfiii3Init, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojo = {
	"jojo", NULL, NULL, "1998",
	"JoJo's Venture / JoJo no Kimyouna Bouken (Japan 990108)\0", NULL, "Capcom", "CPS-3",
	L"\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A\0JoJo's Venture (Japan 990108)\0", NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_CLONE | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, 0,
	NULL, jojoRomInfo, jojoRomName, cps3InputInfo, jojoJapanDIPInfo,
	jojoInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojoa = {
	"jojoa", "jojo", NULL, "1998",
	"JoJo's Venture / JoJo no Kimyouna Bouken (Japan 981202)\0", NULL, "Capcom", "CPS-3",
	L"\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A\0JoJo's Venture (Japan 981202)\0", NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, 0,
	NULL, jojoaRomInfo, jojoaRomName, cps3InputInfo, jojoJapanDIPInfo,
	jojoaltInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojon = {
	"jojon", "jojo", NULL, "1998",
	"JoJo's Venture / JoJo no Kimyouna Bouken (Asia 990108, NO CD)\0", NULL, "Capcom", "CPS-3",
	L"JoJo's Venture\0\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A (Asia 990108, NO CD)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, 0,
	NULL, jojonRomInfo, jojonRomName, cps3InputInfo, jojoAsiaDIPInfo,
	jojoInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojoan = {
	"jojoan", "jojo", NULL, "1998",
	"JoJo's Venture / JoJo no Kimyouna Bouken (Asia 981202, NO CD)\0", NULL, "Capcom", "CPS-3",
	L"JoJo's Venture\0\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A (Asia 981202, NO CD)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, 0,
	NULL, jojoanRomInfo, jojoanRomName, cps3InputInfo, jojoAsiaDIPInfo,
	jojoaltInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojoba = {
	"jojoba", NULL, NULL, "1999",
	"JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (Japan 990913)\0", NULL, "Capcom", "CPS-3",
	L"\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A: \u672A\u6765\u3078\u306E\u907A\u7523\0JoJo's Bizarre Adventure (Japan 990913)\0", NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, 0,
	NULL, jojobaRomInfo, jojobaRomName, cps3InputInfo, jojobaJapanDIPInfo,
	jojobaInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojoban = {
	"jojoban", "jojoba", NULL, "1999",
	"JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (Japan 990913, NO CD)\0", NULL, "Capcom", "CPS-3",
	L"\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A: \u672A\u6765\u3078\u306E\u907A\u7523\0JoJo's Bizarre Adventure (Japan 990913, NO CD)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, 0,
	NULL, jojobanRomInfo, jojobanRomName, cps3InputInfo, jojobaJapanDIPInfo,
	jojobaInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvJojobane = {
	"jojobane", "jojoba", NULL, "1999",
	"JoJo's Bizarre Adventure: Heritage for the Future / JoJo no Kimyouna Bouken: Miraie no Isan (Euro 990913, NO CD)\0", NULL, "Capcom", "CPS-3",
	L"JoJo's Bizarre Adventure\0\u30B8\u30E7\u30B8\u30E7\u306E \u5947\u5999\u306A\u5192\u967A: \u672A\u6765\u3078\u306E\u907A\u7523 (Euro 990913, NO CD)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, 0,
	NULL, jojobaneRomInfo, jojobaneRomName, cps3InputInfo, jojobaEuroDIPInfo,
	jojobaInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvRedearth = {
	"redearth", NULL, NULL, "1996",
	"Red Earth / War-Zard (Euro 961121)\0", NULL, "Capcom", "CPS-3",
	L"Red Earth\0War-Zard (Euro 961121)\0", NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, 0,
	NULL, redearthRomInfo, redearthRomName, cps3InputInfo, redearthEuroDIPInfo,
	redearthInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvWarzard = {
	"warzard", "redearth", NULL, "1996",
	"War-Zard / Red Earth (Japan 961121)\0", NULL, "Capcom", "CPS-3",
	L"War-Zard\0Red Earth (Japan 961121)\0", NULL, NULL, NULL,
	BDF_16BIT_ONLY | BDF_CLONE | BDF_HACK, 2, HARDWARE_CAPCOM_CPS3, GBF_VSFIGHT, 0,
	NULL, warzardRomInfo, warzardRomName, cps3InputInfo, warzardJapanDIPInfo,
	redearthInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};

struct BurnDriver BurnDrvRedeartn = {
	"redeartn", "redearth", NULL, "1996",
	"Red Earth / War-Zard (Euro 961121, NO CD)\0", NULL, "Capcom", "CPS-3",
	L"Red Earth\0War-Zard (Euro 961121, NO CD)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY | BDF_CLONE, 2, HARDWARE_CAPCOM_CPS3 | HARDWARE_CAPCOM_CPS3_NO_CD, GBF_VSFIGHT, 0,
	NULL, redeartnRomInfo, redeartnRomName, cps3InputInfo, redeartnEuroDIPInfo,
	redearthInit, cps3Exit, cps3Frame, NULL, cps3Scan, 0, NULL, NULL, NULL, &cps3_palette_change,
	384, 224, 4, 3
};
