#define FN(a,b,c,d) RenderTile ## a ## _ROT ## b ## c ## d
#define FUNCTIONNAME(a,b,c,d) FN(a,b,c,d)

#define ISOPAQUE 0

#if ROT == 0
 #define ADVANCECOLUMN pPixel += (BPP >> 3)
 #ifndef BUILD_PSP
  #define ADVANCEROW pTileRow += ((BPP >> 3) * 320)
 #else
//  #define ADVANCEROW pTileRow += ((BPP >> 3) * 512)
  #define ADVANCEROW pTileRow += ((BPP >> 3) << 9)
 #endif
#elif ROT == 270
 #define ADVANCECOLUMN pPixel -= ((BPP >> 3) * 240)
 #define ADVANCEROW pTileRow += (BPP >> 3)
#else
 #error unsupported rotation angle specified.
#endif

#if DOCLIP == 0
 #define CLIP _NOCLIP
 #define TESTCLIP(x) 1
#elif DOCLIP == 1
 #define CLIP _CLIP
 #define TESTCLIP(x) (nTileXPos + x) >= 0 && (nTileXPos + x) < 320
#else
 #error illegal doclip value.
#endif

#if ISOPAQUE == 0
 #define OPACITY _TRANS
 #define TESTCOLOUR(x) x
#elif ISOPAQUE == 1
 #define OPACITY _OPAQUE
 #define TESTCOLOUR(x) 1
#else
 #error illegal isopaque value
#endif

#if BPP == 16
 #define PLOTPIXEL(a,b) if (TESTCOLOUR(b) && TESTCLIP(a)) {			\
   	*((unsigned short *)pPixel) = (unsigned short)pTilePalette[b];	\
 }
#elif BPP == 24
 #define PLOTPIXEL(a,b) if (TESTCOLOUR(b) && TESTCLIP(a)) {			\
	unsigned int nRGB = pTilePalette[b];							\
	pPixel[0] = (unsigned char)nRGB;								\
	pPixel[1] = (unsigned char)(nRGB >> 8);							\
	pPixel[2] = (unsigned char)(nRGB >> 16);						\
 }
#elif BPP == 32
 #define PLOTPIXEL(a,b) if (TESTCOLOUR(b) && TESTCLIP(a)) {			\
	 *((unsigned int *)pPixel) = (unsigned int)pTilePalette[b];		\
 }
#else
 #error unsupported bitdepth specified.
#endif

#if ROWMODE == 0
 #define MODE _NORMAL
#elif ROWMODE == 1
 #define MODE _ROWSEL
#else
 #error unsupported rowmode specified.
#endif

static void FUNCTIONNAME(BPP,ROT,CLIP,MODE)()
{
#if ROWMODE == 0
	unsigned char *pTileRow, *pPixel;
	int y, nColour;

	for (y = 0, pTileRow = pTile; y < 8; y++, ADVANCEROW) {
		pPixel = pTileRow;
#else
	unsigned char *pPixel = pTile;
	int nColour;
#endif

		nColour = *pTileData++;
		PLOTPIXEL(0,nColour >> 4);
		ADVANCECOLUMN;
		PLOTPIXEL(1,nColour & 0x0F);
		ADVANCECOLUMN;

		nColour = *pTileData++;
		PLOTPIXEL(2,nColour >> 4);
		ADVANCECOLUMN;
		PLOTPIXEL(3,nColour & 0x0F);
		ADVANCECOLUMN;

		nColour = *pTileData++;
		PLOTPIXEL(4,nColour >> 4);
		ADVANCECOLUMN;
		PLOTPIXEL(5,nColour & 0x0F);
		ADVANCECOLUMN;

#if ROWMODE == 0
		nColour = *pTileData++;
#else
		nColour = *pTileData;
#endif
		PLOTPIXEL(6,nColour >> 4);
		ADVANCECOLUMN;
		PLOTPIXEL(7,nColour & 0x0F);
#if ROWMODE == 0
	}
#endif
}

#undef MODE
#undef PLOTPIXEL
#undef TESTCLIP
#undef TESTCOLOUR
#undef ADVANCEROW
#undef ADVANCECOLUMN
#undef CLIP
#undef FUNCTIONNAME
#undef FN
