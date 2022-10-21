#ifndef ICS2115_H
#define ICS2115_H

extern UINT8 *ICSSNDROM;
extern UINT32 nICSSNDROMLen;

extern UINT8 ics2115read(UINT8 offset);
extern void ics2115write(UINT8 offset, UINT8 data);

extern char ics2115_init(float output_gain, float nRefreshRate);
extern void ics2115_exit();
extern void ics2115_reset();

extern UINT16 ics2115_soundlatch_r(UINT8 i);
extern void ics2115_soundlatch_w(UINT8 i, UINT16 d);

extern void ics2115_frame();
extern void ics2115_update();
extern void ics2115_scan(int nAction, int *pnMin);

#endif
