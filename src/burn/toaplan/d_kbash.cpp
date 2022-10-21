// Knuckle Bash

#include "toaplan.h"

static unsigned char ALIGN_DATA DrvButton[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvJoy2[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static unsigned char ALIGN_DATA DrvInput[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static unsigned char DrvReset = 0;
static unsigned char bDrawScreen;
static bool bVBlank;

static const unsigned char kbash_cmd_snd[128] =
{
/*00*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*08*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*10*/  0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
/*18*/  0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
/*20*/  0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
/*28*/  0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
/*30*/  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
/*38*/  0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41,
/*40*/  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
/*48*/  0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
/*50*/  0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
/*58*/  0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61,
/*60*/  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
/*68*/  0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x00,
/*70*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*78*/  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Rom information
static struct BurnRomInfo drvRomDesc[] = {
	{ "kbash01.bin",  0x080000, 0x2965F81D, BRF_ESS | BRF_PRG }, //  0 CPU #0 code

	{ "kbash03.bin",  0x200000, 0x32AD508B, BRF_GRA },			 //  1 GP9001 Tile data
	{ "kbash05.bin",  0x200000, 0xB84C90EB, BRF_GRA },			 //  2
	{ "kbash04.bin",  0x200000, 0xE493C077, BRF_GRA },			 //  3
	{ "kbash06.bin",  0x200000, 0x9084B50A, BRF_GRA },			 //  4

	{ "kbash02.bin",  0x008000, 0x4CD882A1, BRF_ESS | BRF_PRG }, //  5 Sound CPU
	{ "kbash07.bin",  0x040000, 0x3732318F, BRF_SND },			 //  6 ADPCM data
};


STD_ROM_PICK(drv);
STD_ROM_FN(drv);

static struct BurnInputInfo kbashInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvButton + 3,	"p1 coin"},
	{"P1 Start",	BIT_DIGITAL,	DrvButton + 5,	"p1 start"},

	{"P1 Up",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 up"},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 down"},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy1 + 2,	"p1 left"},
	{"P1 Right",	BIT_DIGITAL,	DrvJoy1 + 3,	"p1 right"},
	{"P1 Button 1",	BIT_DIGITAL,	DrvJoy1 + 4,	"p1 fire 1"},
	{"P1 Button 2",	BIT_DIGITAL,	DrvJoy1 + 5,	"p1 fire 2"},
	{"P1 Button 3",	BIT_DIGITAL,	DrvJoy1 + 6,	"p1 fire 3"},

	{"P2 Coin",		BIT_DIGITAL,	DrvButton + 4,	"p2 coin"},
	{"P2 Start",	BIT_DIGITAL,	DrvButton + 6,	"p2 start"},

	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p2 up"},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p2 down"},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p2 left"},
	{"P2 Right",	BIT_DIGITAL,	DrvJoy2 + 3,	"p2 right"},
	{"P2 Button 1",	BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"},
	{"P2 Button 2",	BIT_DIGITAL,	DrvJoy2 + 5,	"p2 fire 2"},
	{"P2 Button 3",	BIT_DIGITAL,	DrvJoy2 + 6,	"p2 fire 3"},

	{"Reset",		BIT_DIGITAL,	&DrvReset,		"reset"},
	{"Diagnostics",	BIT_DIGITAL,	DrvButton + 0,	"diag"},
	{"Dip A",		BIT_DIPSWITCH,	DrvInput + 3,	"dip"},
	{"Dip B",		BIT_DIPSWITCH,	DrvInput + 4,	"dip"},
	{"Dip C",		BIT_DIPSWITCH,	DrvInput + 5,	"dip"},
};

STDINPUTINFO(kbash);

static struct BurnDIPInfo kbashDIPList[] = {
	// Defaults
	{0x14,	0xFF, 0xFF,	0x00, NULL},
	{0x15,	0xFF, 0xFF,	0x00, NULL},
	{0x16,	0xFF, 0x0F,	0x00, NULL},

	// DIP 1
	{0,		0xFE, 0,	2,	  NULL},
	{0x14,	0x01, 0x01,	0x00, "Discount off"},
	{0x14,	0x01, 0x01,	0x01, "Continue discount"},
	{0,		0xFE, 0,	2,	  NULL},
	{0x14,	0x01, 0x02,	0x00, "Normal screen"},
	{0x14,	0x01, 0x02,	0x02, "Invert screen"},
	{0,		0xFE, 0,	2,	  NULL},
	{0x14,	0x01, 0x04,	0x00, "Normal mode"},
	{0x14,	0x01, 0x04,	0x04, "Test mode"},
	{0,		0xFE, 0,	2,	  "Advertise sound"},
	{0x14,	0x01, 0x08,	0x00, "On"},
	{0x14,	0x01, 0x08,	0x08, "Off"},
	{0,		0xFE, 0,	4,	  "Coin A"},
	{0x14,	0x01, 0x30,	0x00, "1 coin 1 play"},
	{0x14,	0x01, 0x30,	0x10, "1 coin 2 plays"},
	{0x14,	0x01, 0x30,	0x20, "2 coins 1 play"},
	{0x14,	0x01, 0x30,	0x30, "2 coins 3 plays"},
	{0,		0xFE, 0,	4,	  "Coin B"},
	{0x14,	0x01, 0xC0,	0x00, "1 coin 1 play"},
	{0x14,	0x01, 0xC0,	0x40, "1 coin 2 plays"},
	{0x14,	0x01, 0xC0,	0x80, "2 coins 1 play"},
	{0x14,	0x01, 0xC0,	0xC0, "2 coins 3 plays"},

	// DIP 2
	{0,		0xFE, 0,	4,	  "Game difficulty"},
	{0x15,	0x01, 0x03,	0x00, "B"},
	{0x15,	0x01, 0x03,	0x01, "A"},
	{0x15,	0x01, 0x03,	0x02, "C"},
	{0x15,	0x01, 0x03,	0x03, "D"},
	{0,		0xFE, 0,	4,	  "Extend"},
	{0x15,	0x01, 0x0C,	0x00, "100000, 400000"},
	{0x15,	0x01, 0x0C,	0x04, "100000 only"},
	{0x15,	0x01, 0x0C,	0x08, "200000 only"},
	{0x15,	0x01, 0x0C,	0x0C, "No extend"},
	{0,		0xFE, 0,	4,	  "Hero counts"},
	{0x15,	0x01, 0x30,	0x00, "2"},
	{0x15,	0x01, 0x30,	0x01, "5"},
	{0x15,	0x01, 0x30,	0x02, "4"},
	{0x15,	0x01, 0x30,	0x03, "2"},
	{0,		0xFE, 0,	2,	  NULL},
    {0x15,	0x01, 0x40,	0x00, "Normal game"},
    {0x15,	0x01, 0x40,	0x40, "no-death, stop mode"},
	{0,		0xFE, 0,	2,	  "Continue play"},
    {0x15,	0x01, 0x80,	0x00, "On"},
	{0x15,	0x01, 0x80,	0x80, "Off"},

	// DIP 3
	{0,		0xFE, 0,	7,	  "For"},
	{0x16,	0x01, 0x0F,	0x00, "Japan"},
	{0x16,	0x01, 0x0F,	0x01, "USA"},
	{0x16,	0x01, 0x0F,	0x02, "Europe"},
	{0x16,	0x01, 0x0F,	0x03, "Korea"},
	{0x16,	0x01, 0x0F,	0x04, "Hong Kong"},
	{0x16,	0x01, 0x0F,	0x05, "Taiwan"},
	{0x16,	0x01, 0x0F,	0x06, "Asia"},
	{0x16,	0x01, 0x0F,	0x07, "U.S.A."},
	{0x16,	0x01, 0x0F,	0x08, "Japan"},
	{0x16,	0x01, 0x0F,	0x09, "U.S.A."},
	{0x16,	0x01, 0x0F,	0x0A, "Europe"},
	{0x16,	0x01, 0x0F,	0x0B, "Korea"},
	{0x16,	0x01, 0x0F,	0x0C, "Hong Kong"},
	{0x16,	0x01, 0x0F,	0x0D, "Taiwan"},
	{0x16,	0x01, 0x0F,	0x0E, "Asia"},
	{0x16,	0x01, 0x0F,	0x0F, ""},
};

STDDIPINFO(kbash);

static unsigned char *Mem = NULL, *MemEnd = NULL;
static unsigned char *RamStart, *RamEnd;
static unsigned char *Rom01;
static unsigned char *Ram01, *RamPal;

static int nColCount = 0x0800;

// This routine is called first to determine how much memory is needed (MemEnd-(unsigned char *)0),
// and then afterwards to set up all the pointers
static int MemIndex()
{
	unsigned char *Next; Next = Mem;
	Rom01		= Next; Next += 0x080000;		// 68000 ROM
	GP9001ROM[0]= Next; Next += nGP9001ROMSize[0];	// GP9001 tile data
	MSM6295ROM	= Next; Next += 0x040000;			// MSM6295 ADPCM data
	RamStart	= Next;
	Ram01		= Next; Next += 0x004000;		// CPU #0 work RAM
	RamPal		= Next; Next += 0x001000;		// palette
	GP9001RAM[0]= Next; Next += 0x004000;
	GP9001Reg[0]= (unsigned short*)Next; Next += 0x0100 * sizeof(short);
	RamEnd		= Next;
	ToaPalette	= (unsigned int *)Next; Next += nColCount * sizeof(unsigned int);
	MemEnd		= Next;

	return 0;
}

// Scan ram
static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x020997;
	}
	if (nAction & ACB_VOLATILE) {		// Scan volatile ram
		memset(&ba, 0, sizeof(ba));
    	ba.Data		= RamStart;
		ba.nLen		= RamEnd-RamStart;
		ba.szName	= "All Ram";
		BurnAcb(&ba);

		SekScan(nAction);				// scan 68000 states

		ToaScanGP9001(nAction, pnMin);

		MSM6295Scan(0, nAction);
//		BurnYM2151Scan(nAction);
	}

	return 0;
}

static int LoadRoms()
{
	// Load 68000 ROM
	BurnLoadRom(Rom01, 0, 1);

	// Load GP9001 tile data
	ToaLoadGP9001Tiles(GP9001ROM[0], 1, 4, nGP9001ROMSize[0]);

	// Load MSM6295 ADPCM data
	BurnLoadRom(MSM6295ROM, 6, 1);

	return 0;
}

inline static unsigned int ZX80Status()
{
	return 0xFF;
}

unsigned char __fastcall kbashReadByte(unsigned int sekAddress)
{
	switch (sekAddress) {
		case 0x208011:								// Player 1 inputs
			return DrvInput[0];
		case 0x208015:								// Player 2 inputs
			return DrvInput[1];
		case 0x208019:								// Other inputs
			return DrvInput[2];

		case 0x200001:
		 	return ZX80Status();

		case 0x200005:								// Dipswitch 1
			return DrvInput[3];
		case 0x200007:			   					// Dipswitch 2
			return DrvInput[4];
		case 0x200009:								// Dipswitch 3 - Territory
			return DrvInput[5];

		case 0x30000D:								// VBlank
			return ToaVBlankRegister();

//		default:
//			printf("Attempt to read byte value of location %x\n", sekAddress);
	}

	return 0;
}

unsigned short __fastcall kbashReadWord(unsigned int sekAddress)
{
	switch (sekAddress) {

		case 0x208010:								// Player 1 inputs
			return DrvInput[0];
		case 0x208014:								// Player 2 inputs
			return DrvInput[1];
		case 0x208018:								// Other inputs
			return DrvInput[2];

		case 0x200000:
		 	return ZX80Status();

		case 0x200004:								// Dipswitch 1
			return DrvInput[3];
		case 0x200006:								// Dipswitch 2
			return DrvInput[4];
		case 0x200008:								// Dipswitch 3 - Territory
			return DrvInput[5];

		case 0x300004:
			return ToaGP9001ReadRAM_Hi(0);
		case 0x300006:
			return ToaGP9001ReadRAM_Lo(0);

		case 0x30000C:
			return ToaVBlankRegister();

		case 0x700000:
			return ToaScanlineRegister();

//		default:
//			printf("Attempt to read word value of location %x\n", sekAddress);
	}

	return 0;
}

void __fastcall kbashWriteByte(unsigned int sekAddress, unsigned char byteValue)
{
	switch (sekAddress) {
		case 0x200001:
			if (byteValue == 0) {
				MSM6295Command(0, 0x78);		// Stop playing effects
			}
			else if((byteValue > 0) && (byteValue < 128)) {
				play_oki_sound(kbash_cmd_snd[byteValue], byteValue);
			}
			break;
		
//		default:
//			printf("Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
	}
}

void __fastcall kbashWriteWord(unsigned int sekAddress, unsigned short wordValue)
{
	switch (sekAddress) {
// 		case 0x20001C:
//			break;

		case 0x300000:								// Set GP9001 VRAM address-pointer
			ToaGP9001SetRAMPointer(wordValue);
			break;

		case 0x300004:
			ToaGP9001WriteRAM(wordValue, 0);
			break;
		case 0x300006:
			ToaGP9001WriteRAM(wordValue, 0);
			break;

		case 0x300008:
			ToaGP9001SelectRegister(wordValue);
			break;

		case 0x30000C:
			ToaGP9001WriteRegister(wordValue);
			break;

//		default:
//			printf("Attempt to write word value %x to location %x\n", wordValue, sekAddress);
	}
}

static int DrvDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();

	MSM6295Reset(0);
//	BurnYM2151Reset();
	
	return 0;
}

static int DrvInit()
{
	int nLen;

#ifdef DRIVER_ROTATION
	bToaRotateScreen = false;
#endif

	nGP9001ROMSize[0] = 0x800000;

	// Find out how much memory is needed
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (unsigned char *)0;
	if ((Mem = (unsigned char *)malloc(nLen)) == NULL) {
		return 1;
	}
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();													// Index the allocated memory

	// Load the roms into memory
	if (LoadRoms()) {
		return 1;
	}

	{
		SekInit(0, 0x68000);									// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,		0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,		0x100000, 0x103FFF, SM_RAM);
		SekMapMemory(RamPal,	0x400000, 0x400FFF, SM_RAM);	// Palette RAM

		SekSetReadWordHandler(0, kbashReadWord);
		SekSetReadByteHandler(0, kbashReadByte);
		SekSetWriteWordHandler(0, kbashWriteWord);
		SekSetWriteByteHandler(0, kbashWriteByte);

		SekClose();
	}

	nSpriteYOffset = 0x0011;

	nLayer0XOffset = -0x01D6;
	nLayer1XOffset = -0x01D8;
	nLayer2XOffset = -0x01DA;

	ToaInitGP9001();

//	BurnYM2151Init(27000000 / 8, 100.0);
	MSM6295Init(0, 32000000 / 32 / 132, 100.0, 0);

	nToaPalLen = nColCount;
	ToaPalSrc = RamPal;
	ToaPalInit();

	bDrawScreen = true;

	DrvDoReset();			// Reset machine
	return 0;
}

static int DrvExit()
{
	MSM6295Exit(0);
//	BurnYM2151Exit();
	
	ToaPalExit();

	ToaExitGP9001();
	SekExit();				// Deallocate 68000s

	// Deallocate all used memory
	free(Mem);
	Mem = NULL;

	return 0;
}

static int DrvDraw()
{
	ToaClearScreen();

	if (bDrawScreen) {
		ToaGetBitmap();
		ToaRenderGP9001();					// Render GP9001 graphics
	}

	ToaPalUpdate();							// Update the palette

	return 0;
}

inline static int CheckSleep(int)
{
	return 0;
}

static int DrvFrame()
{
	int nInterleave = 4;

	if (DrvReset) {														// Reset machine
		DrvDoReset();
	}

	// Compile digital inputs
	DrvInput[0] = 0x00;													// Buttons
	DrvInput[1] = 0x00;													// Player 1
	DrvInput[2] = 0x00;													// Player 2
	for (int i = 0; i < 8; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
		DrvInput[2] |= (DrvButton[i] & 1) << i;
	}
	ToaClearOpposites(&DrvInput[0]);
	ToaClearOpposites(&DrvInput[1]);

	SekNewFrame();

//	nCyclesTotal[0] = (int)((long long)16000000 * nBurnCPUSpeedAdjust / (0x0100 * 60));
	nCyclesTotal[0] = (int)((long long)16000000 / 60);
	nCyclesDone[0] = 0;

	SekSetCyclesScanline(nCyclesTotal[0] / 262);
	nToaCyclesDisplayStart = nCyclesTotal[0] - ((nCyclesTotal[0] * (TOA_VBLANK_LINES + 240)) / 262);
	nToaCyclesVBlankStart = nCyclesTotal[0] - ((nCyclesTotal[0] * TOA_VBLANK_LINES) / 262);
	bVBlank = false;

	int nSoundBufferPos = 0;

	SekOpen(0);

	for (int i = 1; i <= nInterleave; i++) {
    	int nCurrentCPU;
		int nNext;

		// Run 68000
		nCurrentCPU = 0;
		nNext = i * nCyclesTotal[nCurrentCPU] / nInterleave;


		// Trigger VBlank interrupt
		if (!bVBlank && (nNext > nToaCyclesVBlankStart)) {
			if (nCyclesDone[nCurrentCPU] < nToaCyclesVBlankStart) {
				nCyclesSegment = nToaCyclesVBlankStart - nCyclesDone[nCurrentCPU];
				nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
			}

			bVBlank = true;

			ToaBufferGP9001Sprites();

			// Trigger VBlank interrupt
			SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);
		}

		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		if (bVBlank || (!CheckSleep(nCurrentCPU))) {					// See if this CPU is busywaiting
			nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		} else {
			nCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
		}

		// Render sound segment
		if (pBurnSoundOut) {
			int nSegmentLength = (nBurnSoundLen * i / nInterleave) - nSoundBufferPos;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
//			BurnYM2151Render(pSoundBuf, nSegmentLength);
			MSM6295Render(0, pSoundBuf, nSegmentLength);
			nSoundBufferPos += nSegmentLength;
		}
	}

	SekClose();

	{
		// Make sure the buffer is entirely filled.
		if (pBurnSoundOut) {
			int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
			short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			if (nSegmentLength) {
//				BurnYM2151Render(pSoundBuf, nSegmentLength);
				MSM6295Render(0, pSoundBuf, nSegmentLength);
			}
		}
	}

	if (pBurnDraw) {
		DrvDraw();												// Draw screen if needed
	}

	return 0;
}

struct BurnDriver BurnDrvKBash = {
	"kbash", NULL, NULL, "1993",
	"Knuckle Bash\0", "No sound (sound MCU not emulated)", "Toaplan", "Toaplan GP9001 based",
	L"Knuckle Bash\0Knuckle Bash \u30CA\u30C3\u30AF\u30EB\u30D0\u30C3\u30B7\u30E5\0", NULL, NULL, NULL,
	1, 2, HARDWARE_TOAPLAN_68K_Zx80, GBF_SCRFIGHT, 0,
	NULL, drvRomInfo, drvRomName, kbashInputInfo,kbashDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan, 0, NULL, NULL, NULL, &ToaRecalcPalette,
	320, 240, 4, 3
};

