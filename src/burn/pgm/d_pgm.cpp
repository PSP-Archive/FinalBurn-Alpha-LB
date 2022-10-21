/*
 * PGM System (c)1997 IGS
 * Based on Information from ElSemi
 *
 * IGS driver for Finalburn Alpha ported from MAME v0.115
 *
 * Port by OopsWare 2007
 * http://oopsware.googlepages.com
 *
 * FBA shuffle - Maintained by Creamymami && KOF2112
 */


#include "pgm.h"


static struct BurnRomInfo emptyRomDesc[] = {
	{ "",                    0,          0, 0 },
};

static struct BurnInputInfo ALIGN_DATA pgmInputList[] = {
	{"P1 Coin",			BIT_DIGITAL,	PgmBtn1 + 0,	"p1 coin"  },
	{"P1 Start",		BIT_DIGITAL,	PgmJoy1 + 0,	"p1 start" },
	{"P1 Up",			BIT_DIGITAL,	PgmJoy1 + 1,	"p1 up"    },
	{"P1 Down",			BIT_DIGITAL,	PgmJoy1 + 2,	"p1 down"  },
	{"P1 Left",			BIT_DIGITAL,	PgmJoy1 + 3,	"p1 left"  },
	{"P1 Right",		BIT_DIGITAL,	PgmJoy1 + 4,	"p1 right" },
	{"P1 Button 1",		BIT_DIGITAL,	PgmJoy1 + 5,	"p1 fire 1"},
	{"P1 Button 2",		BIT_DIGITAL,	PgmJoy1 + 6,	"p1 fire 2"},
	{"P1 Button 3",		BIT_DIGITAL,	PgmJoy1 + 7,	"p1 fire 3"},
	{"P1 Button 4",		BIT_DIGITAL,	PgmBtn2 + 0,	"p1 fire 4"},

	{"P2 Coin",			BIT_DIGITAL,	PgmBtn1 + 1,	"p2 coin"  },
	{"P2 Start",		BIT_DIGITAL,	PgmJoy2 + 0,	"p2 start" },
	{"P2 Up",			BIT_DIGITAL,	PgmJoy2 + 1,	"p2 up"    },
	{"P2 Down",			BIT_DIGITAL,	PgmJoy2 + 2,	"p2 down"  },
	{"P2 Left",			BIT_DIGITAL,	PgmJoy2 + 3,	"p2 left"  },
	{"P2 Right",		BIT_DIGITAL,	PgmJoy2 + 4,	"p2 right" },
	{"P2 Button 1",		BIT_DIGITAL,	PgmJoy2 + 5,	"p2 fire 1"},
	{"P2 Button 2",		BIT_DIGITAL,	PgmJoy2 + 6,	"p2 fire 2"},
	{"P2 Button 3",		BIT_DIGITAL,	PgmJoy2 + 7,	"p2 fire 3"},
	{"P2 Button 4",		BIT_DIGITAL,	PgmBtn2 + 1,	"p2 fire 4"},

	{"P3 Coin",			BIT_DIGITAL,	PgmBtn1 + 2,	"p3 coin"  },
	{"P3 Start",		BIT_DIGITAL,	PgmJoy3 + 0,	"p3 start" },
	{"P3 Up",			BIT_DIGITAL,	PgmJoy3 + 1,	"p3 up"    },
	{"P3 Down",			BIT_DIGITAL,	PgmJoy3 + 2,	"p3 down"  },
	{"P3 Left",			BIT_DIGITAL,	PgmJoy3 + 3,	"p3 left"  },
	{"P3 Right",		BIT_DIGITAL,	PgmJoy3 + 4,	"p3 right" },
	{"P3 Button 1",		BIT_DIGITAL,	PgmJoy3 + 5,	"p3 fire 1"},
	{"P3 Button 2",		BIT_DIGITAL,	PgmJoy3 + 6,	"p3 fire 2"},
	{"P3 Button 3",		BIT_DIGITAL,	PgmJoy3 + 7,	"p3 fire 3"},
	{"P3 Button 4",		BIT_DIGITAL,	PgmBtn2 + 2,	"p3 fire 4"},

	{"P4 Coin",			BIT_DIGITAL,	PgmBtn1 + 3,	"p4 coin"  },
	{"P4 Start",		BIT_DIGITAL,	PgmJoy4 + 0,	"p4 start" },
	{"P4 Up",			BIT_DIGITAL,	PgmJoy4 + 1,	"p4 up"    },
	{"P4 Down",			BIT_DIGITAL,	PgmJoy4 + 2,	"p4 down"  },
	{"P4 Left",			BIT_DIGITAL,	PgmJoy4 + 3,	"p4 left"  },
	{"P4 Right",		BIT_DIGITAL,	PgmJoy4 + 4,	"p4 right" },
	{"P4 Button 1",		BIT_DIGITAL,	PgmJoy4 + 5,	"p4 fire 1"},
	{"P4 Button 2",		BIT_DIGITAL,	PgmJoy4 + 6,	"p4 fire 2"},
	{"P4 Button 3",		BIT_DIGITAL,	PgmJoy4 + 7,	"p4 fire 3"},
	{"P4 Button 4",		BIT_DIGITAL,	PgmBtn2 + 3,	"p4 fire 4"},

	{"Reset",			BIT_DIGITAL,	&PgmReset,		"reset"    },
	{"Diagnostics 1",	BIT_DIGITAL,	PgmBtn1 + 4,	"diag"     },
	{"Diagnostics 2",	BIT_DIGITAL,	PgmBtn1 + 6,	""         },
	{"Service 1",		BIT_DIGITAL,	PgmBtn1 + 5,	"service"  },
	{"Service 2",		BIT_DIGITAL,	PgmBtn1 + 7,	"service2" },

	{"Dip A",			BIT_DIPSWITCH,	PgmInput + 6,	"dip"      },
	{"Dip B",			BIT_DIPSWITCH,	PgmInput + 7,	"dip"      },
	{"Dip C",			BIT_DIPSWITCH,  PgmInput + 8,   "dip"      },
};

STDINPUTINFO(pgm);

static struct BurnDIPInfo pgmDIPList[] = {
	{0x2D,	0xFF, 0xFF,	0x00, NULL},
	{0x2F,	0xFF, 0x01,	0x01, NULL},

	{0,		0xFE, 0,	2,	  "Test mode"},
	{0x2D,	0x01, 0x01,	0x00, "No"},
	{0x2D,	0x01, 0x01,	0x01, "Yes"},

	{0,		0xFE, 0,	2,	  "Music"},
	{0x2D,	0x01, 0x02,	0x02, "No"},
	{0x2D,	0x01, 0x02,	0x00, "Yes"},

	{0,		0xFE, 0,	2,	  "Voice"},
	{0x2D,	0x01, 0x04,	0x04, "No"},
	{0x2D,	0x01, 0x04,	0x00, "Yes"},

	{0,		0xFE, 0,	2,	  "Free play"},
	{0x2D,	0x01, 0x08,	0x00, "No"},
	{0x2D,	0x01, 0x08,	0x08, "Yes"},

	{0,		0xFE, 0,	2,	  "Stop mode"},
	{0x2D,	0x01, 0x10,	0x00, "No"},
	{0x2D,	0x01, 0x10,	0x10, "Yes"},

	{0,     0xFE, 0,    2,	  "Bios select (Fake)"},
	{0x2F,  0x01, 0x01, 0x00, "PGM Bios V1"},
	{0x2F,  0x01, 0x01, 0x01, "PGM Bios V2"},
};

STDDIPINFO(pgm);

static struct BurnDIPInfo jammaDIPList[] = {
	{0x2D,	0xFF, 0xFF,	0x00, NULL},

	{0,		0xFE, 0,	2,	  "Test mode"},
	{0x2D,	0x01, 0x01,	0x00, "No"},
	{0x2D,	0x01, 0x01,	0x01, "Yes"},

	{0,		0xFE, 0,	2,	  "Music"},
	{0x2D,	0x01, 0x02,	0x02, "No"},
	{0x2D,	0x01, 0x02,	0x00, "Yes"},

	{0,		0xFE, 0,	2,	  "Voice"},
	{0x2D,	0x01, 0x04,	0x04, "No"},
	{0x2D,	0x01, 0x04,	0x00, "Yes"},

	{0,		0xFE, 0,	2,	  "Free play"},
	{0x2D,	0x01, 0x08,	0x00, "No"},
	{0x2D,	0x01, 0x08,	0x08, "Yes"},

	{0,		0xFE, 0,	2,	  "Stop mode"},
	{0x2D,	0x01, 0x10,	0x00, "No"},
	{0x2D,	0x01, 0x10,	0x10, "Yes"},
};

STDDIPINFO(jamma);


static struct BurnDIPInfo ddp2DIPList[] = {

	// Defaults
	{0x2E,	0xFF, 0xFF,	0x02, NULL			},

	// DIP 2
	{0,		0xFE, 0,	6,	  "Region (Fake)"},
	{0x2E,	0x01, 0x07,	0x00, "China"},
	{0x2E,	0x01, 0x07,	0x01, "Taiwan"},
	{0x2E,	0x01, 0x07,	0x02, "Japan (Cave License)"},
	{0x2E,	0x01, 0x07,	0x03, "Korea"},
	{0x2E,	0x01, 0x07,	0x04, "Hong Kong"},
	{0x2E,	0x01, 0x07,	0x05, "World"},
};

STDDIPINFOEXT(ddp2, pgm, ddp2);


// -----------------------------------------------------------------------------
// BIOS

// PGM (Polygame Master) System BIOS

static struct BurnRomInfo pgmRomDesc[] = {
	{ "pgm_t01s.rom",  0x200000, 0x1a7123a0, BRF_GRA | BRF_BIOS }, 	// 0x80 - 8x8 Text Layer Tiles
	{ "pgm_m01s.rom",  0x200000, 0x45ae7159, BRF_SND | BRF_BIOS },	// 0x81 - Samples

	{ "pgm_p01s.u20",  0x020000, 0xe42b166e, BRF_PRG | BRF_BIOS },	// 0x82 - 68K BIOS (V0001, older - 02/26/97 - 11:14:09)
	{ "pgm_p02s.u20",  0x020000, 0x78c15fa2, BRF_PRG | BRF_BIOS },	// 0x83 - 68K BIOS (V0001, newer - 07/10/97 - 16:36:08)
};

STD_ROM_PICK(pgm);
STD_ROM_FN(pgm);

struct BurnDriver BurnDrvPgm = {
	"pgm", NULL, NULL, "1997",
	"PGM (Polygame Master) System BIOS\0", "BIOS only", "IGS", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_BOARDROM, 0, HARDWARE_IGS_PGM, GBF_BIOS, 0,
	NULL, pgmRomInfo, pgmRomName, pgmInputInfo, pgmDIPInfo,
	pgmInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	448, 224, 4, 3
};

static struct BurnRomInfo ketsuiBiosRomDesc[] = {
	{ "pgm_t01s.rom",  0x200000, 0x1a7123a0, BRF_GRA | BRF_BIOS }, 	// 0x80 - 8x8 Text Layer Tiles
	{ "",              0,        0,          0 },					// 0x81 - Samples

	{ "",              0,        0,          0 },					// 0x82 - 68K BIOS
};

STD_ROM_PICK(ketsuiBios);
STD_ROM_FN(ketsuiBios);

static struct BurnRomInfo espgalBiosRomDesc[] = {
	{ "t01s.u18",      0x200000, 0x1a7123a0, BRF_GRA | BRF_BIOS }, 	// 0x80 - 8x8 Text Layer Tiles
	{ "",              0,        0,          0 },					// 0x81 - Samples

	{ "",              0,        0,          0 },					// 0x82 - 68K BIOS
};

STD_ROM_PICK(espgalBios);
STD_ROM_FN(espgalBios);

static struct BurnRomInfo ddp3BiosRomDesc[] = {
	{ "pgm_t01s.rom",  0x200000, 0x1a7123a0, BRF_GRA | BRF_BIOS }, 	// 0x80 - 8x8 Text Layer Tiles
	{ "pgm_m01s.rom",  0x200000, 0x45ae7159, BRF_SND | BRF_BIOS },	// 0x81 - Samples

	{ "ddp3_bios.u37", 0x080000, 0xb3cc5c8f, BRF_PRG | BRF_BIOS },	// 0x82 - 68K BIOS (V0001, custom? 07/17/97 19:44:59)
};

STD_ROM_PICK(ddp3Bios);
STD_ROM_FN(ddp3Bios);


// -----------------------------------------------------------------------------
// Normal Games


// Bee Storm - DoDonPachi II (ver. 102)

static struct BurnRomInfo ddp2RomDesc[] = {
	{ "v102.u8",            0x200000, 0x5a9ea040, 1 | BRF_ESS | BRF_PRG },   // 68K Code

	{ "t1300.u21",          0x800000, 0xe748f0cb, 2 | BRF_GRA },			 // Tile data

	{ "a1300.u1",           0x800000, 0xfc87a405, 3 | BRF_GRA },			 // Sprite Color Data
	{ "a1301.u2",           0x800000, 0x0c8520da, 3 | BRF_GRA },

	{ "b1300.u7",           0x800000, 0xef646604, 4 | BRF_GRA },			 // Sprite Masks & Color Indexes

	{ "m1300.u5",           0x400000, 0x82d4015d, 5 | BRF_SND },			 // Samples

//	{ "ddp2_igs027a.bin",   0x004000, 0x8c566319, 7 | BRF_ESS | BRF_PRG },   // IGS027(A) Internal Code, SHA1(bb001d8ada56bf446f9ab88e00936501652daf11)

	{ "v100.u23",           0x020000, 0x06c3dd29, 8 | BRF_ESS | BRF_PRG },   // External ARM rom
};

STDROMPICKEXT(ddp2, ddp2, pgm);
STD_ROM_FN(ddp2);

static int ddp2Init()
{
//	pPgmInitCallback = pgm_decrypt_ddp2;

	int ret = pgmInit();

	if (ret == 0) {
		install_protection_ddp2();
	}

	return ret;
}

struct BurnDriver BurnDrvDdp2 = {
	"ddp2", NULL, "pgm", "2001",
	"Bee Storm - DoDonPachi II (ver. 102)\0", "Not working", "IGS", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_USE_ARM_CPU, GBF_VERSHOOT, 0,
	NULL, ddp2RomInfo, ddp2RomName, pgmInputInfo, ddp2DIPInfo,
	ddp2Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// Bee Storm - DoDonPachi II (ver. 101)

static struct BurnRomInfo ddp2101RomDesc[] = {
	{ "v101_16m.u8",        0x200000, 0x5e5786fd, 1 | BRF_ESS | BRF_PRG },   // 68K Code

	{ "t1300.u21",          0x800000, 0xe748f0cb, 2 | BRF_GRA },			 // Tile data

	{ "a1300.u1",           0x800000, 0xfc87a405, 3 | BRF_GRA },			 // Sprite Color Data
	{ "a1301.u2",           0x800000, 0x0c8520da, 3 | BRF_GRA },

	{ "b1300.u7",           0x800000, 0xef646604, 4 | BRF_GRA },			 // Sprite Masks & Color Indexes

	{ "m1300.u5",           0x400000, 0x82d4015d, 5 | BRF_SND },			 // Samples

//	{ "ddp2_igs027a.bin",   0x004000, 0x8c566319, 7 | BRF_ESS | BRF_PRG },   // IGS027(A) Internal Code, SHA1(bb001d8ada56bf446f9ab88e00936501652daf11)

	{ "v100.u23",           0x020000, 0x06c3dd29, 8 | BRF_ESS | BRF_PRG },   // External ARM rom
};

STDROMPICKEXT(ddp2101, ddp2101, pgm);
STD_ROM_FN(ddp2101);

struct BurnDriver BurnDrvDdp2101 = {
	"ddp2101", "ddp2", "pgm", "2001",
	"Bee Storm - DoDonPachi II (ver. 101)\0", "Not working", "IGS", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_USE_ARM_CPU, GBF_VERSHOOT, 0,
	NULL, ddp2101RomInfo, ddp2101RomName, pgmInputInfo, ddp2DIPInfo,
	ddp2Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// Bee Storm - DoDonPachi II (ver. 100)

static struct BurnRomInfo ddp2100RomDesc[] = {
	{ "v100.u8",            0x200000, 0x0c8aa8ea, 1 | BRF_ESS | BRF_PRG },   // 68K Code

	{ "t1300.u21",          0x800000, 0xe748f0cb, 2 | BRF_GRA },			 // Tile data

	{ "a1300.u1",           0x800000, 0xfc87a405, 3 | BRF_GRA },			 // Sprite Color Data
	{ "a1301.u2",           0x800000, 0x0c8520da, 3 | BRF_GRA },

	{ "b1300.u7",           0x800000, 0xef646604, 4 | BRF_GRA },			 // Sprite Masks & Color Indexes

	{ "m1300.u5",           0x400000, 0x82d4015d, 5 | BRF_SND },			 // Samples

//	{ "ddp2_igs027a.bin",   0x004000, 0x8c566319, 7 | BRF_ESS | BRF_PRG },   // IGS027(A) Internal Code, SHA1(bb001d8ada56bf446f9ab88e00936501652daf11)

	{ "v100.u23",           0x020000, 0x06c3dd29, 8 | BRF_ESS | BRF_PRG },   // External ARM rom
};

STDROMPICKEXT(ddp2100, ddp2100, pgm);
STD_ROM_FN(ddp2100);

struct BurnDriver BurnDrvDdp2100 = {
	"ddp2100", "ddp2", "pgm", "2001",
	"Bee Storm - DoDonPachi II (ver. 100)\0", "Not working", "IGS", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_USE_ARM_CPU, GBF_VERSHOOT, 0,
	NULL, ddp2100RomInfo, ddp2100RomName, pgmInputInfo, ddp2DIPInfo,
	ddp2Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// -----------------------------------------------------------------------------
// PCB Versions!

// Ketsui: Kizuna Jigoku Tachi (V100, third revision)

static struct BurnRomInfo ketRomDesc[] = {
	{ "ketsui_v100.u38",     0x200000, 0xdfe62f3b, 1 | BRF_ESS | BRF_PRG },				 //  0 68K Code

	{ "t04701w064.u19",      0x800000, 0x2665b041, 2 | BRF_GRA },						 //  1 32x32 BG Tiles

	{ "a04701w064.u7",       0x800000, 0x5ef1b94b, 3 | BRF_GRA },						 //  2 Sprite Colour Data
	{ "a04702w064.u8",       0x800000, 0x26d6da7f, 3 | BRF_GRA },						 //  3

	{ "b04701w064.u1",       0x800000, 0x1bec008d, 4 | BRF_GRA },						 //  4 Sprite Masks + Colour Indexes

	{ "m04701b032.u17",      0x400000, 0xb46e22d1, 5 | BRF_SND },						 //  5 Samples

//	{ "ket_igs027a.bin",     0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ket_defaults.nv",     0x020000, 0x3ca892d8, 0 | BRF_OPT },						 //  6 NV RAM
};

STDROMPICKEXT(ket, ket, ketsuiBios);
STD_ROM_FN(ket);

static int ketInit()
{
	nCavePgm = 1;
	nPGMEnableIRQ4 = 1;

	pPgmInitCallback = pgm_decrypt_ket;
	pPgmResetCallback = reset_ddp3;

	int ret = pgmInit();

	if (ret == 0) {
		install_protection_ket();
	}

	return ret;
}

struct BurnDriver BurnDrvKet = {
	"ket", NULL, "pgm", "2002",
	"Ketsui: Kizuna Jigoku Tachi (V100, third revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_JAMMAPCB, GBF_VERSHOOT, 0,
	NULL, ketRomInfo, ketRomName, pgmInputInfo, jammaDIPInfo,
	ketInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// Ketsui: Kizuna Jigoku Tachi (V100, second revision)

static struct BurnRomInfo ketaRomDesc[] = {
	{ "ketsui_prg_revised.bin", 0x200000, 0x69fcf5eb, 1 | BRF_ESS | BRF_PRG },			 // 68K Code

	{ "t04701w064.u19",      0x800000, 0x2665b041, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04701w064.u7",       0x800000, 0x5ef1b94b, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04702w064.u8",       0x800000, 0x26d6da7f, 3 | BRF_GRA },

	{ "b04701w064.u1",       0x800000, 0x1bec008d, 4 | BRF_GRA },						 // Sprite Masks + Colour Indexes

	{ "m04701b032.u17",      0x400000, 0xb46e22d1, 5 | BRF_SND },						 // Samples

//	{ "ket_igs027a.bin",     0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ket_defaults.nv",     0x020000, 0x3ca892d8, 0 | BRF_OPT },						 // NV RAM
};

STDROMPICKEXT(keta, keta, ketsuiBios);
STD_ROM_FN(keta);

struct BurnDriver BurnDrvKeta = {
	"keta", "ket", "pgm", "2002",
	"Ketsui: Kizuna Jigoku Tachi (V100, second revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_JAMMAPCB, GBF_VERSHOOT, 0,
	NULL, ketaRomInfo, ketaRomName, pgmInputInfo, jammaDIPInfo,
	ketInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// Ketsui: Kizuna Jigoku Tachi (V100, first revision)

static struct BurnRomInfo ketbRomDesc[] = {
	{ "ketsui_prg_original.bin", 0x200000, 0xcca5e153, 1 | BRF_ESS | BRF_PRG },			 // 68K Code

	{ "t04701w064.u19",      0x800000, 0x2665b041, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04701w064.u7",       0x800000, 0x5ef1b94b, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04702w064.u8",       0x800000, 0x26d6da7f, 3 | BRF_GRA },

	{ "b04701w064.u1",       0x800000, 0x1bec008d, 4 | BRF_GRA },						 // Sprite Masks + Colour Indexes

	{ "m04701b032.u17",      0x400000, 0xb46e22d1, 5 | BRF_SND },						 // Samples

//	{ "ket_igs027a.bin",     0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ket_defaults.nv",     0x020000, 0x3ca892d8, 0 | BRF_OPT },						 // NV RAM
};

STDROMPICKEXT(ketb, ketb, ketsuiBios);
STD_ROM_FN(ketb);

struct BurnDriver BurnDrvKetb = {
	"ketb", "ket", "pgm", "2002",
	"Ketsui: Kizuna Jigoku Tachi (V100, first revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_JAMMAPCB, GBF_VERSHOOT, 0,
	NULL, ketbRomInfo, ketbRomName, pgmInputInfo, jammaDIPInfo,
	ketInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// EspGaluda (V100, first revision)

static struct BurnRomInfo espgalRomDesc[] = {
	{ "espgaluda_v100.u38",  0x200000, 0x08ecec34, 1 | BRF_ESS | BRF_PRG },				 // 68K Code

	{ "t04801w064.u19",      0x800000, 0x6021c79e, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04801w064.u7",       0x800000, 0x26dd4932, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04802w064.u8",       0x800000, 0x0e6bf7a9, 3 | BRF_GRA },

	{ "b04801w064.u1",       0x800000, 0x98dce13a, 4 | BRF_GRA },						 // Sprite Masks + Colour Indexes

	{ "w04801b032.u17",      0x400000, 0x60298536, 5 | BRF_SND },						 // Samples

//	{ "espgal_igs027a.bin",  0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code
};

STDROMPICKEXT(espgal, espgal, espgalBios);
STD_ROM_FN(espgal);

static int espgalInit()
{
	nCavePgm = 1;
	nPGMEnableIRQ4 = 1;

	pPgmInitCallback = pgm_decrypt_espgal;
	pPgmResetCallback = reset_ddp3;

	int ret = pgmInit();

	if (ret == 0) {
		install_protection_ket();
	}

	return ret;
}

struct BurnDriver BurnDrvEspgal = {
	"espgal", NULL, "pgm", "2003",
	"EspGaluda (V100, first revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM | HARDWARE_IGS_JAMMAPCB, GBF_VERSHOOT, 0,
	NULL, espgalRomInfo, espgalRomName, pgmInputInfo, jammaDIPInfo,
	espgalInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// DoDonPachi Dai-Ou-Jou (V101)

static struct BurnRomInfo ddp3RomDesc[] = {
	{ "ddp3_v101.u36",       0x200000, 0x195b5c1e, 1 | BRF_ESS | BRF_PRG },				 // 68K Code

	{ "t04401w064.u19",      0x800000, 0x3a95f19c, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04401w064.u7",       0x800000, 0xed229794, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04402w064.u8",       0x800000, 0xf7816273, 3 | BRF_GRA | BRF_NODUMP },

	{ "b04401w064.u1",       0x800000, 0x830aab7d, 4 | BRF_GRA | BRF_NODUMP },			 // Sprite Masks + Colour Indexes

	{ "m04401b032.u17",      0x400000, 0xa118560c, 5 | BRF_SND | BRF_NODUMP },			 // Samples

//	{ "ddp3_igs027a.bin",    0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ddp3_defaults.nv",    0x020000, 0x571e96c0, 0 | BRF_OPT },						 // NV RAM
};

STDROMPICKEXT(ddp3, ddp3, ddp3Bios);
STD_ROM_FN(ddp3);

static int ddp3Init()
{
	nCavePgm = 1;
	nPGMEnableIRQ4 = 1;

	pPgmInitCallback = pgm_decrypt_py2k2;
	pPgmResetCallback = reset_ddp3;

	int ret = pgmInit();

	if (ret == 0) {
		install_protection_ddp3();
	}

	return ret;
}

struct BurnDriver BurnDrvDdp3 = {
	"ddp3", NULL, "pgm", "2002",
	"DoDonPachi Dai-Ou-Jou (V101)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM, GBF_VERSHOOT, 0,
	NULL, ddp3RomInfo, ddp3RomName, pgmInputInfo, jammaDIPInfo,
	ddp3Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// DoDonPachi Dai-Ou-Jou (V100, second revision)

static struct BurnRomInfo ddp3aRomDesc[] = {
	{ "ddp3_d_d_1_0.u36",    0x200000, 0x5d3f85ba, 1 | BRF_ESS | BRF_PRG },				 // 68K Code

	{ "t04401w064.u19",      0x800000, 0x3a95f19c, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04401w064.u7",       0x800000, 0xed229794, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04402w064.u8",       0x800000, 0xf7816273, 3 | BRF_GRA | BRF_NODUMP },

	{ "b04401w064.u1",       0x800000, 0x830aab7d, 4 | BRF_GRA | BRF_NODUMP },			 // Sprite Masks + Colour Indexes

	{ "m04401b032.u17",      0x400000, 0xa118560c, 5 | BRF_SND | BRF_NODUMP },			 // Samples

//	{ "ddp3_igs027a.bin",    0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ddp3_defaults.nv",    0x020000, 0x571e96c0, 0 | BRF_OPT },						 // NV RAM
};

STDROMPICKEXT(ddp3a, ddp3a, ddp3Bios);
STD_ROM_FN(ddp3a);

struct BurnDriver BurnDrvDdp3a = {
	"ddp3a", "ddp3", "pgm", "2002",
	"DoDonPachi Dai-Ou-Jou (V100, second revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM, GBF_VERSHOOT, 0,
	NULL, ddp3aRomInfo, ddp3aRomName, pgmInputInfo, jammaDIPInfo,
	ddp3Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// DoDonPachi Dai-Ou-Jou (V100, first revision)

static struct BurnRomInfo ddp3bRomDesc[] = {
	{ "dd v100.bin",         0x200000, 0x7da0c1e4, 1 | BRF_ESS | BRF_PRG },				 // 68K Code

	{ "t04401w064.u19",      0x800000, 0x3a95f19c, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04401w064.u7",       0x800000, 0xed229794, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04402w064.u8",       0x800000, 0xf7816273, 3 | BRF_GRA | BRF_NODUMP },

	{ "b04401w064.u1",       0x800000, 0x830aab7d, 4 | BRF_GRA | BRF_NODUMP },			 // Sprite Masks + Colour Indexes

	{ "m04401b032.u17",      0x400000, 0xa118560c, 5 | BRF_SND | BRF_NODUMP },			 // Samples

//	{ "ddp3_igs027a.bin",    0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ddp3_defaults.nv",    0x020000, 0x571e96c0, 0 | BRF_OPT },						 // NV RAM
};

STDROMPICKEXT(ddp3b, ddp3b, ddp3Bios);
STD_ROM_FN(ddp3b);

struct BurnDriver BurnDrvDdp3b = {
	"ddp3b", "ddp3", "pgm", "2002",
	"DoDonPachi Dai-Ou-Jou (V100, first revision)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM, GBF_VERSHOOT, 0,
	NULL, ddp3bRomInfo, ddp3bRomName, pgmInputInfo, jammaDIPInfo,
	ddp3Init, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};


// DoDonPachi Dai-Ou-Jou (Black Label)

static struct BurnRomInfo ddp3blkRomDesc[] = {
	{ "ddb10.u45",           0x200000, 0x72b35510, 1 | BRF_ESS | BRF_PRG },				 // 68K Code

	{ "t04401w064.u19",      0x800000, 0x3a95f19c, 2 | BRF_GRA },						 // 32x32 BG Tiles

	{ "a04401w064.u7",       0x800000, 0xed229794, 3 | BRF_GRA },						 // Sprite Colour Data
	{ "a04402w064.u8",       0x800000, 0xf7816273, 3 | BRF_GRA | BRF_NODUMP },

	{ "b04401w064.u1",       0x800000, 0x830aab7d, 4 | BRF_GRA | BRF_NODUMP },			 // Sprite Masks + Colour Indexes

	{ "m04401b032.u17",      0x400000, 0xa118560c, 5 | BRF_SND | BRF_NODUMP },			 // Samples

//	{ "ddp3_igs027a.bin",    0x004000, 0x00000000, 7 | BRF_ESS | BRF_PRG | BRF_NODUMP }, // IGS027(A) Internal Code

	{ "ddp3blk_defaults.nv", 0x020000, 0xa1651904, 0 | BRF_OPT },						 // NV RAM (patch below instead)
};

STDROMPICKEXT(ddp3blk, ddp3blk, ddp3Bios);
STD_ROM_FN(ddp3blk);

static void ddp3blkPatchRAM()
{
	SekOpen(0);
	SekWriteLong(0x803800, 0x95804803); // cmpi.l  #$95804803,($803800).l
	SekWriteLong(0x803804, 0x23879065); // cmpi.l  #$23879065,($803804).l
	SekClose();

	// enable asic test
//	*((unsigned short*)(PGM68KROM + 0x03c0f4)) = 0x0012;
}

static int ddp3blkInit()
{
	nCavePgm = 1;
	nPGMEnableIRQ4 = 1;

	pPgmInitCallback = pgm_decrypt_py2k2;
	pPgmResetCallback = reset_ddp3;

	int ret = pgmInit();

	if (ret == 0) {
		ddp3blkPatchRAM();
		install_protection_ddp3();
	}

	return ret;
}

struct BurnDriver BurnDrvDdp3blk = {
	"ddp3blk", "ddp3", "pgm", "2002",
	"DoDonPachi Dai-Ou-Jou (Black Label)\0", NULL, "Cave", "PGM",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 4, HARDWARE_IGS_PGM, GBF_VERSHOOT, 0,
	NULL, ddp3blkRomInfo, ddp3blkRomName, pgmInputInfo, jammaDIPInfo,
	ddp3blkInit, pgmExit, pgmFrame, pgmDraw, pgmScan, 0, NULL, NULL, NULL, &nPgmPalRecalc,
	224, 448, 3, 4
};

