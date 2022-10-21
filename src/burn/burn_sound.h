// burn_sound.h - General sound support functions
// based on code by Daniel Moreno (ComaC) < comac2k@teleline.es >

#ifndef BURN_SOUND_H
#define BURN_SOUND_H

void BurnSoundCopyClamp_C(int *Src, short *Dest, short Len);
void BurnSoundCopyClamp_Add_C(int *Src, short *Dest, short Len);
void BurnSoundCopyClamp_Mono_C(int *Src, short *Dest, short Len);
void BurnSoundCopyClamp_Mono_Add_C(int *Src, short *Dest, short Len);

#endif
