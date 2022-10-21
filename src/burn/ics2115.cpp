/******************************************************
 ICS 2115 sound synthesizer.

   ICS WaveFront ICS2115V Wavetable Midi Synthesizer,
 clocked at 33.8688MHz

 Original ics2115.c in MAME
   By O. Galibert, with a lot of help from the nebula
   ics emulation by Elsemi.

 Port to FB Alpha by OopsWare
 ******************************************************/

#include "burnint.h"
#include "ics2115.h"

UINT8 *ICSSNDROM;
UINT32 nICSSNDROMLen;

enum { V_ON = 1, V_DONE = 2 };

typedef struct {
	UINT8 *rom;
	INT16 ulaw[256];
	UINT16 volume[4096];

	struct {
		UINT16 fc, addrh, addrl, strth, strtl, endh, endl, volacc;
		UINT8 saddr, pan, conf, ctl;
		UINT8 vstart, vend, vctl, volincr;
		UINT8 state, tout;
	} voice[32];

	struct {
		UINT8 scale, preset;
		bool active;
		UINT32 period;
	} timer[2];

	UINT8 reg, osc, active_osc, vmode;

	UINT8 irq_en, irq_pend;
	UINT8 irq_on;

} sICS2115;

#define VOLUME_BITS  (15)
static sICS2115 *chip = NULL;
static INT16 *sndbuffer = NULL;

UINT16 ALIGN_DATA nSoundlatch[3] = { 0, 0, 0 };
UINT8  ALIGN_DATA bSoundlatchRead[3] = { 0, 0, 0 };

#define ICS2115_RATE  (33075)
UINT16 nSoundFrameSize;
UINT32 nSoundDelta;
UINT16 nOutputGain;


/* ICS2115V chip emu */

static void recalc_irq()
{
    UINT8 irq = 0;
	if (chip->irq_en & chip->irq_pend) irq = 1;

	for (UINT8 i = 0; (!irq) && (i < 32); i++) {
		if (chip->voice[i].state & V_DONE) irq = 1;
	}

	if (irq != chip->irq_on) {
		chip->irq_on = irq;

		if (irq) {
			ZetSetIRQLine(0xFF, ZET_IRQSTATUS_ACK);
		} else {
			ZetSetIRQLine(0x00, ZET_IRQSTATUS_NONE);
		}
	}
}

static void timer_cb_0()
{
	chip->irq_pend |= (1 << 0);
	recalc_irq();
}

static void timer_cb_1()
{
	chip->irq_pend |= (1 << 1);
	recalc_irq();
}

static void recalc_timer(UINT8 timer)
{
	UINT32 period = 0;

	if (chip->timer[timer].scale && chip->timer[timer].preset)
		period = 628206; // 1/62.8206;

	if(chip->timer[timer].period != period) {
//		bprintf(PRINT_NORMAL, _T("ICS2115: timer %d freq=%gHz  %4.1f%%\n"), timer,  1.0 * period / 10000, 6.0 * ZetTotalCycles() / 8468.0);
		chip->timer[timer].period = period;

		if(period) chip->timer[timer].active = true;
		else       chip->timer[timer].active = false;
	}
}

UINT16 ics2115read_reg(UINT8 reg)
{
	UINT16 ret;
	switch (reg) {
		case 0x00: // [osc] Oscillator Configuration
			ret = chip->voice[chip->osc].conf << 8;
			break;
		case 0x01: // [osc] Wavesample frequency
			ret = chip->voice[chip->osc].fc;
			break;
		case 0x02: // [osc] Wavesample loop start high
			// TODO: are these returns valid? might be 0x00ff for this one...
			ret = chip->voice[chip->osc].strth;
			break;
		case 0x03: // [osc] Wavesample loop start low
			ret = chip->voice[chip->osc].strtl;
			break;
		case 0x04: // [osc] Wavesample loop end high
			ret = chip->voice[chip->osc].endh;
			break;
		case 0x05: // [osc] Wavesample loop end low
			ret = (chip->voice[chip->osc].endl) & 0xff00;
			break;
		case 0x06: // [osc] Volume Increment
			ret = chip->voice[chip->osc].volincr;
			break;
		case 0x07: // [osc] Volume Start
			ret = chip->voice[chip->osc].vstart;
			break;
		case 0x08: // [osc] Volume End
			ret = chip->voice[chip->osc].vend;
			break;
		case 0x09: // [osc] Volume accumulator
			ret = chip->voice[chip->osc].volacc;
			break;
		case 0x0A: // [osc] Wavesample address
			ret = chip->voice[chip->osc].addrh;
			break;
		case 0x0B: // [osc] Wavesample address
			ret = chip->voice[chip->osc].addrl & 0xfff8;
			break;
		case 0x0C: // [osc] Pan
			ret = chip->voice[chip->osc].pan << 8;
			break;
		case 0x0D: // [osc] Volume Enveloppe Control
			ret = 0x100;
			break;
		case 0x0e: // Active Voices
			ret = chip->active_osc;
			break;
		case 0x0F: // [osc] Interrupt source/oscillator
			ret = 0xFF;
			for (UINT8 osc = 0; osc <= chip->active_osc; osc++) {
				if (chip->voice[osc].state & V_DONE) {
					chip->voice[osc].state &= ~V_DONE;
					recalc_irq();
					ret = 0x40 | osc; // 0x40 ? 0x80 ?
					break;
				}
			}
			ret <<= 8;
			break;
		case 0x10: // [osc] Oscillator Control
			ret = chip->voice[chip->osc].ctl << 8;
			break;
		case 0x11: // [osc] Wavesample static address 27-20
			ret = chip->voice[chip->osc].saddr << 8;
			break;
		case 0x40: // Timer 0 clear irq
			chip->irq_pend &= ~(1 << 0);
			recalc_irq();
			ret = chip->timer[0].preset;
			break;
		case 0x41: // Timer 1 clear irq
			chip->irq_pend &= ~(1 << 1);
			recalc_irq();
			ret = chip->timer[1].preset;
			break;
		case 0x43: // Timer status
			ret = chip->irq_pend & 3;
			break;
		case 0x4a: // IRQ Pending
			ret = chip->irq_pend;
			break;
		case 0x4b: // Address of Interrupting Oscillator
			ret = 0x80;
			break;
		case 0x4c: // Chip revision
			ret = 0x01;
			break;
		default:
			ret = 0;
			break;
	}

	return ret;
}

void ics2115write_reg(UINT8 reg, UINT8 data, UINT8 msb)
{
//	bprintf(PRINT_NORMAL, _T("ics2115write_reg(%02x, %02x, %d);  %4.1f%%\n"), reg, data, msb, 6.0 * ZetTotalCycles() / 8468.0 );

	chip->voice[chip->osc].tout = 40;

	switch (reg) {
		case 0x00: // [osc] Oscillator Configuration
			if (msb) {
				chip->voice[chip->osc].conf &= 0x80;
				chip->voice[chip->osc].conf |= (data & 0x7F);
			}
			break;
		case 0x01: // [osc] Wavesample frequency
			if (msb) chip->voice[chip->osc].fc = (chip->voice[chip->osc].fc & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].fc = (chip->voice[chip->osc].fc & 0xff00) | (data & 0xFe); //last bit not used
			// bprintf(PRINT_NORMAL, _T("ICS2115: %2d: fc = %04x (%dHz)\n"), chip->osc,chip->voice[chip->osc].fc, chip->voice[chip->osc].(fc*33075+512)/1024);
			break;
		case 0x02: // [osc] Wavesample loop start address 19-4
			if (msb) chip->voice[chip->osc].strth = (chip->voice[chip->osc].strth & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].strth = (chip->voice[chip->osc].strth & 0xff00) | data;
			break;
		case 0x03: // [osc] Wavesample loop start address 3-0.3-0
			if (msb) chip->voice[chip->osc].strtl = data << 8;
			break;
		case 0x04: // [osc] Wavesample loop end address 19-4
			if (msb) chip->voice[chip->osc].endh = (chip->voice[chip->osc].endh & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].endh = (chip->voice[chip->osc].endh & 0xff00) | data;
			break;
		case 0x05: // [osc] Wavesample loop end address 3-0.3-0
			// lsb is unused?
			if (msb) chip->voice[chip->osc].endl = (chip->voice[chip->osc].endl & 0x00ff) | (data << 8);
			break;
		case 0x06: // [osc] Volume Increment
			if (msb) chip->voice[chip->osc].volincr = data;
			break;
		case 0x07: // [osc] Volume Start
			if (!msb) chip->voice[chip->osc].vstart = data;
			break;
		case 0x08: // [osc] Volume End
			if (!msb) chip->voice[chip->osc].vend = data;
			break;
		case 0x09: // [osc] Volume accumulator
			if (msb) chip->voice[chip->osc].volacc = (chip->voice[chip->osc].volacc & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].volacc = (chip->voice[chip->osc].volacc & 0xff00) | data;
			// bprintf(PRINT_NORMAL, _T("ICS2115: %2d: volacc = %04x\n"), chip->osc,chip->voice[chip->osc].volacc);
			break;
		case 0x0A: // [osc] Wavesample address 19-4
			if (msb) chip->voice[chip->osc].addrh = (chip->voice[chip->osc].addrh & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].addrh = (chip->voice[chip->osc].addrh & 0xff00) | data;
			break;
		case 0x0B: // [osc] Wavesample address 3-0.8-0
			if (msb) chip->voice[chip->osc].addrl = (chip->voice[chip->osc].addrl & 0x00ff) | (data << 8);
			else     chip->voice[chip->osc].addrl = (chip->voice[chip->osc].addrl & 0xff00) | (data & 0xF8);
			break;
		case 0x0C: // [osc] Pan
			if (msb) chip->voice[chip->osc].pan = data;
			break;
		case 0x0D: // [osc] Volume Enveloppe Control
			if (msb) {
				chip->voice[chip->osc].vctl &= 0x80;
				chip->voice[chip->osc].vctl |= data & 0x7F;
			}
			break;
		case 0x0e: // Active Voices
			//Does this value get added to 1? Not sure. Could trace for writes of 32.
			if (msb) chip->active_osc = data & 0x1F; // & 0x1F ? (Guessing)
			break;
		case 0x10: // [osc] Oscillator Control
			//[7 R | 6 M2 | 5 M1 | 4-2 Reserve | 1 - Timer 2 Strt | 0 - Timer 1 Strt]
			if (msb) {
				chip->voice[chip->osc].ctl = data;
				if (data == 0) {
					chip->voice[chip->osc].state |= V_ON;
//					bprintf(PRINT_NORMAL, _T("ICS2115: KEYON %2d volacc = %04x fc = %04x (%dHz)\n"),
//							chip->osc, chip->voice[chip->osc].volacc, chip->voice[chip->osc].fc, (chip->voice[chip->osc].fc*33075 + 512) / 1024  );
				} else if (data == 0x0F) {
					if (!chip->vmode) {
						chip->voice[chip->osc].state &= ~V_ON;
					}
				}
			}
			break;
		case 0x11: // [osc] Wavesample static address 27-20
			if (msb) chip->voice[chip->osc].saddr = data;
			break;
		case 0x12: //Could be per voice! -- investigate.
			if (msb) chip->vmode = data;
			break;
		case 0x40: // Timer 1 Preset
			if (!msb) {
				chip->timer[0].preset = data;
				recalc_timer(0);
			}
			break;
		case 0x41: // Timer 2 Preset
			if (!msb) {
				chip->timer[1].preset = data;
				recalc_timer(1);
			}
			break;
		case 0x42: // Timer 1 Prescaler
			if (!msb) {
				chip->timer[0].scale = data;
				recalc_timer(0);
			}
			break;
		case 0x43: // Timer 2 Prescaler
			if (!msb) {
				chip->timer[1].scale = data;
				recalc_timer(1);
			}
			break;
		case 0x4a: // IRQ Enable
			if (!msb) {
				chip->irq_en = data;
				recalc_irq();
			}
			break;
		case 0x4f: // Oscillator Address being Programmed
			if (!msb) chip->osc = data % (chip->active_osc + 1);
			break;
	}
}

UINT8 ics2115read(UINT8 offset)
{
	switch ( offset ) {
		case 0x00: {
			UINT8 res = 0;
			if (chip->irq_on) {
				res |= 0x80;
				if (chip->irq_en & chip->irq_pend & 3) res |= 1; // Timer irq
				for (UINT8 i = 0; i <= chip->active_osc; i++) {
					if(chip->voice[i].state & V_DONE) {
						res |= 2;
						break;
					}
				}
			}
			return res;
		}

		case 0x01:
			return chip->reg;

		case 0x02:
			return ics2115read_reg(chip->reg) & 0xff;

		case 0x03:
		default:
			return ics2115read_reg(chip->reg) >> 8;
	}
}

void ics2115write(UINT8 offset, UINT8 data)
{
	switch (offset) {
		case 0x01:
			chip->reg = data;
			break;
		case 0x02:
			ics2115write_reg(chip->reg, data, 0);
			break;
		case 0x03:
			ics2115write_reg(chip->reg, data, 1);
			break;
		default:
			break;
	}
}

char ics2115_init(float output_gain, float nRefreshRate)
{
	chip = (sICS2115 *)memalign(4, sizeof(sICS2115));	// ICS2115V
	if (chip == NULL) return 1;

	nSoundFrameSize = (UINT16)(ICS2115_RATE / nRefreshRate);
	sndbuffer = (INT16 *)malloc(nSoundFrameSize * sizeof(INT16));
	if (sndbuffer == NULL) return 1;

	memset(chip, 0, sizeof(chip));
	memset(sndbuffer, 0, sizeof(sndbuffer));

	nOutputGain = (UINT16)(1024.0 * output_gain / 100.0);

	return 0;
}

void ics2115_exit()
{
	free( chip );
	chip = NULL;

	nICSSNDROMLen = 0;

	free(ICSSNDROM);
	ICSSNDROM = NULL;

	free( sndbuffer );
	sndbuffer = NULL;
}

void ics2115_reset()
{
	memset(chip, 0, sizeof(sICS2115));

	chip->rom = ICSSNDROM;
//	chip->timer[0].timer = timer_alloc_ptr(timer_cb_0, chip);
//	chip->timer[1].timer = timer_alloc_ptr(timer_cb_1, chip);
//	chip->stream = stream_create(0, 2, 33075, chip, update);
//	if(!chip->timer[0].timer || !chip->timer[1].timer) return NULL;

	chip->active_osc = 31;

	for (UINT8 osc = 0; osc < 32; osc++) {
		chip->voice[osc].conf = 2;
		chip->voice[osc].pan = 0x7F;
		chip->voice[osc].ctl = 1;
	}

	// u-Law table as per MIL-STD-188-113
	UINT16 lut[8];
	UINT16 lut_initial = 33 << 2;   //shift up 2-bits for 16-bit range.
	for(int i = 0; i < 8; i++)
		lut[i] = (lut_initial << i) - lut_initial;
	for(int i = 0; i < 256; i++) {
		UINT8 exponent = (~i >> 4) & 0x07;
		UINT8 mantissa = ~i & 0x0f;
		INT16 value = lut[exponent] + (mantissa << (exponent + 3));
		chip->ulaw[i] = (i & 0x80) ? -value : value;
	}

	// austere's table, derived from patent 5809466:
	// See section V starting from page 195
	// Subsection F (column 124, page 198) onwards
	for (UINT16 i = 0; i < 4096; i++) {
		chip->volume[i] = ((0x100 | (i & 0xFF)) << (VOLUME_BITS - 9)) >> (15 - (i >> 8));
	}

	if (nBurnSoundLen) {
		nSoundDelta = nSoundFrameSize * 0x10000 / nBurnSoundLen;
	} else {
		nSoundDelta = nSoundFrameSize * 0x10000 / 184; // 11025Hz
	}

	recalc_irq();

	memset(nSoundlatch, 0, sizeof(nSoundlatch));
	memset(bSoundlatchRead, 0, sizeof(bSoundlatchRead));
}

UINT16 ics2115_soundlatch_r(UINT8 i)
{
//	bprintf(PRINT_NORMAL, _T("soundlatch_r(%d)  %4.1f%% of frame\n"), i, 6.0 * SekTotalCycles() / 20000.0 );
	bSoundlatchRead[i] = 1;
	return nSoundlatch[i];
}

void ics2115_soundlatch_w(UINT8 i, UINT16 d)
{
//	if  ( !bSoundlatchRead[i] && nSoundlatch[i] != d )
//		bprintf(PRINT_ERROR, _T("soundlatch_w(%d, %04x)  %4.1f%% of frame\n"), i, d, 6.0 * SekTotalCycles() / 20000.0);
//	else
//		bprintf(PRINT_NORMAL, _T("soundlatch_w(%d, %04x)  %4.1f%% of frame\n"), i, d, 6.0 * SekTotalCycles() / 20000.0);
	nSoundlatch[i] = d;
	bSoundlatchRead[i] = 0;
}

void ics2115_frame()
{
	if (chip->timer[0].active ) timer_cb_0();
	if (chip->timer[1].active ) timer_cb_1();
}

void ics2115_update()
{
	UINT8 rec_irq = 0;

	memset(sndbuffer, 0, nSoundFrameSize * sizeof(INT16));

	for (UINT8 osc = 0; osc <= chip->active_osc; osc++) {
		if (chip->voice[osc].state & V_ON) {
			UINT32 badr = (chip->voice[osc].saddr << 20) & 0x00FFFFFF;
			UINT32 adr  = (chip->voice[osc].addrh << 16) | chip->voice[osc].addrl;
			UINT32 end  = (chip->voice[osc].endh  << 16) | chip->voice[osc].endl;
			UINT32 loop = (chip->voice[osc].strth << 16) | chip->voice[osc].strtl;
			
			UINT8 conf = chip->voice[osc].conf;
			UINT16 vol = chip->volume[chip->voice[osc].volacc >> 4];
			UINT32 delta = chip->voice[osc].fc << 2;
			
			if (chip->voice[osc].tout > 0) chip->voice[osc].tout--;
			
			for (UINT16 i = 0; i < nSoundFrameSize; i++) {
				INT32 v;
				UINT32 curaddr = badr | (adr >> 12);
				
				if (conf & 0x01) {
					v = chip->ulaw[chip->rom[curaddr]];
				} else {
					INT16 sample1, sample2;
					sample1 = ((INT8)chip->rom[curaddr + 0]) << 8;
					sample2 = ((INT8)chip->rom[curaddr + 1]) << 8;
					
					// linear interpolation as in US patent 6,246,774 B1, column 2 row 59
					// LEN=1, BLEN=0, DIR=0, start+end interpolation
					INT32 diff = sample2 - sample1;
					UINT16 fract = (chip->voice[osc].volacc >> 3) & 0x1FF;
					v = (((INT32)sample1 << 9) + diff * fract) >> 9;
				}
				
				// 15-bit volume + (5-bit worth of 32 channel sum)
				v = (v * vol) >> (VOLUME_BITS + 5);
				
				if (!chip->vmode) sndbuffer[i] += v;
				
				adr += delta;
				if (adr >= end) {
					//if (ICS2115LOGERROR) logerror("ICS2115: KEYDONE %2d\n", osc);
					adr -= (end - loop);
					if ((!(conf & 0x08)) || chip->voice[osc].tout == 0) {
						chip->voice[osc].state &= ~V_ON;
						chip->voice[osc].state |= V_DONE;
					}
					rec_irq = 1;
					break;
				}
			}
			
			chip->voice[osc].addrh = adr >> 16;
			chip->voice[osc].addrl = adr;
		}
	}

	if (rec_irq) recalc_irq();

	if (pBurnSoundOut) {
		INT32 pos = 0;
		INT16 *pOut = (INT16 *)pBurnSoundOut;
		
		for (UINT16 i = 0; i < nBurnSoundLen; i++, pOut += 2, pos += nSoundDelta) {
			pOut[0] = pOut[1] = ((INT32)sndbuffer[pos >> 16] * nOutputGain) >> 10;
		}
	}
}

void ics2115_scan(int nAction,int * /*pnMin*/)
{
	struct BurnArea ba;

	if ( nAction & ACB_DRIVER_DATA ) {
		UINT8 *rom = chip->rom;

		ba.Data		= chip;
		ba.nLen		= sizeof(sICS2115);
		ba.nAddress = 0;
		ba.szName	= "ICS 2115";
		BurnAcb(&ba);

		chip->rom = rom;

		SCAN_VAR(nSoundlatch);
		SCAN_VAR(bSoundlatchRead);
	}
}

