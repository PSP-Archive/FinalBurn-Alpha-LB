// Hotdog Storm

#include "cave.h"
#include "msm6295.h"
#include "burn_ym2203.h"

static UINT8  ALIGN_DATA DrvJoy1[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT8  ALIGN_DATA DrvJoy2[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT16 ALIGN_DATA DrvInput[2] = { 0x0000, 0x0000 };

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;
static UINT8 *Rom01, *RomZ80;
static UINT8 *Ram01, *RamZ80;
static UINT8 *MSM6295ROMSrc;

static UINT8 DrvReset = 0;
static UINT8 bDrawScreen;
static bool bVBlank;

static INT8 nVideoIRQ;
static INT8 nSoundIRQ;
static INT8 nUnknownIRQ;

static INT8 nIRQPending;

static UINT16 DrvSoundLatch;
static UINT8 DrvZ80Bank;
static UINT8 DrvOkiBank1, DrvOkiBank2;

static UINT8 bSpriteUpdate;

static struct BurnInputInfo ALIGN_DATA hotdogstInputList[] = {
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

STDINPUTINFO(hotdogst);

static void UpdateIRQStatus()
{
	nIRQPending = ((nVideoIRQ == 0) || (nSoundIRQ == 0) || (nUnknownIRQ == 0));
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

UINT16 __fastcall hotdogstReadWord(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0xa80000:
		case 0xa80002: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xa80004: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0xa80006: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		
//		case 0xa8006e: {
//			return 0xff;
//		}
		
		case 0xc80000:
			return (DrvInput[0] ^ 0xFFFF);
		case 0xC80002:
			return (DrvInput[1] ^ 0xF7FF) | (EEPROMRead() << 11);
		
//		default: {
//			bprintf(PRINT_NORMAL, _T("Attempt to read word value of location %x\n"), sekAddress);
//		}
	}
	return 0;
}

void __fastcall hotdogstWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	switch (sekAddress) {
		case 0xa80000:
			nCaveXOffset = wordValue;
			return;
		case 0xa80002:
			nCaveYOffset = wordValue;
			return;
			
		case 0xa80008:
			bSpriteUpdate = 1;
			nCaveSpriteBank = wordValue;
			return;
		
		case 0xa8006e: {
			DrvSoundLatch = wordValue;
			
			ZetOpen(0);
			ZetNmi();
			ZetClose();
			return;
		}
		
		case 0xb00000:
			CaveTileReg[0][0] = wordValue;
			break;
		case 0xb00002:
			CaveTileReg[0][1] = wordValue;
			break;
		case 0xb00004:
			CaveTileReg[0][2] = wordValue;
			break;
			
		case 0xb80000:
			CaveTileReg[1][0] = wordValue;
			break;
		case 0xb80002:
			CaveTileReg[1][1] = wordValue;
			break;
		case 0xb80004:
			CaveTileReg[1][2] = wordValue;
			break;
		
		case 0xc00000:
			CaveTileReg[2][0] = wordValue;
			break;
		case 0xc00002:
			CaveTileReg[2][1] = wordValue;
			break;
		case 0xc00004:
			CaveTileReg[2][2] = wordValue;
			break;
			
		case 0xD00000:
			wordValue >>= 8;
			EEPROMWrite(wordValue & 0x04, wordValue & 0x02, wordValue & 0x08);
			break;
			
		case 0xd00002: {
			//nop
			return;
		}	

//		default: {
//			bprintf(PRINT_NORMAL, _T("Attempt to write word value %x to location %x\n"), wordValue, sekAddress);
//		}
	}
}

UINT8 __fastcall hotdogstZIn(UINT16 nAddress)
{
	nAddress &= 0xFF;

	switch (nAddress) {
		case 0x30: {
			return DrvSoundLatch & 0xff; 
		}
		case 0x40: {
			return (DrvSoundLatch & 0xff00) >> 8;
		}
		
		case 0x50: {
			return BurnYM2203Read(0, 0);
		}
		case 0x51: {
			return BurnYM2203Read(0, 1);
		}
		
		case 0x60: {
			return MSM6295ReadStatus(0);
		}
		
//		default: {
//			bprintf(PRINT_NORMAL, _T("Z80 Port Read %x\n"), nAddress);
//		}
	}

	return 0;
}

void __fastcall hotdogstZOut(UINT16 nAddress, UINT8 nValue)
{
	nAddress &= 0xFF;

	switch (nAddress) {
		case 0x00: {
			DrvZ80Bank = nValue & 0x0f;
			
			ZetOpen(0);
			ZetMapArea(0x4000, 0x7FFF, 0, RomZ80 + (DrvZ80Bank << 14));
			ZetMapArea(0x4000, 0x7FFF, 2, RomZ80 + (DrvZ80Bank << 14));
			ZetClose();
			return;
		}
		
		case 0x50: {
			BurnYM2203Write(0, 0, nValue);
			return;
		}
		case 0x51: {
			BurnYM2203Write(0, 1, nValue);
			return;
		}
		
		case 0x60: {
			MSM6295Command(0, nValue);
			return;
		}
		case 0x70: {
			DrvOkiBank1 = (nValue >> 0) & 0x03;
			DrvOkiBank2 = (nValue >> 4) & 0x03;
			
			memcpy(MSM6295ROM + 0x00000, MSM6295ROMSrc + (DrvOkiBank1 << 17), 0x20000);
			memcpy(MSM6295ROM + 0x20000, MSM6295ROMSrc + (DrvOkiBank2 << 17), 0x20000);
			return;
		}
		
//		default: {
//			bprintf(PRINT_NORMAL, _T("Z80 Port Write %x, %x\n"), nAddress, nValue);
//		}
	}
}

void __fastcall hotdogstWriteBytePalette(UINT32 sekAddress, UINT8 byteValue)
{
	CavePalWriteByte(sekAddress & 0x0FFF, byteValue);
}

void __fastcall hotdogstWriteWordPalette(UINT32 sekAddress, UINT16 wordValue)
{
	CavePalWriteWord(sekAddress & 0x0FFF, wordValue);
}

void __fastcall hotdogstWriteByteSprite(UINT32 sekAddress, UINT8 byteValue)
{
	bSpriteUpdate = 1;
	CaveSpriteRAM[(sekAddress ^ 0x01) & 0xFFFF] = byteValue;
}

void __fastcall hotdogstWriteWordSprite(UINT32 sekAddress, UINT16 wordValue)
{
	bSpriteUpdate = 1;
	((UINT16 *)CaveSpriteRAM)[(sekAddress & 0xFFFF) >> 1] = wordValue;
}

UINT8 __fastcall hotdogstZRead(UINT16 a)
{
//	switch (a) {
//		default: {
//			bprintf(PRINT_NORMAL, _T("Z80 Read => %04X\n"), a);
//		}
//	}

	return 0;
}

void __fastcall hotdogstZWrite(UINT16 a, UINT8 d)
{
//	switch (a) {
//		default: {
//			bprintf(PRINT_NORMAL, _T("Z80 Write => %04X, %02X\n"), a, d);
//		}
//	}
}

static int DrvExit()
{
	EEPROMExit();

	MSM6295Exit(0);

	CaveTileExit();
	CaveSpriteExit();
    CavePalExit();

	SekExit();				// Deallocate 68000s
	ZetExit();
	
	BurnYM2203Exit();
	
	DrvSoundLatch = 0;
	DrvZ80Bank = 0;
	DrvOkiBank1 = 0;
	DrvOkiBank2 = 0;

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
	
	ZetOpen(0);
	ZetReset();
	ZetClose();
	
	BurnYM2203Reset();
	MSM6295Reset(0);

	EEPROMReset();

	nVideoIRQ = 1;
	nSoundIRQ = 1;
	nUnknownIRQ = 1;

	nIRQPending = 0;
	
	DrvSoundLatch = 0;
	DrvZ80Bank = 0;
	DrvOkiBank1 = 0;
	DrvOkiBank2 = 0;

	return 0;
}

static int DrvDraw()
{
	CavePalUpdate4Bit(0, 128);				// Update the palette
	CaveClearScreen(CavePalette[0x3F00]);

	if (bDrawScreen) {
//		CaveGetBitmap();

		CaveTileRender(1);					// Render tiles
	}

	return 0;
}

inline static INT8 CheckSleep(int)
{
	return 0;
}

#define CAVE_INTERLEAVE  (80)

#ifndef BUILD_PSP
	#define M68K_CYCLES_PER_FRAME  ((int)((INT64)16000000 * nBurnCPUSpeedAdjust / (0x0100 * CAVE_REFRESHRATE)))
#else
	#define M68K_CYCLES_PER_FRAME  ((int)(16000000 / CAVE_REFRESHRATE))
#endif

#define  Z80_CYCLES_PER_FRAME  ((int)(4000000 / CAVE_REFRESHRATE))

#define M68K_CYCLES_VBLANK (M68K_CYCLES_PER_FRAME - (int)((M68K_CYCLES_PER_FRAME * CAVE_VBLANK_LINES) / 271.5))

#define M68K_CYLECS_PER_INTER  (M68K_CYCLES_PER_FRAME / CAVE_INTERLEAVE)
#define  Z80_CYCLES_PER_INTER  ( Z80_CYCLES_PER_FRAME / CAVE_INTERLEAVE)

static int DrvFrame()
{
	int ALIGN_DATA nCyclesTotal[2] = { M68K_CYCLES_PER_FRAME, Z80_CYCLES_PER_FRAME };
	int ALIGN_DATA nCyclesDone[2]  = { 0, 0 };
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
	ZetNewFrame();
	
	SekOpen(0);
	
	nCyclesVBlank = M68K_CYCLES_VBLANK;
	bVBlank = false;
	
	for (INT8 i = 1; i <= CAVE_INTERLEAVE; i++) {
		
		// Run 68000
    	INT8 nCurrentCPU = 0;
		int nNext = i * M68K_CYLECS_PER_INTER;
		
		// See if we need to trigger the VBlank interrupt
		if (!bVBlank && (nNext > nCyclesVBlank)) {
			if (nCyclesDone[nCurrentCPU] < nCyclesVBlank) {
				nCyclesSegment = nCyclesVBlank - nCyclesDone[nCurrentCPU];
				if (!CheckSleep(nCurrentCPU)) {							// See if this CPU is busywaiting
					nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
				} else {
					nCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
				}
			}
			
			if (pBurnDraw != NULL) {
				DrvDraw();												// Draw screen if needed
			}
			
			if (bSpriteUpdate) {
				CaveSpriteBuffer();
				bSpriteUpdate = 0;
			}
			
			ZetOpen(0);
			BurnTimerEndFrame(nCyclesTotal[1]);
			if (pBurnSoundOut) {
				BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
				MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
			}
			ZetClose();
			
			bVBlank = true;
			nVideoIRQ = 0;
			UpdateIRQStatus();
		}
		
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		if (!CheckSleep(nCurrentCPU)) {									// See if this CPU is busywaiting
			nCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		} else {
			nCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
		}
	}
	
	SekClose();
	
	return 0;
}

// This routine is called first to determine how much memory is needed (MemEnd-(UINT8 *)0),
// and then afterwards to set up all the pointers
static INT8 MemIndex()
{
	UINT8* Next; Next = Mem;
	Rom01			= Next; Next += 0x100000;		// 68K program
	RomZ80			= Next; Next += 0x040000;
/*
	CaveSpriteROM	= Next; Next += 0x800000;
	CaveTileROM[0]	= Next; Next += 0x100000;		// Tile layer 0
	CaveTileROM[1]	= Next; Next += 0x100000;		// Tile layer 1
	CaveTileROM[2]	= Next; Next += 0x100000;		// Tile layer 2
*/
	MSM6295ROM		= Next; Next += 0x040000;
	MSM6295ROMSrc	= Next; Next += 0x080000;

	RamStart		= Next;
	Ram01			= Next; Next += 0x010000;		// CPU #0 work RAM
	RamZ80			= Next; Next += 0x002000;
	CaveTileRAM[0]	= Next; Next += 0x008000;
	CaveTileRAM[1]	= Next; Next += 0x008000;
	CaveTileRAM[2]	= Next; Next += 0x008000;
	CaveSpriteRAM	= Next; Next += 0x010000;
	CavePalSrc		= Next; Next += 0x001000;		// palette
	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}


static INT8 LoadRoms()
{
	BurnLoadRom(Rom01 + 1, 0, 2);
	BurnLoadRom(Rom01 + 0, 1, 2);
	
	BurnLoadRom(RomZ80, 2, 1);
/*
	BurnLoadRom(CaveSpriteROM + 0x000000, 3, 1);
	BurnLoadRom(CaveSpriteROM + 0x200000, 4, 1);
	CaveNibbleSwap1(CaveSpriteROM, 0x400000);

	BurnLoadRom(CaveTileROM[0], 5, 1);
	CaveNibbleSwap2(CaveTileROM[0], 0x080000);
	BurnLoadRom(CaveTileROM[1], 6, 1);
	CaveNibbleSwap2(CaveTileROM[1], 0x080000);
	BurnLoadRom(CaveTileROM[2], 7, 1);
	CaveNibbleSwap2(CaveTileROM[2], 0x080000);
*/
	// Load MSM6295 ADPCM data
	BurnLoadRom(MSM6295ROMSrc, 8, 1);

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

		SekScan(nAction);
		ZetScan(nAction);

		BurnYM2203Scan(nAction, pnMin);
		MSM6295Scan(0, nAction);

		SCAN_VAR(nVideoIRQ);
		SCAN_VAR(nSoundIRQ);
		SCAN_VAR(nUnknownIRQ);
		SCAN_VAR(bVBlank);

		CaveScanGraphics();

		SCAN_VAR(DrvInput);
		SCAN_VAR(DrvSoundLatch);
		SCAN_VAR(DrvZ80Bank);
		SCAN_VAR(DrvOkiBank1);
		SCAN_VAR(DrvOkiBank2);
		
		if (nAction & ACB_WRITE) {
			ZetOpen(0);
			ZetMapArea(0x4000, 0x7FFF, 0, RomZ80 + (DrvZ80Bank << 14));
			ZetMapArea(0x4000, 0x7FFF, 2, RomZ80 + (DrvZ80Bank << 14));
			ZetClose();
			
			memcpy(MSM6295ROM + 0x00000, MSM6295ROMSrc + (DrvOkiBank1 << 17), 0x20000);
			memcpy(MSM6295ROM + 0x20000, MSM6295ROMSrc + (DrvOkiBank2 << 17), 0x20000);
			
			CaveSpriteBuffer();
			CaveRecalcPalette = 1;
		}
	}

	return 0;
}

static void DrvFMIRQHandler(int, int nStatus)
{
	if (nStatus & 1) {
		ZetSetIRQLine(0xff, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0,    ZET_IRQSTATUS_NONE);
	}
}

static int DrvSynchroniseStream(int nSoundRate)
{
	return (INT64)ZetTotalCycles() * nSoundRate / 4000000;
}

static float DrvGetTime()
{
	return (float)ZetTotalCycles() / 4000000;
}

static INT8 drvZInit()
{
	ZetInit(1);
	ZetOpen(0);

	ZetSetInHandler(hotdogstZIn);
	ZetSetOutHandler(hotdogstZOut);
	ZetSetReadHandler(hotdogstZRead);
	ZetSetWriteHandler(hotdogstZWrite);

	// ROM bank 1
	ZetMapArea    (0x0000, 0x3FFF, 0, RomZ80 + 0x0000); // Direct Read from ROM
	ZetMapArea    (0x0000, 0x3FFF, 2, RomZ80 + 0x0000); // Direct Fetch from ROM
	// ROM bank 2
	ZetMapArea    (0x4000, 0x7FFF, 0, RomZ80 + 0x4000); // Direct Read from ROM
	ZetMapArea    (0x4000, 0x7FFF, 2, RomZ80 + 0x4000); //
	// RAM
	ZetMapArea    (0xE000, 0xFFFF, 0, RamZ80);			// Direct Read from RAM
	ZetMapArea    (0xE000, 0xFFFF, 1, RamZ80);			// Direct Write to RAM
	ZetMapArea    (0xE000, 0xFFFF, 2, RamZ80);			//

	ZetMemEnd();
	ZetClose();

	return 0;
}

static const UINT8 ALIGN_DATA hotdogst_default_eeprom[0x80] =
{
	0xF3, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
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
	
	initUniCache(0xB00000);
	
	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x800000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x100000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x100000;
	
	if (needCreateCache) {
		if ((uniCacheHead = (UINT8 *)malloc(0x800000)) == NULL) return 1;
		
		// Sprite
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead + 0x000000, 3, 1);
		BurnLoadRom(uniCacheHead + 0x200000, 4, 1);
		CaveNibbleSwap1(uniCacheHead,  0x400000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead + 0x000000, 5, 1);  // Tile Layer 0
		BurnLoadRom(uniCacheHead + 0x080000, 6, 1);  // Tile Layer 1
		BurnLoadRom(uniCacheHead + 0x100000, 7, 1);  // Tile Layer 2
		CaveNibbleSwap2(uniCacheHead,  0x180000);
		sceIoWrite(cacheFile, uniCacheHead, 0x300000);
		
		sceIoClose( cacheFile );
		cacheFile = sceIoOpen( filePathName, PSP_O_RDONLY, 0777);
		
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
	EEPROMLoadRom(9);
	if (!EEPROMAvailable()) {
		EEPROMFill(hotdogst_default_eeprom, 0, sizeof(hotdogst_default_eeprom));
	}

	initCacheStructure(0.9);

	{
		SekInit(0, 0x68000);											// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,				0x000000, 0x0FFFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,				0x300000, 0x30FFFF, SM_RAM);

		SekMapMemory(CavePalSrc,		0x408000, 0x408FFF, SM_ROM);	// Palette RAM
		SekMapHandler(1,				0x408000, 0x408FFF, SM_WRITE);

		SekMapMemory(CaveTileRAM[0],	0x880000, 0x887FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],	0x900000, 0x907FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2],	0x980000, 0x987FFF, SM_RAM);

		SekMapMemory(CaveSpriteRAM,		0xF00000, 0xF0FFFF, SM_ROM);
		SekMapHandler(2,				0xF00000, 0xF0FFFF, SM_WRITE);

		SekSetReadWordHandler (0, hotdogstReadWord);
		SekSetWriteWordHandler(0, hotdogstWriteWord);

		SekSetWriteWordHandler(1, hotdogstWriteWordPalette);
		SekSetWriteByteHandler(1, hotdogstWriteBytePalette);

		SekSetWriteWordHandler(2, hotdogstWriteWordSprite);
		SekSetWriteByteHandler(2, hotdogstWriteByteSprite);

		SekClose();
	}
	
	drvZInit();

	CavePalInit(0x8000);
	CaveTileInit();
	CaveSpriteInit(2, 0x0800000);
	CaveTileInitLayer(0, 0x100000, 8, 0);
	CaveTileInitLayer(1, 0x100000, 8, 0);
	CaveTileInitLayer(2, 0x100000, 8, 0);
	
	nCaveExtraXOffset = -32;
	nCaveExtraYOffset = 32;
	
	BurnYM2203Init(1, 4000000, 20.0, 80.0, &DrvFMIRQHandler, DrvSynchroniseStream, DrvGetTime, 0);
	BurnTimerAttachZet(4000000);
	
	memcpy(MSM6295ROM, MSM6295ROMSrc, 0x40000);
	MSM6295Init(0, 1056000 / 132, 100.0, 1);
	
	bDrawScreen = true;

#if defined FBA_DEBUG && defined USE_SPEEDHACKS
	bprintf(PRINT_IMPORTANT, _T("  * Using speed-hacks (detecting idle loops).\n"));
#endif

	DrvDoReset(); // Reset machine

	return 0;
}

// Rom information
static struct BurnRomInfo hotdogstRomDesc[] = {
	{ "mp3u29",            0x080000, 0x1f4e5479, BRF_ESS | BRF_PRG },	//  0 CPU #0 code
	{ "mp4u28",            0x080000, 0x6f1c3c4b, BRF_ESS | BRF_PRG },	//  1
	
	{ "mp2u19",            0x040000, 0xff979ebe, BRF_ESS | BRF_PRG },	//  2 Z80 Code

	{ "mp9u55",            0x200000, 0x258d49ec, BRF_GRA },				//  3 Sprite data
	{ "mp8u54",            0x200000, 0xbdb4d7b8, BRF_GRA },				//  4

	{ "mp7u56",            0x080000, 0x87c21c50, BRF_GRA },				//  5 Layer 0 Tile data
	{ "mp6u61",            0x080000, 0x4dafb288, BRF_GRA },				//  6 Layer 1 Tile data
	{ "mp5u64",            0x080000, 0x9b26458c, BRF_GRA },				//  7 Layer 2 Tile data

	{ "mp1u65",            0x080000, 0x4868be1b, BRF_SND },				//  8 MSM6295 #1 ADPCM data

	{ "eeprom-hotdogst.bin", 0x0080, 0x12b4f934, BRF_OPT },				//  9 EEPROM 93C46 data
};

STD_ROM_PICK(hotdogst);
STD_ROM_FN(hotdogst);

struct BurnDriver BurnDrvhotdogst = {
	"hotdogst", NULL, NULL, "1996",
	"Hotdog Storm (International)\0", NULL, "Marble", "Cave",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, hotdogstRomInfo, hotdogstRomName, hotdogstInputInfo, NULL,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 384, 3, 4
};

