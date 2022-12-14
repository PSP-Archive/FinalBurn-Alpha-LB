#ifndef BURN_8255PPI_H
#define BURN_8255PPI_H

typedef UINT8 (*PPIPortRead)();
typedef void (*PPIPortWrite)(UINT8 data);
extern PPIPortRead PPI0PortReadA;
extern PPIPortRead PPI0PortReadB;
extern PPIPortRead PPI0PortReadC;
extern PPIPortWrite PPI0PortWriteA;
extern PPIPortWrite PPI0PortWriteB;
extern PPIPortWrite PPI0PortWriteC;
extern PPIPortRead PPI1PortReadA;
extern PPIPortRead PPI1PortReadB;
extern PPIPortRead PPI1PortReadC;
extern PPIPortWrite PPI1PortWriteA;
extern PPIPortWrite PPI1PortWriteB;
extern PPIPortWrite PPI1PortWriteC;
extern PPIPortRead PPI2PortReadA;
extern PPIPortRead PPI2PortReadB;
extern PPIPortRead PPI2PortReadC;
extern PPIPortWrite PPI2PortWriteA;
extern PPIPortWrite PPI2PortWriteB;
extern PPIPortWrite PPI2PortWriteC;

void ppi8255_init(int num);
void ppi8255_exit();
void ppi8255_scan();
UINT8 ppi8255_r(int which, int offset);
void ppi8255_w(int which, int offset, UINT8 data);
void ppi8255_set_portC( int which, UINT8 data );

#endif
