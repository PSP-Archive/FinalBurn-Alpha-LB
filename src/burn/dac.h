#ifndef _DAC_H_
#define _DAC_H_

extern void DACUpdate(short* Buffer, int Length);
extern void DACWrite(UINT8 Data);
extern void DACInit(int Clock);
extern void DACReset();
extern void DACExit();
extern int DACScan(int nAction,int *pnMin);

#endif
