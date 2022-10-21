#include "pgm.h"

int ddp3Scan(int nAction, int *);


//-------------------------------------------------------------------------------------------
// ddp2 - preliminary (kludgy)

static int ddp2_asic27_0xd10000 = 0;

static void __fastcall Ddp2WriteByte(UINT32 address, UINT8 data)
{
	if ((address & 0xffe000) == 0xd00000) {
		PGMUSER0[(address & 0x1fff) ^ 1] = data;
		*((UINT16 *)(PGMUSER0 + 0x0010)) = 0;
		*((UINT16 *)(PGMUSER0 + 0x0020)) = 1;
		return;
	}

	if ((address & 0xffffffe) == 0xd10000) {
		ddp2_asic27_0xd10000 = data;
		return;
	}
}

static void __fastcall Ddp2WriteWord(UINT32 address, UINT16 data)
{
	if ((address & 0xffe000) == 0xd00000) {
		*((UINT16 *)(PGMUSER0 + (address & 0x1ffe))) = data;
		*((UINT16 *)(PGMUSER0 + 0x0010)) = 0;
		*((UINT16 *)(PGMUSER0 + 0x0020)) = 1;
		return;
	}

	if ((address & 0xffffffe) == 0xd10000) {
		ddp2_asic27_0xd10000 = data;
		return;
	}
}

static UINT8 __fastcall Ddp2ReadByte(UINT32 address)
{
	if ((address & 0xfffffe) == 0xd10000) {
		ddp2_asic27_0xd10000++;
		ddp2_asic27_0xd10000 &= 0x7f;
		return ddp2_asic27_0xd10000;
	}

	if ((address & 0xffe000) == 0xd00000) {
		*((UINT16 *)(PGMUSER0 + 0x0002)) = PgmInput[7]; // region
		*((UINT16 *)(PGMUSER0 + 0x1f00)) = 0;
		return PGMUSER0[(address & 0x1fff) ^ 1];
	}

	return 0;
}

static UINT16 __fastcall Ddp2ReadWord(UINT32 address)
{
	if ((address & 0xfffffe) == 0xd10000) {
		ddp2_asic27_0xd10000++;
		ddp2_asic27_0xd10000 &= 0x7f;
		return ddp2_asic27_0xd10000;
	}

	if ((address & 0xffe000) == 0xd00000) {
		*((UINT16 *)(PGMUSER0 + 0x0002)) = PgmInput[7]; // region
		*((UINT16 *)(PGMUSER0 + 0x1f00)) = 0;
      	return *((UINT16 *)(PGMUSER0 + (address & 0x1ffe)));
	}

	return 0;
}

void install_protection_ddp2()
{
	memset (PGMUSER0, 0, 0x2000);

	SekOpen(0);
	SekMapHandler(4, 0xd00000, 0xd1ffff, SM_READ | SM_WRITE);
	SekSetReadWordHandler (4, Ddp2ReadWord);
	SekSetReadByteHandler (4, Ddp2ReadByte);
	SekSetWriteWordHandler(4, Ddp2WriteWord);
	SekSetWriteByteHandler(4, Ddp2WriteByte);
	SekClose();
}


//----------------------------------------------------------------------------------------------------------
// Ketsui, EspGaluda, DoDonPachi Dai-Ou-Jou

static UINT16 value0, value1, valuekey;
static UINT32 valueresponse;
static UINT8 ddp3internal_slot = 0;
static UINT32 ddp3slots[0x100];

static void __fastcall ddp3_asic_write(UINT32 offset, UINT16 data)
{
	switch (offset & 0x06) {
		case 0:
			value0 = data;
			return;

		case 2: {
			if ((data >> 8) == 0xff) valuekey = 0xffff;

			value1 = data ^ valuekey;
			value0 ^= valuekey;

			switch (value1 & 0xff) {
				case 0x40:
				valueresponse = 0x880000;
				ddp3slots[(value0 >> 10) & 0x1f] = (ddp3slots[(value0 >> 5) & 0x1f] + ddp3slots[(value0 >> 0) & 0x1f]) & 0xffffff;
				break;

				case 0x67:
				valueresponse = 0x880000;
				ddp3internal_slot = (value0 & 0xff00) >> 8;
				ddp3slots[ddp3internal_slot] = (value0 & 0x00ff) << 16;
				break;

				case 0xe5:
				valueresponse = 0x880000;
				ddp3slots[ddp3internal_slot] |= (value0 & 0xffff);
				break;

				case 0x8e:
				valueresponse = ddp3slots[value0 & 0xff];
				break;

				case 0x99: // reset?
				valuekey = 0;
				valueresponse = 0x880000;
				break;

				default:
				valueresponse = 0x880000;
				break;
			}

			valuekey = (valuekey + 0x0100) & 0xff00;
			if (valuekey == 0xff00) valuekey = 0x0100;
			valuekey |= valuekey >> 8;
		}
		return;

		case 4: return;
	}
}

static UINT16 __fastcall ddp3_asic_read(UINT32 offset)
{

	switch (offset & 0x02) {
		case 0: return (valueresponse >>  0) ^ valuekey;
		case 2: return (valueresponse >> 16) ^ valuekey;
	}

	return 0;
}

void reset_ddp3()
{
	value0 = 0;
	value1 = 0;
	valuekey = 0;
	valueresponse = 0;
	ddp3internal_slot = 0;

	memset (ddp3slots, 0, 0x100 * sizeof(int));
}

void install_protection_ket()
{
	pPgmScanCallback = ddp3Scan;

	SekOpen(0);
	SekMapHandler(4, 0x400000, 0x400005, SM_READ | SM_WRITE);
	SekSetReadWordHandler (4, ddp3_asic_read);
	SekSetWriteWordHandler(4, ddp3_asic_write);
	SekClose();
}

void install_protection_ddp3()
{
	pPgmScanCallback = ddp3Scan;

	SekOpen(0);
	SekMapHandler(4, 0x500000, 0x500005, SM_READ | SM_WRITE);
	SekSetReadWordHandler (4, ddp3_asic_read);
	SekSetWriteWordHandler(4, ddp3_asic_write);
	SekClose();
}

int ddp3Scan(int nAction, int *)
{
	struct BurnArea ba;

	if (nAction & ACB_MEMORY_RAM) {
		ba.Data         = (UINT8 *)ddp3slots;
		ba.nLen         = 0x0000100 * sizeof(int);
		ba.nAddress     = 0xff00000;
		ba.szName       = "ProtRAM";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(value0);
		SCAN_VAR(value1);
		SCAN_VAR(valuekey);
		SCAN_VAR(valueresponse);
		SCAN_VAR(ddp3internal_slot);
	}

	return 0;
}

