// This file was generated by src/scripts/gamelist.pl (perl 5.010001)

// Declaration of all drivers
#define DRV extern struct BurnDriver
DRV		BurnDrvAirGallet;
DRV		BurnDrvAirGalletH;
DRV		BurnDrvAirGalletK;
DRV		BurnDrvAirGalletT;
DRV		BurnDrvAirGalletU;
DRV		BurnDrvAirGalletJ;
DRV		BurnDrvDdp2100;
DRV		BurnDrvDdp2101;
DRV		BurnDrvDdp2;
DRV		BurnDrvSailorMoonJ;
DRV		BurnDrvSailorMoonBJ;
DRV		BurnDrvDFeveron;
DRV		BurnDrvDoDonpachi;
DRV		BurnDrvDoDonpachiJ;
DRV		BurnDrvDdp3blk;
DRV		BurnDrvDdp3b;
DRV		BurnDrvDdp3a;
DRV		BurnDrvDdp3;
DRV		BurnDrvdonpachihk;
DRV		BurnDrvdonpachij;
DRV		BurnDrvDonpachikr;
DRV		BurnDrvDonpachi;
DRV		BurnDrvEsprade;
DRV		BurnDrvEspradejo;
DRV		BurnDrvEspradej;
DRV		BurnDrvEspgal;
DRV		BurnDrvFeverSOS;
DRV		BurnDrvGaia;
DRV		BurnDrvpwrinst2j;
DRV		BurnDrvplegendsj;
DRV		BurnDrvGuwange;
DRV		BurnDrvhotdogst;
DRV		BurnDrvKetb;
DRV		BurnDrvKeta;
DRV		BurnDrvKet;
DRV		BurnDrvmazingerj;
DRV		BurnDrvmazinger;
DRV		BurnDrvmetmqstr;
DRV		BurnDrvnmaster;
DRV		BurnDrvPgm;
DRV		BurnDrvpwrinst2;
DRV		BurnDrvplegends;
DRV		BurnDrvSailorMoon;
DRV		BurnDrvSailorMoonH;
DRV		BurnDrvSailorMoonK;
DRV		BurnDrvSailorMoonT;
DRV		BurnDrvSailorMoonU;
DRV		BurnDrvSailorMoonB;
DRV		BurnDrvSailorMoonBH;
DRV		BurnDrvSailorMoonBK;
DRV		BurnDrvSailorMoonBT;
DRV		BurnDrvSailorMoonBU;
DRV		BurnDrvUoPoko;
DRV		BurnDrvUoPokoj;
DRV		BurnDrvTheroes;
#undef DRV

// Structure containing addresses of all drivers
// Needs to be kept sorted (using the full game name as the key) to prevent problems with the gamelist in Kaillera
static struct BurnDriver* pDriver[] = {
	&BurnDrvAirGallet,			// Air Gallet (Europe)
	&BurnDrvAirGalletH,			// Air Gallet (Hong Kong)
	&BurnDrvAirGalletK,			// Air Gallet (Korea)
	&BurnDrvAirGalletT,			// Air Gallet (Taiwan)
	&BurnDrvAirGalletU,			// Air Gallet (USA)
	&BurnDrvAirGalletJ,			// Akuu Gallet (Japan)
	&BurnDrvDdp2100,			// Bee Storm - DoDonPachi II (ver. 100)
	&BurnDrvDdp2101,			// Bee Storm - DoDonPachi II (ver. 101)
	&BurnDrvDdp2,				// Bee Storm - DoDonPachi II (ver. 102)
	&BurnDrvSailorMoonJ,		// Bishoujo Senshi Sailor Moon (Ver. 95/03/22, Japan)
	&BurnDrvSailorMoonBJ,		// Bishoujo Senshi Sailor Moon (Ver. 95/03/22B, Japan)
	&BurnDrvDFeveron,			// Dangun Feveron (Japan, Ver. 98/09/17)
	&BurnDrvDoDonpachi,			// DoDonPachi (International, Master Ver. 97/02/05)
	&BurnDrvDoDonpachiJ,		// DoDonPachi (Japan, Master Ver. 97/02/05)
	&BurnDrvDdp3blk,			// DoDonPachi Dai-Ou-Jou (Black Label)
	&BurnDrvDdp3b,				// DoDonPachi Dai-Ou-Jou (V100, first revision)
	&BurnDrvDdp3a,				// DoDonPachi Dai-Ou-Jou (V100, second revision)
	&BurnDrvDdp3,				// DoDonPachi Dai-Ou-Jou (V101)
	&BurnDrvdonpachihk,			// DonPachi (Hong Kong)
	&BurnDrvdonpachij,			// DonPachi (Japan)
	&BurnDrvDonpachikr,			// DonPachi (Korea)
	&BurnDrvDonpachi,			// DonPachi (US)
	&BurnDrvEsprade,			// ESP Ra.De. (International, Ver. 98/04/22)
	&BurnDrvEspradejo,			// ESP Ra.De. (Japan, Ver. 98/04/14)
	&BurnDrvEspradej,			// ESP Ra.De. (Japan, Ver. 98/04/21)
	&BurnDrvEspgal,				// EspGaluda (V100, first revision)
	&BurnDrvFeverSOS,			// Fever SOS (International, Ver. 98/09/25)
	&BurnDrvGaia,				// Gaia Crusaders
	&BurnDrvpwrinst2j,			// Gouketsuji Ichizoku 2 (Japan, Ver. 94/04/08)
	&BurnDrvplegendsj,			// Gouketsuji Ichizoku Saikyou Densetsu (Japan, Ver. 95/06/20)
	&BurnDrvGuwange,			// Guwange (Japan, Master Ver. 99/06/24)
	&BurnDrvhotdogst,			// Hotdog Storm (International)
	&BurnDrvKetb,				// Ketsui: Kizuna Jigoku Tachi (V100, first revision)
	&BurnDrvKeta,				// Ketsui: Kizuna Jigoku Tachi (V100, second revision)
	&BurnDrvKet,				// Ketsui: Kizuna Jigoku Tachi (V100, third revision)
	&BurnDrvmazingerj,			// Mazinger Z (Japan)
	&BurnDrvmazinger,			// Mazinger Z (World)
	&BurnDrvmetmqstr,			// Metamoqester (International)
	&BurnDrvnmaster,			// Oni - The Ninja Master (Japan)
	&BurnDrvPgm,				// PGM (Polygame Master) System BIOS [BIOS only, NOT WORKING]
	&BurnDrvpwrinst2,			// Power Instinct 2 (US, Ver. 94/04/08)
	&BurnDrvplegends,			// Power Instinct Legends (US, Ver. 95/06/20)
	&BurnDrvSailorMoon,			// Pretty Soldier Sailor Moon (Ver. 95/03/22, Europe)
	&BurnDrvSailorMoonH,		// Pretty Soldier Sailor Moon (Ver. 95/03/22, Hong Kong)
	&BurnDrvSailorMoonK,		// Pretty Soldier Sailor Moon (Ver. 95/03/22, Korea)
	&BurnDrvSailorMoonT,		// Pretty Soldier Sailor Moon (Ver. 95/03/22, Taiwan)
	&BurnDrvSailorMoonU,		// Pretty Soldier Sailor Moon (Ver. 95/03/22, USA)
	&BurnDrvSailorMoonB,		// Pretty Soldier Sailor Moon (Ver. 95/03/22B, Europe)
	&BurnDrvSailorMoonBH,		// Pretty Soldier Sailor Moon (Ver. 95/03/22B, Hong Kong)
	&BurnDrvSailorMoonBK,		// Pretty Soldier Sailor Moon (Ver. 95/03/22B, Korea)
	&BurnDrvSailorMoonBT,		// Pretty Soldier Sailor Moon (Ver. 95/03/22B, Taiwan)
	&BurnDrvSailorMoonBU,		// Pretty Soldier Sailor Moon (Ver. 95/03/22B, USA)
	&BurnDrvUoPoko,				// Puzzle Uo Poko (International)
	&BurnDrvUoPokoj,			// Puzzle Uo Poko (Japan)
	&BurnDrvTheroes,			// Thunder Heroes
};
