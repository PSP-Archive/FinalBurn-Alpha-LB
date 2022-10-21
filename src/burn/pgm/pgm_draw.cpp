#include "pgm.h"
#include "UniCache.h"

static UINT32 nPgmTileMask;
static UINT8 *tiletrans;	// tile transparency table
static UINT8 *texttrans;	// text transparency table
static UINT8 *sprmsktab;

typedef struct {
	UINT16 wide; UINT16 high;
	INT16 xpos; INT16 ypos;
	UINT8 flipx; UINT8 flipy;
	UINT32 xzoom; UINT8 xgrow;
	UINT32 yzoom; UINT8 ygrow;
	UINT16 palt;
	UINT32 boffset;
} PgmSprite;


#ifndef BUILD_PSP
	#define PGM_WIDTH	448
#else
	#define PGM_WIDTH	512
#endif

inline static UINT8 *getBlockTile(UINT32 offset, UINT32 size)
{
	return getBlock(nPGMTileROMOffset + offset, size);
}

inline static UINT8 *getBlockTileExp(UINT32 offset, UINT32 size)
{
	return getBlock(nPGMTileROMExpOffset + offset, size);
}

inline static UINT8 *getBlockSPRMask(UINT32 offset, UINT32 size)
{
	return getBlock(nPGMSPRMaskROMOffset + offset, size);
}

inline static UINT8 *getBlockSPRCol(UINT32 offset, UINT32 size)
{
	return getBlock(nPGMSPRColROMOffset + offset, size);
}


#define SPRITE_PLOTPIXEL_FLIPX(x)									\
{																	\
	if(!(msk & (1 << x))) {											\
		dst[sx - x] = *(PgmPalette + (*adata++ | spr->palt));		\
	}																\
}																	\

#define PgmRenderSpriteFlipX()										\
{																	\
	SPRITE_PLOTPIXEL_FLIPX( 0);										\
	SPRITE_PLOTPIXEL_FLIPX( 1);										\
	SPRITE_PLOTPIXEL_FLIPX( 2);										\
	SPRITE_PLOTPIXEL_FLIPX( 3);										\
	SPRITE_PLOTPIXEL_FLIPX( 4);										\
	SPRITE_PLOTPIXEL_FLIPX( 5);										\
	SPRITE_PLOTPIXEL_FLIPX( 6);										\
	SPRITE_PLOTPIXEL_FLIPX( 7);										\
	SPRITE_PLOTPIXEL_FLIPX( 8);										\
	SPRITE_PLOTPIXEL_FLIPX( 9);										\
	SPRITE_PLOTPIXEL_FLIPX(10);										\
	SPRITE_PLOTPIXEL_FLIPX(11);										\
	SPRITE_PLOTPIXEL_FLIPX(12);										\
	SPRITE_PLOTPIXEL_FLIPX(13);										\
	SPRITE_PLOTPIXEL_FLIPX(14);										\
	SPRITE_PLOTPIXEL_FLIPX(15);										\
}																	\

#define SPRITE_PLOTPIXEL_FLIPX_CLIP(x)								\
{																	\
	if(!(msk & (1 << x))) {											\
		if (((sx - x) >= 0) && ((sx - x) < 448)) {					\
			dst[sx - x] = *(PgmPalette + (*adata | spr->palt));		\
		}															\
		adata++;													\
	}																\
}																	\

#define PgmRenderSpriteFlipX_Clip()									\
{																	\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 0);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 1);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 2);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 3);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 4);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 5);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 6);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 7);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 8);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP( 9);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(10);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(11);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(12);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(13);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(14);								\
	SPRITE_PLOTPIXEL_FLIPX_CLIP(15);								\
}																	\

#define SPRITE_PLOTPIXEL(x)											\
{																	\
	if(!(msk & (1 << x))) {											\
		dst[sx + x] = *(PgmPalette + (*adata++ | spr->palt));		\
	}																\
}																	\

#define PgmRenderSprite()											\
{																	\
	SPRITE_PLOTPIXEL( 0);											\
	SPRITE_PLOTPIXEL( 1);											\
	SPRITE_PLOTPIXEL( 2);											\
	SPRITE_PLOTPIXEL( 3);											\
	SPRITE_PLOTPIXEL( 4);											\
	SPRITE_PLOTPIXEL( 5);											\
	SPRITE_PLOTPIXEL( 6);											\
	SPRITE_PLOTPIXEL( 7);											\
	SPRITE_PLOTPIXEL( 8);											\
	SPRITE_PLOTPIXEL( 9);											\
	SPRITE_PLOTPIXEL(10);											\
	SPRITE_PLOTPIXEL(11);											\
	SPRITE_PLOTPIXEL(12);											\
	SPRITE_PLOTPIXEL(13);											\
	SPRITE_PLOTPIXEL(14);											\
	SPRITE_PLOTPIXEL(15);											\
}																	\

#define SPRITE_PLOTPIXEL_CLIP(x)									\
{																	\
	if(!(msk & (1 << x))) {											\
		if (((sx + x) >= 0) && ((sx + x) < 448)) {					\
			dst[sx + x] = *(PgmPalette + (*adata | spr->palt));		\
		}															\
		adata++;													\
	}																\
}																	\

#define PgmRenderSprite_Clip()										\
{																	\
	SPRITE_PLOTPIXEL_CLIP( 0);										\
	SPRITE_PLOTPIXEL_CLIP( 1);										\
	SPRITE_PLOTPIXEL_CLIP( 2);										\
	SPRITE_PLOTPIXEL_CLIP( 3);										\
	SPRITE_PLOTPIXEL_CLIP( 4);										\
	SPRITE_PLOTPIXEL_CLIP( 5);										\
	SPRITE_PLOTPIXEL_CLIP( 6);										\
	SPRITE_PLOTPIXEL_CLIP( 7);										\
	SPRITE_PLOTPIXEL_CLIP( 8);										\
	SPRITE_PLOTPIXEL_CLIP( 9);										\
	SPRITE_PLOTPIXEL_CLIP(10);										\
	SPRITE_PLOTPIXEL_CLIP(11);										\
	SPRITE_PLOTPIXEL_CLIP(12);										\
	SPRITE_PLOTPIXEL_CLIP(13);										\
	SPRITE_PLOTPIXEL_CLIP(14);										\
	SPRITE_PLOTPIXEL_CLIP(15);										\
}																	\

#define PgmRenderSpriteSkipLine()									\
{																	\
	for (INT16 xcnt = 0 ; xcnt < spr->wide ; xcnt += 16) {			\
		msk = *bdata++;												\
		adata += sprmsktab[msk];									\
	}																\
}																	\

static void pgm_drawsprite_no_zoom(PgmSprite *spr)
{
	UINT32 WideHigh = spr->wide * spr->high;
	UINT16 *bdata = (UINT16 *)getBlockSPRMask(spr->boffset, (WideHigh << 1) + 4);
	if (bdata == NULL) return;
	
	UINT32 aoffset = bdata[0] | (bdata[1] << 16);
	UINT8 *adata = getBlockSPRCol((aoffset >> 2) * 3, WideHigh << 4);
	if (adata == NULL) return;
	
	bdata += 2; // because the first dword is the a data offset
	
#ifndef BUILD_PSP
	UINT16 *dst = (UINT16 *)pBurnDraw + (spr->ypos * PGM_WIDTH);
#else
	UINT16 *dst = (UINT16 *)pBurnDraw + (spr->ypos << 9);
#endif
	
	INT16 yflip_dir = PGM_WIDTH;
	
	if (spr->flipy) {
		spr->ypos += (spr->high - 1);
		
#ifndef BUILD_PSP
		dst += (spr->high - 1) * PGM_WIDTH;
#else
		dst += ((spr->high - 1) << 9);
#endif
		
		yflip_dir = -PGM_WIDTH;
	}
	
	spr->wide <<= 4;
	
	if (spr->flipx) {
		spr->xpos += (spr->wide - 1);
	}
	
	for (INT16 ycnt = 0 ; ycnt < spr->high ; ycnt++) {
		INT16 sy;
		UINT16 msk;
		
		if (spr->flipy) {
			sy = spr->ypos - ycnt;
			if (sy < 0) break;
		} else {
			sy = spr->ypos + ycnt;
			if (sy >= 224) break;
		}
		
		if ((sy < 0) || (sy >= 224)) {
			PgmRenderSpriteSkipLine();
			dst += yflip_dir;
			continue;
		}
		
		if (spr->flipx) {
			for (INT16 xcnt = 0 ; xcnt < spr->wide ; xcnt += 16) {
				msk = *bdata++;
				if (msk == 0xFFFF) {
					continue;
				}
				
				INT16 sx = spr->xpos - xcnt;
				if ((sx >= 15) && (sx < 448)) {
					PgmRenderSpriteFlipX();
				} else if ((sx >= 0) && (sx < (448 + 15))) {
					PgmRenderSpriteFlipX_Clip();
				} else {
					adata += sprmsktab[msk];
				}
			}
		} else {
			for (INT16 xcnt = 0 ; xcnt < spr->wide; xcnt += 16) {
				msk = *bdata++;
				if (msk == 0xFFFF) {
					continue;
				}
				
				INT16 sx = spr->xpos + xcnt;
				if ((sx >= 0) && (sx < (448 - 15))) {
					PgmRenderSprite();
				} else if ((sx >= -15) && (sx < 448)) {
					PgmRenderSprite_Clip();
				} else {
					adata += sprmsktab[msk];
				}
			}
		}
		
		dst += yflip_dir;
	}
}


#define PgmRenderSpriteDoubleLine()												\
{																				\
	INT16 sx = spr->xpos;														\
	for (INT16 xcnt = 0 ; xcnt < spr->wide ; xcnt += 16) {						\
		UINT16 xzoom_h = (spr->xzoom >> (xcnt & 0x10)) & 0xFFFF;				\
																				\
		msk = *bdata++;															\
		if (msk == 0xFFFF) {													\
			sx += (xoffset << 4);												\
			if (spr->xzoom != 0) {												\
				if (spr->xgrow == 1) {											\
					sx += (sprmsktab[xzoom_h ^ 0xFFFF] * xoffset);				\
				} else {														\
					sx -= (sprmsktab[xzoom_h ^ 0xFFFF] * xoffset);				\
				}																\
			}																	\
			continue;															\
		}																		\
																				\
		for(INT16 xz = 0; xz < 16; xz++) {										\
			UINT8 xzoombit = (xzoom_h >> xz) & 0x01;							\
																				\
			if ((xzoombit == 1) && (spr->xgrow == 1)) {							\
				/* double line, double column */								\
				if (!(msk & 0x01)) {											\
					INT16 sx_double = sx + xoffset;								\
					INT16 sy_double = sy + yoffset;								\
					UINT16 d;													\
					bool p = false;												\
					if ((sx >= 0) && (sx < 448)) {								\
						if ((sy >= 0) && (sy < 224)) {							\
							d = *(PgmPalette + (*adata | spr->palt));			\
							p = true;											\
							dst[sx] = d;										\
						}														\
																				\
						if ((sy_double >= 0) && (sy_double < 224)) {			\
							if (!p) {											\
								d = *(PgmPalette + (*adata | spr->palt));		\
								p = true;										\
							}													\
							dst[sx + yflip_dir] = d;							\
						}														\
					}															\
					if ((sx_double >= 0) && (sx_double < 448)) {				\
						if ((sy >= 0) && (sy < 224)) {							\
							if (!p) {											\
								d = *(PgmPalette + (*adata | spr->palt));		\
								p = true;										\
							}													\
							dst[sx_double] = d;									\
						}														\
																				\
						if ((sy_double >= 0) && (sy_double < 224)) {			\
							if (!p) d = *(PgmPalette + (*adata | spr->palt));	\
							dst[sx_double + yflip_dir] = d;						\
						}														\
					}															\
					adata++;													\
				}																\
				sx += (xoffset << 1);											\
																				\
			} else if (xzoombit == 1 && spr->xgrow == 0) {						\
				/* skip column */												\
				if (!(msk & 0x01)) {											\
					adata++;													\
				}																\
			} else {															\
				/* double line, normal column */								\
				if (!(msk & 0x01)) {											\
					if ((sx >= 0) && (sx < 448)) {								\
						UINT16 d;												\
						bool p = false;											\
						if ((sy >= 0) && (sy < 224)) {							\
							d = *(PgmPalette + (*adata | spr->palt));			\
							p = true;											\
							dst[sx] = d;										\
						}														\
																				\
						if (((sy + yoffset) >= 0) && ((sy + yoffset) < 224)) {	\
							if (!p) d = *(PgmPalette + (*adata | spr->palt));	\
							dst[sx + yflip_dir] = d;							\
						}														\
					}															\
					adata++;													\
				}																\
				sx += xoffset;													\
			}																	\
			msk >>= 1;															\
		}																		\
	}																			\
}																				\

#define PgmRenderSpriteNormalLine()												\
{																				\
	INT16 sx = spr->xpos;														\
	for (INT16 xcnt = 0 ; xcnt < spr->wide ; xcnt += 16) {						\
		UINT16 xzoom_h = (spr->xzoom >> (xcnt & 0x10)) & 0xFFFF;				\
																				\
		msk = *bdata++;															\
		if (msk == 0xFFFF) {													\
			sx += (xoffset << 4);												\
			if (spr->xzoom != 0) {												\
				if (spr->xgrow == 1) {											\
					sx += (sprmsktab[xzoom_h ^ 0xFFFF] * xoffset);				\
				} else {														\
					sx -= (sprmsktab[xzoom_h ^ 0xFFFF] * xoffset);				\
				}																\
			}																	\
			continue;															\
		}																		\
																				\
		for(INT16 xz = 0; xz < 16; xz++) {										\
			UINT8 xzoombit = (xzoom_h >> xz) & 0x01;							\
																				\
			if ((xzoombit == 1) && (spr->xgrow == 1)) {							\
				/* normal line, double column */								\
				if (!(msk & 0x01)) {											\
					UINT16 d;													\
					bool p = false;												\
					if ((sx >= 0) && (sx < 448)) {								\
						d = *(PgmPalette + (*adata | spr->palt));				\
						p = true;												\
						dst[sx] = d;											\
					}															\
					if (((sx + xoffset) >= 0) && ((sx + xoffset) < 448)) {		\
						if (!p) d = *(PgmPalette + (*adata | spr->palt));		\
						dst[sx + xoffset] = d;									\
					}															\
					adata++;													\
				}																\
				sx += (xoffset << 1);											\
																				\
			} else if (xzoombit == 1 && spr->xgrow == 0) {						\
				/* skip column */												\
				if (!(msk & 0x01)) {											\
					adata++;													\
				}																\
			} else {															\
				/* normal line, normal column */								\
				if (!(msk & 0x01)) {											\
					if ((sx >= 0) && (sx < 448)) {								\
						dst[sx] = *(PgmPalette + (*adata | spr->palt));			\
					}															\
					adata++;													\
				}																\
				sx += xoffset;													\
			}																	\
			msk >>= 1;															\
		}																		\
	}																			\
}																				\

static void pgm_drawsprite_zoom(PgmSprite *spr)
{
	UINT32 WideHigh = spr->wide * spr->high;
	UINT16 *bdata = (UINT16 *)getBlockSPRMask(spr->boffset, (WideHigh << 1) + 4);
	if (bdata == NULL) return;
	
	UINT32 aoffset = bdata[0] | (bdata[1] << 16);
	UINT8 *adata = getBlockSPRCol((aoffset >> 2) * 3, WideHigh << 4);
	if (adata == NULL) return;
	
	bdata += 2; // because the first dword is the a data offset
	
#ifndef BUILD_PSP
	UINT16 *dst = (UINT16 *)pBurnDraw + (spr->ypos * PGM_WIDTH);
#else
	UINT16 *dst = (UINT16 *)pBurnDraw + (spr->ypos << 9);
#endif
	
	INT8 yoffset = 1;
	INT16 yflip_dir = PGM_WIDTH;
	
	if (spr->flipy) {
		UINT16 draw_high = spr->high;
		if (spr->yzoom != 0) {
			UINT16 yzoomcnt = 0;
/*
			for (INT16 yzcnt = 0 ; yzcnt < draw_high; yzcnt++) {
				yzoomcnt += (spr->yzoom >> (yzcnt & 0x1F)) & 0x01;
			}
*/
			// multiple of 16(?)
			for (INT16 yzcnt = 0 ; yzcnt < draw_high; yzcnt += 16) {
				UINT16 yzoom_hw = (spr->yzoom >> (yzcnt & 0x10)) ^ 0xFFFF;
				yzoomcnt += sprmsktab[yzoom_hw];
			}
			
			if (spr->ygrow == 1) {
				draw_high += yzoomcnt;
			} else {
				draw_high -= yzoomcnt;
			}
		}
		
		spr->ypos += (draw_high - 1);
		
#ifndef BUILD_PSP
		dst += (draw_high - 1) * PGM_WIDTH;
#else
		dst += ((draw_high - 1) << 9);
#endif
		
		yoffset = -1;
		yflip_dir = -PGM_WIDTH;
	}
	
	INT8 xoffset = 1;
	spr->wide <<= 4;
	
	if (spr->flipx) {
		UINT16 draw_wide = spr->wide;
		if (spr->xzoom != 0) {
			UINT16 xzoomcnt = 0;
			for (INT16 xzcnt = 0 ; xzcnt < draw_wide; xzcnt += 16) {
				UINT16 xzoom_hw = (spr->xzoom >> (xzcnt & 0x10)) ^ 0xFFFF;
				xzoomcnt += sprmsktab[xzoom_hw];
			}
			
			if (spr->xgrow == 1) {
				draw_wide += xzoomcnt;
			} else {
				draw_wide -= xzoomcnt;
			}
		}
		
		spr->xpos += (draw_wide - 1);
		xoffset = -1;
	}
	
	INT16 sy = spr->ypos;
	
	for (INT16 ycnt = 0 ; ycnt < spr->high ; ycnt++) {
		
		if (spr->flipy) {
			if (sy < 0) break;
		} else {
			if (sy >= 224) break;
		}
		
		UINT16 msk;
		UINT8 yzoombit = (spr->yzoom >> (ycnt & 0x1F)) & 0x01;
		
		if (yzoombit == 1 && spr->ygrow == 1) {
			/* double line */
			if ((sy < (0 - 1)) || (sy >= (224 + 1))) {
				PgmRenderSpriteSkipLine();
				sy += (yoffset << 1);
				dst += (yflip_dir << 1);
				continue;
			}
			
			PgmRenderSpriteDoubleLine();
			
			sy  += (yoffset << 1);
			dst += (yflip_dir << 1);
			
		} else if (yzoombit ==1 && spr->ygrow == 0) {
			/* skip line */
			PgmRenderSpriteSkipLine();
			
		} else {
			 /* normal line */
			if ((sy < 0) || (sy >= 224)) {
				PgmRenderSpriteSkipLine();
				sy += yoffset;
				dst += yflip_dir;
				continue;
			}
			
			PgmRenderSpriteNormalLine();
			
			sy  += yoffset;
			dst += yflip_dir;
		}
	}
}


/*
	Sprite format:
	32 color packed three 5 bit pixels to 2 bytes (x333332222211111)
	Sprites also include a mask of 8 pixels per byte, A 1 bit indicates an omitted pixel.
	Pixels that are marked as omitted in the mask are not included in the pixel data, and are
	skipped over on the screen.
	Sprites can be any size (multiple of 16(?) pixels wide, but also need to allow 3 pixels to be packed into 2 bytes, or have the non existent pixels marked as omitted in the mask).

	Sprite table is located at 0x800000 - 0x800A00

	10 byte per sprite, 256 sprites

	Sprite data format:
	XXXX Xxxx xxxx xxxx
	YYYY Y-yy yyyy yyyy
	-Ffp pppp Pvvv vvvv
	vvvv vvvv vvvv vvvv
	wwww wwwh hhhh hhhh

	X = X zoom ($8000 is default)
	Y = Y zoom ($8000 is default)
	x = X position
	y = Y position
	p = Palette
	F = Y flip
	f = X flip
	P = Priority
	v = Sprite data offset / 2 (in B rom)
	w = Width / 16(?)
	h = Height
*/


static void pgm_drawsprites(UINT8 priority)
{
	PgmSprite ALIGN_DATA spr = { 0 };
	
	UINT16 *pgm_sprite_source = PgmSprBuf;
	const UINT16 *finish = PgmSprBuf + (0xA00 / 2);
	
	UINT16 *pgm_sprite_zoomtable = &PgmVidReg[0x1000 / 2];
	
	while (pgm_sprite_source < finish) {
		spr.xpos = (INT16)(pgm_sprite_source[0] << 5) >> 5;
		spr.ypos = (INT16)(pgm_sprite_source[1] << 6) >> 6;
		
		spr.xgrow   =  (pgm_sprite_source[0] & 0x8000) >> 15;
		UINT8 xzom  =  (pgm_sprite_source[0] & 0x7800) >> 11;
		spr.ygrow   =  (pgm_sprite_source[1] & 0x8000) >> 15;
		UINT8 yzom  =  (pgm_sprite_source[1] & 0x7800) >> 11;
		
		spr.palt    =  (pgm_sprite_source[2] & 0x1f00) >> 3;
		
		spr.flipx   =  (pgm_sprite_source[2] & 0x2000) >> 13;
		spr.flipy   =  (pgm_sprite_source[2] & 0x4000) >> 14;
		
		UINT8 pri   =  (pgm_sprite_source[2] & 0x0080) >>  7;
		spr.boffset = ((pgm_sprite_source[2] & 0x007F) << 16) | pgm_sprite_source[3];
		
		spr.wide    =  (pgm_sprite_source[4] & 0x7e00) >> 9;
		spr.high    =   pgm_sprite_source[4] & 0x01FF;
		
		if (spr.high == 0) break; /* is this right? */
		if ((priority == 1) && (pri == 0)) break;
		
		if (spr.xgrow) {
			xzom = 0x10 - xzom; // this way it doesn't but there is a bad line when zooming after the level select?
		}
		if (spr.ygrow) {
			yzom = 0x10 - yzom;
		}
		spr.xzoom = (pgm_sprite_zoomtable[xzom << 1] << 16) | pgm_sprite_zoomtable[(xzom << 1) + 1];
		spr.yzoom = (pgm_sprite_zoomtable[yzom << 1] << 16) | pgm_sprite_zoomtable[(yzom << 1) + 1];
		
		spr.boffset <<= 1;
		
		if ((spr.xzoom != 0) || (spr.yzoom != 0)) {
			pgm_drawsprite_zoom(&spr);
		} else {
			pgm_drawsprite_no_zoom(&spr);
		}
		
		pgm_sprite_source += 5;
    }
}


#define RENDER_TILE_NORMAL  (0x00)
#define RENDER_TILE_FLIPX   (0x40)
#define RENDER_TILE_FLIPY   (0x80)
#define RENDER_TILE_FLIPXY  (0xC0)

#ifndef BUILD_PSP
	#define PgmRender8x8FixStartPositionFlipY()    p += ( 7 * PGM_WIDTH)
	#define PgmRender32x32FixStartPositionFlipY()  p += (31 * PGM_WIDTH)
#else
	#define PgmRender8x8FixStartPositionFlipY()    p += ( 7 << 9)
	#define PgmRender32x32FixStartPositionFlipY()  p += (31 << 9)
#endif

#define PLOTPIXEL(x)												\
{																	\
	p[x] = *(pal + (d[x] | color));									\
}																	\

#define PLOTPIXEL_FLIPX(x, a)										\
{																	\
	p[x] = *(pal + (d[a] | color));									\
}																	\

#define PLOTPIXEL_MASK(x, mc)										\
{																	\
	if (d[x] != mc) {												\
		p[x] = *(pal + (d[x] | color));								\
	}																\
}																	\

#define PLOTPIXEL_MASK_FLIPX(x, a, mc)								\
{																	\
	if (d[a] != mc) {												\
		p[x] = *(pal + (d[a] | color));								\
	}																\
}																	\

#define CLIPPIXEL(x, sx, mx, a)										\
{																	\
	if (((sx + x) >= 0) && ((sx + x) < mx)) {						\
		a;															\
	}																\
}																	\

#define PgmRender8x8Tile_Clip()										\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL(0));						\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL(1));						\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL(2));						\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL(3));						\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL(4));						\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL(5));						\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL(6));						\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL(7));						\
	}																\
}																	\

#define PgmRender8x8Tile_FlipX_Clip()								\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_FLIPX(7, 0));				\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_FLIPX(6, 1));				\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_FLIPX(5, 2));				\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_FLIPX(4, 3));				\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_FLIPX(3, 4));				\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_FLIPX(2, 5));				\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_FLIPX(1, 6));				\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_FLIPX(0, 7));				\
	}																\
}																	\

#define PgmRender8x8Tile_FlipY_Clip()								\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL(0));						\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL(1));						\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL(2));						\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL(3));						\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL(4));						\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL(5));						\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL(6));						\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL(7));						\
	}																\
}																	\

#define PgmRender8x8Tile_FlipXY_Clip()								\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_FLIPX(7, 0));				\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_FLIPX(6, 1));				\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_FLIPX(5, 2));				\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_FLIPX(4, 3));				\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_FLIPX(3, 4));				\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_FLIPX(2, 5));				\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_FLIPX(1, 6));				\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_FLIPX(0, 7));				\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_Clip()								\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_MASK(0, 15));				\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_MASK(1, 15));				\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_MASK(2, 15));				\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_MASK(3, 15));				\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_MASK(4, 15));				\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_MASK(5, 15));				\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_MASK(6, 15));				\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_MASK(7, 15));				\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipX_Clip()							\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_MASK_FLIPX(7, 0, 15));		\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_MASK_FLIPX(6, 1, 15));		\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_MASK_FLIPX(5, 2, 15));		\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_MASK_FLIPX(4, 3, 15));		\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_MASK_FLIPX(3, 4, 15));		\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_MASK_FLIPX(2, 5, 15));		\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_MASK_FLIPX(1, 6, 15));		\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_MASK_FLIPX(0, 7, 15));		\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipY_Clip()							\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_MASK(0, 15));				\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_MASK(1, 15));				\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_MASK(2, 15));				\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_MASK(3, 15));				\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_MASK(4, 15));				\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_MASK(5, 15));				\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_MASK(6, 15));				\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_MASK(7, 15));				\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipXY_Clip()							\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(7, sx, 448, PLOTPIXEL_MASK_FLIPX(7, 0, 15));		\
		CLIPPIXEL(6, sx, 448, PLOTPIXEL_MASK_FLIPX(6, 1, 15));		\
		CLIPPIXEL(5, sx, 448, PLOTPIXEL_MASK_FLIPX(5, 2, 15));		\
		CLIPPIXEL(4, sx, 448, PLOTPIXEL_MASK_FLIPX(4, 3, 15));		\
		CLIPPIXEL(3, sx, 448, PLOTPIXEL_MASK_FLIPX(3, 4, 15));		\
		CLIPPIXEL(2, sx, 448, PLOTPIXEL_MASK_FLIPX(2, 5, 15));		\
		CLIPPIXEL(1, sx, 448, PLOTPIXEL_MASK_FLIPX(1, 6, 15));		\
		CLIPPIXEL(0, sx, 448, PLOTPIXEL_MASK_FLIPX(0, 7, 15));		\
	}																\
}																	\

#define PgmRender8x8Tile()											\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		PLOTPIXEL(0);												\
		PLOTPIXEL(1);												\
		PLOTPIXEL(2);												\
		PLOTPIXEL(3);												\
		PLOTPIXEL(4);												\
		PLOTPIXEL(5);												\
		PLOTPIXEL(6);												\
		PLOTPIXEL(7);												\
	}																\
}																	\

#define PgmRender8x8Tile_FlipX()									\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_FLIPX(7, 0);										\
		PLOTPIXEL_FLIPX(6, 1);										\
		PLOTPIXEL_FLIPX(5, 2);										\
		PLOTPIXEL_FLIPX(4, 3);										\
		PLOTPIXEL_FLIPX(3, 4);										\
		PLOTPIXEL_FLIPX(2, 5);										\
		PLOTPIXEL_FLIPX(1, 6);										\
		PLOTPIXEL_FLIPX(0, 7);										\
	}																\
}																	\

#define PgmRender8x8Tile_FlipY()									\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		PLOTPIXEL(0);												\
		PLOTPIXEL(1);												\
		PLOTPIXEL(2);												\
		PLOTPIXEL(3);												\
		PLOTPIXEL(4);												\
		PLOTPIXEL(5);												\
		PLOTPIXEL(6);												\
		PLOTPIXEL(7);												\
	}																\
}																	\

#define PgmRender8x8Tile_FlipXY()									\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_FLIPX(7, 0);										\
		PLOTPIXEL_FLIPX(6, 1);										\
		PLOTPIXEL_FLIPX(5, 2);										\
		PLOTPIXEL_FLIPX(4, 3);										\
		PLOTPIXEL_FLIPX(3, 4);										\
		PLOTPIXEL_FLIPX(2, 5);										\
		PLOTPIXEL_FLIPX(1, 6);										\
		PLOTPIXEL_FLIPX(0, 7);										\
	}																\
}																	\

#define PgmRender8x8Tile_Mask()										\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_MASK(0, 15);										\
		PLOTPIXEL_MASK(1, 15);										\
		PLOTPIXEL_MASK(2, 15);										\
		PLOTPIXEL_MASK(3, 15);										\
		PLOTPIXEL_MASK(4, 15);										\
		PLOTPIXEL_MASK(5, 15);										\
		PLOTPIXEL_MASK(6, 15);										\
		PLOTPIXEL_MASK(7, 15);										\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipX()								\
{																	\
	for (INT8 k = 0; k < 8; k++, p += PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_MASK_FLIPX(7, 0, 15);								\
		PLOTPIXEL_MASK_FLIPX(6, 1, 15);								\
		PLOTPIXEL_MASK_FLIPX(5, 2, 15);								\
		PLOTPIXEL_MASK_FLIPX(4, 3, 15);								\
		PLOTPIXEL_MASK_FLIPX(3, 4, 15);								\
		PLOTPIXEL_MASK_FLIPX(2, 5, 15);								\
		PLOTPIXEL_MASK_FLIPX(1, 6, 15);								\
		PLOTPIXEL_MASK_FLIPX(0, 7, 15);								\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipY()								\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_MASK(0, 15);										\
		PLOTPIXEL_MASK(1, 15);										\
		PLOTPIXEL_MASK(2, 15);										\
		PLOTPIXEL_MASK(3, 15);										\
		PLOTPIXEL_MASK(4, 15);										\
		PLOTPIXEL_MASK(5, 15);										\
		PLOTPIXEL_MASK(6, 15);										\
		PLOTPIXEL_MASK(7, 15);										\
	}																\
}																	\

#define PgmRender8x8Tile_Mask_FlipXY()								\
{																	\
	PgmRender8x8FixStartPositionFlipY();							\
																	\
	for (INT8 k = 7; k >= 0; k--, p -= PGM_WIDTH, d += 8) {			\
		PLOTPIXEL_MASK_FLIPX(7, 0, 15);								\
		PLOTPIXEL_MASK_FLIPX(6, 1, 15);								\
		PLOTPIXEL_MASK_FLIPX(5, 2, 15);								\
		PLOTPIXEL_MASK_FLIPX(4, 3, 15);								\
		PLOTPIXEL_MASK_FLIPX(3, 4, 15);								\
		PLOTPIXEL_MASK_FLIPX(2, 5, 15);								\
		PLOTPIXEL_MASK_FLIPX(1, 6, 15);								\
		PLOTPIXEL_MASK_FLIPX(0, 7, 15);								\
	}																\
}																	\

/* TX Layer */

/*
	0x904000 - 0x905fff is the Text Overlay Ram ( RamTx )
	each tile uses 4 bytes, the tilemap is 64x32?

	the layer uses 4bpp 8x8 tiles from the 'T' roms

	Text format:
	8x8 16 color tiles
	Each nibble represents a pixel
	The low nibble is left pixel of the two.
	(eg. 1000 0001 0100 0010 would be 1824 on the screen.)

	Text/Background map format:
	---- ---- Ffpp ppp- nnnn nnnn nnnn nnnn

	n = tile number
	p = palette
	F = Y flip
	f = X flip

	Palette format:
	xRRRRRGGGGGBBBBB

	Palettes:
	0xA01000 0xA011FF	Text		(16 * 2 bytes x 32 palettes)

	Scroll registers:
	0xB05000	Text scroll Y register (WORD)
	0xB06000	Text scroll X register (WORD)
*/

static void draw_text()
{
	UINT16 *vram = (UINT16 *)PgmTxRam;
	UINT32 *pal = &PgmPalette[0x800];
	
	INT16 scrollx = ((INT16)PgmVidReg[0x6000 / 2]) & 0x1ff;
	INT16 scrolly = ((INT16)PgmVidReg[0x5000 / 2]) & 0x0ff;
	
	for (INT16 offs = 0; offs < 64 * 32; offs++) {
		UINT16 code = vram[offs << 1];
		
		if (texttrans[code] == 0) continue; // transparent
		
		INT16 sx = (offs & 0x3f) << 3;
		INT16 sy = (offs >> 6) << 3;
		
		sx -= scrollx;
		if (sx < -7) sx += 512;
		sy -= scrolly;
		if (sy < -7) sy += 256;
		
		if ((sx >= (448 + 7)) || (sy >= (224 + 7))) continue;
		
		UINT16 attr = vram[(offs << 1) + 1];
		UINT16 color = (attr & 0x3e) << 3;
		UINT8  flip  = (attr & 0xC0);
		
#ifndef BUILD_PSP
		UINT16 *p = (UINT16 *) pBurnDraw + sy * PGM_WIDTH + sx;
#else
		UINT16 *p = (UINT16 *) pBurnDraw + (sy << 9) + sx;
#endif
		UINT8  *d = getBlockTile(code << 6, 64);
		if (d == NULL) return;
		
		if ((sx >= 7) && (sy >= 7) && (sx < (448 - 7)) && (sy < (224 - 7))) {
			if (texttrans[code] & 2) { // opaque
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender8x8Tile();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender8x8Tile_FlipX();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender8x8Tile_FlipY();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender8x8Tile_FlipXY();
					break;
				}
			} else {
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender8x8Tile_Mask();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender8x8Tile_Mask_FlipX();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender8x8Tile_Mask_FlipY();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender8x8Tile_Mask_FlipXY();
					break;
				}
			}
		} else {
			if (texttrans[code] & 2) { // opaque
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender8x8Tile_Clip();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender8x8Tile_FlipX_Clip();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender8x8Tile_FlipY_Clip();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender8x8Tile_FlipXY_Clip();
					break;
				}
			} else {
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender8x8Tile_Mask_Clip();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender8x8Tile_Mask_FlipX_Clip();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender8x8Tile_Mask_FlipY_Clip();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender8x8Tile_Mask_FlipXY_Clip();
					break;
				}
			}
		}
	}
}


#define PgmRender32x32Tile_Clip()									\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL( 0));						\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL( 1));						\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL( 2));						\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL( 3));						\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL( 4));						\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL( 5));						\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL( 6));						\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL( 7));						\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL( 8));						\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL( 9));						\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL(10));						\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL(11));						\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL(12));						\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL(13));						\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL(14));						\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL(15));						\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL(16));						\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL(17));						\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL(18));						\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL(19));						\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL(20));						\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL(21));						\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL(22));						\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL(23));						\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL(24));						\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL(25));						\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL(26));						\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL(27));						\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL(28));						\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL(29));						\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL(30));						\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL(31));						\
	}																\
}																	\

#define PgmRender32x32Tile_FlipX_Clip()								\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL_FLIPX(31,  0));			\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL_FLIPX(30,  1));			\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL_FLIPX(29,  2));			\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL_FLIPX(28,  3));			\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL_FLIPX(27,  4));			\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL_FLIPX(26,  5));			\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL_FLIPX(25,  6));			\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL_FLIPX(24,  7));			\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL_FLIPX(23,  8));			\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL_FLIPX(22,  9));			\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL_FLIPX(21, 10));			\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL_FLIPX(20, 11));			\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL_FLIPX(19, 12));			\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL_FLIPX(18, 13));			\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL_FLIPX(17, 14));			\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL_FLIPX(16, 15));			\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL_FLIPX(15, 16));			\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL_FLIPX(14, 17));			\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL_FLIPX(13, 18));			\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL_FLIPX(12, 19));			\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL_FLIPX(11, 20));			\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL_FLIPX(10, 21));			\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL_FLIPX( 9, 22));			\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL_FLIPX( 8, 23));			\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL_FLIPX( 7, 24));			\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL_FLIPX( 6, 25));			\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL_FLIPX( 5, 26));			\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL_FLIPX( 4, 27));			\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL_FLIPX( 3, 28));			\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL_FLIPX( 2, 29));			\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL_FLIPX( 1, 30));			\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL_FLIPX( 0, 31));			\
	}																\
}																	\

#define PgmRender32x32Tile_FlipY_Clip()								\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL( 0));						\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL( 1));						\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL( 2));						\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL( 3));						\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL( 4));						\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL( 5));						\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL( 6));						\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL( 7));						\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL( 8));						\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL( 9));						\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL(10));						\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL(11));						\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL(12));						\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL(13));						\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL(14));						\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL(15));						\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL(16));						\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL(17));						\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL(18));						\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL(19));						\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL(20));						\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL(21));						\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL(22));						\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL(23));						\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL(24));						\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL(25));						\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL(26));						\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL(27));						\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL(28));						\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL(29));						\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL(30));						\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL(31));						\
	}																\
}																	\

#define PgmRender32x32Tile_FlipXY_Clip()							\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL_FLIPX(31,  0));			\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL_FLIPX(30,  1));			\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL_FLIPX(29,  2));			\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL_FLIPX(28,  3));			\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL_FLIPX(27,  4));			\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL_FLIPX(26,  5));			\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL_FLIPX(25,  6));			\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL_FLIPX(24,  7));			\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL_FLIPX(23,  8));			\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL_FLIPX(22,  9));			\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL_FLIPX(21, 10));			\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL_FLIPX(20, 11));			\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL_FLIPX(19, 12));			\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL_FLIPX(18, 13));			\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL_FLIPX(17, 14));			\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL_FLIPX(16, 15));			\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL_FLIPX(15, 16));			\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL_FLIPX(14, 17));			\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL_FLIPX(13, 18));			\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL_FLIPX(12, 19));			\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL_FLIPX(11, 20));			\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL_FLIPX(10, 21));			\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL_FLIPX( 9, 22));			\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL_FLIPX( 8, 23));			\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL_FLIPX( 7, 24));			\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL_FLIPX( 6, 25));			\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL_FLIPX( 5, 26));			\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL_FLIPX( 4, 27));			\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL_FLIPX( 3, 28));			\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL_FLIPX( 2, 29));			\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL_FLIPX( 1, 30));			\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL_FLIPX( 0, 31));			\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_Clip()								\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL_MASK( 0, 31));				\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL_MASK( 1, 31));				\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL_MASK( 2, 31));				\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL_MASK( 3, 31));				\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL_MASK( 4, 31));				\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL_MASK( 5, 31));				\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL_MASK( 6, 31));				\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL_MASK( 7, 31));				\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL_MASK( 8, 31));				\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL_MASK( 9, 31));				\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL_MASK(10, 31));				\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL_MASK(11, 31));				\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL_MASK(12, 31));				\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL_MASK(13, 31));				\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL_MASK(14, 31));				\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL_MASK(15, 31));				\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL_MASK(16, 31));				\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL_MASK(17, 31));				\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL_MASK(18, 31));				\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL_MASK(19, 31));				\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL_MASK(20, 31));				\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL_MASK(21, 31));				\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL_MASK(22, 31));				\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL_MASK(23, 31));				\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL_MASK(24, 31));				\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL_MASK(25, 31));				\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL_MASK(26, 31));				\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL_MASK(27, 31));				\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL_MASK(28, 31));				\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL_MASK(29, 31));				\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL_MASK(30, 31));				\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL_MASK(31, 31));				\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipX_Clip()						\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		if ((sy + k) < 0) continue;									\
		if ((sy + k) >= 224) break;									\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL_MASK_FLIPX(31,  0, 31));	\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL_MASK_FLIPX(30,  1, 31));	\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL_MASK_FLIPX(29,  2, 31));	\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL_MASK_FLIPX(28,  3, 31));	\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL_MASK_FLIPX(27,  4, 31));	\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL_MASK_FLIPX(26,  5, 31));	\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL_MASK_FLIPX(25,  6, 31));	\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL_MASK_FLIPX(24,  7, 31));	\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL_MASK_FLIPX(23,  8, 31));	\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL_MASK_FLIPX(22,  9, 31));	\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL_MASK_FLIPX(21, 10, 31));	\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL_MASK_FLIPX(20, 11, 31));	\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL_MASK_FLIPX(19, 12, 31));	\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL_MASK_FLIPX(18, 13, 31));	\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL_MASK_FLIPX(17, 14, 31));	\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL_MASK_FLIPX(16, 15, 31));	\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL_MASK_FLIPX(15, 16, 31));	\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL_MASK_FLIPX(14, 17, 31));	\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL_MASK_FLIPX(13, 18, 31));	\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL_MASK_FLIPX(12, 19, 31));	\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL_MASK_FLIPX(11, 20, 31));	\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL_MASK_FLIPX(10, 21, 31));	\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL_MASK_FLIPX( 9, 22, 31));	\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL_MASK_FLIPX( 8, 23, 31));	\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL_MASK_FLIPX( 7, 24, 31));	\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL_MASK_FLIPX( 6, 25, 31));	\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL_MASK_FLIPX( 5, 26, 31));	\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL_MASK_FLIPX( 4, 27, 31));	\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL_MASK_FLIPX( 3, 28, 31));	\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL_MASK_FLIPX( 2, 29, 31));	\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL_MASK_FLIPX( 1, 30, 31));	\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL_MASK_FLIPX( 0, 31, 31));	\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipY_Clip()						\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL( 0, sx, 448, PLOTPIXEL_MASK( 0, 31));				\
		CLIPPIXEL( 1, sx, 448, PLOTPIXEL_MASK( 1, 31));				\
		CLIPPIXEL( 2, sx, 448, PLOTPIXEL_MASK( 2, 31));				\
		CLIPPIXEL( 3, sx, 448, PLOTPIXEL_MASK( 3, 31));				\
		CLIPPIXEL( 4, sx, 448, PLOTPIXEL_MASK( 4, 31));				\
		CLIPPIXEL( 5, sx, 448, PLOTPIXEL_MASK( 5, 31));				\
		CLIPPIXEL( 6, sx, 448, PLOTPIXEL_MASK( 6, 31));				\
		CLIPPIXEL( 7, sx, 448, PLOTPIXEL_MASK( 7, 31));				\
		CLIPPIXEL( 8, sx, 448, PLOTPIXEL_MASK( 8, 31));				\
		CLIPPIXEL( 9, sx, 448, PLOTPIXEL_MASK( 9, 31));				\
		CLIPPIXEL(10, sx, 448, PLOTPIXEL_MASK(10, 31));				\
		CLIPPIXEL(11, sx, 448, PLOTPIXEL_MASK(11, 31));				\
		CLIPPIXEL(12, sx, 448, PLOTPIXEL_MASK(12, 31));				\
		CLIPPIXEL(13, sx, 448, PLOTPIXEL_MASK(13, 31));				\
		CLIPPIXEL(14, sx, 448, PLOTPIXEL_MASK(14, 31));				\
		CLIPPIXEL(15, sx, 448, PLOTPIXEL_MASK(15, 31));				\
		CLIPPIXEL(16, sx, 448, PLOTPIXEL_MASK(16, 31));				\
		CLIPPIXEL(17, sx, 448, PLOTPIXEL_MASK(17, 31));				\
		CLIPPIXEL(18, sx, 448, PLOTPIXEL_MASK(18, 31));				\
		CLIPPIXEL(19, sx, 448, PLOTPIXEL_MASK(19, 31));				\
		CLIPPIXEL(20, sx, 448, PLOTPIXEL_MASK(20, 31));				\
		CLIPPIXEL(21, sx, 448, PLOTPIXEL_MASK(21, 31));				\
		CLIPPIXEL(22, sx, 448, PLOTPIXEL_MASK(22, 31));				\
		CLIPPIXEL(23, sx, 448, PLOTPIXEL_MASK(23, 31));				\
		CLIPPIXEL(24, sx, 448, PLOTPIXEL_MASK(24, 31));				\
		CLIPPIXEL(25, sx, 448, PLOTPIXEL_MASK(25, 31));				\
		CLIPPIXEL(26, sx, 448, PLOTPIXEL_MASK(26, 31));				\
		CLIPPIXEL(27, sx, 448, PLOTPIXEL_MASK(27, 31));				\
		CLIPPIXEL(28, sx, 448, PLOTPIXEL_MASK(28, 31));				\
		CLIPPIXEL(29, sx, 448, PLOTPIXEL_MASK(29, 31));				\
		CLIPPIXEL(30, sx, 448, PLOTPIXEL_MASK(30, 31));				\
		CLIPPIXEL(31, sx, 448, PLOTPIXEL_MASK(31, 31));				\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipXY_Clip()						\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		if ((sy + k) >= 224) continue;								\
		if ((sy + k) < 0) break;									\
		CLIPPIXEL(31, sy, 448, PLOTPIXEL_MASK_FLIPX(31,  0, 31));	\
		CLIPPIXEL(30, sy, 448, PLOTPIXEL_MASK_FLIPX(30,  1, 31));	\
		CLIPPIXEL(29, sy, 448, PLOTPIXEL_MASK_FLIPX(29,  2, 31));	\
		CLIPPIXEL(28, sy, 448, PLOTPIXEL_MASK_FLIPX(28,  3, 31));	\
		CLIPPIXEL(27, sy, 448, PLOTPIXEL_MASK_FLIPX(27,  4, 31));	\
		CLIPPIXEL(26, sy, 448, PLOTPIXEL_MASK_FLIPX(26,  5, 31));	\
		CLIPPIXEL(25, sy, 448, PLOTPIXEL_MASK_FLIPX(25,  6, 31));	\
		CLIPPIXEL(24, sy, 448, PLOTPIXEL_MASK_FLIPX(24,  7, 31));	\
		CLIPPIXEL(23, sy, 448, PLOTPIXEL_MASK_FLIPX(23,  8, 31));	\
		CLIPPIXEL(22, sy, 448, PLOTPIXEL_MASK_FLIPX(22,  9, 31));	\
		CLIPPIXEL(21, sy, 448, PLOTPIXEL_MASK_FLIPX(21, 10, 31));	\
		CLIPPIXEL(20, sy, 448, PLOTPIXEL_MASK_FLIPX(20, 11, 31));	\
		CLIPPIXEL(19, sy, 448, PLOTPIXEL_MASK_FLIPX(19, 12, 31));	\
		CLIPPIXEL(18, sy, 448, PLOTPIXEL_MASK_FLIPX(18, 13, 31));	\
		CLIPPIXEL(17, sy, 448, PLOTPIXEL_MASK_FLIPX(17, 14, 31));	\
		CLIPPIXEL(16, sy, 448, PLOTPIXEL_MASK_FLIPX(16, 15, 31));	\
		CLIPPIXEL(15, sy, 448, PLOTPIXEL_MASK_FLIPX(15, 16, 31));	\
		CLIPPIXEL(14, sy, 448, PLOTPIXEL_MASK_FLIPX(14, 17, 31));	\
		CLIPPIXEL(13, sy, 448, PLOTPIXEL_MASK_FLIPX(13, 18, 31));	\
		CLIPPIXEL(12, sy, 448, PLOTPIXEL_MASK_FLIPX(12, 19, 31));	\
		CLIPPIXEL(11, sy, 448, PLOTPIXEL_MASK_FLIPX(11, 20, 31));	\
		CLIPPIXEL(10, sy, 448, PLOTPIXEL_MASK_FLIPX(10, 21, 31));	\
		CLIPPIXEL( 9, sy, 448, PLOTPIXEL_MASK_FLIPX( 9, 22, 31));	\
		CLIPPIXEL( 8, sy, 448, PLOTPIXEL_MASK_FLIPX( 8, 23, 31));	\
		CLIPPIXEL( 7, sy, 448, PLOTPIXEL_MASK_FLIPX( 7, 24, 31));	\
		CLIPPIXEL( 6, sy, 448, PLOTPIXEL_MASK_FLIPX( 6, 25, 31));	\
		CLIPPIXEL( 5, sy, 448, PLOTPIXEL_MASK_FLIPX( 5, 26, 31));	\
		CLIPPIXEL( 4, sy, 448, PLOTPIXEL_MASK_FLIPX( 4, 27, 31));	\
		CLIPPIXEL( 3, sy, 448, PLOTPIXEL_MASK_FLIPX( 3, 28, 31));	\
		CLIPPIXEL( 2, sy, 448, PLOTPIXEL_MASK_FLIPX( 2, 29, 31));	\
		CLIPPIXEL( 1, sy, 448, PLOTPIXEL_MASK_FLIPX( 1, 30, 31));	\
		CLIPPIXEL( 0, sy, 448, PLOTPIXEL_MASK_FLIPX( 0, 31, 31));	\
	}																\
}																	\

#define PgmRender32x32Tile()										\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		PLOTPIXEL( 0);												\
		PLOTPIXEL( 1);												\
		PLOTPIXEL( 2);												\
		PLOTPIXEL( 3);												\
		PLOTPIXEL( 4);												\
		PLOTPIXEL( 5);												\
		PLOTPIXEL( 6);												\
		PLOTPIXEL( 7);												\
		PLOTPIXEL( 8);												\
		PLOTPIXEL( 9);												\
		PLOTPIXEL(10);												\
		PLOTPIXEL(11);												\
		PLOTPIXEL(12);												\
		PLOTPIXEL(13);												\
		PLOTPIXEL(14);												\
		PLOTPIXEL(15);												\
		PLOTPIXEL(16);												\
		PLOTPIXEL(17);												\
		PLOTPIXEL(18);												\
		PLOTPIXEL(19);												\
		PLOTPIXEL(20);												\
		PLOTPIXEL(21);												\
		PLOTPIXEL(22);												\
		PLOTPIXEL(23);												\
		PLOTPIXEL(24);												\
		PLOTPIXEL(25);												\
		PLOTPIXEL(26);												\
		PLOTPIXEL(27);												\
		PLOTPIXEL(28);												\
		PLOTPIXEL(29);												\
		PLOTPIXEL(30);												\
		PLOTPIXEL(31);												\
	}																\
}																	\

#define PgmRender32x32Tile_FlipX()									\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_FLIPX(31,  0);									\
		PLOTPIXEL_FLIPX(30,  1);									\
		PLOTPIXEL_FLIPX(29,  2);									\
		PLOTPIXEL_FLIPX(28,  3);									\
		PLOTPIXEL_FLIPX(27,  4);									\
		PLOTPIXEL_FLIPX(26,  5);									\
		PLOTPIXEL_FLIPX(25,  6);									\
		PLOTPIXEL_FLIPX(24,  7);									\
		PLOTPIXEL_FLIPX(23,  8);									\
		PLOTPIXEL_FLIPX(22,  9);									\
		PLOTPIXEL_FLIPX(21, 10);									\
		PLOTPIXEL_FLIPX(20, 11);									\
		PLOTPIXEL_FLIPX(19, 12);									\
		PLOTPIXEL_FLIPX(18, 13);									\
		PLOTPIXEL_FLIPX(17, 14);									\
		PLOTPIXEL_FLIPX(16, 15);									\
		PLOTPIXEL_FLIPX(15, 16);									\
		PLOTPIXEL_FLIPX(14, 17);									\
		PLOTPIXEL_FLIPX(13, 18);									\
		PLOTPIXEL_FLIPX(12, 19);									\
		PLOTPIXEL_FLIPX(11, 20);									\
		PLOTPIXEL_FLIPX(10, 21);									\
		PLOTPIXEL_FLIPX( 9, 22);									\
		PLOTPIXEL_FLIPX( 8, 23);									\
		PLOTPIXEL_FLIPX( 7, 24);									\
		PLOTPIXEL_FLIPX( 6, 25);									\
		PLOTPIXEL_FLIPX( 5, 26);									\
		PLOTPIXEL_FLIPX( 4, 27);									\
		PLOTPIXEL_FLIPX( 3, 28);									\
		PLOTPIXEL_FLIPX( 2, 29);									\
		PLOTPIXEL_FLIPX( 1, 30);									\
		PLOTPIXEL_FLIPX( 0, 31);									\
	}																\
}																	\

#define PgmRender32x32Tile_FlipY()									\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		PLOTPIXEL( 0);												\
		PLOTPIXEL( 1);												\
		PLOTPIXEL( 2);												\
		PLOTPIXEL( 3);												\
		PLOTPIXEL( 4);												\
		PLOTPIXEL( 5);												\
		PLOTPIXEL( 6);												\
		PLOTPIXEL( 7);												\
		PLOTPIXEL( 8);												\
		PLOTPIXEL( 9);												\
		PLOTPIXEL(10);												\
		PLOTPIXEL(11);												\
		PLOTPIXEL(12);												\
		PLOTPIXEL(13);												\
		PLOTPIXEL(14);												\
		PLOTPIXEL(15);												\
		PLOTPIXEL(16);												\
		PLOTPIXEL(17);												\
		PLOTPIXEL(18);												\
		PLOTPIXEL(19);												\
		PLOTPIXEL(20);												\
		PLOTPIXEL(21);												\
		PLOTPIXEL(22);												\
		PLOTPIXEL(23);												\
		PLOTPIXEL(24);												\
		PLOTPIXEL(25);												\
		PLOTPIXEL(26);												\
		PLOTPIXEL(27);												\
		PLOTPIXEL(28);												\
		PLOTPIXEL(29);												\
		PLOTPIXEL(30);												\
		PLOTPIXEL(31);												\
	}																\
}																	\

#define PgmRender32x32Tile_FlipXY()									\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_FLIPX(31,  0);									\
		PLOTPIXEL_FLIPX(30,  1);									\
		PLOTPIXEL_FLIPX(29,  2);									\
		PLOTPIXEL_FLIPX(28,  3);									\
		PLOTPIXEL_FLIPX(27,  4);									\
		PLOTPIXEL_FLIPX(26,  5);									\
		PLOTPIXEL_FLIPX(25,  6);									\
		PLOTPIXEL_FLIPX(24,  7);									\
		PLOTPIXEL_FLIPX(23,  8);									\
		PLOTPIXEL_FLIPX(22,  9);									\
		PLOTPIXEL_FLIPX(21, 10);									\
		PLOTPIXEL_FLIPX(20, 11);									\
		PLOTPIXEL_FLIPX(19, 12);									\
		PLOTPIXEL_FLIPX(18, 13);									\
		PLOTPIXEL_FLIPX(17, 14);									\
		PLOTPIXEL_FLIPX(16, 15);									\
		PLOTPIXEL_FLIPX(15, 16);									\
		PLOTPIXEL_FLIPX(14, 17);									\
		PLOTPIXEL_FLIPX(13, 18);									\
		PLOTPIXEL_FLIPX(12, 19);									\
		PLOTPIXEL_FLIPX(11, 20);									\
		PLOTPIXEL_FLIPX(10, 21);									\
		PLOTPIXEL_FLIPX( 9, 22);									\
		PLOTPIXEL_FLIPX( 8, 23);									\
		PLOTPIXEL_FLIPX( 7, 24);									\
		PLOTPIXEL_FLIPX( 6, 25);									\
		PLOTPIXEL_FLIPX( 5, 26);									\
		PLOTPIXEL_FLIPX( 4, 27);									\
		PLOTPIXEL_FLIPX( 3, 28);									\
		PLOTPIXEL_FLIPX( 2, 29);									\
		PLOTPIXEL_FLIPX( 1, 30);									\
		PLOTPIXEL_FLIPX( 0, 31);									\
	}																\
}																	\

#define PgmRender32x32Tile_Mask()									\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_MASK( 0, 31);										\
		PLOTPIXEL_MASK( 1, 31);										\
		PLOTPIXEL_MASK( 2, 31);										\
		PLOTPIXEL_MASK( 3, 31);										\
		PLOTPIXEL_MASK( 4, 31);										\
		PLOTPIXEL_MASK( 5, 31);										\
		PLOTPIXEL_MASK( 6, 31);										\
		PLOTPIXEL_MASK( 7, 31);										\
		PLOTPIXEL_MASK( 8, 31);										\
		PLOTPIXEL_MASK( 9, 31);										\
		PLOTPIXEL_MASK(10, 31);										\
		PLOTPIXEL_MASK(11, 31);										\
		PLOTPIXEL_MASK(12, 31);										\
		PLOTPIXEL_MASK(13, 31);										\
		PLOTPIXEL_MASK(14, 31);										\
		PLOTPIXEL_MASK(15, 31);										\
		PLOTPIXEL_MASK(16, 31);										\
		PLOTPIXEL_MASK(17, 31);										\
		PLOTPIXEL_MASK(18, 31);										\
		PLOTPIXEL_MASK(19, 31);										\
		PLOTPIXEL_MASK(20, 31);										\
		PLOTPIXEL_MASK(21, 31);										\
		PLOTPIXEL_MASK(22, 31);										\
		PLOTPIXEL_MASK(23, 31);										\
		PLOTPIXEL_MASK(24, 31);										\
		PLOTPIXEL_MASK(25, 31);										\
		PLOTPIXEL_MASK(26, 31);										\
		PLOTPIXEL_MASK(27, 31);										\
		PLOTPIXEL_MASK(28, 31);										\
		PLOTPIXEL_MASK(29, 31);										\
		PLOTPIXEL_MASK(30, 31);										\
		PLOTPIXEL_MASK(31, 31);										\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipX()								\
{																	\
	for (INT8 k = 0; k < 32; k++, p += PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_MASK_FLIPX(31,  0, 31);							\
		PLOTPIXEL_MASK_FLIPX(30,  1, 31);							\
		PLOTPIXEL_MASK_FLIPX(29,  2, 31);							\
		PLOTPIXEL_MASK_FLIPX(28,  3, 31);							\
		PLOTPIXEL_MASK_FLIPX(27,  4, 31);							\
		PLOTPIXEL_MASK_FLIPX(26,  5, 31);							\
		PLOTPIXEL_MASK_FLIPX(25,  6, 31);							\
		PLOTPIXEL_MASK_FLIPX(24,  7, 31);							\
		PLOTPIXEL_MASK_FLIPX(23,  8, 31);							\
		PLOTPIXEL_MASK_FLIPX(22,  9, 31);							\
		PLOTPIXEL_MASK_FLIPX(21, 10, 31);							\
		PLOTPIXEL_MASK_FLIPX(20, 11, 31);							\
		PLOTPIXEL_MASK_FLIPX(19, 12, 31);							\
		PLOTPIXEL_MASK_FLIPX(18, 13, 31);							\
		PLOTPIXEL_MASK_FLIPX(17, 14, 31);							\
		PLOTPIXEL_MASK_FLIPX(16, 15, 31);							\
		PLOTPIXEL_MASK_FLIPX(15, 16, 31);							\
		PLOTPIXEL_MASK_FLIPX(14, 17, 31);							\
		PLOTPIXEL_MASK_FLIPX(13, 18, 31);							\
		PLOTPIXEL_MASK_FLIPX(12, 19, 31);							\
		PLOTPIXEL_MASK_FLIPX(11, 20, 31);							\
		PLOTPIXEL_MASK_FLIPX(10, 21, 31);							\
		PLOTPIXEL_MASK_FLIPX( 9, 22, 31);							\
		PLOTPIXEL_MASK_FLIPX( 8, 23, 31);							\
		PLOTPIXEL_MASK_FLIPX( 7, 24, 31);							\
		PLOTPIXEL_MASK_FLIPX( 6, 25, 31);							\
		PLOTPIXEL_MASK_FLIPX( 5, 26, 31);							\
		PLOTPIXEL_MASK_FLIPX( 4, 27, 31);							\
		PLOTPIXEL_MASK_FLIPX( 3, 28, 31);							\
		PLOTPIXEL_MASK_FLIPX( 2, 29, 31);							\
		PLOTPIXEL_MASK_FLIPX( 1, 30, 31);							\
		PLOTPIXEL_MASK_FLIPX( 0, 31, 31);							\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipY()								\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_MASK( 0, 31);										\
		PLOTPIXEL_MASK( 1, 31);										\
		PLOTPIXEL_MASK( 2, 31);										\
		PLOTPIXEL_MASK( 3, 31);										\
		PLOTPIXEL_MASK( 4, 31);										\
		PLOTPIXEL_MASK( 5, 31);										\
		PLOTPIXEL_MASK( 6, 31);										\
		PLOTPIXEL_MASK( 7, 31);										\
		PLOTPIXEL_MASK( 8, 31);										\
		PLOTPIXEL_MASK( 9, 31);										\
		PLOTPIXEL_MASK(10, 31);										\
		PLOTPIXEL_MASK(11, 31);										\
		PLOTPIXEL_MASK(12, 31);										\
		PLOTPIXEL_MASK(13, 31);										\
		PLOTPIXEL_MASK(14, 31);										\
		PLOTPIXEL_MASK(15, 31);										\
		PLOTPIXEL_MASK(16, 31);										\
		PLOTPIXEL_MASK(17, 31);										\
		PLOTPIXEL_MASK(18, 31);										\
		PLOTPIXEL_MASK(19, 31);										\
		PLOTPIXEL_MASK(20, 31);										\
		PLOTPIXEL_MASK(21, 31);										\
		PLOTPIXEL_MASK(22, 31);										\
		PLOTPIXEL_MASK(23, 31);										\
		PLOTPIXEL_MASK(24, 31);										\
		PLOTPIXEL_MASK(25, 31);										\
		PLOTPIXEL_MASK(26, 31);										\
		PLOTPIXEL_MASK(27, 31);										\
		PLOTPIXEL_MASK(28, 31);										\
		PLOTPIXEL_MASK(29, 31);										\
		PLOTPIXEL_MASK(30, 31);										\
		PLOTPIXEL_MASK(31, 31);										\
	}																\
}																	\

#define PgmRender32x32Tile_Mask_FlipXY()							\
{																	\
	PgmRender32x32FixStartPositionFlipY();							\
																	\
	for (INT8 k = 31; k >= 0; k--, p -= PGM_WIDTH, d += 32) {		\
		PLOTPIXEL_MASK_FLIPX(31,  0, 31);							\
		PLOTPIXEL_MASK_FLIPX(30,  1, 31);							\
		PLOTPIXEL_MASK_FLIPX(29,  2, 31);							\
		PLOTPIXEL_MASK_FLIPX(28,  3, 31);							\
		PLOTPIXEL_MASK_FLIPX(27,  4, 31);							\
		PLOTPIXEL_MASK_FLIPX(26,  5, 31);							\
		PLOTPIXEL_MASK_FLIPX(25,  6, 31);							\
		PLOTPIXEL_MASK_FLIPX(24,  7, 31);							\
		PLOTPIXEL_MASK_FLIPX(23,  8, 31);							\
		PLOTPIXEL_MASK_FLIPX(22,  9, 31);							\
		PLOTPIXEL_MASK_FLIPX(21, 10, 31);							\
		PLOTPIXEL_MASK_FLIPX(20, 11, 31);							\
		PLOTPIXEL_MASK_FLIPX(19, 12, 31);							\
		PLOTPIXEL_MASK_FLIPX(18, 13, 31);							\
		PLOTPIXEL_MASK_FLIPX(17, 14, 31);							\
		PLOTPIXEL_MASK_FLIPX(16, 15, 31);							\
		PLOTPIXEL_MASK_FLIPX(15, 16, 31);							\
		PLOTPIXEL_MASK_FLIPX(14, 17, 31);							\
		PLOTPIXEL_MASK_FLIPX(13, 18, 31);							\
		PLOTPIXEL_MASK_FLIPX(12, 19, 31);							\
		PLOTPIXEL_MASK_FLIPX(11, 20, 31);							\
		PLOTPIXEL_MASK_FLIPX(10, 21, 31);							\
		PLOTPIXEL_MASK_FLIPX( 9, 22, 31);							\
		PLOTPIXEL_MASK_FLIPX( 8, 23, 31);							\
		PLOTPIXEL_MASK_FLIPX( 7, 24, 31);							\
		PLOTPIXEL_MASK_FLIPX( 6, 25, 31);							\
		PLOTPIXEL_MASK_FLIPX( 5, 26, 31);							\
		PLOTPIXEL_MASK_FLIPX( 4, 27, 31);							\
		PLOTPIXEL_MASK_FLIPX( 3, 28, 31);							\
		PLOTPIXEL_MASK_FLIPX( 2, 29, 31);							\
		PLOTPIXEL_MASK_FLIPX( 1, 30, 31);							\
		PLOTPIXEL_MASK_FLIPX( 0, 31, 31);							\
	}																\
}																	\

/* BG Layer */

/*
	Background format:
	32x32 32 color tiles
	The data is arranged in 32 pixel rows.
	Each row of pixels is packed into 20 bytes.

	Text/Background map format:
	---- ---- Ffpp ppp- nnnn nnnn nnnn nnnn

	n = tile number
	p = palette
	F = Y flip
	f = X flip

	Palette format:
	xRRRRRGGGGGBBBBB

	Palettes:
	0xA00800 0xA00FFF	Background	(32 * 2 bytes x 32 palettes)

	Scroll registers:
	0xB02000	Background scroll Y register (WORD)
	0xB03000	Background scroll X register (WORD)
*/

static void draw_bg()
{
	// no line scroll (fast)
	
	UINT16 *vram = (UINT16 *)PgmBgRam;
	UINT32 *pal = &PgmPalette[0x400];
	
	INT16 xscroll = (INT16)PgmVidReg[0x3000 / 2];
	INT16 yscroll = (INT16)PgmVidReg[0x2000 / 2];
	
	xscroll &= 0x7FF;
	yscroll &= 0x1FF;
	
	for (INT16 offs = 0; offs < 64 * 16; offs++) {
		INT16 sx = (offs & 0x3F) << 5;
		INT16 sy = (offs >> 6) << 5;
		
		sx -= xscroll;
		if (sx < -31) sx += 2048;
		sy -= yscroll;
		if (sy < -31) sy += 512;
		
		if ((sx >= (448 + 31)) || (sy >= (224 + 31))) continue;
		
		UINT16 code = vram[offs << 1];
		if (code >= nPgmTileMask) continue;
		if (tiletrans[code] == 0) continue; // transparent
		
		UINT16 attr = vram[(offs << 1) + 1];
		UINT16 color = (attr & 0x3e) << 4;
		UINT8  flip  = (attr & 0xC0);
		
#ifndef BUILD_PSP
		UINT16 *p = (UINT16 *)pBurnDraw + sy * PGM_WIDTH + sx;
#else
		UINT16 *p = (UINT16 *)pBurnDraw + (sy << 9) + sx;
#endif
		UINT8  *d = getBlockTileExp(code << 10, 1024);
		if (d == NULL) return;
		
		if ((sx >= 31) && (sy >= 31) && (sx < (448 - 31)) && (sy < (224 - 31))) {
			if (tiletrans[code] & 2) { // opaque
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender32x32Tile();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender32x32Tile_FlipX();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender32x32Tile_FlipY();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender32x32Tile_FlipXY();
					break;
				}
			} else {
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender32x32Tile_Mask();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender32x32Tile_Mask_FlipX();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender32x32Tile_Mask_FlipY();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender32x32Tile_Mask_FlipXY();
					break;
				}
			}
		} else{
			if (tiletrans[code] & 2) { // opaque
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender32x32Tile_Clip();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender32x32Tile_FlipX_Clip();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender32x32Tile_FlipY_Clip();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender32x32Tile_FlipXY_Clip();
					break;
				}
			} else {
				switch (flip) {
				case RENDER_TILE_NORMAL:
					PgmRender32x32Tile_Mask_Clip();
					break;
				case RENDER_TILE_FLIPX:
					PgmRender32x32Tile_Mask_FlipX_Clip();
					break;
				case RENDER_TILE_FLIPY:
					PgmRender32x32Tile_Mask_FlipY_Clip();
					break;
				case RENDER_TILE_FLIPXY:
					PgmRender32x32Tile_Mask_FlipXY_Clip();
					break;
				}
			}
		}
	}
}

#define PgmRender32x32Tile_RowScroll()								\
{																	\
	PLOTPIXEL_FLIPX(sx +  0,  0 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  1,  1 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  2,  2 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  3,  3 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  4,  4 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  5,  5 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  6,  6 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  7,  7 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  8,  8 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx +  9,  9 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 10, 10 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 11, 11 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 12, 12 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 13, 13 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 14, 14 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 15, 15 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 16, 16 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 17, 17 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 18, 18 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 19, 19 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 20, 20 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 21, 21 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 22, 22 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 23, 23 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 24, 24 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 25, 25 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 26, 26 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 27, 27 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 28, 28 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 29, 29 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 30, 30 ^ flipx);							\
	PLOTPIXEL_FLIPX(sx + 31, 31 ^ flipx);							\
}																	\

#define PgmRender32x32Tile_Mask_RowScroll()							\
{																	\
	PLOTPIXEL_MASK_FLIPX(sx +  0,  0 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  1,  1 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  2,  2 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  3,  3 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  4,  4 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  5,  5 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  6,  6 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  7,  7 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  8,  8 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx +  9,  9 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 10, 10 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 11, 11 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 12, 12 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 13, 13 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 14, 14 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 15, 15 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 16, 16 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 17, 17 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 18, 18 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 19, 19 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 20, 20 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 21, 21 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 22, 22 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 23, 23 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 24, 24 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 25, 25 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 26, 26 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 27, 27 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 28, 28 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 29, 29 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 30, 30 ^ flipx, 31);					\
	PLOTPIXEL_MASK_FLIPX(sx + 31, 31 ^ flipx, 31);					\
}																	\

static void draw_bg_rowscroll()
{
	// do line scroll (slow)
	
	UINT16 *vram = (UINT16 *)PgmBgRam;
	UINT16 *rowscroll = PgmRsRam;
	UINT32 *pal = &PgmPalette[0x400];
	
	INT16 xscroll = (INT16)PgmVidReg[0x3000 / 2];
	INT16 yscroll = (INT16)PgmVidReg[0x2000 / 2];
	
	UINT16 *p = (UINT16 *)pBurnDraw;
	
	for (INT16 y = 0; y < 224; y++, p += PGM_WIDTH) {
		INT16 scrollx = (xscroll + rowscroll[y]) & 0x7ff;
		INT16 scrolly = (yscroll + y) & 0x7ff;
		
		for (INT16 x = 0; x < 480; x += 32) {
			INT16 sx = x - (scrollx & 0x1f);
			
			if (sx >= (448 + 31)) break;
			
			UINT16 offs = ((scrolly & 0x1e0) << 2) | (((scrollx + x) & 0x7e0) >> 4);
			UINT16 code = vram[offs];
			
			if (code >= nPgmTileMask) continue;
			if (tiletrans[code] == 0) continue;
			
			UINT16 attr   = vram[offs + 1];
			UINT16 color = (attr & 0x3e) << 4;
			UINT8  flipx  = (attr & 0x40) ? 0x1f : 0x00;
			UINT8  flipy  = (attr & 0x80) ? 0x1f : 0x00;
			
			UINT8 *d = getBlockTileExp((code << 10) + (((scrolly ^ flipy) & 0x1f) << 5), 1024);
			if (d == NULL) return;
			
			if ((sx >= 0) && (sx < (448 - 31))) {
				if (tiletrans[code] & 2) {
					PgmRender32x32Tile_RowScroll();
				} else {
					PgmRender32x32Tile_Mask_RowScroll();
				}
			} else {
				for (INT16 xx = 0; xx < 32; xx++, sx++) {
					if (sx < 0) continue;
					if (sx >= 448) break;
					PLOTPIXEL_MASK_FLIPX(sx, xx ^ flipx, 31);
				}
			}
		}
	}
}

static void draw_background()
{
	// check to see if we need to do line scroll
	UINT8 t = 0;
	{
		UINT16 *rs = PgmRsRam;
		for (INT16 i = 1; i < 224; i++) {
			if (rs[0] != rs[i]) {
				t = 1;
				break;
			}
		}
	}
	
	if (t == 0) {
		draw_bg();
	} else {
		draw_bg_rowscroll();
	}
}


int pgmDraw()
{
	if (nPgmPalRecalc) {
		for (INT16 i = 0; i < (0x1200 / 2); i++) {
			PgmPalette[i] = PgmCalcCol(PgmPalRam[i]);
		}
		nPgmPalRecalc = 0;
	}
	
#ifndef BUILD_PSP
	memset(pBurnDraw, 0, PGM_WIDTH * 224 * 2);
#else
	extern void clear_gui_texture(int color, UINT16 w, UINT16 h);
	clear_gui_texture(0, 448, 224);
#endif
	
	pgm_drawsprites(1);
	draw_background();
	pgm_drawsprites(0);
	draw_text();
	
	return 0;
}

void pgmInitDraw() // preprocess some things...
{
	// Find transparent tiles so we can skip them
	{
		nPgmTileMask = ((nPGMTileROMLen / 5) * 8) / 0x400; // also used to set max. tile
		
		// background tiles
		tiletrans = (UINT8 *)memalign(4, nPgmTileMask);
		memset (tiletrans, 0, nPgmTileMask);
		
		UINT8 *d;
		
		for (INT32 i = 0; i < (nPgmTileMask << 10); i += 0x400) {
			UINT8 k = 0x1f;
			
			d = getBlockTileExp(i, 0x400);
			if (d == NULL) break;
			
			for (INT32 j = 0; j < 0x400; j++) {
				if (d[j] != 0x1f) {
					tiletrans[i / 0x400] = 1;
				}
				k &= (d[j] ^ 0x1f);
			}
			if (k) tiletrans[i / 0x400] |= 2;
		}
		
		// character tiles
		texttrans = (UINT8 *)memalign(4, 0x10000);
		memset (texttrans, 0, 0x10000);
		
		for (INT32 i = 0; i < 0x400000; i += 0x40) {
			UINT8 k = 0xf;
			
			d = getBlockTile(i, 0x400);
			if (d == NULL) break;
			
			for (INT32 j = 0; j < 0x40; j++) {
				if (d[j] != 0xf) {
					texttrans[i / 0x40] = 1;
				}
				k &= (d[j] ^ 0xf);
			}
			if (k) texttrans[i / 0x40] |= 2;
		}
	}
	
	// set up table to count bits in sprite mask data
	// gives a good speedup in sprite drawing. ^^
	{
		sprmsktab = (UINT8 *)memalign(4, 0x10000);
		memset (sprmsktab, 0, 0x10000);
		
		for (INT32 i = 0; i < 0x10000; i++) {
			for (INT32 j = 0; j < 16; j++) {
				if (!(i & (1 << j))) {
					sprmsktab[i]++;
				}
			}
		}
	}
}

void pgmExitDraw()
{
	nPgmTileMask = 0;
	free (tiletrans);
	free (texttrans);
	free (sprmsktab);
}

