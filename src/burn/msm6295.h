// MSM6295 module header

#ifndef MSM6295_H
#define MSM6295_H

#define MAX_MSM6295 (2)

INT8 MSM6295Init(UINT8 nChip, int nSamplerate, float fVolume, bool bAddSignal);
void MSM6295Reset(UINT8 nChip);
void MSM6295Exit(UINT8 nChip);

INT8 MSM6295Render(UINT8 nChip, INT16 *pSoundBuf, UINT16 nSegmenLength);
void MSM6295Command(UINT8 nChip, UINT8 nCommand);
INT8 MSM6295Scan(UINT8 nChip, int nAction);

extern UINT8 *MSM6295ROM;
extern UINT8 *MSM6295SampleInfo[MAX_MSM6295][4];
extern UINT8 *MSM6295SampleData[MAX_MSM6295][4];

inline static UINT8 MSM6295ReadStatus(const UINT8 nChip)
{
	extern UINT8 nMSM6295Status[MAX_MSM6295];

	return nMSM6295Status[nChip];
}

#endif
