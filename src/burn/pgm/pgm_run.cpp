#include "pgm.h"
#include "UniCache.h"

UINT8 ALIGN_DATA PgmJoy1[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmJoy2[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmJoy3[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmJoy4[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmBtn1[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmBtn2[8] = { 0,0,0,0,0,0,0,0 };
UINT8 ALIGN_DATA PgmInput[9] = { 0,0,0,0,0,0,0,0,0 };
UINT8 PgmReset = 0;

UINT32 nPGM68KROMLen = 0;
UINT32 nPGMTileROMLen = 0;
UINT32 nPGMTileROMExpLen = 0;
UINT32 nPGMSPRColROMLen = 0;
UINT32 nPGMSPRMaskROMLen = 0;
UINT32 nPGMSNDROMLen = 0;
UINT32 nPGMExternalARMLen = 0;

UINT32 *PgmBgRam, *PgmTxRam, *PgmPalette;
UINT16 *PgmRsRam, *PgmPalRam, *PgmVidReg, *PgmSprBuf;

UINT8 *PGM68KBIOS, *PGM68KROM;
UINT8 *PGMUSER0, *PGMUSER1;

static UINT16 *PgmSprRam;
static UINT8 *PgmZ80Ram, *Pgm68KRam;

static UINT8 *Mem = NULL, *MemEnd = NULL;
static UINT8 *RamStart, *RamEnd;

UINT8 nPgmPalRecalc = 0;
UINT8 nPgmZ80Work = 0;

UINT8 nCavePgm = 0;
UINT8 nPGMEnableIRQ4 = 0;

static INT8 nPgmCurrentBios = -1;

void (*pPgmInitCallback)() = NULL;
int  (*pPgmScanCallback)(int, int*) = NULL;
void (*pPgmResetCallback)() = NULL;

UINT32 nPGMTileROMOffset;
UINT32 nPGMTileROMExpOffset;
UINT32 nPGMSPRColROMOffset;
UINT32 nPGMSPRMaskROMOffset;
// UINT32 nPGMSNDROMOffset;

#define PGM_REFRESHRATE       (60.00f)
#define CAVE_PGM_REFRESHRATE  (59.17f) // cave pcb

#define M68K_CYCS_PER_FRAME      (int)(20000000     / PGM_REFRESHRATE)
#define  Z80_CYCS_PER_FRAME      (int)(33868800 / 4 / PGM_REFRESHRATE)

#define M68K_CYCS_PER_FRAME_CAVE (int)(20000000     / CAVE_PGM_REFRESHRATE)
#define  Z80_CYCS_PER_FRAME_CAVE (int)(33868800 / 4 / CAVE_PGM_REFRESHRATE)

#define PGM_INTER_LEAVE  8

#define M68K_CYCS_PER_INTER      (M68K_CYCS_PER_FRAME / PGM_INTER_LEAVE)
#define  Z80_CYCS_PER_INTER      ( Z80_CYCS_PER_FRAME / PGM_INTER_LEAVE)

#define M68K_CYCS_PER_INTER_CAVE (M68K_CYCS_PER_FRAME_CAVE / PGM_INTER_LEAVE)
#define  Z80_CYCS_PER_INTER_CAVE ( Z80_CYCS_PER_FRAME_CAVE / PGM_INTER_LEAVE)

static int ALIGN_DATA nCyclesPerInterLeave[2] = { M68K_CYCS_PER_INTER, Z80_CYCS_PER_INTER };


#define PGM_CREATE_CACHE_BUFFER_SIZE (0x1000000)
#define PGM_CACHE_SIZE (0x400000 + nPGMTileROMExpLen + nPGMSPRColROMLen + nPGMSPRMaskROMLen)


static INT8 pgmMemIndex()
{
	UINT8 *Next; Next = Mem;
	
	PGM68KBIOS	= Next; Next += 0x0080000;				// 68000 BIOS
	PGM68KROM	= Next; Next += nPGM68KROMLen;			// 68000 PRG (max 0x400000)
	
	PGMUSER0	= Next; Next += nPGMExternalARMLen;
	PGMUSER1	= Next; Next += 0x10000;

	RamStart	= Next;
	
	Pgm68KRam	= Next; Next += 0x0020000;				// 128K Main RAM
	PgmZ80Ram	= Next; Next += 0x0010000;
	
	PgmBgRam	= (UINT32 *)Next; Next += 0x0001000;
	PgmTxRam	= (UINT32 *)Next; Next += 0x0002000;
	PgmRsRam	= (UINT16 *)Next; Next += 0x0001000;	// Row Scroll
	PgmPalRam	= (UINT16 *)Next; Next += 0x0001200;	// Palette R5G5B5
	PgmVidReg	= (UINT16 *)Next; Next += 0x0010000;	// Video Regs inc. Zoom Table
	
	RamEnd		= Next;
	
	PgmSprRam	= (UINT16 *)Pgm68KRam;	// first 0xa00 of main ram = sprites, seems to be buffered, DMA? 
	PgmSprBuf	= (UINT16 *)Next; Next += 0x0000a00;
	PgmPalette	= (UINT32 *)Next; Next += 0x0001200 * sizeof(UINT32);
	
	MemEnd		= Next;
	
	return 0;
}


static void loadAndWriteRomToCache(INT8 i, UINT32 romLength)
{
	BurnLoadRom(uniCacheHead, i, 1);
	
	for (INT8 j = 0; j < 5; j++) {
		sceIoLseek( cacheFile, cacheFileSize, SEEK_SET );
		if (romLength == sceIoWrite(cacheFile, uniCacheHead, romLength))
			break;
	}
}

static void pgmCreateCacheTiles(INT8 num, int len)
{
	UINT8 *src = uniCacheHead + (PGM_CREATE_CACHE_BUFFER_SIZE - (len + 0x180000));
	UINT8 *dst = uniCacheHead;
	
	BurnLoadRom(src, 0x0080, 1);
	BurnLoadRom(src + 0x180000, num, 1);
	
	// expands 8x8 4-bit data
	for (int i = 0; i < 0x200000; i++) {
		dst[(i << 1) + 0] = src[i] & 0x0F;
		dst[(i << 1) + 1] = src[i] >> 4;
	}
	sceIoWrite(cacheFile, uniCacheHead, 0x400000);
	
	// expands 32x32 5-bit data
	for (int i = 0; i < (len + 0x180000) / 5; i++) {
		dst[(i * 8) + 0] = ((src[(i * 5) + 0] >> 0) & 0x1F);
		dst[(i * 8) + 1] = ((src[(i * 5) + 0] >> 5) & 0x07) | ((src[(i * 5) + 1] << 3) & 0x18);
		dst[(i * 8) + 2] = ((src[(i * 5) + 1] >> 2) & 0x1F);
		dst[(i * 8) + 3] = ((src[(i * 5) + 1] >> 7) & 0x01) | ((src[(i * 5) + 2] << 1) & 0x1e);
		dst[(i * 8) + 4] = ((src[(i * 5) + 2] >> 4) & 0x0F) | ((src[(i * 5) + 3] << 4) & 0x10);
		dst[(i * 8) + 5] = ((src[(i * 5) + 3] >> 1) & 0x1F);
		dst[(i * 8) + 6] = ((src[(i * 5) + 3] >> 6) & 0x03) | ((src[(i * 5) + 4] << 2) & 0x1C);
		dst[(i * 8) + 7] = ((src[(i * 5) + 4] >> 3) & 0x1F);
	}
	sceIoWrite(cacheFile, uniCacheHead, (((len + 0x180000) / 5) * 8) + 0x1000);
}

static void pgmCreateCacheSpriteColour(INT8 num, int len)
{
	UINT8 *src = uniCacheHead + (PGM_CREATE_CACHE_BUFFER_SIZE - len);
	UINT8 *dst = uniCacheHead;
	UINT16 colpack;
	
	BurnLoadRom(src, num, 1);
	
	// convert from 3bpp packed
	for (int cnt = 0; cnt < len / 2; cnt++) {
		colpack = ((src[cnt * 2]) | (src[cnt * 2 + 1] << 8));
		dst[cnt * 3 + 0] = (colpack >> 0 ) & 0x1f;
		dst[cnt * 3 + 1] = (colpack >> 5 ) & 0x1f;
		dst[cnt * 3 + 2] = (colpack >> 10) & 0x1f;
	}
	
	sceIoWrite(cacheFile, uniCacheHead, (len / 2 * 3));
}

static INT8 pgmGetRoms(bool bLoad)
{
	const char *pRomName;
	struct BurnRomInfo ri;
	struct BurnRomInfo pi;
	
	UINT8 *PGM68KROMLoad = PGM68KROM;
	UINT8 *PGMUSER0Load  = PGMUSER0;
	UINT8 *PGMSNDROMLoad = ICSSNDROM + 0x400000;
	
	cacheFileSize = 0;
	nPGMTileROMOffset = 0xffffffff;
	nPGMTileROMExpOffset = 0xffffffff;
	nPGMSPRColROMOffset = 0xffffffff;
	nPGMSPRMaskROMOffset = 0xffffffff;
//	nPGMSNDROMOffset = 0xffffffff;
	
	for (INT8 i = 0; !BurnDrvGetRomName(&pRomName, i, 0); i++) {
		
		BurnDrvGetRomInfo(&ri, i);
		
		// 68K Code
		if ((ri.nType & BRF_PRG) && ((ri.nType & 0x0F) == 1)) {
			if (bLoad) {
				if (!needCreateCache) {
					BurnDrvGetRomInfo(&pi, i + 1);
					if ((ri.nLen == 0x80000) && (pi.nLen == 0x80000)) {
						BurnLoadRom(PGM68KROMLoad + 0, i + 0, 2);
						BurnLoadRom(PGM68KROMLoad + 1, i + 1, 2);
						PGM68KROMLoad += pi.nLen;
						i += 1;
					} else {
						BurnLoadRom(PGM68KROMLoad, i, 1);
					}
					PGM68KROMLoad += ri.nLen;
				}
			} else {
				nPGM68KROMLen += ri.nLen;
			}
			continue;
		}
		
		// 8x8 Text Tiles + 32x32 BG Tiles
		if ((ri.nType & BRF_GRA) && ((ri.nType & 0x0F) == 2)) {
			if (bLoad) {
				// 8x8 Text Tiles
				if (nPGMTileROMOffset == 0xffffffff) {
					nPGMTileROMOffset = cacheFileSize;
					cacheFileSize = cacheFileSize + 0x400000;
				}

				// 32x32 BG Tiles
				if (nPGMTileROMExpOffset == 0xffffffff) {
					nPGMTileROMExpOffset = cacheFileSize;
					if (needCreateCache) {
						pgmCreateCacheTiles(i, ri.nLen);
					}
					cacheFileSize = cacheFileSize + ((((ri.nLen + 0x180000) / 5) * 8) + 0x1000);
				}
			} else {
				nPGMTileROMLen += ri.nLen;
			}
			continue;
		}
		
		// Sprite Colour Data
		if ((ri.nType & BRF_GRA) && ((ri.nType & 0x0F) == 3)) {
			if (bLoad) {
				if (nPGMSPRColROMOffset == 0xffffffff) {
					nPGMSPRColROMOffset = cacheFileSize;
				}
				if (needCreateCache) {
					pgmCreateCacheSpriteColour(i, ri.nLen);
				}
				cacheFileSize = cacheFileSize + (ri.nLen / 2 * 3);
			} else {
				nPGMSPRColROMLen += ri.nLen;
			}
			continue;
		}
		
		// Sprite Masks + Colour Indexes
		if ((ri.nType & BRF_GRA) && ((ri.nType & 0x0F) == 4)) {
			if (bLoad) {
				if (nPGMSPRMaskROMOffset == 0xffffffff) {
					nPGMSPRMaskROMOffset = cacheFileSize;
				}
				if (needCreateCache) {
					loadAndWriteRomToCache(i,ri.nLen);
				}
				cacheFileSize = cacheFileSize + ri.nLen;
			} else {
				nPGMSPRMaskROMLen += ri.nLen;
			}
			continue;
		}
		
		// Sound Samples
		if ((ri.nType & BRF_SND) && ((ri.nType & 0x0F) == 5)) {
			if (bLoad) {
				if (!needCreateCache) {
					BurnLoadRom(PGMSNDROMLoad, i, 1);
					PGMSNDROMLoad += ri.nLen;
				}
/*
				if (nPGMSNDROMOffset == 0xffffffff) {
					// Bios Intro Sounds
					nPGMSNDROMOffset = cacheFileSize;
					if (needCreateCache) {
						loadAndWriteRomToCache(0x00081, 0x400000);
					}
					cacheFileSize = cacheFileSize + 0x400000;
				}
				if (needCreateCache) {
					loadAndWriteRomToCache(i,ri.nLen);
				}
				cacheFileSize = cacheFileSize + ri.nLen;
*/
			} else {
				nPGMSNDROMLen += ri.nLen;
			}
			continue;
		}
		
		if ((ri.nType & BRF_PRG) && (ri.nType & 0x0f) == 6)
		{
			if (bLoad) {
				if (!needCreateCache) {
					BurnLoadRom(PGMUSER1, i, 1);
					BurnByteswap(PGMUSER1, 0x10000);
				}
			}
			continue;
		}
		
		if ((ri.nType & BRF_PRG) && (ri.nType & 0x0f) == 8)
		{
			if (BurnDrvGetHardwareCode() & HARDWARE_IGS_USE_ARM_CPU) {
				if (bLoad) {
					if (!needCreateCache) {
						BurnLoadRom(PGMUSER0, i, 1);
						PGMUSER0Load += ri.nLen;
					}
				} else {
					nPGMExternalARMLen += ri.nLen;
				}
			}
			continue;
		}
		
		// NV RAM
		if ((ri.nType & BRF_OPT) && ((ri.nType & 0x0F) == 0)) {
			if (bLoad) {
				if (!needCreateCache) {
					BurnLoadRom(Pgm68KRam, i, 1);
				}
			}
			continue;
		}
	
	}
	
	if (!bLoad) {
		nPGMTileROMLen += 0x180000;
		if (nPGMTileROMLen < 0x400000) nPGMTileROMLen = 0x400000;
		
		nPGMTileROMExpLen = (nPGMTileROMLen / 5 * 8) + 0x1000;
		nPGMSPRColROMLen  = nPGMSPRColROMLen / 2 * 3;
		
		nPGMSNDROMLen += 0x400000;
		nPGMSNDROMLen = ((nPGMSNDROMLen - 1) | 0xfffff) + 1;
		
//		if (nPGMExternalARMLen < 0x200000) nPGMExternalARMLen = 0x200000;
	}
	
	return 0;
}

static INT8 pgmCacheInit(void)
{
	initUniCache(PGM_CACHE_SIZE);
	
	if (needCreateCache) {
		if ((uniCacheHead = (UINT8 *)malloc(PGM_CREATE_CACHE_BUFFER_SIZE)) == NULL) return 1;
		memset(uniCacheHead, 0, PGM_CREATE_CACHE_BUFFER_SIZE);
		
		pgmGetRoms(true);
		
		free(uniCacheHead);
		uniCacheHead=NULL;
		sceIoClose( cacheFile );
		cacheFile = sceIoOpen( filePathName,PSP_O_RDONLY, 0777);
		
		needCreateCache = false;
	}
	
	return 0;
}

/* Calendar Emulation */

static UINT8 CalVal, CalMask, CalCom = 0, CalCnt = 0;

static UINT8 bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}

static UINT8 pgm_calendar_r()
{
	UINT8 calr;
	calr = (CalVal & CalMask) ? 1 : 0;
	CalMask <<= 1;
	return calr;
}


#ifdef BUILD_PSP

#include <psprtc.h>
pspTime LocalTime;

static void pgm_calendar_w(UINT16 data)
{
	CalCom <<= 1;
	CalCom |= data & 1;
	++CalCnt;
	
	if (CalCnt == 4) {
		CalMask = 1;
		CalVal = 1;
		CalCnt = 0;
		
		switch (CalCom & 0xf) {
			case 0x1: case 0x3: case 0x5: case 0x7: case 0x9: case 0xb: case 0xd:
				CalVal++;
				break;
			case 0x0:  // ??
				CalVal = bcd(sceRtcGetDayOfWeek(LocalTime.year, LocalTime.month, LocalTime.day));
				break;
			case 0x2:  // Hours
				CalVal = bcd(LocalTime.hour);
				break;
			case 0x4:  // Seconds
				CalVal = bcd(LocalTime.seconds);
				break;
			case 0x6:  // Month
				CalVal = bcd(LocalTime.month); // not bcd in MVS
				break;
			case 0x8:  // Controls blinking speed, maybe milliseconds
				CalVal = bcd((LocalTime.microseconds / 1000) % 100);
				break;
			case 0xa:  // Day
				CalVal = bcd(LocalTime.day);
				break;
			case 0xc:  // Minute
				CalVal = bcd(LocalTime.minutes);
				break;
			case 0xe:  // Year
				CalVal = bcd(LocalTime.year % 100);
				break;
			case 0xf:  // Load Date
				sceRtcGetCurrentClockLocalTime(&LocalTime);
				break;
		}
	}
}

#else

static void pgm_calendar_w(UINT16 data)
{
	// initialize the time, otherwise it crashes
	time_t nLocalTime = time(NULL);
	tm* tmLocalTime = localtime(&nLocalTime);
	
	CalCom <<= 1;
	CalCom |= data & 1;
	++CalCnt;
	
	if (CalCnt == 4) {
		CalMask = 1;
		CalVal = 1;
		CalCnt = 0;
		
		switch (CalCom & 0xf) {
			case 0x1: case 0x3: case 0x5: case 0x7: case 0x9: case 0xb: case 0xd:
				CalVal++;
				break;
			case 0x0: //Day
				CalVal=bcd(tmLocalTime->tm_wday);
				break;
			case 0x2:  //Hours
				CalVal=bcd(tmLocalTime->tm_hour);
				break;
			case 0x4:  //Seconds
				CalVal=bcd(tmLocalTime->tm_sec);
				break;
			case 0x6:  //Month
				CalVal=bcd(tmLocalTime->tm_mon + 1); // not bcd in MVS
				break;
			case 0x8:  //Milliseconds?
				CalVal=0;
				break;
			case 0xa: //Day
				CalVal=bcd(tmLocalTime->tm_mday);
				break;
			case 0xc: //Minute
				CalVal=bcd(tmLocalTime->tm_min);
				break;
			case 0xe:  //Year
				CalVal=bcd(tmLocalTime->tm_year % 100);
				break;
			case 0xf:  //Load Date
				tmLocalTime = localtime(&nLocalTime);
				break;
		}
	}
}

#endif


/* memory handler */

UINT8 __fastcall PgmReadByte(UINT32 sekAddress)
{
	switch (sekAddress) {
		//case 0xC00005:
		//	return ics2115_soundlatch_r(1);
		
		case 0xC00007:
			return pgm_calendar_r();
		
		case 0xC08007: // dipswitches - (ddp2)
			return ~(PgmInput[6]) | 0xe0;
		
//		default:
//			bprintf(PRINT_NORMAL, ("Attempt to read byte value of location %x (PC: %5.5x)\n"), sekAddress, SekGetPC(-1));
	}
	
	return 0;
}

UINT16 __fastcall PgmReadWord(UINT32 sekAddress)
{
	switch (sekAddress) {
		case 0xC00004:
			return ics2115_soundlatch_r(1);
		
		case 0xC00006:  // ketsui wants this
			return pgm_calendar_r();
		
		case 0xC08000:  // p1+p2 controls
			return ~(PgmInput[0] | (PgmInput[1] << 8));
		
		case 0xC08002:  // p3+p4 controls
			return ~(PgmInput[2] | (PgmInput[3] << 8));
		
		case 0xC08004:  // extra controls
			return ~(PgmInput[4] | (PgmInput[5] << 8));
		
		case 0xC08006: // dipswitches
			return ~(PgmInput[6]) | 0xffe0;
		
//		default:
//			bprintf(PRINT_NORMAL, ("Attempt to read word value of location %x (PC: %5.5x)\n"), sekAddress, SekGetPC(-1));
	}
	
	return 0;
}

void __fastcall PgmWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
//	switch (sekAddress) {
//		default:
//			bprintf(PRINT_NORMAL, _T("Attempt to write byte value %x to location %x\n"), byteValue, sekAddress);
//	}
}

void __fastcall PgmWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	switch (sekAddress) {
		case 0x700006:  // Watchdog?
			break;
		
		case 0xC00002:
			ics2115_soundlatch_w(0, wordValue);
			if (nPgmZ80Work) ZetNmi();
			break;
		
		case 0xC00004:
			ics2115_soundlatch_w(1, wordValue);
			break;
		
		case 0xC00006:
			pgm_calendar_w(wordValue);
			break;
		
		case 0xC00008:
			if (wordValue == 0x5050) {
				ics2115_reset();
				nPgmZ80Work = 1;
				ZetReset();
			} else {
				nPgmZ80Work = 0;
			}
			break;
		
		case 0xC0000A:  // z80_ctrl_w
			break;
		
		case 0xC0000C:
			ics2115_soundlatch_w(2, wordValue);
			break;
		
		case 0xC08006: // unknown
			break;
		
//		default:
//			bprintf(PRINT_NORMAL, ("Attempt to write word value %x to location %x (PC: %5.5x)\n"), wordValue, sekAddress, SekGetPC(-1));
	}
}

void __fastcall PgmPaletteWriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	// 0xA00000 ~ 0xA011FF: 2304 color Palette (X1R5G5B5)
	sekAddress = (sekAddress & 0x1FFF) >> 1;
	PgmPalRam[sekAddress] = wordValue;
	
	PgmPalette[sekAddress] = PgmCalcCol(wordValue);
}

void __fastcall PgmPaletteWriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	sekAddress &= 0x1FFF;
	((UINT8 *)PgmPalRam)[sekAddress ^ 0x01] = byteValue;
	
	sekAddress >>= 1;
	PgmPalette[sekAddress] = PgmCalcCol(*(PgmPalRam + sekAddress));
}

/*
UINT8 __fastcall PgmZ80ReadByte(UINT32 sekAddress)
{
//	switch (sekAddress) {
//		default:
//			bprintf(PRINT_NORMAL, _T("Attempt to read byte value of location %x\n"), sekAddress);
//	}

	return 0;
}
*/

UINT16 __fastcall PgmZ80ReadWord(UINT32 sekAddress)
{
	sekAddress &= 0xFFFF;
	return (PgmZ80Ram[sekAddress + 1] | (PgmZ80Ram[sekAddress] << 8));
}

/*
void __fastcall PgmZ80WriteByte(UINT32 sekAddress, UINT8 byteValue)
{
	switch (sekAddress) {
//		default:
//			bprintf(PRINT_NORMAL, _T("Attempt to write byte value %x to location %x\n"), byteValue, sekAddress);
	}
}
*/

void __fastcall PgmZ80WriteWord(UINT32 sekAddress, UINT16 wordValue)
{
	sekAddress &= 0xFFFF;
	PgmZ80Ram[sekAddress + 0] = wordValue >> 8;
	PgmZ80Ram[sekAddress + 1] = wordValue & 0xFF;
}

UINT8 __fastcall PgmZ80PortRead(UINT16 p)
{
	switch (p >> 8) {
		case 0x80:
			return ics2115read(p & 0xff);
		case 0x81:
			return ics2115_soundlatch_r(2) & 0xff;
		case 0x82:
			return ics2115_soundlatch_r(0) & 0xff;
		case 0x84:
			return ics2115_soundlatch_r(1) & 0xff;
//		default:
//			bprintf(PRINT_NORMAL, _T("Z80 Attempt to read port %04x\n"), p);
	}
	return 0;
}

void __fastcall PgmZ80PortWrite(UINT16 p, UINT8 v)
{
	switch (p >> 8) {
		case 0x80:
			ics2115write(p & 0xff, v);
			break;
		case 0x81:
			ics2115_soundlatch_w(2, v);
			break;
		case 0x82:
			ics2115_soundlatch_w(0, v);
			break;	
		case 0x84:
			ics2115_soundlatch_w(1, v);
			break;
//		default:
//			bprintf(PRINT_NORMAL, _T("Z80 Attempt to write %02x to port %04x\n"), v, p);
	}
}

INT8 PgmDoReset()
{
	// Load the 68k bios if it is changed by the dips
	if (nPgmCurrentBios != PgmInput[8]) {
		nPgmCurrentBios = PgmInput[8];
		
//		bprintf (0, ("Using %s PGM Bios"), PgmInput[8] ? ("V2") : ("V1"));
		BurnLoadRom(PGM68KBIOS, 0x00082 + nPgmCurrentBios, 1);  // 68k bios
	}
	
	SekOpen(0);
	SekSetIRQLine(0, SEK_IRQSTATUS_NONE);
	SekReset();
	SekClose();
	
	ZetOpen(0);
	nPgmZ80Work = 0;
	ZetReset();
	ZetClose();
	
	ics2115_reset();
	
	if (pPgmResetCallback) {
		pPgmResetCallback();
	}
	
	return 0;
}

int pgmInit()
{
	Mem = NULL;
	
	pgmGetRoms(false);
	
	if (pgmCacheInit()) return 1;
	
	pgmMemIndex();
	UINT32 nLen = MemEnd - (UINT8 *)0;
	if ((Mem = (UINT8 *)memalign(4, nLen)) == NULL) return 1;
	memset(Mem, 0, nLen);
	pgmMemIndex();
	
	ICSSNDROM = (UINT8 *)memalign(4, nPGMSNDROMLen);
	memset(ICSSNDROM, 0, nPGMSNDROMLen);
	
	pgmGetRoms(true);
	
	// load bios roms
	BurnLoadRom(ICSSNDROM, 0x00081, 1);		// Bios Intro Sounds
	
	if (nCavePgm) {
		nCyclesPerInterLeave[0] = M68K_CYCS_PER_INTER_CAVE;
		nCyclesPerInterLeave[1] =  Z80_CYCS_PER_INTER_CAVE;
	} else {
		nCyclesPerInterLeave[0] = M68K_CYCS_PER_INTER;
		nCyclesPerInterLeave[1] =  Z80_CYCS_PER_INTER;
	}
	
	if (pPgmInitCallback) {
		pPgmInitCallback();
	}
	
	initCacheStructure(0.95);
	
	{
		SekInit(0, 0x68000);										// Allocate 68000
	    SekOpen(0);
		
		// Map 68000 memory:
		
		// ketsui and espgaluda
		if (BurnDrvGetHardwareCode() & HARDWARE_IGS_JAMMAPCB) {
			SekMapMemory(PGM68KROM,  0x000000, (nPGM68KROMLen - 1), SM_ROM);             // 68000 ROM (no bios)
		} else {
			SekMapMemory(PGM68KBIOS, 0x000000, 0x01ffff, SM_ROM);                        // 68000 BIOS
			SekMapMemory(PGM68KROM,  0x100000, (nPGM68KROMLen - 1) + 0x100000, SM_ROM);  // 68000 ROM
		}
		
		UINT32 i;
		for (i = 0; i < 0x100000; i += 0x20000) {
			// Main Ram + Mirrors
			SekMapMemory(Pgm68KRam, 0x800000 | i, 0x81ffff | i, SM_RAM);
		}
		
		for (i = 0; i < 0x100000; i += 0x08000) {
			// Video Ram + Mirrors
			SekMapMemory((UINT8 *)PgmBgRam, 0x900000 | i, 0x900fff | i, SM_RAM);
			SekMapMemory((UINT8 *)PgmBgRam, 0x901000 | i, 0x901fff | i, SM_RAM); // mirror
			SekMapMemory((UINT8 *)PgmBgRam, 0x902000 | i, 0x902fff | i, SM_RAM); // mirror
			SekMapMemory((UINT8 *)PgmBgRam, 0x903000 | i, 0x903fff | i, SM_RAM); // mirror
			
			SekMapMemory((UINT8 *)PgmTxRam, 0x904000 | i, 0x905fff | i, SM_RAM);
			SekMapMemory((UINT8 *)PgmTxRam, 0x906000 | i, 0x906fff | i, SM_RAM); // mirror
			
			SekMapMemory((UINT8 *)PgmRsRam, 0x907000 | i, 0x907fff | i, SM_RAM);
		}
		
		SekMapMemory((UINT8 *)PgmPalRam, 0xA00000, 0xA011FF, SM_ROM);
		SekMapHandler(1,                 0xA00000, 0xA011FF, SM_WRITE);
		
		SekMapMemory((UINT8 *)PgmVidReg, 0xB00000, 0xB0FFFF, SM_RAM);
		
		SekMapHandler(2,                 0xC10000, 0xC1FFFF, SM_READ | SM_WRITE);
		
		SekSetReadWordHandler (0, PgmReadWord);
		SekSetReadByteHandler (0, PgmReadByte);
		SekSetWriteWordHandler(0, PgmWriteWord);
		SekSetWriteByteHandler(0, PgmWriteByte);
		
		SekSetWriteWordHandler(1, PgmPaletteWriteWord);
		SekSetWriteByteHandler(1, PgmPaletteWriteByte);
		
		SekSetReadWordHandler (2, PgmZ80ReadWord);
//		SekSetReadByteHandler (2, PgmZ80ReadByte);
		SekSetWriteWordHandler(2, PgmZ80WriteWord);
//		SekSetWriteByteHandler(2, PgmZ80WriteByte);
		
		SekClose();
	}
	
	{
		ZetInit(1);
		ZetOpen(0);
		
		ZetMapArea(0x0000, 0xFFFF, 0, PgmZ80Ram);
		ZetMapArea(0x0000, 0xFFFF, 1, PgmZ80Ram);
		ZetMapArea(0x0000, 0xFFFF, 2, PgmZ80Ram);
		
		ZetSetInHandler(PgmZ80PortRead);
		ZetSetOutHandler(PgmZ80PortWrite);
		
		ZetMemEnd();
		
		ZetClose();
	}
	
	pgmInitDraw();
	
	ics2115_init(500.0, nCavePgm ? CAVE_PGM_REFRESHRATE : PGM_REFRESHRATE);
	
	PgmDoReset();
	
	return 0;
}

int pgmExit()
{
	pgmExitDraw();
	
	SekExit();
	ZetExit();
	
	free(Mem);
	Mem = NULL;
	
	ics2115_exit();
	
	PGM68KBIOS = NULL;
	PGM68KROM = NULL;
	// ics2115_exit can free and nil it
	ICSSNDROM = NULL;
	
	nPGM68KROMLen = 0;
	nPGMTileROMLen = 0;
	nPGMTileROMExpLen = 0;
	nPGMSPRColROMLen = 0;
	nPGMSPRMaskROMLen = 0;
	nPGMSNDROMLen = 0;
	nPGMExternalARMLen = 0;
	
	pPgmInitCallback = NULL;
	pPgmScanCallback = NULL;
	pPgmResetCallback = NULL;
	
	nCavePgm = 0;
	nPGMEnableIRQ4 = 0;
	
	nPgmCurrentBios = -1;
	
	destroyUniCache();
	
	return 0;
}


int pgmFrame()
{
	INT32 nCyclesDone[2] = { 0, 0 };
	INT32 nCyclesNext[2] = { 0, 0 };
	
	if (PgmReset) 
		PgmDoReset();
	
	// Compile digital inputs
	memset (PgmInput, 0, 6);
	for (INT8 i = 0; i < 8; i++) {
		PgmInput[0] |= (PgmJoy1[i] & 1) << i;
		PgmInput[1] |= (PgmJoy2[i] & 1) << i;
		PgmInput[2] |= (PgmJoy3[i] & 1) << i;
		PgmInput[3] |= (PgmJoy4[i] & 1) << i;
		PgmInput[4] |= (PgmBtn1[i] & 1) << i;
		PgmInput[5] |= (PgmBtn2[i] & 1) << i;
	}
	// clear opposites
	if ((PgmInput[0] & 0x06) == 0x06) PgmInput[0] &= 0xf9; // up/down
	if ((PgmInput[0] & 0x18) == 0x18) PgmInput[0] &= 0xe7; // left/right
	if ((PgmInput[1] & 0x06) == 0x06) PgmInput[1] &= 0xf9;
	if ((PgmInput[1] & 0x18) == 0x18) PgmInput[1] &= 0xe7;
	if ((PgmInput[2] & 0x06) == 0x06) PgmInput[2] &= 0xf9;
	if ((PgmInput[2] & 0x18) == 0x18) PgmInput[2] &= 0xe7;
	if ((PgmInput[3] & 0x06) == 0x06) PgmInput[3] &= 0xf9;
	if ((PgmInput[3] & 0x18) == 0x18) PgmInput[3] &= 0xe7;
	
	SekNewFrame();
	ZetNewFrame();
	
	SekOpen(0);
	ZetOpen(0);
	
	ics2115_frame();
	
	for(INT8 i = 1; i <= PGM_INTER_LEAVE; i++) {
		
		nCyclesNext[0] += nCyclesPerInterLeave[0];
		nCyclesNext[1] += nCyclesPerInterLeave[1];
		
		nCyclesDone[0] += SekRun(nCyclesNext[0] - nCyclesDone[0]);
		
		if (nPgmZ80Work) {
			nCyclesDone[1] += ZetRun(nCyclesNext[1] - nCyclesDone[1]);
		} else {
			nCyclesDone[1] += nCyclesNext[1] - nCyclesDone[1];
		}
		
		if (i == (PGM_INTER_LEAVE - 1)) {
			if (nPGMEnableIRQ4) {
				SekSetIRQLine(4, SEK_IRQSTATUS_AUTO);
			}
			if (pBurnDraw) {
				pgmDraw();
			}
			ics2115_update();
		}
	}
	
	SekSetIRQLine(6, SEK_IRQSTATUS_AUTO);
	
	ZetClose();
	SekClose();
	
	memcpy(PgmSprBuf, PgmSprRam, 0xa00); // buffer sprites
	
	return 0;
}

int pgmScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin =  0x029671;
	}

	nPgmPalRecalc = 1;

	if (nAction & ACB_MEMORY_ROM) {						// Scan memory rom
		if (BurnDrvGetHardwareCode() & HARDWARE_IGS_JAMMAPCB) {
			ba.Data         = PGM68KROM;
			ba.nLen         = nPGM68KROMLen;
			ba.nAddress     = 0;
			ba.szName       = "68K ROM";
			BurnAcb(&ba);
		} else {
			ba.Data		= PGM68KBIOS;
			ba.nLen		= 0x0080000;
			ba.nAddress = 0;
			ba.szName	= "BIOS ROM";
			BurnAcb(&ba);

			ba.Data		= PGM68KROM;
			ba.nLen		= nPGM68KROMLen;
			ba.nAddress = 0x100000;
			ba.szName	= "68K ROM";
			BurnAcb(&ba);
		}
	}

	if (nAction & ACB_MEMORY_RAM) {						// Scan memory, devices & variables
		ba.Data		= PgmBgRam;
		ba.nLen		= 0x0004000;
		ba.nAddress = 0x900000;
		ba.szName	= "Bg RAM";
		BurnAcb(&ba);

		ba.Data		= PgmTxRam;
		ba.nLen		= 0x0003000;
		ba.nAddress = 0x904000;
		ba.szName	= "Tx RAM";
		BurnAcb(&ba);

		ba.Data		= PgmRsRam;
		ba.nLen		= 0x0001000;
		ba.nAddress = 0x907000;
		ba.szName	= "Row Scroll";
		BurnAcb(&ba);

		ba.Data		= PgmPalRam;
		ba.nLen		= 0x0001200;
		ba.nAddress = 0xA00000;
		ba.szName	= "Palette";
		BurnAcb(&ba);

		ba.Data		= PgmVidReg;
		ba.nLen		= 0x0010000;
		ba.nAddress = 0xB00000;
		ba.szName	= "Video Regs";
		BurnAcb(&ba);
		
		ba.Data		= PgmZ80Ram;
		ba.nLen		= 0x0010000;
		ba.nAddress = 0xC10000;
		ba.szName	= "Z80 RAM";
		BurnAcb(&ba);
		
	}

	if (nAction & ACB_NVRAM) {								// Scan nvram
		ba.Data		= Pgm68KRam;
		ba.nLen		= 0x0020000;
		ba.nAddress = 0x800000;
		ba.szName	= "68K RAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
	
		SekScan(nAction);										// Scan 68000 state
		ZetScan(nAction);										// Scan Z80 state

		// Scan critical driver variables
		SCAN_VAR(PgmInput);

		if (nAction & ACB_WRITE)
			nPgmPalRecalc = 1;

		SCAN_VAR(nPgmZ80Work);
		SCAN_VAR(nPgmCurrentBios);
		ics2115_scan(nAction, pnMin);
	}

	if (pPgmScanCallback) {
		pPgmScanCallback(nAction, pnMin);
	}

 	return 0;
}

