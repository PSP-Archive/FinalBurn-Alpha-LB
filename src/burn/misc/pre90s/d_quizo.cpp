// FB Alpha  Quiz Olympic driver module
// Based on MAME driver by Tomasz Slanina

#include "burnint.h"
#include "driver.h"
extern "C" {
#include "ay8910.h"
}
#include "bitswap.h"

static unsigned char *Mem, *Rom, *Prom, *RomBank, *VideoRam, *framebuffer;
static unsigned char ALIGN_DATA DrvJoy[8], DrvDips, DrvReset;
static unsigned int *Palette;
static unsigned char port60 = 0, port70 = 0, dirty = 0;

static short* pAY8910Buffer[3];
static short *pFMBuffer = NULL;

static struct BurnInputInfo DrvInputList[] = {
	{"Coin 1",		BIT_DIGITAL,	DrvJoy + 0,	"p1 coin"   },
	{"Coin 2",		BIT_DIGITAL,	DrvJoy + 1,	"p1 coin2"  },
	{"Start 1",		BIT_DIGITAL,	DrvJoy + 2,     "p1 start"  },

	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy + 3,	"p1 fire 1" },
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy + 4,	"p1 fire 2" },
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy + 5,	"p1 fire 3" },

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"     },
	{"Tilt",		BIT_DIGITAL,	DrvJoy + 6,	"tilt"      },
	{"Dip Switches",	BIT_DIPSWITCH,	&DrvDips,	"dip"      },
};

STDINPUTINFO(Drv);

static struct BurnDIPInfo DrvDIPList[] =
{
	// Defaults
	{0x08, 0xFF, 0xFF, 0x40, NULL},

	{0,    0xFE, 0,	   2,	 "Test mode"},
	{0x08, 0x01, 0x08, 0x00, "Off"},
	{0x08, 0x01, 0x08, 0x08, "On"},
	{0,    0xFE, 0,	   2,	 "Show the answer"}, // look the star
	{0x08, 0x01, 0x10, 0x00, "Off"},
	{0x08, 0x01, 0x10, 0x10, "On"},
	{0,    0xFE, 0,	   2,	 "Coin A"},
	{0x08, 0x01, 0x40, 0x00, "2 coins 1 credit"},
	{0x08, 0x01, 0x40, 0x40, "1 coin 1 credit"},
};

STDDIPINFO(Drv);


static void quizo_palette_init()
{
	int i;
	unsigned char *color_prom = Prom;

	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		bit0 = 0;
		bit1 = (*color_prom >> 0) & 0x01;
		bit2 = (*color_prom >> 1) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (*color_prom >> 2) & 0x01;
		bit1 = (*color_prom >> 3) & 0x01;
		bit2 = (*color_prom >> 4) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		bit0 = (*color_prom >> 5) & 0x01;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		Palette[i] = (r << 16) | (g << 8) | b;
		color_prom++;
	}
}


static int DrvDraw()
{
	int x,y;
	unsigned int *src = (unsigned int *)framebuffer;

	if(dirty)
	{
		for(y=0;y<200;y++)
		{
			for(x=0;x<80;x++)
			{
				int data=VideoRam[y*80+x];
				int data1=VideoRam[y*80+x+0x4000];
				int pix;

				pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
				src[((x*4+3) + (y * 320))] = Palette[pix&15];
				data>>=1;
				data1>>=1;
				pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
				src[((x*4+2) + (y * 320))] = Palette[pix&15];
				data>>=1;
				data1>>=1;
				pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
				src[((x*4+1) + (y * 320))] = Palette[pix&15];
				data>>=1;
				data1>>=1;
				pix=(data&1)|(((data>>4)&1)<<1)|((data1&1)<<2)|(((data1>>4)&1)<<3);
				src[((x*4+0) + (y * 320))] = Palette[pix&15];
			}
		}
	}
	dirty = 0;

#ifdef BUILD_PSP
	int src_x;
	for (y = 0; y < 200; y++) {
		for (x = 0; x < 320; x++) {
			src_x = x + (y * 320);
			PutPix(pBurnDraw + (x + (y << 9)) * nBurnBpp, BurnHighCol(src[src_x]>>16, src[src_x]>>8, src[src_x], 0));
		}
	}
#else
	for (x = 0; x < 320 * 200; x++) {
		PutPix(pBurnDraw + x * nBurnBpp, BurnHighCol(src[x]>>16, src[x]>>8, src[x], 0));
	}
#endif

	return 0;
}


void port60_w(unsigned short, unsigned char data)
{
	static const unsigned char rombankLookup[]={ 2, 3, 4, 4, 4, 4, 4, 5, 0, 1};

	if (data > 9)
	{
		data=0;
	}

	port60 = data;

	ZetMapArea(0x8000, 0xbfff, 0, RomBank + rombankLookup[data] * 0x4000);
	ZetMapArea(0x8000, 0xbfff, 2, RomBank + rombankLookup[data] * 0x4000);
}

void __fastcall quizo_write(unsigned short a, unsigned char data)
{
	if (a >= 0xc000) {
		int bank = (port70 & 8) ? 1 : 0;
		VideoRam[(a & 0x3fff) + bank * 0x4000] = data;
		dirty=1;
		return;
	}
}

void __fastcall quizo_out_port(unsigned short a, unsigned char d)
{
	switch (a & 0xff)
	{
		case 0x50:
			AY8910Write(0, 0, d);
		break;

		case 0x51:
			AY8910Write(0, 1, d);
		break;

		case 0x60:
			port60_w(0, d);
		break;

		case 0x70:
			port70 = d;
		break;
	}
}

unsigned char __fastcall quizo_in_port(unsigned short a)
{
	switch (a & 0xff)
	{
		case 0x00:	// input port 0
			return (DrvJoy[0] | (DrvJoy[1] << 2) | (DrvJoy[6] << 3) | (DrvJoy[2] << 4)) ^ 0x18;

		case 0x10:	// input port 1
			return (DrvJoy[3] | (DrvJoy[4] << 1) | (DrvJoy[5] << 2)) ^ 0xff;

		case 0x40:	// input port 2
			return DrvDips;	
	}

	return 0;
}

static int DrvDoReset()
{
	dirty = 1;
	port70 = port60 = 0;

	DrvReset = 0;

	ZetOpen(0);
	ZetReset();
	ZetClose();

	AY8910Reset(0);

	memset (Rom + 0x4000, 0, 0x0400);
	memset (VideoRam, 0, 0x8000);
	memset (framebuffer, 0, 320 * 200 * 4);

	return 0;
}


static int DrvInit()
{
	Mem = (unsigned char*)malloc(0x30000 + 0x20 + 0x40 + 0x3e800);
	if (Mem == NULL) {
		return 1;
	}
	memset(Mem, 0, (0x30000 + 0x20 + 0x40 + 0x3e800));

	pFMBuffer = (short *)malloc (nBurnSoundLen * 3 * sizeof(short));
	if (pFMBuffer == NULL) {
		return 1;
	}
	memset(pFMBuffer, 0, nBurnSoundLen * 3 * sizeof(short));

	Rom      = Mem + 0x00000;
	RomBank  = Mem + 0x10000;
	VideoRam = Mem + 0x28000;
	Prom     = Mem + 0x30000;
	Palette  = (unsigned int*)(Mem + 0x30020);
	framebuffer = Mem + 0x30060;

	if (BurnLoadRom(Rom, 0, 1)) return 1;
	memcpy (Rom, Rom + 0x4000, 0x4000);

	if (BurnLoadRom(RomBank + 0x00000, 1, 1)) return 1;
	if (BurnLoadRom(RomBank + 0x08000, 2, 1)) return 1;
	if (BurnLoadRom(RomBank + 0x10000, 3, 1)) return 1;

	if (BurnLoadRom(Prom, 4, 1)) return 1;

	quizo_palette_init();

	ZetInit(1);
	ZetOpen(0);
	ZetSetWriteHandler(quizo_write);
	ZetSetInHandler(quizo_in_port);
	ZetSetOutHandler(quizo_out_port);
	ZetMapArea(0x0000, 0x3fff, 0, Rom + 0x0000);
	ZetMapArea(0x0000, 0x3fff, 2, Rom + 0x0000);
	ZetMapArea(0x4000, 0x47ff, 0, Rom + 0x4000);
	ZetMapArea(0x4000, 0x47ff, 1, Rom + 0x4000);
	ZetMemEnd();
	ZetClose();

	pAY8910Buffer[0] = pFMBuffer + nBurnSoundLen * 0;
	pAY8910Buffer[1] = pFMBuffer + nBurnSoundLen * 1;
	pAY8910Buffer[2] = pFMBuffer + nBurnSoundLen * 2;

	AY8910Init(0, 1342329, nBurnSoundRate, NULL, NULL, NULL, NULL);

	DrvDoReset();

	return 0;
}


static int DrvFrame()
{
	if (DrvReset) DrvDoReset();

	int nSoundBufferPos = 0;

	ZetOpen(0);
	ZetRun(4000000 / 60);
	ZetRaiseIrq(1);
	ZetClose();

	if (pBurnSoundOut) {
		int nSample;
		int nSegmentLength = nBurnSoundLen - nSoundBufferPos;
		short* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
		if (nSegmentLength) {
			AY8910Update(0, &pAY8910Buffer[0], nSegmentLength);
			for (int n = 0; n < nSegmentLength; n++) {
				nSample  = pAY8910Buffer[0][n];
				nSample += pAY8910Buffer[1][n];
				nSample += pAY8910Buffer[2][n];

				nSample /= 4;

				if (nSample < -32768) {
					nSample = -32768;
				} else {
					if (nSample > 32767) {
						nSample = 32767;
					}
				}

				pSoundBuf[(n << 1) + 0] = nSample;
				pSoundBuf[(n << 1) + 1] = nSample;
			}
		}
	}

	if (pBurnDraw) DrvDraw();

	return 0;
}


static int DrvExit()
{
	Mem = Rom = Prom = RomBank = VideoRam = framebuffer = NULL;
	Palette = NULL;
	pFMBuffer = NULL;
	AY8910Exit(0);
	ZetExit();
	free (Mem);
	free (pFMBuffer);

	return 0;
}


static int DrvScan(int nAction,int *pnMin)
{
	struct BurnArea ba;

	if (pnMin) {						// Return minimum compatible version
		*pnMin = 0x029521;
	}

	if (nAction & ACB_VOLATILE) {		// Scan volatile ram		
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = VideoRam;
		ba.nLen	  = 0x08000;
		ba.szName = "Video Ram";
		BurnAcb(&ba);

		ba.Data	  = Rom + 0x4000;
		ba.nLen	  = 0x00400;
		ba.szName = "Main Ram";
		BurnAcb(&ba);

		ba.Data   = framebuffer;
		ba.nLen   = 320 * 200 * 4;
		ba.szName = "Main Ram";
		BurnAcb(&ba);

		ZetScan(nAction);			// Scan Z80
		AY8910Scan(nAction, pnMin);		// Scan AY8910

		// Scan critical driver variables
		SCAN_VAR(port60);
		SCAN_VAR(port70);
		SCAN_VAR(dirty);

		port60_w(0, port60);
	}

	return 0;
}


// Quiz Olympic

static struct BurnRomInfo quizoRomDesc[] = {
	{ "rom1",   0x8000, 0x6731735f, BRF_ESS | BRF_PRG }, //  0 Z80 code

	{ "rom2",   0x8000, 0xa700eb30, BRF_ESS | BRF_PRG }, //  1 Z80 code banks
	{ "rom3",   0x8000, 0xd344f97e, BRF_ESS | BRF_PRG }, //  2
	{ "rom4",   0x8000, 0xab1eb174, BRF_ESS | BRF_PRG }, //  3

	{ "82s123", 0x0020, 0xc3f15914, BRF_GRA },	     //  4 Color Prom
};

STD_ROM_PICK(quizo);
STD_ROM_FN(quizo);

struct BurnDriver BurnDrvQuizo = {
	"quizo", NULL, NULL, "1985",
	"Quiz Olympic\0", NULL, "Seoul Coin Corp.", "Miscellaneous",
	L"\uD034\uC988\uC62C\uB9BC\uD53D\0Quiz Olympic\0", NULL, NULL, NULL,
	BDF_GAME_WORKING, 1, HARDWARE_MISC_PRE90S, GBF_QUIZ, 0,
	NULL, quizoRomInfo, quizoRomName, DrvInputInfo, DrvDIPInfo,
	DrvInit, DrvExit, DrvFrame, NULL, NULL, 0, NULL, NULL, NULL, NULL,
	320, 200, 4, 3
};

