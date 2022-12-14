// Guwange

#include "cave.h"
#include "ymz280b.h"

static UINT8  ALIGN_DATA DrvJoy1[16] = { 0, };
static UINT8  ALIGN_DATA DrvJoy2[16] = { 0, };
static UINT16 ALIGN_DATA DrvInput[2] = { 0, };

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;
static UINT8 *Rom01;
static UINT8 *Ram01;

static UINT8 DrvReset = 0;
static UINT8 bDrawScreen;
static bool bVBlank;

static INT8 nVideoIRQ;
static INT8 nSoundIRQ;
static INT8 nUnknownIRQ;

static INT8 nIRQPending;

static INT8 nCurrentCPU;
static int  nCyclesDone;

static UINT8 bSpriteUpdate;

static struct BurnInputInfo ALIGN_DATA guwangeInputList[] = {
  {"P1 Coin",      BIT_DIGITAL,  DrvJoy2 + 0,  "p1 coin"   },
  {"P1 Start",     BIT_DIGITAL,  DrvJoy1 + 0,  "p1 start"  },

  {"P1 Up",        BIT_DIGITAL,  DrvJoy1 + 1,  "p1 up"     },
  {"P1 Down",      BIT_DIGITAL,  DrvJoy1 + 2,  "p1 down"   },
  {"P1 Left",      BIT_DIGITAL,  DrvJoy1 + 3,  "p1 left"   },
  {"P1 Right",     BIT_DIGITAL,  DrvJoy1 + 4,  "p1 right"  },
  {"P1 Button 1",  BIT_DIGITAL,  DrvJoy1 + 5,  "p1 fire 1" },
  {"P1 Button 2",  BIT_DIGITAL,  DrvJoy1 + 6,  "p1 fire 2" },
  {"P1 Button 3",  BIT_DIGITAL,  DrvJoy1 + 7,  "p1 fire 3" },

  {"P2 Coin",      BIT_DIGITAL,  DrvJoy2 + 1,  "p2 coin"   },
  {"P2 Start",     BIT_DIGITAL,  DrvJoy1 + 8,  "p2 start"  },

  {"P2 Up",        BIT_DIGITAL,  DrvJoy1 + 9,  "p2 up"     },
  {"P2 Down",      BIT_DIGITAL,  DrvJoy1 + 10, "p2 down"   },
  {"P2 Left",      BIT_DIGITAL,  DrvJoy1 + 11, "p2 left"   },
  {"P2 Right",     BIT_DIGITAL,  DrvJoy1 + 12, "p2 right"  },
  {"P2 Button 1",  BIT_DIGITAL,  DrvJoy1 + 13, "p2 fire 1" },
  {"P2 Button 2",  BIT_DIGITAL,  DrvJoy1 + 14, "p2 fire 2" },
  {"P2 Button 3",  BIT_DIGITAL,  DrvJoy1 + 15, "p2 fire 3" },

  {"Reset",        BIT_DIGITAL,  &DrvReset,    "reset"     },
  {"Diagnostics",  BIT_DIGITAL,  DrvJoy2 + 2,  "diag"      },
  {"Service",      BIT_DIGITAL,  DrvJoy2 + 3,  "service"   },
};

STDINPUTINFO(guwange);

static void UpdateIRQStatus()
{
	nIRQPending = ((nVideoIRQ == 0) || (nSoundIRQ == 0) || (nUnknownIRQ == 0));
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

UINT8 __fastcall guwangeReadByte(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x800002:
		case 0x800003: {
			return YMZ280BReadStatus();
		}

		case 0x300000:
		case 0x300001:
		case 0x300002:
		case 0x300003: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0x300004:
		case 0x300005: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0x300006:
		case 0x300007: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0xD00010:
			return (DrvInput[0] >> 8) ^ 0xFF;
		case 0xD00011:
			return (DrvInput[0] & 0xFF) ^ 0xFF;
		case 0xD00012:
			return (DrvInput[1] >> 8) ^ 0xFF;
		case 0xD00013:
			return ((DrvInput[1] & 0x7F) ^ 0x7F) | (EEPROMRead() << 7);

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read byte value of location %x\n", sekAddress);
//		}
	}
	return 0;
}

UINT16 __fastcall guwangeReadWord(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x800002: {
			return YMZ280BReadStatus();
		}

		case 0x300000:
		case 0x300002: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}

		case 0x300004: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0x300006: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0xD00010:
			return (DrvInput[0] ^ 0xFFFF);
		case 0xD00012:
			return (DrvInput[1] ^ 0xFF7F) | (EEPROMRead() << 7);

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read word value of location %x\n", sekAddress);
//		}
	}
	return 0;
}

void __fastcall guwangeWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	switch (sekAddress) {
		case 0x800000:
		case 0x800001:
			YMZ280BSelectRegister(byteValue);
			break;
		case 0x800002:
		case 0x800003:
			YMZ280BWriteRegister(byteValue);
			break;

		case 0xD00011:
			EEPROMWrite(byteValue & 0x40, byteValue & 0x20, byteValue & 0x80);
			break;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
//		}
	}
}

void __fastcall guwangeWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	switch (sekAddress) {

		case 0x300000:
			nCaveXOffset = wordValue;
			return;
		case 0x300002:
			nCaveYOffset = wordValue;
			return;
		case 0x300008:
			bSpriteUpdate = 1;
			nCaveSpriteBank = wordValue;
			return;

		case 0x800000:
			YMZ280BSelectRegister(wordValue & 0xFF);
			break;
		case 0x800002:
			YMZ280BWriteRegister(wordValue & 0xFF);
			break;

		case 0x900000:
			CaveTileReg[0][0] = wordValue;
			break;
		case 0x900002:
			CaveTileReg[0][1] = wordValue;
			break;
		case 0x900004:
			CaveTileReg[0][2] = wordValue;
			break;

		case 0xA00000:
			CaveTileReg[1][0] = wordValue;
			break;
		case 0xA00002:
			CaveTileReg[1][1] = wordValue;
			break;
		case 0xA00004:
			CaveTileReg[1][2] = wordValue;
			break;

		case 0xB00000:
			CaveTileReg[2][0] = wordValue;
			break;
		case 0xB00002:
			CaveTileReg[2][1] = wordValue;
			break;
		case 0xB00004:
			CaveTileReg[2][2] = wordValue;
			break;

		case 0xD00010:
			EEPROMWrite(wordValue & 0x40, wordValue & 0x20, wordValue & 0x80);
			break;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write word value %x to location %x\n", wordValue, sekAddress);
//		}
	}
}

void __fastcall guwangeWriteByteSprite(UINT32 sekAddress, UINT8 byteValue)
{
	bSpriteUpdate = 1;
	CaveSpriteRAM[(sekAddress ^ 0x01) & 0xFFFF] = byteValue;
}

void __fastcall guwangeWriteWordSprite(UINT32 sekAddress, UINT16 wordValue)
{
	bSpriteUpdate = 1;
	((UINT16 *)CaveSpriteRAM)[(sekAddress & 0xFFFF) >> 1] = wordValue;
}

void __fastcall guwangeWriteBytePalette(UINT32 sekAddress, UINT8 byteValue)
{
	CavePalWriteByte(sekAddress & 0xFFFF, byteValue);
}

void __fastcall guwangeWriteWordPalette(UINT32 sekAddress, UINT16 wordValue)
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

inline static void guwangeClearOpposites(UINT8* nJoystickInputs)
{
	if ((*nJoystickInputs & 0x06) == 0x06) {
		*nJoystickInputs &= ~0x06;
	}
	if ((*nJoystickInputs & 0x18) == 0x18) {
		*nJoystickInputs &= ~0x18;
	}
}

inline static INT8 CheckSleep(int)
{
#if 1 && defined USE_SPEEDHACKS
	int nCurrentPC = SekGetPC(-1);

	if (!nIRQPending && (nCurrentPC >= 0x06D6DE) && (nCurrentPC <= 0x06D6F4)) {
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
	DrvInput[0] = 0x0000;  												// Joysticks
	DrvInput[1] = 0x0000;  												// Other controls
	for (INT8 i = 0; i < 16; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
	}
	guwangeClearOpposites(((UINT8*)DrvInput) + 0);
	guwangeClearOpposites(((UINT8*)DrvInput) + 1);
	
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
	CaveSpriteROM	= Next; Next += 0x2000000;
	CaveTileROM[0]	= Next; Next += 0x800000;		// Tile layer 0
	CaveTileROM[1]	= Next; Next += 0x400000;		// Tile layer 1
	CaveTileROM[2]	= Next; Next += 0x400000;		// Tile layer 2
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
		
		for(int i = 0; i < 0x100000; i = i + 2, block1Head++, block2Head++) {
			Rom01[i + 0] = YMZ280BROM[block1Head];
			Rom01[i + 1] = YMZ280BROM[block2Head];
		}
		memset(YMZ280BROM, 0, 0x400000);
	}
/*
	BurnLoadRom(CaveSpriteROM + 0x0000000, 2, 2);
	BurnLoadRom(CaveSpriteROM + 0x0000001, 3, 2);
	CaveNibbleSwap3(CaveSpriteROM, 0x1000000);
	
	BurnLoadRom(CaveSpriteROM + 0x1000000, 4, 2);
	BurnLoadRom(CaveSpriteROM + 0x1000001, 5, 2);
	memcpy(CaveSpriteROM + 0x1800000, CaveSpriteROM + 0x1000000, 0x800000);
	CaveNibbleSwap3(CaveSpriteROM + 0x1000000, 0x1000000);

	BurnLoadRom(CaveTileROM[0] + 0x000000, 6, 1);
	CaveNibbleSwap4(CaveTileROM[0], 0x800000);
	BurnLoadRom(CaveTileROM[1] + 0x000000, 7, 1);
	CaveNibbleSwap4(CaveTileROM[1], 0x400000);
	BurnLoadRom(CaveTileROM[2] + 0x000000, 8, 1);
	CaveNibbleSwap4(CaveTileROM[2], 0x400000);
*/
	// Load YMZ280B data
	BurnLoadRom(YMZ280BROM, 9, 1);

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

static const UINT8 ALIGN_DATA guwange_default_eeprom[0x80] =
{
	0x00, 0x0C, 0x11, 0x0D, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x11, 0x11, 0xFF, 0xFF, 0xFF, 0xFF,
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
	
	initUniCache(0x3000000);
	
	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x2000000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x800000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x400000;
	
	if (needCreateCache) {
		UINT32 block1Head, block2Head;
		UINT8 *uniCacheTemp;
		
		if ((uniCacheHead = (UINT8 *)malloc(0x800000)) == NULL) return 1;
		if ((uniCacheTemp = (UINT8 *)malloc(0x400000)) == NULL) return 1;
		
		// Sprite
		block1Head = 0x000000, block2Head = 0x400000;
		memset(uniCacheHead, 0, 0x800000);
		memset(uniCacheTemp, 0, 0x400000);
		BurnLoadRom(uniCacheHead, 2, 1);
		memcpy(uniCacheTemp, uniCacheHead, 0x400000);
		BurnLoadRom(uniCacheHead, 3, 1);
		memcpy(uniCacheHead + block2Head, uniCacheHead, 0x400000);
		for (int i = 0; i < 0x800000; i = i + 2, block1Head++, block2Head++) {
			uniCacheHead[i + 0] = uniCacheTemp[block1Head];
			uniCacheHead[i + 1] = uniCacheHead[block2Head];
		}
		CaveNibbleSwap3(uniCacheHead, 0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		block1Head = 0x000000, block2Head = 0x400000;
		memset(uniCacheHead, 0, 0x800000);
		memset(uniCacheTemp, 0, 0x400000);
		BurnLoadRom(uniCacheHead, 2, 1);
		memcpy(uniCacheTemp, uniCacheHead + block2Head, 0x400000);
		BurnLoadRom(uniCacheHead, 3, 1);
		for (int i = 0; i < 0x800000; i = i + 2, block1Head++, block2Head++) {
			uniCacheHead[i + 0] = uniCacheTemp[block1Head];
			uniCacheHead[i + 1] = uniCacheHead[block2Head];
		}
		CaveNibbleSwap3(uniCacheHead, 0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		block1Head = 0x000000, block2Head = 0x400000;
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheTemp + block1Head, 4, 1);
		BurnLoadRom(uniCacheHead + block2Head, 5, 1);
		for (int i = 0; i < 0x800000; i = i + 2, block1Head++, block2Head++) {
			uniCacheHead[i + 0] = uniCacheTemp[block1Head];
			uniCacheHead[i + 1] = uniCacheHead[block2Head];
		}
		CaveNibbleSwap3(uniCacheHead, 0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		free(uniCacheTemp);
		uniCacheTemp = NULL;
		
		// Tile Layer 0
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead, 6, 1);
		CaveNibbleSwap4(uniCacheHead, 0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead + 0x000000, 7, 1);  // Tile Layer 1
		BurnLoadRom(uniCacheHead + 0x400000, 8, 1);  // Tile Layer 2
		CaveNibbleSwap4(uniCacheHead,  0x800000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);

		sceIoClose(cacheFile);
		cacheFile = sceIoOpen(filePathName, PSP_O_RDONLY, 0777);
		
		free(uniCacheHead);
		uniCacheHead = NULL;
	}
	
	// Find out how much memory is needed
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (UINT8 *)0;
	if ((Mem = (UINT8 *)memalign(4, nLen)) == NULL) {
		return 1;
	}
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();													// Index the allocated memory

	// Load the roms into memory
	if (LoadRoms()) {
		return 1;
	}

	EEPROMInit(&eeprom_interface_93C46);
	EEPROMLoadRom(14);
	if (!EEPROMAvailable()) {
		EEPROMFill(guwange_default_eeprom, 0, sizeof(guwange_default_eeprom));
	}

	initCacheStructure(0.9);

	{
		SekInit(0, 0x68000);													// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,					0x000000, 0x0FFFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,					0x200000, 0x20FFFF, SM_RAM);

		SekMapMemory(CaveSpriteRAM,			0x400000, 0x40FFFF, SM_ROM);
		SekMapHandler(1,					0x400000, 0x40FFFF, SM_WRITE);

		SekMapMemory(CaveTileRAM[0],		0x500000, 0x507FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],		0x600000, 0x607FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2],		0x700000, 0x707FFF, SM_RAM);

		SekMapMemory(CavePalSrc,			0xC00000, 0xC0FFFF, SM_ROM);	// Palette RAM (write goes through handler)
		SekMapHandler(2,					0xC00000, 0xC0FFFF, SM_WRITE);	//

		SekSetReadWordHandler (0, guwangeReadWord);
		SekSetReadByteHandler (0, guwangeReadByte);
		SekSetWriteWordHandler(0, guwangeWriteWord);
		SekSetWriteByteHandler(0, guwangeWriteByte);

		SekSetWriteWordHandler(1, guwangeWriteWordSprite);
		SekSetWriteByteHandler(1, guwangeWriteByteSprite);

		SekSetWriteWordHandler(2, guwangeWriteWordPalette);
		SekSetWriteByteHandler(2, guwangeWriteBytePalette);

		SekClose();
	}

	nCaveRowModeOffset = 2;

	CavePalInit(0x8000);
	CaveTileInit();
	CaveSpriteInit(1, 0x2000000);
	CaveTileInitLayer(0, 0x800000, 8, 0x4000);
	CaveTileInitLayer(1, 0x400000, 8, 0x4000);
	CaveTileInitLayer(2, 0x400000, 8, 0x4000);

	YMZ280BInit(16934400, &TriggerSoundIRQ, 3);

	bDrawScreen = true;

#if defined FBA_DEBUG && defined USE_SPEEDHACKS
	bprintf(PRINT_IMPORTANT, _T("  * Using speed-hacks (detecting idle loops).\n"));
#endif

	DrvDoReset(); // Reset machine

	return 0;
}

// Rom information
static struct BurnRomInfo guwangeRomDesc[] = {
	{ "gu-u0127.bin",     0x080000, 0xF86B5293, BRF_ESS | BRF_PRG },	//  0 CPU #0 code
	{ "gu-u0129.bin",     0x080000, 0x6C0E3B93, BRF_ESS | BRF_PRG },	//  1

	{ "u083.bin",         0x800000, 0xADC4B9C4, BRF_GRA },				//  2 Sprite data
	{ "u082.bin",         0x800000, 0x3D75876C, BRF_GRA },				//  3
	{ "u086.bin",         0x400000, 0x188E4F81, BRF_GRA },				//  4
	{ "u085.bin",         0x400000, 0xA7D5659E, BRF_GRA },				//  5

	{ "u101.bin",         0x800000, 0x0369491F, BRF_GRA },				//  6 Layer 0 Tile data
	{ "u10102.bin",       0x400000, 0xE28D6855, BRF_GRA },				//  7 Layer 1 Tile data
	{ "u10103.bin",       0x400000, 0x0FE91B8E, BRF_GRA },				//  8 Layer 2 Tile data

	{ "u0462.bin",        0x400000, 0xB3D75691, BRF_SND },				//  9 YMZ280B (AD)PCM data
	
	{ "atc05-1.bin",      0x000001, 0x00000000, BRF_NODUMP },			// 10
	{ "u0259.bin",        0x000001, 0x00000000, BRF_NODUMP },			// 11
	{ "u084.bin",         0x000001, 0x00000000, BRF_NODUMP },			// 12
	{ "u108.bin",         0x000001, 0x00000000, BRF_NODUMP },			// 13

	{ "eeprom-guwange.bin", 0x0080, 0xc3174959, BRF_OPT },				// 14 EEPROM 93C46 data
};

STD_ROM_PICK(guwange);
STD_ROM_FN(guwange);


struct BurnDriver BurnDrvGuwange = {
	"guwange", NULL, NULL, "1999",
	"Guwange (Japan, Master Ver. 99/06/24)\0", NULL, "Cave (Atlus license)", "Cave",
	L"\u3050\u308F\u3093\u3052 (Japan, Master Ver. 99/06/24)\0Guwange\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_ONLY, GBF_VERSHOOT, 0,
	NULL, guwangeRomInfo, guwangeRomName, guwangeInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

