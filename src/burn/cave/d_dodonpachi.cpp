// DoDonpachi

#include "cave.h"
#include "ymz280b.h"

static UINT8  ALIGN_DATA DrvJoy1[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT8  ALIGN_DATA DrvJoy2[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT16 ALIGN_DATA DrvInput[2] = { 0x0000, 0x0000 };

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;
static UINT8 *Rom01;
static UINT8 *Ram01;

static UINT8 DrvReset = 0;
static UINT8 bDrawScreen;
static bool bVBlank;

static int nSpeedhack;

static INT8 nVideoIRQ;
static INT8 nSoundIRQ;
static INT8 nUnknownIRQ;

static INT8 nIRQPending;

static INT8 nCurrentCPU;
static int  nCyclesDone;

static UINT8 bSpriteUpdate;

static struct BurnInputInfo ALIGN_DATA ddonpachInputList[] = {
  {"P1 Coin",      BIT_DIGITAL,  DrvJoy1 + 8,  "p1 coin"   },
  {"P1 Start",     BIT_DIGITAL,  DrvJoy1 + 7,  "p1 start"  },

  {"P1 Up",        BIT_DIGITAL,  DrvJoy1 + 0,  "p1 up"     },
  {"P1 Down",      BIT_DIGITAL,  DrvJoy1 + 1,  "p1 down"   },
  {"P1 Left",      BIT_DIGITAL,  DrvJoy1 + 2,  "p1 left"   },
  {"P1 Right",     BIT_DIGITAL,  DrvJoy1 + 3,  "p1 right"  },
  {"P1 Button 1",  BIT_DIGITAL,  DrvJoy1 + 4,  "p1 fire 1" },
  {"P1 Button 2",  BIT_DIGITAL,  DrvJoy1 + 5,  "p1 fire 2" },
  {"P1 Button 3",  BIT_DIGITAL,  DrvJoy1 + 6,  "p1 fire 3" },

  {"P2 Coin",      BIT_DIGITAL,  DrvJoy2 + 8,  "p2 coin"   },
  {"P2 Start",     BIT_DIGITAL,  DrvJoy2 + 7,  "p2 start"  },

  {"P2 Up",        BIT_DIGITAL,  DrvJoy2 + 0,  "p2 up"     },
  {"P2 Down",      BIT_DIGITAL,  DrvJoy2 + 1,  "p2 down"   },
  {"P2 Left",      BIT_DIGITAL,  DrvJoy2 + 2,  "p2 left"   },
  {"P2 Right",     BIT_DIGITAL,  DrvJoy2 + 3,  "p2 right"  },
  {"P2 Button 1",  BIT_DIGITAL,  DrvJoy2 + 4,  "p2 fire 1" },
  {"P2 Button 2",  BIT_DIGITAL,  DrvJoy2 + 5,  "p2 fire 2" },
  {"P2 Button 3",  BIT_DIGITAL,  DrvJoy2 + 6,  "p2 fire 3" },

  {"Reset",        BIT_DIGITAL,  &DrvReset,    "reset"     },
  {"Diagnostics",  BIT_DIGITAL,  DrvJoy1 + 9,  "diag"      },
  {"Service",      BIT_DIGITAL,  DrvJoy2 + 9,  "service"   },
};

STDINPUTINFO(ddonpach);

static void UpdateIRQStatus()
{
	nIRQPending = ((nVideoIRQ == 0) || (nSoundIRQ == 0) || (nUnknownIRQ == 0));
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

UINT8 __fastcall ddonpachReadByte(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x300002:
		case 0x300003: {
			return YMZ280BReadStatus();
		}

		case 0x800000:
		case 0x800001: {
			UINT8 nRet = 6 | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0x800002:
		case 0x800003:
		case 0x800004:
		case 0x800005:
		case 0x800006:
		case 0x800007: {
			UINT8 nRet = 6 | nVideoIRQ;
			return nRet;
		}

		case 0xD00000:
			return (DrvInput[0] >> 8) ^ 0xFF;
		case 0xD00001:
			return (DrvInput[0] & 0xFF) ^ 0xFF;
		case 0xD00002:
			return ((DrvInput[1] >> 8) ^ 0xF7) | (EEPROMRead() << 3);
		case 0xD00003:
			return (DrvInput[1] & 0xFF) ^ 0xFF;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read byte value of location %x\n", sekAddress);
//		}
	}

	return 0;
}

UINT16 __fastcall ddonpachReadWord(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x300002: {
			return YMZ280BReadStatus();
		}

		case 0x800000: {
			UINT16 nRet = 6 | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0x800002:
		case 0x800004:
		case 0x800006: {
			UINT16 nRet = 6 | nVideoIRQ;
			return nRet;
		}

		case 0xD00000:
			return (DrvInput[0] ^ 0xFFFF);
		case 0xD00002:
			return (DrvInput[1] ^ 0xF7FF) | (EEPROMRead() << 11);

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read word value of location %x\n", sekAddress);
//		}
	}

	return 0;
}

void __fastcall ddonpachWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	switch (sekAddress) {
		case 0x300001:
			YMZ280BSelectRegister(byteValue);
			return;
		case 0x300003:
			YMZ280BWriteRegister(byteValue);
			return;

		case 0xE00000:
			EEPROMWrite(byteValue & 0x04, byteValue & 0x02, byteValue & 0x08);
			return;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
//		}
	}
}

void __fastcall ddonpachWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	switch (sekAddress) {
		case 0x300000:
			YMZ280BSelectRegister(wordValue & 0xFF);
			return;
		case 0x300002:
			YMZ280BWriteRegister(wordValue & 0xFF);
			return;

		case 0x800000:
			nCaveXOffset = wordValue;
			return;
		case 0x800002:
			nCaveYOffset = wordValue;
			return;
		case 0x800008:
			bSpriteUpdate = 1;
			nCaveSpriteBank = wordValue;
			return;

		case 0x900000:
			CaveTileReg[0][0] = wordValue;
			return;
		case 0x900002:
			CaveTileReg[0][1] = wordValue;
			return;
		case 0x900004:
			CaveTileReg[0][2] = wordValue;
			return;

		case 0xA00000:
			CaveTileReg[1][0] = wordValue;
			return;
		case 0xA00002:
			CaveTileReg[1][1] = wordValue;
			return;
		case 0xA00004:
			CaveTileReg[1][2] = wordValue;
			return;

		case 0xB00000:
			CaveTileReg[2][0] = wordValue;
			return;
		case 0xB00002:
			CaveTileReg[2][1] = wordValue;
			return;
		case 0xB00004:
			CaveTileReg[2][2] = wordValue;
			return;

		case 0xE00000:
			wordValue >>= 8;
			EEPROMWrite(wordValue & 0x04, wordValue & 0x02, wordValue & 0x08);
			return;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write word value %x to location %x\n", wordValue, sekAddress);
//		}
	}
}

void __fastcall ddonpachWriteByteSprite(UINT32 sekAddress, UINT8 byteValue)
{
	bSpriteUpdate = 1;
	CaveSpriteRAM[(sekAddress ^ 0x01) & 0xFFFF] = byteValue;
}

void __fastcall ddonpachWriteWordSprite(UINT32 sekAddress, UINT16 wordValue)
{
	bSpriteUpdate = 1;
	((UINT16 *)CaveSpriteRAM)[(sekAddress & 0xFFFF) >> 1] = wordValue;
}

void __fastcall ddonpachWriteBytePalette(UINT32 sekAddress, UINT8 byteValue)
{
	CavePalWriteByte(sekAddress & 0xFFFF, byteValue);
}

void __fastcall ddonpachWriteWordPalette(UINT32 sekAddress, UINT16 wordValue)
{
	CavePalWriteWord(sekAddress & 0xFFFF, wordValue);
}

static void TriggerSoundIRQ(int nStatus)
{
	nSoundIRQ = nStatus ^ 1;
	UpdateIRQStatus();

	if (nIRQPending && (nCurrentCPU != 0)) {
		nCyclesDone += SekRun(0x0400);
	}
}

static int DrvExit()
{
	YMZ280BExit();

	EEPROMExit();

	CaveTileExit();
	CaveSpriteExit();
    CavePalExit();

	SekExit();				// Deallocate 68000s

	// Deallocate all used memory
	free(Mem);
	Mem = NULL;

	destroyUniCache();

	return 0;
}

static INT8 DrvDoReset()
{
	SekOpen(0);
	SekReset();
	SekClose();

	EEPROMReset();

	nVideoIRQ = 1;
	nSoundIRQ = 1;
	nUnknownIRQ = 1;

	nIRQPending = 0;

	YMZ280BReset();

	return 0;
}

static int DrvDraw()
{
	CavePalUpdate8Bit(0, 128);				// Update the palette
	CaveClearScreen(CavePalette[0x7F00]);

	if (bDrawScreen) {
//		CaveGetBitmap();

		CaveTileRender(1);					// Render tiles
	}

	return 0;
}

inline static INT8 CheckSleep(int)
{
#if 1 && defined USE_SPEEDHACKS
	int nCurrentPC = SekGetPC(-1) - nSpeedhack;

	if (!nIRQPending && (nCurrentPC >= 0) && (nCurrentPC <= 12)) {
		return 1;
	}
#endif

	return 0;
}

#define CAVE_INTERLEAVE  (8)

#ifndef BUILD_PSP
	#define M68K_CYCLES_PER_FRAME  ((int)((INT64)16000000 * nBurnCPUSpeedAdjust / (0x0100 * CAVE_REFRESHRATE)))
#else
	#define M68K_CYCLES_PER_FRAME  ((int)(16000000 / CAVE_REFRESHRATE))
#endif

#define M68K_CYCLES_VBLANK (M68K_CYCLES_PER_FRAME - (int)((M68K_CYCLES_PER_FRAME * CAVE_VBLANK_LINES) / 271.5))

#define M68K_CYLECS_PER_INTER  (M68K_CYCLES_PER_FRAME / CAVE_INTERLEAVE)

static int DrvFrame()
{
	int nCyclesVBlank;
	int nCyclesSegment;
	
	if (DrvReset) {														// Reset machine
		DrvDoReset();
	}
	
	// Compile digital inputs
	DrvInput[0] = 0x0000;  												// Player 1
	DrvInput[1] = 0x0000;  												// Player 2
	for (INT8 i = 0; i < 10; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
	}
	CaveClearOpposites(&DrvInput[0]);
	CaveClearOpposites(&DrvInput[1]);
	
	SekNewFrame();
	
	nCyclesDone = 0;
	
	nCyclesVBlank = M68K_CYCLES_VBLANK;
	bVBlank = false;
	
	SekOpen(0);
	
	for (INT8 i = 1; i <= CAVE_INTERLEAVE; i++) {
		int nNext;
		
		// Run 68000
		nCurrentCPU = 0;
		nNext = i * M68K_CYLECS_PER_INTER;
		
		// See if we need to trigger the VBlank interrupt
		if (!bVBlank && (nNext > nCyclesVBlank)) {
			if (nCyclesDone < nCyclesVBlank) {
				nCyclesSegment = nCyclesVBlank - nCyclesDone;
				if (!CheckSleep(nCurrentCPU)) {							// See if this CPU is busywaiting
					nCyclesDone += SekRun(nCyclesSegment);
				} else {
					nCyclesDone += SekIdle(nCyclesSegment);
				}
			}
			
			if (pBurnDraw != NULL) {
				DrvDraw();												// Draw screen if needed
			}
			
			if (bSpriteUpdate) {
				CaveSpriteBuffer();
				bSpriteUpdate = 0;
			}
			
			if (pBurnSoundOut) {
				YMZ280BRender(pBurnSoundOut, nBurnSoundLen);
			}
			
			bVBlank = true;
			nVideoIRQ = 0;
			UpdateIRQStatus();
		}
		
		nCyclesSegment = nNext - nCyclesDone;
		if (!CheckSleep(nCurrentCPU)) {									// See if this CPU is busywaiting
			nCyclesDone += SekRun(nCyclesSegment);
		} else {
			nCyclesDone += SekIdle(nCyclesSegment);
		}
		
		nCurrentCPU = -1;
	}
	
	SekClose();
	
	return 0;
}

// This routine is called first to determine how much memory is needed (MemEnd-(UINT8 *)0),
// and then afterwards to set up all the pointers
static INT8 MemIndex()
{
	UINT8 *Next; Next = Mem;
	Rom01			= Next; Next += 0x100000;		// 68K program
/*
	CaveSpriteROM	= Next; Next += 0x1000000;
	CaveTileROM[0]	= Next; Next += 0x400000;		// Tile layer 0
	CaveTileROM[1]	= Next; Next += 0x400000;		// Tile layer 1
	CaveTileROM[2]	= Next; Next += 0x200000;		// Tile layer 2
*/
	YMZ280BROM		= Next; Next += 0x400000;

	RamStart		= Next;
	Ram01			= Next; Next += 0x010000;		// CPU #0 work RAM
	CaveTileRAM[0]	= Next; Next += 0x008000;
	CaveTileRAM[1]	= Next; Next += 0x008000;
	CaveTileRAM[2]	= Next; Next += 0x008000;
	CaveSpriteRAM	= Next; Next += 0x010000;
	CavePalSrc		= Next; Next += 0x010000;		// palette
	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}

static INT8 LoadRoms()
{
	// Load 68000 ROM
//	BurnLoadRom(Rom01 + 0, 1, 2);
//	BurnLoadRom(Rom01 + 1, 0, 2);
	{
		UINT32 block1Head, block2Head;
		block1Head = 0x00000;
		block2Head = 0x80000;
		
		BurnLoadRom(YMZ280BROM + block1Head, 1, 1);
		BurnLoadRom(YMZ280BROM + block2Head, 0, 1);
		
		for (int i = 0; i < 0x100000; i = i + 2, block1Head++, block2Head++) {
			Rom01[i + 0] = YMZ280BROM[block1Head];
			Rom01[i + 1] = YMZ280BROM[block2Head];
		}
		memset(YMZ280BROM, 0, 0x400000);
	}
/*
	BurnLoadRom(CaveSpriteROM + 0x000000, 2, 1);
	BurnLoadRom(CaveSpriteROM + 0x200000, 3, 1);
	BurnLoadRom(CaveSpriteROM + 0x400000, 4, 1);
	BurnLoadRom(CaveSpriteROM + 0x600000, 5, 1);
	BurnByteswap(CaveSpriteROM, 0x800000);
	CaveNibbleSwap2(CaveSpriteROM, 0x800000);

	BurnLoadRom(CaveTileROM[0], 6, 1);
	CaveNibbleSwap2(CaveTileROM[0], 0x200000);
	BurnLoadRom(CaveTileROM[1], 7, 1);
	CaveNibbleSwap2(CaveTileROM[1], 0x200000);
	BurnLoadRom(CaveTileROM[2], 8, 1);
	CaveNibbleSwap4(CaveTileROM[2], 0x200000);
*/
	// Load YMZ280B data
	BurnLoadRom(YMZ280BROM + 0x000000,  9, 1);
	BurnLoadRom(YMZ280BROM + 0x200000, 10, 1);

	return 0;
}

// Scan ram
static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x020902;
	}

	EEPROMScan(nAction, pnMin);			// Scan EEPROM

	if (nAction & ACB_VOLATILE) {		// Scan volatile ram

		memset(&ba, 0, sizeof(ba));
    	ba.Data		= RamStart;
		ba.nLen		= RamEnd - RamStart;
		ba.szName	= "RAM";
		BurnAcb(&ba);

		SekScan(nAction);				// scan 68000 states

		YMZ280BScan();

		SCAN_VAR(nVideoIRQ);
		SCAN_VAR(nSoundIRQ);
		SCAN_VAR(nUnknownIRQ);
		SCAN_VAR(bVBlank);

		CaveScanGraphics();

		SCAN_VAR(DrvInput);

		if (nAction & ACB_WRITE) {
			CaveSpriteBuffer();
			CaveRecalcPalette = 1;
		}
	}

	return 0;
}

static const UINT8 ALIGN_DATA ddonpach_default_eeprom[0x80] =
{
	0x00, 0x0C, 0xFF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static int DrvInit()
{
	int nLen;

	BurnSetRefreshRate(CAVE_REFRESHRATE);
	
	// Find out how much memory is needed
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (UINT8 *)0;
	if ((Mem = (UINT8 *)memalign(4, nLen)) == NULL) {
		return 1;
	}
	
	initUniCache(0x1A00000);
	
	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x1000000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x400000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x400000;
	
	if (needCreateCache) {
		// Sprite
		for (INT8 i = 2; i <= 5; i++) {
			memset(Mem, 0, nLen);
			BurnLoadRom(Mem, i, 1);
			BurnByteswap(Mem, 0x200000);
			CaveNibbleSwap2(Mem, 0x200000);
			sceIoWrite(cacheFile, Mem, 0x400000);
		}
		
		// Tile Layer 0, 1
		for (INT8 i = 6; i <= 7; i++) {
			memset(Mem, 0, nLen);
			BurnLoadRom(Mem, i, 1);
			CaveNibbleSwap2(Mem, 0x200000);
			sceIoWrite(cacheFile, Mem, 0x400000);
		}
		
		// Tile Layer 2
		memset(Mem, 0, nLen);
		BurnLoadRom(Mem, 8, 1);
		CaveNibbleSwap4(Mem, 0x200000);
		sceIoWrite(cacheFile, Mem, 0x200000);
		
		sceIoClose(cacheFile);
		cacheFile = sceIoOpen(filePathName, PSP_O_RDONLY, 0777);
	}

	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();													// Index the allocated memory

	// Load the roms into memory
	if (LoadRoms()) {
		return 1;
	}

	EEPROMInit(&eeprom_interface_93C46);
	EEPROMLoadRom(11);
	if (!EEPROMAvailable()) {
		EEPROMFill(ddonpach_default_eeprom, 0, sizeof(ddonpach_default_eeprom));
	}

	initCacheStructure(0.90);

	{
		SekInit(0, 0x68000);													// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,						0x000000, 0x0FFFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,						0x100000, 0x10FFFF, SM_RAM);

		SekMapMemory(CaveSpriteRAM,				0x400000, 0x40FFFF, SM_ROM);
		SekMapHandler(1,						0x400000, 0x40FFFF, SM_WRITE);

		SekMapMemory(CaveTileRAM[0],			0x500000, 0x507FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],			0x600000, 0x607FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2] + 0x4000,	0x700000, 0x703FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2] + 0x4000,	0x704000, 0x707FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2] + 0x4000,	0x708000, 0x70BFFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2] + 0x4000,	0x70C000, 0x70FFFF, SM_RAM);

		SekMapMemory(CavePalSrc,				0xC00000, 0xC0FFFF, SM_ROM);	// Palette RAM (write goes through handler)
		SekMapHandler(2,						0xC00000, 0xC0FFFF, SM_WRITE);	//

		SekSetReadWordHandler (0, ddonpachReadWord);
		SekSetReadByteHandler (0, ddonpachReadByte);
		SekSetWriteWordHandler(0, ddonpachWriteWord);
		SekSetWriteByteHandler(0, ddonpachWriteByte);

		SekSetWriteWordHandler(1, ddonpachWriteWordSprite);
		SekSetWriteByteHandler(1, ddonpachWriteByteSprite);

		SekSetWriteWordHandler(2, ddonpachWriteWordPalette);
		SekSetWriteByteHandler(2, ddonpachWriteBytePalette);

		SekClose();
	}

	nCaveRowModeOffset = 1;

	CavePalInit(0x8000);
	CaveTileInit();
	CaveSpriteInit(0, 0x1000000);
	CaveTileInitLayer(0, 0x400000, 8, 0x4000);
	CaveTileInitLayer(1, 0x400000, 8, 0x4000);
	CaveTileInitLayer(2, 0x200000, 8, 0x4000);

	YMZ280BInit(16934400, &TriggerSoundIRQ, 3);

	bDrawScreen = true;

	// US version:    0x0571AC - 0x0571B8
	// Japan version: 0x056DF4 - 0x056E00

	nSpeedhack = (strcmp(BurnDrvGetTextA(DRV_NAME), "ddonpach") == 0) ? 0x0571AC : 0x056DF4;

#if defined FBA_DEBUG && defined USE_SPEEDHACKS
	bprintf(PRINT_IMPORTANT, _T("  * Using speed-hacks (detecting idle loops).\n"));
#endif

	DrvDoReset(); // Reset machine

	return 0;
}

// Rom information
static struct BurnRomInfo ddonpachRomDesc[] = {
	{ "b1.u27",            0x080000, 0xB5CDC8D3, BRF_ESS | BRF_PRG },	//  0 CPU #0 code
	{ "b2.u26",            0x080000, 0x6BBB063A, BRF_ESS | BRF_PRG },	//  1

	{ "u50.bin",           0x200000, 0x14B260EC, BRF_GRA },				//  2 Sprite data
	{ "u51.bin",           0x200000, 0xE7BA8CCE, BRF_GRA },				//  3
	{ "u52.bin",           0x200000, 0x02492EE0, BRF_GRA },				//  4
	{ "u53.bin",           0x200000, 0xCB4C10F0, BRF_GRA },				//  5

	{ "u60.bin",           0x200000, 0x903096A7, BRF_GRA },				//  6 Layer 0 Tile data
	{ "u61.bin",           0x200000, 0xD89B7631, BRF_GRA },				//  7 Layer 1 Tile data
	{ "u62.bin",           0x200000, 0x292BFB6B, BRF_GRA },				//  8 Layer 2 Tile data

	{ "u6.bin",            0x200000, 0x9DFDAFAF, BRF_SND },				//  9 YMZ280B (AD)PCM data
	{ "u7.bin",            0x200000, 0x795B17D5, BRF_SND },				// 10

	{ "eeprom-ddonpach.bin", 0x0080, 0x315fb546, BRF_OPT },				// 11 EEPROM 93C46 data
};


STD_ROM_PICK(ddonpach);
STD_ROM_FN(ddonpach);


// Rom information
static struct BurnRomInfo ddonpachjRomDesc[] = {
	{ "u27.bin",           0x080000, 0x2432FF9B, BRF_ESS | BRF_PRG },	//  0 CPU #0 code
	{ "u26.bin",           0x080000, 0x4F3A914A, BRF_ESS | BRF_PRG },	//  1

	{ "u50.bin",           0x200000, 0x14B260EC, BRF_GRA },				//  2 Sprite data
	{ "u51.bin",           0x200000, 0xE7BA8CCE, BRF_GRA },				//  3
	{ "u52.bin",           0x200000, 0x02492EE0, BRF_GRA },				//  4
	{ "u53.bin",           0x200000, 0xCB4C10F0, BRF_GRA },				//  5

	{ "u60.bin",           0x200000, 0x903096A7, BRF_GRA },				//  6 Layer 0 Tile data
	{ "u61.bin",           0x200000, 0xD89B7631, BRF_GRA },				//  7 Layer 1 Tile data
	{ "u62.bin",           0x200000, 0x292BFB6B, BRF_GRA },				//  8 Layer 2 Tile data

	{ "u6.bin",            0x200000, 0x9DFDAFAF, BRF_SND },				//  9 YMZ280B (AD)PCM data
	{ "u7.bin",            0x200000, 0x795B17D5, BRF_SND },				// 10

	{ "eeprom-ddonpach.bin", 0x0080, 0x315fb546, BRF_OPT },				// 11 EEPROM 93C46 data
};


STD_ROM_PICK(ddonpachj);
STD_ROM_FN(ddonpachj);


struct BurnDriver BurnDrvDoDonpachi = {
	"ddonpach", NULL, NULL, "1997",
	"DoDonPachi (International, Master Ver. 97/02/05)\0", NULL, "Cave (Atlus license)", "Cave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_ONLY, GBF_VERSHOOT, 0,
	NULL, ddonpachRomInfo, ddonpachRomName, ddonpachInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvDoDonpachiJ = {
	"ddonpachj", "ddonpach", NULL, "1997",
	"DoDonPachi (Japan, Master Ver. 97/02/05)\0", NULL, "Cave (Atlus license)", "Cave",
	L"DoDonPachi (Japan, Master Ver. 97/02/05)\0\u6012\u9996\u9818\u8702 (Japan, Master Ver. 97/02/05)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_ONLY, GBF_VERSHOOT, 0,
	NULL, ddonpachjRomInfo, ddonpachjRomName, ddonpachInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

