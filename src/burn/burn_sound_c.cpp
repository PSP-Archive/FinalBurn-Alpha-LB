#include "burnint.h"
#include "burn_sound.h"

#ifndef BUILD_PSP
	#define CLIP(A) (((A) < -0x8000) ? -0x8000 : (((A) > 0x7fff) ? 0x7fff : (A)))
#else
	inline static int CLIP(int A)
	{
		A = __builtin_allegrex_min(A,  32767);
		A = __builtin_allegrex_max(A, -32768);
		
		return A;
	}
#endif

void BurnSoundCopyClamp_C(int *Src, short *Dest, short Len)
{
	Len <<= 1;
	while (Len--) {
		*Dest = CLIP((*Src >> 8));
		Src++;
		Dest++;
	}
}

void BurnSoundCopyClamp_Add_C(int *Src, short *Dest, short Len)
{
	Len <<= 1;
	while (Len--) {
		*Dest = CLIP((*Src >> 8) + *Dest);
		Src++;
		Dest++;
	}
}

void BurnSoundCopyClamp_Mono_C(int *Src, short *Dest, short Len)
{
	while (Len--) {
		Dest[0] = CLIP((*Src >> 8));
		Dest[1] = CLIP((*Src >> 8));
		Src++;
		Dest += 2;
	}
}

void BurnSoundCopyClamp_Mono_Add_C(int *Src, short *Dest, short Len)
{
	while (Len--) {
		Dest[0] = CLIP((*Src >> 8) + Dest[0]);
		Dest[1] = CLIP((*Src >> 8) + Dest[1]);
		Src++;
		Dest += 2;
	}
}

#undef CLIP
