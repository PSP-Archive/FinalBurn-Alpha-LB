#include "burnint.h"
#include "msm6295.h"
#include "x1010.h"

unsigned char *X1010SNDROM;

struct x1_010_info * x1_010_chip = NULL;

void x1010_sound_bank_w(unsigned int offset, unsigned short data)
{
	//int banks = (memory_region_length( REGION_SOUND1 ) - 0x100000) / 0x20000;
	//if ( data >= banks ) {
	//	bprintf(PRINT_NORMAL, _T("invalid sound bank %04x\n"), data);
	//	data %= banks;
	//}
	memcpy(X1010SNDROM + offset * 0x20000, X1010SNDROM + 0x100000 + data * 0x20000, 0x20000);

	// backup sound bank index, need when game load status
	x1_010_chip->sound_banks[ offset ] = data;
}

unsigned char x1010_sound_read(unsigned int offset)
{
	offset ^= x1_010_chip->address;
	return x1_010_chip->reg[offset];
}

unsigned short x1010_sound_read_word(unsigned int offset)
{
	UINT16 ret;
	
	ret = x1_010_chip->HI_WORD_BUF[offset] << 8;
	ret += x1010_sound_read(offset);
	
	return ret;
}

void x1010_sound_update()
{
	short* pSoundBuf = pBurnSoundOut;
	memset(pSoundBuf, 0, nBurnSoundLen * sizeof(short) * 2);

	X1_010_CHANNEL	*reg;
	int		ch, i, volL, volR, freq;
	register signed char *start, *end, data;
	register unsigned char *env;
	register unsigned int smp_offs, smp_step, env_offs, env_step, delta;

	for( ch = 0; ch < SETA_NUM_CHANNELS; ch++ ) {
		reg = (X1_010_CHANNEL *) & (x1_010_chip->reg[ch * sizeof(X1_010_CHANNEL)]);
		if( reg->status & 1 ) {	// Key On
			short *bufL = pSoundBuf + 0;
			short *bufR = pSoundBuf + 1;
			if( (reg->status & 2) == 0 ) { // PCM sampling
				start    = (signed char *)( (reg->start << 12) + X1010SNDROM );
				end      = (signed char *)(((0x100 - reg->end) << 12) + X1010SNDROM );
				volL     = ((reg->volume >> 4) & 0xf) * VOL_BASE;
				volR     = ((reg->volume >> 0) & 0xf) * VOL_BASE;
				smp_offs = x1_010_chip->smp_offset[ch];
				freq     = reg->frequency & 0x1f;
				// Meta Fox does not write the frequency register. Ever
				if( freq == 0 ) freq = 4;

				// smp_step = (unsigned int)((float)x1_010_chip->base_clock / 8192.0
				//			* freq * (1 << FREQ_BASE_BITS) / (float)x1_010_chip->rate );
				smp_step = (unsigned int)((float)x1_010_chip->rate / (float)nBurnSoundRate / 8.0 * freq * (1 << FREQ_BASE_BITS) );

				//if( smp_offs == 0 ) {
				//	logerror( "Play sample %06X - %06X, channel %X volume %d freq %X step %X offset %X\n",
				//		start, end, ch, vol, freq, smp_step, smp_offs );
				//}

				for( i = 0; i < nBurnSoundLen; i++ ) {
					delta = smp_offs >> FREQ_BASE_BITS;
					// sample ended?
					if( start + delta >= end ) {
						reg->status &= 0xfe;					// Key off
						break;
					}
					data = *(start + delta);
					*bufL += ((data * volL) >> 8); bufL += 2;
					*bufR += ((data * volR) >> 8); bufR += 2;
					smp_offs += smp_step;
				}
				x1_010_chip->smp_offset[ch] = smp_offs;

			} else { // Wave form
				start    = (signed char *) & (x1_010_chip->reg[(reg->volume << 7) + 0x1000]);
				smp_offs = x1_010_chip->smp_offset[ch];
				freq     = (reg->pitch_hi << 8) + reg->frequency;
				// smp_step = (unsigned int)((float)x1_010_chip->base_clock / 128.0 / 1024.0 / 4.0 * freq * (1 << FREQ_BASE_BITS) / (float)x1_010_chip->rate);
				smp_step = (unsigned int)((float)x1_010_chip->rate / (float)nBurnSoundRate / 128.0 / 4.0 * freq * (1 << FREQ_BASE_BITS) );

				env      = (unsigned char *) & (x1_010_chip->reg[reg->end << 7]);
				env_offs = x1_010_chip->env_offset[ch];
				//env_step = (unsigned int)((float)x1_010->base_clock / 128.0 / 1024.0 / 4.0 * reg->start * (1 << ENV_BASE_BITS) / (float)x1_010->rate);
				env_step = (unsigned int)((float)x1_010_chip->rate / (float)nBurnSoundRate / 128.0 / 4.0 * reg->start * (1 << ENV_BASE_BITS) );

				//if( smp_offs == 0 ) {
				//	logerror( "Play waveform %X, channel %X volume %X freq %4X step %X offset %X\n",
				//		reg->volume, ch, reg->end, freq, smp_step, smp_offs );
				//}

				for( i = 0; i < nBurnSoundLen; i++ ) {
					int vol;
					delta = env_offs>>ENV_BASE_BITS;
	 				// Envelope one shot mode
					if( (reg->status&4) != 0 && delta >= 0x80 ) {
						reg->status &= 0xfe;					// Key off
						break;
					}

					vol = *(env + (delta & 0x7f));
					volL = ((vol >> 4) & 0xf) * VOL_BASE;
					volR = ((vol >> 0) & 0xf) * VOL_BASE;
					data  = *(start + ((smp_offs >> FREQ_BASE_BITS) & 0x7f));
					*bufL += ((data * volL) >> 8); bufL += 2;
					*bufR += ((data * volR) >> 8); bufR += 2;
					smp_offs += smp_step;
					env_offs += env_step;
				}
				x1_010_chip->smp_offset[ch] = smp_offs;
				x1_010_chip->env_offset[ch] = env_offs;
			}
		}
	}
}

void x1010_sound_init(unsigned int base_clock, int address)
{
	x1_010_chip = (struct x1_010_info *) malloc( sizeof(struct x1_010_info) );
	memset(x1_010_chip, 0, sizeof(struct x1_010_info));
	
	x1_010_chip->base_clock = base_clock;
	x1_010_chip->rate = x1_010_chip->base_clock >> 10;
	x1_010_chip->address = address;
}

void x1010_scan(int nAction,int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin =  0x029672;
	}
	
	if ( nAction & ACB_DRIVER_DATA ) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = x1_010_chip;
		ba.nLen	  = sizeof(struct x1_010_info);
		ba.szName = "X1-010";
		BurnAcb(&ba);
	}
}

void x1010_exit()
{
	free(x1_010_chip);
	x1_010_chip = NULL;
}
