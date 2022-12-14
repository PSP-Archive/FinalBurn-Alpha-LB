// Pretty Soldier Sailor Moon / Air Gallet

#include "cave.h"
#include "msm6295.h"
#include "burn_ym2151.h"

#include "bitswap.h"

static UINT8  ALIGN_DATA DrvJoy1[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT8  ALIGN_DATA DrvJoy2[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static UINT16 ALIGN_DATA DrvInput[2] = { 0x0000, 0x0000 };

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;
static UINT8 *Rom01, *Rom02, *RomZ80;
static UINT8 *Ram01, *Ram02, *Ram03, *Ram04, *RamZ80;

static UINT8 DrvReset = 0;
static UINT8 bDrawScreen;
static INT8 nVBlank;

static UINT8 nCurrentBank;

static int SoundLatch;
static int ALIGN_DATA SoundLatchReply[48];
static int SoundLatchStatus;
static int SoundLatchReplyIndex;
static int SoundLatchReplyMax;

static UINT8 DrvOkiBank1, DrvOkiBank2;

static INT8 nVideoIRQ;
static INT8 nSoundIRQ;
static INT8 nUnknownIRQ;

static int ALIGN_DATA nCaveCyclesDone[2];

static UINT8 bSpriteUpdate;

UINT8 nWhichGame;		// 0 - sailormn / sailormo
						// 1 - agallet

static struct BurnInputInfo ALIGN_DATA sailormnInputList[] = {
  {"P1 Coin",      BIT_DIGITAL,   DrvJoy1 + 8,  "p1 coin"    },
  {"P1 Start",     BIT_DIGITAL,   DrvJoy1 + 7,  "p1 start"   },

  {"P1 Up",        BIT_DIGITAL,   DrvJoy1 + 0,  "p1 up"      },
  {"P1 Down",      BIT_DIGITAL,   DrvJoy1 + 1,  "p1 down"    },
  {"P1 Left",      BIT_DIGITAL,   DrvJoy1 + 2,  "p1 left"    },
  {"P1 Right",     BIT_DIGITAL,   DrvJoy1 + 3,  "p1 right"   },
  {"P1 Button 1",  BIT_DIGITAL,   DrvJoy1 + 4,  "p1 fire 1"  },
  {"P1 Button 2",  BIT_DIGITAL,   DrvJoy1 + 5,  "p1 fire 2"  },
  {"P1 Button 3",  BIT_DIGITAL,   DrvJoy1 + 6,  "p1 fire 3"  },

  {"P2 Coin",      BIT_DIGITAL,   DrvJoy2 + 8,  "p2 coin"    },
  {"P2 Start",     BIT_DIGITAL,   DrvJoy2 + 7,  "p2 start"   },

  {"P2 Up",        BIT_DIGITAL,   DrvJoy2 + 0,  "p2 up"      },
  {"P2 Down",      BIT_DIGITAL,   DrvJoy2 + 1,  "p2 down"    },
  {"P2 Left",      BIT_DIGITAL,   DrvJoy2 + 2,  "p2 left"    },
  {"P2 Right",     BIT_DIGITAL,   DrvJoy2 + 3,  "p2 right"   },
  {"P2 Button 1",  BIT_DIGITAL,   DrvJoy2 + 4,  "p2 fire 1"  },
  {"P2 Button 2",  BIT_DIGITAL,   DrvJoy2 + 5,  "p2 fire 2"  },
  {"P2 Button 3",  BIT_DIGITAL,   DrvJoy2 + 6,  "p2 fire 3"  },

  {"Reset",        BIT_DIGITAL,   &DrvReset,    "reset"      },
  {"Diagnostics",  BIT_DIGITAL,   DrvJoy1 + 9,  "diag"       },
  {"Service",      BIT_DIGITAL,   DrvJoy2 + 9,  "service"    },
};

STDINPUTINFO(sailormn);


static void UpdateIRQStatus()
{
	INT8 nIRQPending = ((nVideoIRQ == 0) || (nSoundIRQ == 0) || (nUnknownIRQ == 0));
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

static void drvZ80Bankswitch(UINT8 nBank)
{
	nBank &= 0x1F;
	if (nBank != nCurrentBank) {
		UINT8 *nStartAddress = RomZ80 + (nBank << 14);
		ZetMapArea(0x4000, 0x7FFF, 0, nStartAddress);
		ZetMapArea(0x4000, 0x7FFF, 2, nStartAddress);
		nCurrentBank = nBank;
	}
}

static void drvOkiBankswitch(UINT8 chip, UINT8 bank)
{
	UINT32 offset = 0x0200000 * chip;
	
	UINT32 bank1 = (bank & 0x0F) << 17;
	UINT32 bank2 = (bank & 0xF0) << 13;
	
	MSM6295SampleInfo[chip][0] = MSM6295ROM + offset + bank1;
	MSM6295SampleInfo[chip][1] = MSM6295ROM + offset + bank1 + 0x0100;
	MSM6295SampleInfo[chip][2] = MSM6295ROM + offset + bank1 + 0x0200;
	MSM6295SampleInfo[chip][3] = MSM6295ROM + offset + bank1 + 0x0300;
	
	MSM6295SampleData[chip][0] = MSM6295ROM + offset + bank1;
	MSM6295SampleData[chip][1] = MSM6295ROM + offset + bank1 + 0x010000;
	MSM6295SampleData[chip][2] = MSM6295ROM + offset + bank2;
	MSM6295SampleData[chip][3] = MSM6295ROM + offset + bank2 + 0x010000;
}

static void drvYM2151IRQHandler(int nStatus)
{
	if (nStatus) {
//		ZetRaiseIrq(255);
//		nCaveCyclesDone[1] += ZetRun(0x0400);
		ZetSetIRQLine(0xff, ZET_IRQSTATUS_ACK);
	} else {
		ZetSetIRQLine(0, ZET_IRQSTATUS_NONE);
	}
}

UINT8 __fastcall sailormnZIn(UINT16 nAddress)
{
	nAddress &= 0xFF;

	switch (nAddress) {
		case 0x20: {
//			bprintf(PRINT_NORMAL, "Z80 read thingie.\n");
//			int nStatus = SoundLatchStatus;
//			SoundLatchStatus &= ~2;
//			return nStatus;

			return 0;
		}
		case 0x30:
//			bprintf(PRINT_NORMAL, "Z80 read soundlatch(lo)\n");
			SoundLatchStatus |= 0x04;
			return SoundLatch & 0xFF;
		case 0x40:
//			bprintf(PRINT_NORMAL, "Z80 read soundlatch(hi)\n");
			SoundLatchStatus |= 0x08;
			return SoundLatch >> 8;

		case 0x50:
		case 0x51:
//			bprintf(PRINT_NORMAL, "YM2151 status read.\n");
			return BurnYM2151ReadStatus(nAddress & 0x01);
		
		case 0x60:
//			bprintf(PRINT_NORMAL, "MSM6295 #0 status read.\n");
			return MSM6295ReadStatus(0);
		case 0x80:
//			bprintf(PRINT_NORMAL, "MSM6295 #1 status read.\n");
			return MSM6295ReadStatus(1);
	}

	return 0;
}

void __fastcall sailormnZOut(UINT16 nAddress, UINT8 nValue)
{
	nAddress &= 0xFF;

	switch (nAddress) {

		case 0x00:
			drvZ80Bankswitch(nValue);
			break;

		case 0x10:
//			SoundLatchStatus |= 0x02;
//			SoundLatchReply = nValue;
			if (SoundLatchReplyIndex > SoundLatchReplyMax) {
				SoundLatchReplyMax = -1;
				SoundLatchReplyIndex = 0;
			}
			SoundLatchReplyMax++;
			SoundLatchReply[SoundLatchReplyMax] = nValue;
			break;

		case 0x50:
			BurnYM2151SelectRegister(nValue);
			break;
		case 0x51:
//			bprintf(PRINT_NORMAL, "YM2151 reg %02X -> %02X\n", CaveCurrentYM2151Register, nValue);
			BurnYM2151WriteRegister(nValue);
			break;

		case 0x60:
//			bprintf(PRINT_NORMAL, "MSM6295 #0 command sent.\n");
			MSM6295Command(0, nValue);
			break;
		case 0x70:
			DrvOkiBank1 = nValue;
			drvOkiBankswitch(0, DrvOkiBank1);
			break;

		case 0x80:
//			bprintf(PRINT_NORMAL, "MSM6295 #1 command sent.\n");
			MSM6295Command(1, nValue);
			break;
		case 0xC0:
			DrvOkiBank2 = nValue;
			drvOkiBankswitch(1, DrvOkiBank2);
			break;
	}
}

static INT8 drvZInit()
{
	ZetInit(1);
	ZetOpen(0);

	ZetSetInHandler(sailormnZIn);
	ZetSetOutHandler(sailormnZOut);

	// ROM bank 1
	ZetMapArea    (0x0000, 0x3FFF, 0, RomZ80 + 0x0000); // Direct Read from ROM
	ZetMapArea    (0x0000, 0x3FFF, 2, RomZ80 + 0x0000); // Direct Fetch from ROM
	// ROM bank 2
	ZetMapArea    (0x4000, 0x7FFF, 0, RomZ80 + 0x0000); // Direct Read from ROM
	ZetMapArea    (0x4000, 0x7FFF, 2, RomZ80 + 0x0000); //
	// RAM
	ZetMapArea    (0xC000, 0xDFFF, 0, RamZ80);			// Direct Read from RAM
	ZetMapArea    (0xC000, 0xDFFF, 1, RamZ80);			// Direct Write to RAM
	ZetMapArea    (0xC000, 0xDFFF, 2, RamZ80);			//
	// RAM mirror
	ZetMapArea    (0xE000, 0xFFFF, 0, RamZ80);			// Direct Read from RAM
	ZetMapArea    (0xE000, 0xFFFF, 1, RamZ80);			// Direct Write to RAM
	ZetMapArea    (0xE000, 0xFFFF, 2, RamZ80);			//

	ZetMemEnd();
	ZetClose();

	return 0;
}

UINT8 __fastcall sailormnReadByte(UINT32 sekAddress)
{
//	bprintf(PRINT_NORMAL, "Attempt to read byte value of location %x\n", sekAddress);

	switch (sekAddress) {
		case 0xB80000:
		case 0xB80001: {
			UINT8 nRet = ((nVBlank ^ 1) << 2) | (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xB80002:
		case 0xB80003: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xB80004:
		case 0xB80005: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0xB80006:
		case 0xB80007: {
			UINT8 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0x600000:
			return (DrvInput[0] >> 8) ^ 0xFF;
		case 0x600001:
			return (DrvInput[0] & 0xFF) ^ 0xFF;
		case 0x600002:
			return ((DrvInput[1] >> 8) ^ 0xF7) | (EEPROMRead() << 3);
		case 0x600003:
			return (DrvInput[1] & 0xFF) ^ 0xFF;

#if 0
		case 0xB8006C:
		case 0xB8006D:
			bprintf(PRINT_NORMAL, "Soundlatch status read (byte).\n");
//			return SoundLatchStatus & 3;
			return 0;
		case 0xB8006E:
		case 0xB8006F:
			bprintf(PRINT_NORMAL, "Sound latch read (byte).\n");
//			SoundLatchStatus = 2;
//			return SoundLatchReply;
			return 0;
#endif

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read byte value of location %x\n", sekAddress);
//		}
	}

	return 0;
}

UINT16 __fastcall sailormnReadWord(UINT32 sekAddress)
{
//	bprintf(PRINT_NORMAL, "Attempt to read word value of location %x\n", sekAddress);

	switch (sekAddress) {
		case 0xB80000: {
			UINT16 nRet = ((nVBlank ^ 1) << 2) | (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xB80002: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0xB80004: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0xB80006: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0xB8006C:
//			SoundLatchStatus &= ~4;
//			return SoundLatchStatus & 3;

			if (SoundLatchReplyIndex > SoundLatchReplyMax) {
				return 2;
			}
			return 0;
		case 0xB8006E:
//			bprintf(PRINT_NORMAL, "Sound latch read.\n");
//			SoundLatchStatus = 2;
//			return SoundLatchReply;

			if (SoundLatchReplyIndex > SoundLatchReplyMax) {
				SoundLatchReplyIndex = 0;
				SoundLatchReplyMax = -1;
				return 0;
			}
//			bprintf(PRINT_NORMAL, "Sound latch reply read (%02X).\n", SoundLatchReply[SoundLatchReplyIndex]);
			return SoundLatchReply[SoundLatchReplyIndex++];

		case 0x600000:
			return (DrvInput[0] ^ 0xFFFF);
		case 0x600002:
			return (DrvInput[1] ^ 0xF7FF) | (EEPROMRead() << 11);

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read word value of location %x\n", sekAddress);
//		}
	}

	return 0;
}

void __fastcall sailormnWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
//	bprintf(PRINT_NORMAL, "Attempt to write byte value %x to location %x\n", byteValue, sekAddress);

	switch (sekAddress) {
//		case 0xB8006E:
//			SoundLatch &= 0xFF00;
//			SoundLatch |= byteValue << 8;
//			break;
//		case 0xB8006F:
//			SoundLatch &= 0x00FF;
//			SoundLatch |= byteValue;
//			break;

	case 0x700000:
			nCaveTileBank = byteValue & 1;
			EEPROMWrite(byteValue & 0x04, byteValue & 0x02, byteValue & 0x08);
			return;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
//		}
	}
}

void __fastcall sailormnWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
//	bprintf(PRINT_NORMAL, "Attempt to write word value %x to location %x\n", wordValue, sekAddress);

	switch (sekAddress) {
		case 0xA00000:
			CaveTileReg[0][0] = wordValue;
			return;
		case 0xA00002:
			CaveTileReg[0][1] = wordValue;
			return;
		case 0xA00004:
			CaveTileReg[0][2] = wordValue;
			return;

		case 0xA80000:
			CaveTileReg[1][0] = wordValue;
			return;
		case 0xA80002:
			CaveTileReg[1][1] = wordValue;
			return;
		case 0xA80004:
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

		case 0xB80000:
			nCaveXOffset = wordValue;
			return;
		case 0xB80002:
			nCaveYOffset = wordValue;
			return;
		case 0xB80008:
			bSpriteUpdate = 1;
			nCaveSpriteBank = wordValue;
			return;

		case 0xB8006E:
//			bprintf(PRINT_NORMAL, "Sound command sent: %04X\n", wordValue);

			SoundLatch = wordValue;
			SoundLatchStatus |= 0x0C;

			ZetOpen(0);
			ZetNmi();
			nCaveCyclesDone[1] += ZetRun(0x0400);
			ZetClose();
			return;

		case 0x700000:
			wordValue >>= 8;
			nCaveTileBank = wordValue & 1;
			EEPROMWrite(wordValue & 0x04, wordValue & 0x02, wordValue & 0x08);
			return;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write word value %x to location %x\n", wordValue, sekAddress);
//		}
	}
}

void __fastcall sailormnWriteByteSprite(UINT32 sekAddress, UINT8 byteValue)
{
	bSpriteUpdate = 1;
	CaveSpriteRAM[(sekAddress ^ 0x01) & 0xFFFF] = byteValue;
}

void __fastcall sailormnWriteWordSprite(UINT32 sekAddress, UINT16 wordValue)
{
	bSpriteUpdate = 1;
	((UINT16 *)CaveSpriteRAM)[(sekAddress & 0xFFFF) >> 1] = wordValue;
}

void __fastcall sailormnWriteBytePalette(UINT32 sekAddress, UINT8 byteValue)
{
	CavePalWriteByte(sekAddress & 0xFFFF, byteValue);
}

void __fastcall sailormnWriteWordPalette(UINT32 sekAddress, UINT16 wordValue)
{
	CavePalWriteWord(sekAddress & 0xFFFF, wordValue);
}

static int DrvExit()
{
	EEPROMExit();

	MSM6295Exit(1);
	MSM6295Exit(0);
	BurnYM2151Exit();

	CaveTileExit();
	CaveSpriteExit();
    CavePalExit();

	ZetExit();

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

	nCurrentBank = -1;
	drvZ80Bankswitch(0);

	ZetReset();

	EEPROMReset();

	nVideoIRQ = 1;
	nSoundIRQ = 1;
	nUnknownIRQ = 1;

	SoundLatch = 0;
	SoundLatchStatus = 0x0C;

	memset(SoundLatchReply, 0, sizeof(SoundLatchReply));
	SoundLatchReplyIndex = 0;
	SoundLatchReplyMax = -1;

	nCaveCyclesDone[0] = nCaveCyclesDone[1] = 0;

	MSM6295Reset(0);
	MSM6295Reset(1);
	BurnYM2151Reset();

	return 0;
}

static int DrvDraw()
{
	if (CaveRecalcPalette) {
		CavePalUpdate8Bit(0x4400, 12);
		CaveRecalcPalette = 1;
	}
	CavePalUpdate4Bit(0, 64);

	CaveClearScreen(CavePalette[nWhichGame ? 0x3F00 : 0x5FF0]);

	if (bDrawScreen) {
//		CaveGetBitmap();

#if 0
		CaveTileRender(1);
#else
		if (nWhichGame) {
			// Air Gallet always enables row-scroll and row-select for 16x16 layers, but always without effect
			// So, force tile drawing routines to ignore row-scroll and row-select
			CaveTileRender(0);
		} else {
			CaveTileRender(1);
		}
#endif
	
	}

	return 0;
}

inline static INT8 CheckSleep(int)
{
	return 0;
}

#define CAVE_INTERLEAVE  (4)

#ifndef BUILD_PSP
	#define M68K_CYCLES_PER_FRAME  ((int)((INT64)16000000 * nBurnCPUSpeedAdjust / (0x0100 * CAVE_REFRESHRATE)))
#else
	#define M68K_CYCLES_PER_FRAME  ((int)(16000000 / CAVE_REFRESHRATE))
#endif

#define Z80_CYCLES_PER_FRAME  ((int)(8000000 / CAVE_REFRESHRATE))

#define M68K_CYCLES_VBLANK (M68K_CYCLES_PER_FRAME - (int)((M68K_CYCLES_PER_FRAME * CAVE_VBLANK_LINES) / 271.5))

#define M68K_CYLECS_PER_INTER  (M68K_CYCLES_PER_FRAME / CAVE_INTERLEAVE)
#define  Z80_CYCLES_PER_INTER  ( Z80_CYCLES_PER_FRAME / CAVE_INTERLEAVE)

static int DrvFrame()
{
	int nCyclesSegment;
	int nCyclesVBlank;
	
	INT16 nSegmentLength = nBurnSoundLen / CAVE_INTERLEAVE;
	INT16 *pSoundBuf = pBurnSoundOut;
	INT16 nSoundBufferPos = 0;
	
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
	
	nCaveCyclesDone[0] = 0;
	nCaveCyclesDone[1] -= Z80_CYCLES_PER_FRAME;
	
	if (nCaveCyclesDone[1] < 0) {
		nCaveCyclesDone[1] = 0;
	}
	
	nCyclesVBlank = M68K_CYCLES_VBLANK;
	nVBlank = 0;
	
	SekOpen(0);
	
	for (INT8 i = 1; i <= CAVE_INTERLEAVE; i++) {
		INT8 nCurrentCPU;
		int nNext;
		
		// Run 68000
		nCurrentCPU = 0;
		nNext = i * M68K_CYLECS_PER_INTER;
		
		// Trigger VBlank interrupt
		if (!nVBlank && (nNext > nCyclesVBlank)) {
			if (nCaveCyclesDone[nCurrentCPU] < nCyclesVBlank) {
				nCyclesSegment = nCyclesVBlank - nCaveCyclesDone[nCurrentCPU];
				nCaveCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
			}
			
			if (pBurnDraw != NULL) {
				DrvDraw();												// Draw screen if needed
			}
			
			if (bSpriteUpdate) {
				CaveSpriteBuffer();
				bSpriteUpdate = 0;
			}
			
			nVBlank = 1;
			nVideoIRQ = 0;
			UpdateIRQStatus();
		}
		
		nCyclesSegment = nNext - nCaveCyclesDone[nCurrentCPU];
		if (nVBlank || (!CheckSleep(nCurrentCPU))) {					// See if this CPU is busywaiting
			nCaveCyclesDone[nCurrentCPU] += SekRun(nCyclesSegment);
		} else {
			nCaveCyclesDone[nCurrentCPU] += SekIdle(nCyclesSegment);
		}
		
		// Run Z80
		nCurrentCPU = 1;
		nNext = i * Z80_CYCLES_PER_INTER;
		ZetOpen(0);
		nCyclesSegment = nNext - nCaveCyclesDone[nCurrentCPU];
		nCaveCyclesDone[nCurrentCPU] += ZetRun(nCyclesSegment);
		ZetClose();
		
		// Render sound segment
		if (pBurnSoundOut) {
			pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			ZetOpen(0);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			MSM6295Render(0, pSoundBuf, nSegmentLength);
			MSM6295Render(1, pSoundBuf, nSegmentLength);
			ZetClose();
			nSoundBufferPos += nSegmentLength;
		}
	}
	
	SekClose();
	
	// Make sure the buffer is entirely filled.
	if (pBurnSoundOut) {
		nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
		if (nSegmentLength) {
			ZetOpen(0);
			BurnYM2151Render(pSoundBuf, nSegmentLength);
			MSM6295Render(0, pSoundBuf, nSegmentLength);
			MSM6295Render(1, pSoundBuf, nSegmentLength);
			ZetClose();
		}
	}
	
	return 0;
}

// This routine is called first to determine how much memory is needed (MemEnd-(UINT8 *)0),
// and then afterwards to set up all the pointers
static INT8 MemIndex()
{
	UINT8 *Next; Next = Mem;
	Rom01			= Next; Next += 0x080000;		// 68K program
	Rom02			= Next; Next += 0x200000;
	RomZ80			= Next; Next += 0x080000;
/*
	CaveSpriteROM	= Next; Next += 0x800000;
	CaveTileROM[0]	= Next; Next += 0x400000;		// Tile layer 0
	CaveTileROM[1]	= Next; Next += 0x400000;		// Tile layer 1
	if (nWhichGame) {
		CaveTileROM[2]	= Next; Next += 0x400000;		// Tile layer 2 (agallet)
	} else {
		CaveTileROM[2]	= Next; Next += 0x01400000;		// Tile layer 2 (Sailor Moon)
	}
*/
	MSM6295ROM		= Next; Next += 0x400000;		// MSM6295 ADPCM data

	RamStart		= Next;
	Ram01			= Next; Next += 0x010002;		// CPU #0 work RAM
	Ram02			= Next; Next += 0x008000;		//
	Ram03			= Next; Next += 0x005002;		//
	RamZ80			= Next; Next += 0x002000;
	CaveTileRAM[0]	= Next; Next += 0x008000;
	CaveTileRAM[1]	= Next; Next += 0x008000;
	CaveTileRAM[2]	= Next; Next += 0x008002;
	CaveSpriteRAM	= Next; Next += 0x010000;
	Ram04			= Next; Next += 0x000002;		//
	CavePalSrc		= Next; Next += 0x010000;		// palette
	RamEnd			= Next;
	MemEnd			= Next;

	return 0;
}


static INT8 sailormnLoadRoms()
{
	UINT8 *pTemp;

	// Load 68000 ROM
	BurnLoadRom(Rom01, 0, 1);
	BurnLoadRom(Rom02, 1, 1);

	// Load Z80 ROM
	BurnLoadRom(RomZ80, 2, 1);
/*
	pTemp = (UINT8 *)malloc(0x400000);
	BurnLoadRom(pTemp + 0x000000, 3, 1);
	BurnLoadRom(pTemp + 0x200000, 4, 1);
	for (int i = 0; i < 0x400000; i++) {
		CaveSpriteROM[i ^ 0x950C4] = pTemp[BITSWAP24(i, 23, 22, 21, 20, 15, 10, 12, 6, 11, 1, 13, 3, 16, 17, 2, 5, 14, 7, 18, 8, 4, 19, 9, 0)];
	}
	free(pTemp);
	CaveNibbleSwap1(CaveSpriteROM, 0x400000);

	BurnLoadRom(CaveTileROM[0], 5, 1);
	CaveNibbleSwap2(CaveTileROM[0], 0x200000);
	BurnLoadRom(CaveTileROM[1], 6, 1);
	CaveNibbleSwap2(CaveTileROM[1], 0x200000);
	BurnLoadRom(CaveTileROM[2] + 0x000000, 7, 1);
	BurnLoadRom(CaveTileROM[2] + 0x200000, 8, 1);
	BurnLoadRom(CaveTileROM[2] + 0x400000, 9, 1);
	BurnLoadRom(CaveTileROM[2] + 0x600000, 10, 1);
	BurnLoadRom(CaveTileROM[2] + 0x800000, 11, 1);
	CaveNibbleSwap2(CaveTileROM[2], 0xA00000);

	pTemp = (UINT8 *)malloc(0x600000);
	BurnLoadRom(pTemp + 0x000000, 12, 1);
	BurnLoadRom(pTemp + 0x200000, 13, 1);
	BurnLoadRom(pTemp + 0x400000, 14, 1);
	for (int i = 0; i < 0x500000; i++) {
		CaveTileROM[2][(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
		CaveTileROM[2][(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
		CaveTileROM[2][(i << 2) + 2] |= (pTemp[i] & 0x30);
		CaveTileROM[2][(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
	}
	free(pTemp);
*/
	// Load OKIM6295 data
	BurnLoadRom(MSM6295ROM + 0x0000000, 15, 1);
	BurnLoadRom(MSM6295ROM + 0x0200000, 16, 1);
	BurnLoadRom(MSM6295ROM + 0x0280000, 16, 1);
	BurnLoadRom(MSM6295ROM + 0x0300000, 16, 1);
	BurnLoadRom(MSM6295ROM + 0x0380000, 16, 1);

	return 0;
}

static INT8 agalletLoadRoms()
{
	// Load 68000 ROM
	BurnLoadRom(Rom01, 0, 1);

	// Load Z80 ROM
	BurnLoadRom(RomZ80, 1, 1);
/*
	BurnLoadRom(CaveSpriteROM + 0x000000, 2, 1);
	BurnLoadRom(CaveSpriteROM + 0x200000, 3, 1);
	CaveNibbleSwap1(CaveSpriteROM, 0x400000);

	BurnLoadRom(CaveTileROM[0], 4, 1);
	CaveNibbleSwap2(CaveTileROM[0], 0x200000);
	BurnLoadRom(CaveTileROM[1], 5, 1);
	CaveNibbleSwap2(CaveTileROM[1], 0x200000);
	BurnLoadRom(CaveTileROM[2], 6, 1);
	CaveNibbleSwap2(CaveTileROM[2], 0x200000);

	UINT8* pTemp = (UINT8 *)malloc(0x200000);
	BurnLoadRom(pTemp, 7, 1);
	for (int i = 0; i < 0x0100000; i++) {
		CaveTileROM[2][(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
		CaveTileROM[2][(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
		CaveTileROM[2][(i << 2) + 2] |= (pTemp[i] & 0x30);
		CaveTileROM[2][(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
	}
	free(pTemp);
*/
	// Load OKIM6295 data
	BurnLoadRom(MSM6295ROM + 0x0000000, 8, 1);
	BurnLoadRom(MSM6295ROM + 0x0200000, 9, 1);

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

		SekScan(nAction);				// scan 68000
		ZetScan(nAction);				// scan Z80

		SCAN_VAR(SoundLatch);
		SCAN_VAR(SoundLatchStatus);
		SCAN_VAR(SoundLatchReply);
		SCAN_VAR(SoundLatchReplyIndex);
		SCAN_VAR(SoundLatchReplyMax);

		SCAN_VAR(nCurrentBank);

		MSM6295Scan(0, nAction);
		MSM6295Scan(1, nAction);
		BurnYM2151Scan(nAction);

		SCAN_VAR(DrvOkiBank1);
		SCAN_VAR(DrvOkiBank2);

		SCAN_VAR(nVideoIRQ);
		SCAN_VAR(nSoundIRQ);
		SCAN_VAR(nUnknownIRQ);
		SCAN_VAR(nVBlank);

		CaveScanGraphics();

		SCAN_VAR(DrvInput);

		if (nAction & ACB_WRITE) {
			UINT8 nBank = nCurrentBank;
			nCurrentBank = -1;
			drvZ80Bankswitch(nBank);

			drvOkiBankswitch(0, DrvOkiBank1);
			drvOkiBankswitch(1, DrvOkiBank2);

			CaveSpriteBuffer();
			CaveRecalcPalette = 1;
		}
	}

	return 0;
}


// Air Gallet (Europe)
static const UINT8 ALIGN_DATA agallet_default_eeprom[0x80] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Sailor Moon (Europe)
static const UINT8 ALIGN_DATA sailormn_default_eeprom[0x80] =
{
	0xA5, 0x00, 0xA5, 0x00, 0xA5, 0x00, 0xA5, 0x00, 0xA5, 0x01, 0xA5, 0x01, 0xA5, 0x04, 0xA5, 0x01,
	0xA5, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static INT8 gameInit()
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
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();													// Index the allocated memory

	EEPROMInit(&eeprom_interface_93C46);
	
	if (nWhichGame) {
		// Load the roms into memory
		if (agalletLoadRoms()) return 1;
		
		EEPROMLoadRom(10);
		if (!EEPROMAvailable()) EEPROMFill(agallet_default_eeprom,0, sizeof(agallet_default_eeprom));
	} else {
		// Load the roms into memory
		if (sailormnLoadRoms()) return 1;
		
		EEPROMLoadRom(17);
		if (!EEPROMAvailable()) EEPROMFill(sailormn_default_eeprom,0, sizeof(sailormn_default_eeprom));
	}

	initCacheStructure(0.8);

	{
		SekInit(0, 0x68000);												// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,					0x000000, 0x07FFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,					0x100000, 0x110001, SM_RAM);	// ram (Air Gallet tests too far)
		SekMapMemory(Rom02,					0x200000, 0x3FFFFF, SM_ROM);
		SekMapMemory(Ram02,					0x400000, 0x407FFF, SM_RAM);
		SekMapMemory(Ram03,					0x40C000, 0x410001, SM_RAM);	// RAM (Air Gallet tests too far)

		SekMapMemory(CaveSpriteRAM,			0x500000, 0x50FFFF, SM_ROM);	// Sprite RAM (Air Gallet tests too far)
		SekMapHandler(1,					0x500000, 0x50FFFF, SM_WRITE);	// Sprite RAM (Air Gallet tests too far)

		SekMapMemory(Ram04,					0x510000, 0x510001, SM_RAM);	// RAM

		SekMapMemory(CaveTileRAM[0],	   	0x800000, 0x807FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],	   	0x880000, 0x887FFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2],		0x900000, 0x908001, SM_RAM);	// Air Gallet tests too far

		SekMapMemory(CavePalSrc,		   	0x408000, 0x4087FF, SM_RAM);	// Palette RAM
		SekMapMemory(CavePalSrc + 0x8800,	0x408800, 0x40BFFF, SM_ROM);	// Palette RAM (write goes through handler)
		SekMapHandler(2,					0x408800, 0x40BFFF, SM_WRITE);	//

		SekSetReadWordHandler (0, sailormnReadWord);
		SekSetReadByteHandler (0, sailormnReadByte);
		SekSetWriteWordHandler(0, sailormnWriteWord);
		SekSetWriteByteHandler(0, sailormnWriteByte);

		SekSetWriteWordHandler(1, sailormnWriteWordSprite);
		SekSetWriteByteHandler(1, sailormnWriteByteSprite);

		SekSetWriteWordHandler(2, sailormnWriteWordPalette);
		SekSetWriteByteHandler(2, sailormnWriteBytePalette);

		SekClose();
	}

	drvZInit();

	nCaveExtraXOffset = -1;
	nCaveRowModeOffset = 2;
	CaveSpriteVisibleXOffset = -1;

	CavePalInit(0x8000);
	CaveTileInit();

	if (nWhichGame) {
		CaveSpriteInit(1, 0x0800000);
		CaveTileInitLayer(0, 0x00400000, 4, 0x4400);
		CaveTileInitLayer(1, 0x00400000, 4, 0x4800);
		CaveTileInitLayer(2, 0x00400000, 6, 0x4C00);
	} else {
		CaveSpriteInit(2, 0x0800000);
		CaveTileInitLayer(0, 0x00400000, 4, 0x4400);
		CaveTileInitLayer(1, 0x00400000, 4, 0x4800);
		CaveTileInitLayer(2, 0x01400000, 6, 0x4C00);
	}

	BurnYM2151Init(16000000 / 4, 25.0);
	BurnYM2151SetIrqHandler(&drvYM2151IRQHandler);

	MSM6295Init(0, 2112000 / 132, 80.0, 1);
	MSM6295Init(1, 2112000 / 132, 80.0, 1);

	bDrawScreen = true;

#if 0

#if defined FBA_DEBUG && defined USE_SPEEDHACKS
	bprintf(PRINT_IMPORTANT, "  * Using speed-hacks (detecting idle loops).\n");
#endif

#endif

	DrvDoReset();

	return 0;
}

static int sailormnInit()
{
	nWhichGame = 0;

	initUniCache(0x2400000);

	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x800000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x400000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x400000;
	
	if (needCreateCache) {
		UINT8 *pTemp;
		if ( (uniCacheHead = (UINT8 *)malloc(0xc00000)) == NULL ) return 1;
		
		// Sprite
		pTemp = uniCacheHead + 0x800000;
		memset(uniCacheHead, 0, 0xc00000);
		BurnLoadRom(pTemp + 0x000000, 3, 1);
		BurnLoadRom(pTemp + 0x200000, 4, 1);
		for (int i = 0; i < 0x400000; i++) {
			uniCacheHead[i ^ 0x950C4] = pTemp[BITSWAP24(i, 23, 22, 21, 20, 15, 10, 12, 6, 11, 1, 13, 3, 16, 17, 2, 5, 14, 7, 18, 8, 4, 19, 9, 0)];
		}
		CaveNibbleSwap1(uniCacheHead, 0x400000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0xc00000);
		BurnLoadRom(uniCacheHead + 0x000000, 5, 1);  // Tile Layer 0
		BurnLoadRom(uniCacheHead + 0x200000, 6, 1);  // Tile Layer 1
		CaveNibbleSwap2(uniCacheHead, 0x400000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		// Tile Layer 2
		memset(uniCacheHead, 0, 0xc00000);
		BurnLoadRom(uniCacheHead + 0x000000, 7, 1);
		BurnLoadRom(uniCacheHead + 0x200000, 8, 1);
		CaveNibbleSwap2(uniCacheHead, 0x400000);
		BurnLoadRom(pTemp, 12, 1);
		for (int i = 0; i < 0x200000; i++) {
			uniCacheHead[(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
			uniCacheHead[(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
			uniCacheHead[(i << 2) + 2] |= (pTemp[i] & 0x30);
			uniCacheHead[(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
		}
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0xc00000);
		BurnLoadRom(uniCacheHead + 0x000000,  9, 1);
		BurnLoadRom(uniCacheHead + 0x200000, 10, 1);
		CaveNibbleSwap2(uniCacheHead, 0x400000);
		BurnLoadRom(pTemp, 13, 1);
		for (int i = 0; i < 0x200000; i++) {
			uniCacheHead[(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
			uniCacheHead[(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
			uniCacheHead[(i << 2) + 2] |= (pTemp[i] & 0x30);
			uniCacheHead[(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
		}
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0xc00000);
		BurnLoadRom(uniCacheHead, 11, 1);
		CaveNibbleSwap2(uniCacheHead, 0x200000);
		BurnLoadRom(pTemp, 14, 1);
		for (int i = 0; i < 0x100000; i++) {
			uniCacheHead[(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
			uniCacheHead[(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
			uniCacheHead[(i << 2) + 2] |= (pTemp[i] & 0x30);
			uniCacheHead[(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
		}
		sceIoWrite(cacheFile, uniCacheHead, 0x400000);
		
		sceIoClose(cacheFile);
		cacheFile = sceIoOpen(filePathName, PSP_O_RDONLY, 0777);
		
		free(uniCacheHead);
		uniCacheHead = NULL;
	}

	return gameInit();
}

static int agalletInit()
{
	nWhichGame = 1;

	initUniCache(0x1400000);

	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x800000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x400000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x400000;
	
	if (needCreateCache) {
		UINT8 *pTemp;
		if ( (uniCacheHead = (UINT8 *)malloc(0x800000)) == NULL ) return 1;
		
		// Sprite
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead + 0x000000, 2, 1);
		BurnLoadRom(uniCacheHead + 0x200000, 3, 1);
		CaveNibbleSwap1(uniCacheHead, 0x400000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead + 0x000000, 4, 1);  // Tile Layer 0
		BurnLoadRom(uniCacheHead + 0x200000, 5, 1);  // Tile Layer 1
		CaveNibbleSwap2(uniCacheHead, 0x400000);
		sceIoWrite(cacheFile, uniCacheHead, 0x800000);
		
		// Tile Layer 2
		memset(uniCacheHead, 0, 0x800000);
		BurnLoadRom(uniCacheHead, 6, 1);
		CaveNibbleSwap2(uniCacheHead, 0x200000);
		
		pTemp = uniCacheHead + 0x400000;
		BurnLoadRom(pTemp, 7, 1);
		for (int i = 0; i < 0x100000; i++) {
			uniCacheHead[(i << 2) + 0] |= (pTemp[i] & 0x03) << 4;
			uniCacheHead[(i << 2) + 1] |= (pTemp[i] & 0x0C) << 2;
			uniCacheHead[(i << 2) + 2] |= (pTemp[i] & 0x30);
			uniCacheHead[(i << 2) + 3] |= (pTemp[i] & 0xC0) >> 2;
		}
		sceIoWrite(cacheFile, uniCacheHead, 0x400000);
		
		sceIoClose(cacheFile);
		cacheFile = sceIoOpen(filePathName, PSP_O_RDONLY, 0777);
		
		free(uniCacheHead);
		uniCacheHead = NULL;
	}

	return gameInit();
}


// Rom information

#define ROMS_SAILORMN                                                                             \
  { "bpsm945a.u45", 0x080000, 0x898C9515, BRF_ESS | BRF_PRG }, /*  0 CPU #0 code */               \
  { "bpsm.u46",     0x200000, 0x32084E80, BRF_ESS | BRF_PRG }, /*  1             */               \
                                                                                                  \
  { "bpsm945a.u9",  0x080000, 0x438DE548, BRF_ESS | BRF_PRG }, /*  2 Z80 code */                  \
                                                                                                  \
  { "bpsm.u76",     0x200000, 0xA243A5BA, BRF_GRA },           /*  3 Sprite data */               \
  { "bpsm.u77",     0x200000, 0x5179A4AC, BRF_GRA },           /*  4             */               \
                                                                                                  \
  { "bpsm.u53",     0x200000, 0xB9B15F83, BRF_GRA },           /*  5 Layer 0 Tile data */         \
  { "bpsm.u54",     0x200000, 0x8F00679D, BRF_GRA },           /*  6 Layer 1 Tile data */         \
                                                                                                  \
  { "bpsm.u57",     0x200000, 0x86BE7B63, BRF_GRA },           /*  7 Layer 2 Tile data */         \
  { "bpsm.u58",     0x200000, 0xE0BBA83B, BRF_GRA },           /*  8                   */         \
  { "bpsm.u62",     0x200000, 0xA1E3BFAC, BRF_GRA },           /*  9                   */         \
  { "bpsm.u61",     0x200000, 0x6A014B52, BRF_GRA },           /* 10                   */         \
  { "bpsm.u60",     0x200000, 0x992468C0, BRF_GRA },           /* 11                   */         \
                                                                                                  \
  { "bpsm.u65",     0x200000, 0xF60FB7B5, BRF_GRA },           /* 12                   */         \
  { "bpsm.u64",     0x200000, 0x6559D31C, BRF_GRA },           /* 13                   */         \
  { "bpsm.u63",     0x200000, 0xD57A56B4, BRF_GRA },           /* 14                   */         \
                                                                                                  \
  { "bpsm.u48",     0x200000, 0x498E4ED1, BRF_SND },           /* 15 MSM6295 #0 ADPCM data */     \
  { "bpsm.u47",     0x080000, 0x0F2901B9, BRF_SND },           /* 16 MSM6295 #1 ADPCM data */     \


static struct BurnRomInfo sailormnRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_europe.nv",   0x0080, 0x59a7dc50, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormn);
STD_ROM_FN(sailormn);


static struct BurnRomInfo sailormnuRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_usa.nv",      0x0080, 0x3915abe3, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnu);
STD_ROM_FN(sailormnu);


static struct BurnRomInfo sailormnjRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_japan.nv",    0x0080, 0xea03c30a, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnj);
STD_ROM_FN(sailormnj);


static struct BurnRomInfo sailormnkRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_korea.nv",    0x0080, 0x0e7de398, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnk);
STD_ROM_FN(sailormnk);


static struct BurnRomInfo sailormntRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_taiwan.nv",   0x0080, 0x6c7e8c2a, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnt);
STD_ROM_FN(sailormnt);


static struct BurnRomInfo sailormnhRomDesc[] = {
  ROMS_SAILORMN

  { "sailormn_hongkong.nv", 0x0080, 0x4d24c874, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnh);
STD_ROM_FN(sailormnh);


#define ROMS_SAILORMNO                                                                            \
  { "smprg.u45",    0x080000, 0x234F1152, BRF_ESS | BRF_PRG }, /*  0 CPU #0 code */               \
  { "bpsm.u46",     0x200000, 0x32084E80, BRF_ESS | BRF_PRG }, /*  1             */               \
                                                                                                  \
  { "bpsm945a.u9",  0x080000, 0x438DE548, BRF_ESS | BRF_PRG }, /*  2 Z80 code */                  \
                                                                                                  \
  { "bpsm.u76",     0x200000, 0xA243A5BA, BRF_GRA },           /*  3 Sprite data */               \
  { "bpsm.u77",     0x200000, 0x5179A4AC, BRF_GRA },           /*  4             */               \
                                                                                                  \
  { "bpsm.u53",     0x200000, 0xB9B15F83, BRF_GRA },           /*  5 Layer 0 Tile data */         \
  { "bpsm.u54",     0x200000, 0x8F00679D, BRF_GRA },           /*  6 Layer 1 Tile data */         \
                                                                                                  \
  { "bpsm.u57",     0x200000, 0x86BE7B63, BRF_GRA },           /*  7 Layer 2 Tile data */         \
  { "bpsm.u58",     0x200000, 0xE0BBA83B, BRF_GRA },           /*  8                   */         \
  { "bpsm.u62",     0x200000, 0xA1E3BFAC, BRF_GRA },           /*  9                   */         \
  { "bpsm.u61",     0x200000, 0x6A014B52, BRF_GRA },           /* 10                   */         \
  { "bpsm.u60",     0x200000, 0x992468C0, BRF_GRA },           /* 11                   */         \
                                                                                                  \
  { "bpsm.u65",     0x200000, 0xF60FB7B5, BRF_GRA },           /* 12                   */         \
  { "bpsm.u64",     0x200000, 0x6559D31C, BRF_GRA },           /* 13                   */         \
  { "bpsm.u63",     0x200000, 0xD57A56B4, BRF_GRA },           /* 14                   */         \
                                                                                                  \
  { "bpsm.u48",     0x200000, 0x498E4ED1, BRF_SND },           /* 15 MSM6295 #0 ADPCM data */     \
  { "bpsm.u47",     0x080000, 0x0F2901B9, BRF_SND },           /* 16 MSM6295 #1 ADPCM data */     \


static struct BurnRomInfo sailormnoRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_europe.nv",   0x0080, 0x59a7dc50, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormno);
STD_ROM_FN(sailormno);


static struct BurnRomInfo sailormnouRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_usa.nv",      0x0080, 0x3915abe3, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnou);
STD_ROM_FN(sailormnou);


static struct BurnRomInfo sailormnojRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_japan.nv",    0x0080, 0xea03c30a, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnoj);
STD_ROM_FN(sailormnoj);


static struct BurnRomInfo sailormnokRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_korea.nv",    0x0080, 0x0e7de398, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnok);
STD_ROM_FN(sailormnok);


static struct BurnRomInfo sailormnotRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_taiwan.nv",   0x0080, 0x6c7e8c2a, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnot);
STD_ROM_FN(sailormnot);


static struct BurnRomInfo sailormnohRomDesc[] = {
  ROMS_SAILORMNO

  { "sailormn_hongkong.nv", 0x0080, 0x4d24c874, BRF_OPT },     /* 17 EEPROM 93C46 data */
};

STD_ROM_PICK(sailormnoh);
STD_ROM_FN(sailormnoh);


#define ROMS_AGALLET                                                                              \
  { "bp962a.u45",   0x080000, 0x24815046, BRF_ESS | BRF_PRG }, /*  0 CPU #0 code */               \
  { "bp962a.u9",    0x080000, 0x06CADDBE, BRF_ESS | BRF_PRG }, /*  1 Z80 code    */               \
                                                                                                  \
  { "bp962a.u76",   0x200000, 0x858DA439, BRF_GRA },           /*  2 Sprite data */               \
  { "bp962a.u77",   0x200000, 0xEA2BA35E, BRF_GRA },           /*  3             */               \
                                                                                                  \
  { "bp962a.u53",   0x200000, 0xFCD9A107, BRF_GRA },           /*  4 Layer 0 Tile data */         \
  { "bp962a.u54",   0x200000, 0x0CFA3409, BRF_GRA },           /*  5 Layer 1 Tile data */         \
  { "bp962a.u57",   0x200000, 0x6D608957, BRF_GRA },           /*  6 Layer 2 Tile data */         \
  { "bp962a.u65",   0x200000, 0x135FCF9A, BRF_GRA },           /*  7                   */         \
                                                                                                  \
  { "bp962a.u48",   0x200000, 0xAE00A1CE, BRF_SND },           /*  8 MSM6295 #0 ADPCM data */     \
  { "bp962a.u47",   0x200000, 0x6D4E9737, BRF_SND },           /*  9 MSM6295 #1 ADPCM data */     \

static struct BurnRomInfo agalletRomDesc[] = {
  ROMS_AGALLET

  { "agallet_europe.nv",   0x0080, 0xec38bf65, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agallet);
STD_ROM_FN(agallet);


static struct BurnRomInfo agalletuRomDesc[] = {
  ROMS_AGALLET

  { "agallet_usa.nv",      0x0080, 0x72e65056, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agalletu);
STD_ROM_FN(agalletu);


static struct BurnRomInfo agalletjRomDesc[] = {
  ROMS_AGALLET

  { "agallet_japan.nv",    0x0080, 0x0753f547, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agalletj);
STD_ROM_FN(agalletj);


static struct BurnRomInfo agalletkRomDesc[] = {
  ROMS_AGALLET

  { "agallet_korea.nv",    0x0080, 0x7f41c253, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agalletk);
STD_ROM_FN(agalletk);


static struct BurnRomInfo agallettRomDesc[] = {
  ROMS_AGALLET

  { "agallet_taiwan.nv",   0x0080, 0x0af46742, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agallett);
STD_ROM_FN(agallett);


static struct BurnRomInfo agallethRomDesc[] = {
  ROMS_AGALLET

  { "agallet_hongkong.nv", 0x0080, 0x998d1a74, BRF_OPT },      /* 10 EEPROM 93C46 data */
};

STD_ROM_PICK(agalleth);
STD_ROM_FN(agalleth);


struct BurnDriver BurnDrvSailorMoonB = {
	"sailormn", NULL, NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Europe)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Europe)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, Europe)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnRomInfo, sailormnRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonBU = {
	"sailormnu", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22B, USA)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22B, USA)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, USA)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnuRomInfo, sailormnuRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonBJ = {
	"sailormnj", "sailormn", NULL, "1995",
	"Bishoujo Senshi Sailor Moon (Ver. 95/03/22B, Japan)\0", NULL, "BanPresto", "Cave",
	L"Bishoujo Senshi Sailor Moon (Ver. 95/03/22B, Japan)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, Japan)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnjRomInfo, sailormnjRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonBK = {
	"sailormnk", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Korea)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Korea)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, Korea)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnkRomInfo, sailormnkRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonBT = {
	"sailormnt", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Taiwan)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Taiwan)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, Taiwan)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormntRomInfo, sailormntRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonBH = {
	"sailormnh", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Hong Kong)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22B, Hong Kong)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22B, Hong Kong)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnhRomInfo, sailormnhRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoon = {
	"sailormno", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22, Europe)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22, Europe)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, Europe)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnoRomInfo, sailormnoRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonU = {
	"sailormnou", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22, USA)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22, USA)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, USA)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnouRomInfo, sailormnouRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonJ = {
	"sailormnoj", "sailormn", NULL, "1995",
	"Bishoujo Senshi Sailor Moon (Ver. 95/03/22, Japan)\0", NULL, "BanPresto", "Cave",
	L"Bishoujo Senshi Sailor Moon (Ver. 95/03/22, Japan)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, Japan)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnojRomInfo, sailormnojRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonK = {
	"sailormnok", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22, Korea)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22, Korea)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, Korea)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnokRomInfo, sailormnokRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonT = {
	"sailormnot", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22, Taiwan)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22, Taiwan)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, Taiwan)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnotRomInfo, sailormnotRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvSailorMoonH = {
	"sailormnoh", "sailormn", NULL, "1995",
	"Pretty Soldier Sailor Moon (Ver. 95/03/22, Hong Kong)\0", NULL, "BanPresto", "Cave",
	L"Pretty Soldier Sailor Moon (Ver. 95/03/22, Hong Kong)\0\u7F8E\u5C11\u5973\u6226\u58EB \u30BB\u30FC\u30E9\u30FC\u30E0\u30FC\u30F3 (Ver. 95/03/22, Hong Kong)\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_SCRFIGHT, 0,
	NULL, sailormnohRomInfo, sailormnohRomName, sailormnInputInfo, NULL,
	sailormnInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 240, 4, 3
};

struct BurnDriver BurnDrvAirGallet = {
	"agallet", NULL, NULL, "1996",
	"Air Gallet (Europe)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agalletRomInfo, agalletRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvAirGalletU = {
	"agalletu", "agallet", NULL, "1996",
	"Air Gallet (USA)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agalletuRomInfo, agalletuRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvAirGalletJ = {
	"agalletj", "agallet", NULL, "1996",
	"Akuu Gallet (Japan)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agalletjRomInfo, agalletjRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvAirGalletK = {
	"agalletk", "agallet", NULL, "1996",
	"Air Gallet (Korea)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agalletkRomInfo, agalletkRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvAirGalletT = {
	"agallett", "agallet", NULL, "1996",
	"Air Gallet (Taiwan)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agallettRomInfo, agallettRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

struct BurnDriver BurnDrvAirGalletH = {
	"agalleth", "agallet", NULL, "1996",
	"Air Gallet (Hong Kong)\0", NULL, "BanPresto / Gazelle", "Cave",
	L"Air Gallet\0\u30A2\u30AF\u30A6\u30AE\u30E3\u30EC\u30C3\u30C8\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_Z80, GBF_VERSHOOT, 0,
	NULL, agallethRomInfo, agallethRomName, sailormnInputInfo, NULL,
	agalletInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	240, 320, 3, 4
};

