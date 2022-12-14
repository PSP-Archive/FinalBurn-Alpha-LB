// Gaia Crusaders / Thunder Heroes

#include "cave.h"
#include "ymz280b.h"

static UINT8  ALIGN_DATA DrvJoy1[16] = { 0, };
static UINT8  ALIGN_DATA DrvJoy2[16] = { 0, };
static UINT16 ALIGN_DATA DrvInput[3] = { 0, };

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;
static UINT8 *Rom01;
static UINT8 *Ram01;

static UINT8 DrvReset = 0;
static UINT8 bDrawScreen;
static INT8 nVBlank;

static INT8 nVideoIRQ;
static INT8 nSoundIRQ;
static INT8 nUnknownIRQ;

static INT8 nIRQPending;

static INT8 nCurrentCPU;
static int  nCyclesDone;

static UINT8 bSpriteUpdate;

static struct BurnInputInfo ALIGN_DATA gaiaInputList[] = {
  {"P1 Coin",      BIT_DIGITAL,  DrvJoy2 +  0,  "p1 coin"   },
  {"P1 Start",     BIT_DIGITAL,  DrvJoy2 +  4,  "p1 start"  },

  {"P1 Up",        BIT_DIGITAL,  DrvJoy1 +  0,  "p1 up"     },
  {"P1 Down",      BIT_DIGITAL,  DrvJoy1 +  1,  "p1 down"   },
  {"P1 Left",      BIT_DIGITAL,  DrvJoy1 +  2,  "p1 left"   },
  {"P1 Right",     BIT_DIGITAL,  DrvJoy1 +  3,  "p1 right"  },
  {"P1 Button 1",  BIT_DIGITAL,  DrvJoy1 +  4,  "p1 fire 1" },
  {"P1 Button 2",  BIT_DIGITAL,  DrvJoy1 +  5,  "p1 fire 2" },
  {"P1 Button 3",  BIT_DIGITAL,  DrvJoy1 +  6,  "p1 fire 3" },
  {"P1 Button 4",  BIT_DIGITAL,  DrvJoy1 +  7,  "p1 fire 4" },

  {"P2 Coin",      BIT_DIGITAL,  DrvJoy2 +  1,  "p2 coin"   },
  {"P2 Start",     BIT_DIGITAL,  DrvJoy2 +  5,  "p2 start"  },

  {"P2 Up",        BIT_DIGITAL,  DrvJoy1 +  8,  "p2 up"     },
  {"P2 Down",      BIT_DIGITAL,  DrvJoy1 +  9,  "p2 down"   },
  {"P2 Left",      BIT_DIGITAL,  DrvJoy1 + 10,  "p2 left"   },
  {"P2 Right",     BIT_DIGITAL,  DrvJoy1 + 11,  "p2 right"  },
  {"P2 Button 1",  BIT_DIGITAL,  DrvJoy1 + 12,  "p2 fire 1" },
  {"P2 Button 2",  BIT_DIGITAL,  DrvJoy1 + 13,  "p2 fire 2" },
  {"P2 Button 3",  BIT_DIGITAL,  DrvJoy1 + 14,  "p2 fire 3" },
  {"P2 Button 4",  BIT_DIGITAL,  DrvJoy1 + 15,  "p2 fire 4" },

  {"Reset",        BIT_DIGITAL,  &DrvReset,     "reset"     },
  {"Diagnostics",  BIT_DIGITAL,  DrvJoy2 +  2,  "diag"      },
  {"Service",      BIT_DIGITAL,  DrvJoy2 +  3,  "service"   },

  {"DIP A",        BIT_DIPSWITCH,  (UINT8*)(DrvInput + 2) + 0,  "dip"},
  {"DIP B",        BIT_DIPSWITCH,  (UINT8*)(DrvInput + 2) + 1,  "dip"},
};

STDINPUTINFO(gaia);

static struct BurnDIPInfo gaiaDIPList[] = {
	{0x17,	0xFF, 0xFF,	0x00, NULL},
	{0x18,	0xFF, 0xFF,	0x00, NULL},

	{0,		0xFE, 0,	2,	  "V reverse"},
	{0x17,	0x01, 0x01, 0x00, "Off"},
	{0x17,	0x01, 0x01, 0x01, "On"},
	{0,		0xFE, 0,	2,	  "Demo sound"},
	{0x17,	0x01, 0x02, 0x02, "Off"},
	{0x17,	0x01, 0x02, 0x00, "On"},
	{0,		0xFE, 0,	2,	  "Language"},
	{0x17,	0x01, 0x04, 0x00, "Japanese"},
	{0x17,	0x01, 0x04, 0x04, "English"},
	{0,		0xFE, 0,	9,	  "Coin"},
	{0x17,	0x01, 0x78, 0x00, "1 coin 1 credit"},
	{0x17,	0x01, 0x78, 0x08, "1 coin 2 credits"},
	{0x17,	0x01, 0x78, 0x10, "1 coin 3 credits"},
	{0x17,	0x01, 0x78, 0x18, "2 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x20, "2 coins 3 credits"},
	{0x17,	0x01, 0x78, 0x28, "3 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x30, "4 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x38, "2 coins 1 credit 1 continue"},
	{0x17,	0x01, 0x78, 0x40, "Free play"},
	{0x17,	0x01, 0x78, 0x48, "Free play"},
	{0x17,	0x01, 0x78, 0x50, "Free play"},
	{0x17,	0x01, 0x78, 0x58, "Free play"},
	{0x17,	0x01, 0x78, 0x60, "Free play"},
	{0x17,	0x01, 0x78, 0x68, "Free play"},
	{0x17,	0x01, 0x78, 0x70, "Free play"},
	{0x17,	0x01, 0x78, 0x78, "Free play"},
	{0,		0xFE, 0,	2,	  "Continue"},
	{0x17,	0x01, 0x80, 0x80, "Off"},
	{0x17,	0x01, 0x80, 0x00, "On"},

	{0,		0xFE, 0,	8,	  "Level"},
	{0x18,	0x01, 0xE0, 0x20, "Very easy"},
	{0x18,	0x01, 0xE0, 0x40, "Easy"},
	{0x18,	0x01, 0xE0, 0x00, "Medium"},
	{0x18,	0x01, 0xE0, 0x80, "Medium hard"},
	{0x18,	0x01, 0xE0, 0x60, "Hard 1"},
	{0x18,	0x01, 0xE0, 0xC0, "Hard 2"},
	{0x18,	0x01, 0xE0, 0xA0, "Very hard"},
	{0x18,	0x01, 0xE0, 0xE0, "Hardest"},
	{0,		0xFE, 0,	2,	  "Extend"},
	{0x18,	0x01, 0x04, 0x04, "Nothing"},
	{0x18,	0x01, 0x04, 0x00, "150,000/350,000"},
//	{0x18,	0x01, 0x04, 0x00, "150,000/350,000"},
//	{0x18,	0x01, 0x04, 0x00, "150,000/400,000"},
//	{0x18,	0x01, 0x04, 0x00, "200,000/500,000"},
	{0,		0xFE, 0,	4,	  "Life"},
	{0x18,	0x01, 0x03, 0x02, "1 player"},
	{0x18,	0x01, 0x03, 0x03, "2 players"},
	{0x18,	0x01, 0x03, 0x00, "3 players"},
	{0x18,	0x01, 0x03, 0x01, "4 players"},
	{0,		0xFE, 0,	4,	  "Damage"},
	{0x18,	0x01, 0x18, 0x00, "+0"},
	{0x18,	0x01, 0x18, 0x08, "+1"},
	{0x18,	0x01, 0x18, 0x10, "+2"},
	{0x18,	0x01, 0x18, 0x18, "+3"},
};

STDDIPINFO(gaia);

static struct BurnDIPInfo theroesDIPList[] = {
	{0x17,	0xFF, 0xFF,	0x04, NULL},
	{0x18,	0xFF, 0xFF,	0x00, NULL},

	{0,		0xFE, 0,	2,	  "V reverse"},
	{0x17,	0x01, 0x01, 0x00, "Off"},
	{0x17,	0x01, 0x01, 0x01, "On"},
	{0,		0xFE, 0,	2,	  "Demo sound"},
	{0x17,	0x01, 0x02, 0x02, "Off"},
	{0x17,	0x01, 0x02, 0x00, "On"},
	{0,		0xFE, 0,	2,	  "Language"},
	{0x17,	0x01, 0x04, 0x00, "Chinese"},
	{0x17,	0x01, 0x04, 0x04, "English"},
	{0,		0xFE, 0,	9,	  "Coin"},
	{0x17,	0x01, 0x78, 0x00, "1 coin 1 credit"},
	{0x17,	0x01, 0x78, 0x08, "1 coin 2 credits"},
	{0x17,	0x01, 0x78, 0x10, "1 coin 3 credits"},
	{0x17,	0x01, 0x78, 0x18, "2 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x20, "2 coins 3 credits"},
	{0x17,	0x01, 0x78, 0x28, "3 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x30, "4 coins 1 credit"},
	{0x17,	0x01, 0x78, 0x38, "2 coins 1 credit 1 continue"},
	{0x17,	0x01, 0x78, 0x40, "Free play"},
	{0x17,	0x01, 0x78, 0x48, "Free play"},
	{0x17,	0x01, 0x78, 0x50, "Free play"},
	{0x17,	0x01, 0x78, 0x58, "Free play"},
	{0x17,	0x01, 0x78, 0x60, "Free play"},
	{0x17,	0x01, 0x78, 0x68, "Free play"},
	{0x17,	0x01, 0x78, 0x70, "Free play"},
	{0x17,	0x01, 0x78, 0x78, "Free play"},
	{0,		0xFE, 0,	2,	  "Continue"},
	{0x17,	0x01, 0x80, 0x80, "Off"},
	{0x17,	0x01, 0x80, 0x00, "On"},

	{0,		0xFE, 0,	8,	  "Level"},
	{0x18,	0x01, 0xE0, 0x20, "Very easy"},
	{0x18,	0x01, 0xE0, 0x40, "Easy"},
	{0x18,	0x01, 0xE0, 0x00, "Medium"},
	{0x18,	0x01, 0xE0, 0x80, "Medium hard"},
	{0x18,	0x01, 0xE0, 0x60, "Hard 1"},
	{0x18,	0x01, 0xE0, 0xC0, "Hard 2"},
	{0x18,	0x01, 0xE0, 0xA0, "Very hard"},
	{0x18,	0x01, 0xE0, 0xE0, "Hardest"},
	{0,		0xFE, 0,	2,	  "Extend"},
	{0x18,	0x01, 0x04, 0x04, "Nothing"},
	{0x18,	0x01, 0x04, 0x00, "150,000/350,000"},
//	{0x18,	0x01, 0x04, 0x00, "150,000/350,000"},
//	{0x18,	0x01, 0x04, 0x00, "150,000/400,000"},
//	{0x18,	0x01, 0x04, 0x00, "200,000/500,000"},
	{0,		0xFE, 0,	4,	  "Life"},
	{0x18,	0x01, 0x03, 0x02, "1 player"},
	{0x18,	0x01, 0x03, 0x03, "2 players"},
	{0x18,	0x01, 0x03, 0x00, "3 players"},
	{0x18,	0x01, 0x03, 0x01, "4 players"},
	{0,		0xFE, 0,	4,	  "Damage"},
	{0x18,	0x01, 0x18, 0x00, "+0"},
	{0x18,	0x01, 0x18, 0x08, "+1"},
	{0x18,	0x01, 0x18, 0x10, "+2"},
	{0x18,	0x01, 0x18, 0x18, "+3"},
};

STDDIPINFO(theroes);

static void UpdateIRQStatus()
{
	nIRQPending = ((nVideoIRQ == 0) || (nSoundIRQ == 0) || (nUnknownIRQ == 0));
	SekSetIRQLine(1, nIRQPending ? SEK_IRQSTATUS_ACK : SEK_IRQSTATUS_NONE);
}

UINT8 __fastcall gaiaReadByte(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x300003: {
			return YMZ280BReadStatus();
		}

#if 0
		case 0x800000:
		case 0x800001: {
			UINT16 nRet = ((nVBlank ^ 1) << 2) | (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
#else
		case 0x800000:
		case 0x800001:
#endif
		case 0x800002:
		case 0x800003: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0x800004:
		case 0x800005: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0x800006:
		case 0x800007: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
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
			return (DrvInput[1] & 0xFF) ^ 0xFF;
		case 0xD00014:
			return (DrvInput[2] >> 8) ^ 0xFF;
		case 0xD00015:
			return (DrvInput[2] & 0xFF) ^ 0xFF;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read byte value of location %x\n", sekAddress);
//		}
	}
	return 0;
}

UINT16 __fastcall gaiaReadWord(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0x300002: {
			return YMZ280BReadStatus();
		}

#if 0
		case 0x800000: {
			UINT16 nRet = ((nVBlank ^ 1) << 2) | (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
#else
		case 0x800000:
#endif
		case 0x800002: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			return nRet;
		}
		case 0x800004: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nVideoIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}
		case 0x800006: {
			UINT16 nRet = (nUnknownIRQ << 1) | nVideoIRQ;
			nUnknownIRQ = 1;
			UpdateIRQStatus();
			return nRet;
		}

		case 0xD00010:
			return DrvInput[0] ^ 0xFFFF;
		case 0xD00012:
			return DrvInput[1] ^ 0xFFFF;
		case 0xD00014:
			return DrvInput[2] ^ 0xFFFF;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to read word value of location %x\n", sekAddress);
//		}
	}
	return 0;
}

void __fastcall gaiaWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	switch (sekAddress) {
		case 0x300001:
			YMZ280BSelectRegister(byteValue);
			break;
		case 0x300003:
			YMZ280BWriteRegister(byteValue);
			break;

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write byte value %x to location %x\n", byteValue, sekAddress);
//		}
	}
}

void __fastcall gaiaWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	switch (sekAddress) {
		case 0x300000:
			YMZ280BSelectRegister(wordValue & 0xFF);
			break;
		case 0x300002:
			YMZ280BWriteRegister(wordValue & 0xFF);
			break;

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

//		default: {
//			bprintf(PRINT_NORMAL, "Attempt to write word value %x to location %x\n", wordValue, sekAddress);
//		}
	}
}

void __fastcall gaiaWriteByteSprite(UINT32 sekAddress, UINT8 byteValue)
{
	bSpriteUpdate = 1;
	CaveSpriteRAM[(sekAddress ^ 0x01) & 0xFFFF] = byteValue;
}

void __fastcall gaiaWriteWordSprite(UINT32 sekAddress, UINT16 wordValue)
{
	bSpriteUpdate = 1;
	((UINT16 *)CaveSpriteRAM)[(sekAddress & 0xFFFF) >> 1] = wordValue;
}

void __fastcall gaiaWriteBytePalette(UINT32 sekAddress, UINT8 byteValue)
{
	CavePalWriteByte(sekAddress & 0xFFFF, byteValue);
}

void __fastcall gaiaWriteWordPalette(UINT32 sekAddress, UINT16 wordValue)
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
	CaveClearScreen(CavePalette[0x0000]);

	if (bDrawScreen) {
//		CaveGetBitmap();

		CaveTileRender(1);					// Render tiles
	}

	return 0;
}

inline static void gaiaClearOpposites(UINT8* nJoystickInputs)
{
	if ((*nJoystickInputs & 0x03) == 0x03) {
		*nJoystickInputs &= ~0x03;
	}
	if ((*nJoystickInputs & 0x0C) == 0x0C) {
		*nJoystickInputs &= ~0xC;
	}
}

inline static INT8 CheckSleep(int)
{
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
	for (INT8 i = 0; i < 16; i++) {
		DrvInput[0] |= (DrvJoy1[i] & 1) << i;
		DrvInput[1] |= (DrvJoy2[i] & 1) << i;
	}
	gaiaClearOpposites(((UINT8 *)DrvInput) + 0);
	gaiaClearOpposites(((UINT8 *)DrvInput) + 1);
	
	SekNewFrame();
	
	nCyclesDone = 0;
	
	nCyclesVBlank = M68K_CYCLES_VBLANK;
	nVBlank = 0;
	
	SekOpen(0);
	
	for (INT8 i = 1; i <= CAVE_INTERLEAVE; i++) {
		int nNext;
		
		// Run 68000
    	nCurrentCPU = 0;
		nNext = i * M68K_CYLECS_PER_INTER;
		
		// See if we need to trigger the VBlank interrupt
		if (!nVBlank && (nNext > nCyclesVBlank)) {
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
			
			nVBlank = 1;
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
	CaveTileROM[2]	= Next; Next += 0x400000;		// Tile layer 2
*/
	YMZ280BROM		= Next; Next += 0xC00000;

	RamStart		= Next;
	Ram01			= Next; Next += 0x010000;		// CPU #0 work RAM
	CaveTileRAM[0]	= Next; Next += 0x010000;
	CaveTileRAM[1]	= Next; Next += 0x010000;
	CaveTileRAM[2]	= Next; Next += 0x010000;
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
		memset(YMZ280BROM, 0, 0xC00000);
	}
/*
	BurnLoadRom(CaveSpriteROM + 0x000000, 2, 1);
	BurnLoadRom(CaveSpriteROM + 0x400000, 3, 1);
	CaveNibbleSwap1(CaveSpriteROM, 0x800000);

	BurnLoadRom(CaveTileROM[0] + 0x000000, 4, 1);
	CaveNibbleSwap4(CaveTileROM[0], 0x400000);
	BurnLoadRom(CaveTileROM[1] + 0x000000, 5, 1);
	CaveNibbleSwap4(CaveTileROM[1], 0x400000);
	BurnLoadRom(CaveTileROM[2] + 0x000000, 6, 1);
	CaveNibbleSwap4(CaveTileROM[2], 0x400000);
*/
	// Load YMZ280B data
	BurnLoadRom(YMZ280BROM + 0x000000, 7, 1);
	BurnLoadRom(YMZ280BROM + 0x400000, 8, 1);
	BurnLoadRom(YMZ280BROM + 0x800000, 9, 1);

	return 0;
}

// Scan ram
static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x020902;
	}

	if (nAction & ACB_MEMORY_ROM) {								// Scan all memory, devices & variables
		ba.Data		= Rom01;
		ba.nLen		= 0x00100000;
		ba.nAddress = 0;
		ba.szName	= "68K ROM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_MEMORY_RAM) {								// Scan all memory, devices & variables
		ba.Data		= Ram01;
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "68K RAM";
		BurnAcb(&ba);

		ba.Data		= CaveTileRAM[0];
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "Tilemap 0";
		BurnAcb(&ba);

		ba.Data		= CaveTileRAM[1];
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "Tilemap 1";
		BurnAcb(&ba);

		ba.Data		= CaveTileRAM[2];
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "Tile layer 2";
		BurnAcb(&ba);

		ba.Data		= CaveSpriteRAM;
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "Sprite tables";
		BurnAcb(&ba);

		ba.Data		= CavePalSrc;
		ba.nLen		= 0x00010000;
		ba.nAddress = 0;
		ba.szName	= "Palette";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {

		SekScan(nAction);				// scan 68000 states

		YMZ280BScan();

		SCAN_VAR(nVideoIRQ);
		SCAN_VAR(nSoundIRQ);
		SCAN_VAR(nUnknownIRQ);
		SCAN_VAR(nVBlank);

		CaveScanGraphics();

		SCAN_VAR(DrvInput);

		if (nAction & ACB_WRITE) {
			CaveSpriteBuffer();
			CaveRecalcPalette = 1;
		}
	}

	return 0;
}

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
	
	initUniCache(0x1C00000);
	
	// Load Sprite and Tile
	CaveSpriteROMOffset  = 0;
	CaveTileROMOffset[0] = CaveSpriteROMOffset  + 0x1000000;
	CaveTileROMOffset[1] = CaveTileROMOffset[0] + 0x400000;
	CaveTileROMOffset[2] = CaveTileROMOffset[1] + 0x400000;
	
	// Mem = 0x100000 (68K PRG ROM) + 0xC00000 (YMZ280B ROM) = 0xD00000
	if (needCreateCache) {
		// Sprite
		for (INT8 i = 2; i <= 3; i++) {
			memset(Mem, 0, nLen);
			BurnLoadRom(Mem, i, 1);
			CaveNibbleSwap1(Mem, 0x400000);
			sceIoWrite(cacheFile, Mem, 0x800000);
		}
		
		// Tile
		for (INT8 i = 4; i <= 6; i++) {
			memset(Mem, 0, nLen);
			BurnLoadRom(Mem, i, 1);
			CaveNibbleSwap4(Mem, 0x400000);
			sceIoWrite(cacheFile, Mem, 0x400000);
		}
		
		sceIoClose(cacheFile);
		cacheFile = sceIoOpen(filePathName, PSP_O_RDONLY, 0777);
	}
	
	memset(Mem, 0, nLen);										// blank all memory
	MemIndex();

	// Load the roms into memory
	if (LoadRoms()) {
		return 1;
	}

	initCacheStructure(0.7);													// Index the allocated memory

	{
		SekInit(0, 0x68000);												// Allocate 68000
	    SekOpen(0);

		// Map 68000 memory:
		SekMapMemory(Rom01,					0x000000, 0x0FFFFF, SM_ROM);	// CPU 0 ROM
		SekMapMemory(Ram01,					0x100000, 0x10FFFF, SM_RAM);

		SekMapMemory(CaveSpriteRAM,			0x400000, 0x40FFFF, SM_ROM);
		SekMapHandler(1,					0x400000, 0x40FFFF, SM_WRITE);

		SekMapMemory(CaveTileRAM[0],		0x500000, 0x50FFFF, SM_RAM);
		SekMapMemory(CaveTileRAM[1],		0x600000, 0x60FFFF, SM_RAM);
		SekMapMemory(CaveTileRAM[2],		0x700000, 0x70FFFF, SM_RAM);

		SekMapMemory(CavePalSrc,			0xC00000, 0xC0FFFF, SM_ROM);	// Palette RAM (write goes through handler)
		SekMapHandler(2,					0xC00000, 0xC0FFFF, SM_WRITE);	//

		SekSetReadWordHandler (0, gaiaReadWord);
		SekSetReadByteHandler (0, gaiaReadByte);
		SekSetWriteWordHandler(0, gaiaWriteWord);
		SekSetWriteByteHandler(0, gaiaWriteByte);

		SekSetWriteWordHandler(1, gaiaWriteWordSprite);
		SekSetWriteByteHandler(1, gaiaWriteByteSprite);

		SekSetWriteWordHandler(2, gaiaWriteWordPalette);
		SekSetWriteByteHandler(2, gaiaWriteBytePalette);

		SekClose();
	}

	nCaveRowModeOffset = 0;

	CavePalInit(0x8000);
	CaveTileInit();
	CaveSpriteInit(2, 0x1000000);
	CaveTileInitLayer(0, 0x400000, 8, 0x4000);
	CaveTileInitLayer(1, 0x400000, 8, 0x4000);
	CaveTileInitLayer(2, 0x400000, 8, 0x4000);

	YMZ280BInit(16934400, &TriggerSoundIRQ, 3);

	bDrawScreen = true;

	DrvDoReset(); // Reset machine

	return 0;
}

// Rom information
static struct BurnRomInfo gaiaRomDesc[] = {
	{ "prg1.127",     0x080000, 0x47B904B2, BRF_ESS | BRF_PRG }, //  0 CPU #0 code
	{ "prg2.128",     0x080000, 0x469B7794, BRF_ESS | BRF_PRG }, //  1

	{ "obj1.736",     0x400000, 0xF4F84E5D, BRF_GRA },			 //  2 Sprite data
	{ "obj2.738",     0x400000, 0x15C2A9CE, BRF_GRA },			 //  3

	{ "bg1.989",      0x400000, 0x013A693D, BRF_GRA },			 //  4 Layer 0 Tile data
	{ "bg2.995",      0x400000, 0x783CC62F, BRF_GRA },			 //  5 Layer 1 Tile data
	{ "bg3.998",      0x400000, 0xbcd61d1c, BRF_GRA },			 //  6 Layer 2 Tile data

	{ "snd1.447",     0x400000, 0x92770A52, BRF_SND },			 //  7 YMZ280B (AD)PCM data
	{ "snd2.454",     0x400000, 0x329AE1CF, BRF_SND },			 //  8
	{ "snd3.455",     0x400000, 0x4048D64E, BRF_SND },			 //  9
};

STD_ROM_PICK(gaia);
STD_ROM_FN(gaia);

struct BurnDriver BurnDrvGaia = {
	"gaia", NULL, NULL, "1999",
	"Gaia Crusaders\0", NULL, "Noise Factory", "Cave",
	L"Gaia Crusaders \u5F81\u6226\u8005\0Gaia Crusaders \u30AC\u30A4\u30A2\u30AF\u30EB\u30BB\u30A4\u30C0\u30FC\u30BA\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_ONLY, GBF_SCRFIGHT, 0,
	NULL, gaiaRomInfo, gaiaRomName, gaiaInputInfo, gaiaDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 224, 10, 7
};

static struct BurnRomInfo theroesRomDesc[] = {
	{ "t-hero-epm1.u0127",    0x080000, 0x09db7195, BRF_ESS | BRF_PRG }, //  0 CPU #0 code
	{ "t-hero-epm0.u0129",    0x080000, 0x2d4e3310, BRF_ESS | BRF_PRG }, //  1

	{ "t-hero-obj1.u0736",    0x400000, 0x35090f7c, BRF_GRA },			 //  2 Sprite data
	{ "t-hero-obj2.u0738",    0x400000, 0x71605108, BRF_GRA },			 //  3

	{ "t-hero-bg1.u0999",     0x400000, 0x47b0fb40, BRF_GRA },			 //  4 Layer 0 Tile data
	{ "t-hero-bg2.u0995",     0x400000, 0xb16237a1, BRF_GRA },			 //  5 Layer 1 Tile data
	{ "t-hero-bg3.u0998",     0x400000, 0x08eb5604, BRF_GRA },			 //  6 Layer 2 Tile data

	{ "crvsaders-snd1.u0447", 0x400000, 0x92770A52, BRF_SND },			 //  7 YMZ280B (AD)PCM data
	{ "crvsaders-snd2.u0454", 0x400000, 0x329AE1CF, BRF_SND },			 //  8
	{ "t-hero-snd3.u0455",    0x400000, 0x52b0b2c0, BRF_SND },			 //  9
};

STD_ROM_PICK(theroes);
STD_ROM_FN(theroes);

struct BurnDriver BurnDrvTheroes = {
	"theroes", NULL, NULL, "2001",
	"Thunder Heroes\0", NULL, "Primetek Investments", "Cave",
	L"Thunder Heroes\0\u9739\u96F3\u82F1\u96C4 Thunder Heroes\0", NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_16BIT_ONLY, 2, HARDWARE_CAVE_68K_ONLY, GBF_SCRFIGHT, 0,
	NULL, theroesRomInfo, theroesRomName, gaiaInputInfo, theroesDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, DrvScan,
	0, NULL, NULL, NULL, &CaveRecalcPalette,
	320, 224, 10, 7
};

