#include "psp.h"

#ifdef ENABLE_PRX
	#include "volume_icon.c"
#endif

void *show_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 0);
void *draw_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 1);
void *work_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 2);
void *tex_frame  = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 3);


#define SLICE (64)

typedef struct {
	UINT16 u, v;
	UINT16 x, y, z;
} Vertex;

static UINT8 ALIGN_PSPDATA list[512 * 1024];

#ifdef ENABLE_PRX
	static UINT16 *tex_volicon;
	void load_volume_icon();
	void draw_volume(int volume);
#endif

static int nPrevStage;

static UINT8 screen_mode;
static UINT16 game_screen_vertex_number;

static UINT16 VideoBufferWidth, VideoBufferHeight;
static UINT16 scr_width, scr_height;
static UINT16 scr_left, scr_right, scr_top, scr_bottom;


static int myProgressRangeCallback(float dProgressRange)
{
	return 0;
}

static int myProgressUpdateCallback(float dProgress, const char* pszText, bool bAbs)
{
	bprintf(PRINT_IMPORTANT, "%s %f %d", pszText, dProgress, bAbs);
	return 0;
}

int AppDebugPrintf(int nStatus, char* pszFormat, ...)
{
	va_list vaFormat;
	va_start(vaFormat, pszFormat);
	char buf[256];
	
	sprintf(buf, pszFormat, vaFormat);
	
	UINT16 fc;
	switch (nStatus) {
	case PRINT_UI:			fc = R8G8B8_to_B5G6R5(0x404040); break;
	case PRINT_IMPORTANT:	fc = R8G8B8_to_B5G6R5(0x600000); break;
	case PRINT_ERROR:		fc = R8G8B8_to_B5G6R5(0x606000); break;
	default:				fc = R8G8B8_to_B5G6R5(0x404040); break;
	}
	
	drawRect(GU_FRAME_ADDR(work_frame), 0, 0, 480, 20, fc);
	drawString(buf, GU_FRAME_ADDR(work_frame), 4, 4, R8G8B8_to_B5G6R5(0xffffff));

	drawRect(GU_FRAME_ADDR(tex_frame), 0, 0, 480, 20, fc);
	drawString(buf, GU_FRAME_ADDR(tex_frame), 4, 4, R8G8B8_to_B5G6R5(0xffffff));
	
	va_end(vaFormat);
	
	update_gui();
	
	return 0;
}


void init_gui()
{
	sceGuDisplay(GU_FALSE);
	
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	
	sceGuDrawBuffer(GU_PSM_5650, draw_frame, PSP_LINE_SIZE);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, show_frame, PSP_LINE_SIZE);
	
	sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	
//	sceGuFrontFace(GU_CW);
	
//	sceGuAlphaFunc(GU_LEQUAL, 0, 0x01);
	sceGuDisable(GU_ALPHA_TEST);
	
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuDisable(GU_BLEND);
	
//	sceGuDepthRange(65535, 0);	//sceGuDepthRange(0xc350, 0x2710);
//	sceGuDepthFunc(GU_GEQUAL);
//	sceGuDepthMask(GU_TRUE);
	sceGuDisable(GU_DEPTH_TEST);
	
	sceGuTexMode(GU_PSM_5650, 0, 0, GU_FALSE);
//	sceGuTexScale(1.0 / PSP_LINE_SIZE, 1.0 / PSP_LINE_SIZE);
//	sceGuTexOffset(0.0, 0.0);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	
	sceGuTexImage(0, 512, 512, 512, GUC_FRAME_ADDR(work_frame));
	sceGuTexFlush();
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuEnable(GU_TEXTURE_2D);
	
//	sceGuClutMode(GU_PSM_5650, 0, 0xff, 0);
	
	sceGuDisable(GU_DITHER);
	
//	sceGuClearDepth(0);
	sceGuClearColor(0);
	
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
	
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
	
	nPrevStage = nGameStage;
	//bprintf = AppDebugPrintf;
	
	BurnExtProgressRangeCallback = myProgressRangeCallback;
	BurnExtProgressUpdateCallback = myProgressUpdateCallback;
	
#ifdef ENABLE_PRX
	load_volume_icon();
#endif
}

void exit_gui()
{
	sceGuDisplay(GU_FALSE);
	sceGuTerm();
}

void update_gui()
{
	UINT16 *texture_ptr;
	Vertex *vertices, *vertices_tmp;
	UINT16 vertex_num;
	
	if ( nGameStage ) {
		texture_ptr = GUC_FRAME_ADDR(work_frame);
	} else {
		texture_ptr = GUC_FRAME_ADDR(tex_frame);
	}
	
	sceKernelDcacheWritebackAll();
	
	sceGuStart(GU_DIRECT, list);
	
	sceGuDrawBufferList(GU_PSM_5650, draw_frame, PSP_LINE_SIZE);
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	sceGuClearColor(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_FAST_CLEAR_BIT);
	
	sceGuTexMode(GU_PSM_5650, 0, 0, GU_FALSE);
	sceGuTexImage(0, 512, 512, PSP_LINE_SIZE, texture_ptr);
	
	if ( nGameStage ) {
		vertex_num = 2;
		vertices = (Vertex *)sceGuGetMemory(vertex_num * sizeof(Vertex));
		memset(vertices, 0, vertex_num * sizeof(Vertex));
		
		vertices[0].u = 0;
		vertices[0].v = 0;
		vertices[0].x = 0;
		vertices[0].y = 0;
		
		vertices[1].u = 480;
		vertices[1].v = 272;
		vertices[1].x = SCREEN_WIDTH;
		vertices[1].y = SCREEN_HEIGHT;
	} else {
		UINT16 i;
		vertex_num = game_screen_vertex_number;
		vertices = (Vertex *)sceGuGetMemory(vertex_num * sizeof(Vertex));
		memset(vertices, 0, vertex_num * sizeof(Vertex));
		vertices_tmp = vertices;
		
		switch( screen_mode ) {
			case 0:
			{
				// normal
				float delta = (float)scr_width / VideoBufferWidth;
				for (i = 0; (i + SLICE) < VideoBufferWidth; i += SLICE) {
					if (i < 512) {
						vertices_tmp[0].u = i;
						vertices_tmp[0].v = 0;
						vertices_tmp[0].x = scr_left + (i * delta);
						vertices_tmp[0].y = scr_top;
						
						vertices_tmp[1].u = i + SLICE;
						vertices_tmp[1].v = VideoBufferHeight;
						vertices_tmp[1].x = scr_left + ((i + SLICE) * delta);
						vertices_tmp[1].y = scr_bottom;
					} else {
						// Darius, Darius2 & Ninja Warriors
						vertices_tmp[0].u = i - 512;
						vertices_tmp[0].v = VideoBufferHeight;
						vertices_tmp[0].x = scr_left + (i * delta);
						vertices_tmp[0].y = scr_top;
						
						vertices_tmp[1].u = i - 512 + SLICE;
						vertices_tmp[1].v = VideoBufferHeight << 1;
						vertices_tmp[1].x = scr_left + ((i + SLICE) * delta);
						vertices_tmp[1].y = scr_bottom;
					}
					vertices_tmp += 2;
				}
				if (i < VideoBufferWidth) {
					if (i < 512) {
						vertices_tmp[0].u = i;
						vertices_tmp[0].v = 0;
						vertices_tmp[0].x = scr_left + (i * delta);
						vertices_tmp[0].y = scr_top;
						
						vertices_tmp[1].u = VideoBufferWidth;
						vertices_tmp[1].v = VideoBufferHeight;
						vertices_tmp[1].x = scr_right;
						vertices_tmp[1].y = scr_bottom;
					} else {
						// Darius, Darius2 & Ninja Warriors
						vertices_tmp[0].u = i - 512;
						vertices_tmp[0].v = VideoBufferHeight;
						vertices_tmp[0].x = scr_left + (i * delta);
						vertices_tmp[0].y = scr_top;
						
						vertices_tmp[1].u = VideoBufferWidth - 512;
						vertices_tmp[1].v = VideoBufferHeight << 1;
						vertices_tmp[1].x = scr_right;
						vertices_tmp[1].y = scr_bottom;
					}
				}
			}
			break;
			
			case 1:
			{
				// flip
				float delta = (float)scr_width / VideoBufferWidth;
				for (i = 0; (i + SLICE) < VideoBufferWidth; i += SLICE) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_right - (i * delta);
					vertices_tmp[0].y = scr_bottom;
					
					vertices_tmp[1].u = i + SLICE;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_right - ((i + SLICE) * delta);
					vertices_tmp[1].y = scr_top;
					
					vertices_tmp += 2;
				}
				if (i < VideoBufferWidth) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_right - (i * delta);
					vertices_tmp[0].y = scr_bottom;
					
					vertices_tmp[1].u = VideoBufferWidth;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_left;
					vertices_tmp[1].y = scr_top;
				}
			}
			break;
			
			case 2:
			{
				// rotate
				float delta = (float)scr_height / VideoBufferWidth;
				for (i = 0; (i + SLICE) < VideoBufferWidth; i += SLICE) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_left;
					vertices_tmp[0].y = scr_bottom - (i * delta);
					
					vertices_tmp[1].u = i + SLICE;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_right;
					vertices_tmp[1].y = scr_bottom - ((i + SLICE) * delta);
					
					vertices_tmp += 2;
				}
				if (i < VideoBufferWidth) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_left;
					vertices_tmp[0].y = scr_bottom - (i * delta);
					
					vertices_tmp[1].u = VideoBufferWidth;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_right;
					vertices_tmp[1].y = scr_top;
				}
			}
			break;
			
			case 3:
			{
				// flip & rotate
				float delta = (float)scr_height / VideoBufferWidth;
				for (i = 0; (i + SLICE) < VideoBufferWidth; i += SLICE) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_right;
					vertices_tmp[0].y = scr_top + (i * delta);
					
					vertices_tmp[1].u = i + SLICE;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_left;
					vertices_tmp[1].y = scr_top + ((i + SLICE) * delta);
					
					vertices_tmp += 2;
				}
				if (i < VideoBufferWidth) {
					vertices_tmp[0].u = i;
					vertices_tmp[0].v = 0;
					vertices_tmp[0].x = scr_right;
					vertices_tmp[0].y = scr_top + (i * delta);
					
					vertices_tmp[1].u = VideoBufferWidth;
					vertices_tmp[1].v = VideoBufferHeight;
					vertices_tmp[1].x = scr_left;
					vertices_tmp[1].y = scr_bottom;
				}
			}
			break;
		}
	}
	sceGuDrawArray(	GU_SPRITES,
					GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
					vertex_num, 0, vertices);
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


void clear_gui_texture(int color, UINT16 w, UINT16 h)
{
	sceGuStart(GU_DIRECT, list);
	
 	sceGuDrawBufferList(GU_PSM_5650, tex_frame, PSP_LINE_SIZE);
	sceGuScissor(0, 0, w, h);
	
	sceGuClearColor(color);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_FAST_CLEAR_BIT);
	
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}


void get_screen_size()
{
	UINT8 aspect_x, aspect_y;
	
	BurnDrvGetFullSize(&VideoBufferWidth, &VideoBufferHeight);
	game_screen_vertex_number = 2 * ((UINT16)(VideoBufferWidth / SLICE) + ((VideoBufferWidth % SLICE) ? 1 : 0));
	
	if (BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL & ~screen_rotate) {
		BurnDrvGetAspect(&aspect_y, &aspect_x);
	} else {
		BurnDrvGetAspect(&aspect_x, &aspect_y);
	}
	
	if (((float)aspect_x / aspect_y) < (16.0 / 9.0)) {
		scr_width  = SCREEN_HEIGHT * aspect_x / aspect_y;
		scr_height = SCREEN_HEIGHT;
	} else {
		scr_width  = SCREEN_WIDTH;
		scr_height = SCREEN_WIDTH * aspect_y / aspect_x;
	}
	
	scr_top    = (SCREEN_HEIGHT - scr_height) / 2;
	scr_left   = (SCREEN_WIDTH  - scr_width ) / 2;
	scr_right  = scr_left + scr_width;
	scr_bottom = scr_top  + scr_height;
}

void screen_initialise()
{
	clear_gui_texture(0, 512, 512);
	screen_mode = (BurnDrvGetFlags() & ((BDF_ORIENTATION_VERTICAL & screen_rotate) | BDF_ORIENTATION_FLIPPED)) >> 1;
	get_screen_size();
}


/******************************************************************************/
/* copy from njemu */
/******************************************************************************/

/******************************************************************************
	メインボリューム表示
******************************************************************************/

#ifdef ENABLE_PRX

static UINT16 *texture16_addr(int x, int y)
{
	return (UINT16 *)(0x4000000 + ((x + (y << 9)) << 1));
}

void load_volume_icon()
{
	int x, y, alpha;
	UINT16 *dst;

	if (devkit_version >= 0x03050210) {
		tex_volicon = texture16_addr(PSP_LINE_SIZE - 112, 2000);

		dst = tex_volicon + SPEEKER_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32; x++) {
				if (x & 1)
					alpha = icon_speeker[y][(x >> 1)] >> 4;
				else
					alpha = icon_speeker[y][(x >> 1)] & 0x0f;

				dst[x] = (alpha << 12) | 0x0fff;
			}

			dst += PSP_LINE_SIZE;
		}

		dst = tex_volicon + SPEEKER_SHADOW_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32; x++) {
				if (x & 1)
					alpha = icon_speeker_shadow[y][(x >> 1)] >> 4;
				else
					alpha = icon_speeker_shadow[y][(x >> 1)] & 0x0f;

				dst[x] = alpha << 12;
			}

			dst += PSP_LINE_SIZE;
		}

		dst = tex_volicon + VOLUME_BAR_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 12; x++) {
				if (x & 1)
					alpha = icon_bar[y][(x >> 1)] >> 4;
				else
					alpha = icon_bar[y][(x >> 1)] & 0x0f;

				dst[x] = (alpha << 12) | 0x0fff;
			}

			dst += PSP_LINE_SIZE;
		}

		dst = tex_volicon + VOLUME_BAR_SHADOW_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 12; x++) {
				if (x & 1)
					alpha = icon_bar_shadow[y][(x >> 1)] >> 4;
				else
					alpha = icon_bar_shadow[y][(x >> 1)] & 0x0f;

				dst[x] = alpha << 12;
			}

			dst += PSP_LINE_SIZE;
		}

		dst = tex_volicon + VOLUME_DOT_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 12; x++) {
				if (x & 1)
					alpha = icon_dot[y][(x >> 1)] >> 4;
				else
					alpha = icon_dot[y][(x >> 1)] & 0x0f;

				dst[x] = (alpha << 12) | 0x0fff;
			}

			dst += PSP_LINE_SIZE;
		}

		dst = tex_volicon + VOLUME_DOT_SHADOW_X;
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 12; x++) {
				if (x & 1)
					alpha = icon_dot_shadow[y][(x >> 1)] >> 4;
				else
					alpha = icon_dot_shadow[y][(x >> 1)] & 0x0f;

				dst[x] = alpha << 12;
			}

			dst += PSP_LINE_SIZE;
		}
	}
}

/*------------------------------------------------------
	ボリュームを描画 (CFW 3.52以降のユーザーモードのみ)
------------------------------------------------------*/

void draw_volume(int volume)
{
	Vertex *vertices, *vertices_tmp;

	sceGuStart(GU_DIRECT, list);
	
	sceGuDrawBufferList(GU_PSM_5650, draw_frame, PSP_LINE_SIZE);
	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	
	sceGuEnable(GU_BLEND);
	
	sceGuTexMode(GU_PSM_4444, 0, 0, GU_FALSE);
	sceGuTexImage(0, 512, 512, PSP_LINE_SIZE, tex_volicon);

	vertices = (Vertex *)sceGuGetMemory(2 * 31 * 2 * sizeof(Vertex));

	if (vertices) {
		int i, x;

		memset(vertices, 0, 2 * 31 * 2 * sizeof(Vertex));
		vertices_tmp = vertices;

		x = 24;

		vertices_tmp[0].u = SPEEKER_SHADOW_X;
		vertices_tmp[0].v = 0;
		vertices_tmp[0].x = 3 + x;
		vertices_tmp[0].y = 3 + 230;

		vertices_tmp[1].u = SPEEKER_SHADOW_X + 32;
		vertices_tmp[1].v = 32;
		vertices_tmp[1].x = 3 + x + 32;
		vertices_tmp[1].y = 3 + 230 + 32;

		vertices_tmp += 2;

		vertices_tmp[0].u = SPEEKER_X;
		vertices_tmp[0].v = 0;
		vertices_tmp[0].x = x;
		vertices_tmp[0].y = 230;

		vertices_tmp[1].u = SPEEKER_X + 32;
		vertices_tmp[1].v = 32;
		vertices_tmp[1].x = x + 32;
		vertices_tmp[1].y = 230 + 32;

		vertices_tmp += 2;

		x = 64;

		for (i = 0; i < volume; i++) {
			vertices_tmp[0].u = VOLUME_BAR_SHADOW_X;
			vertices_tmp[0].v = 0;
			vertices_tmp[0].x = 3 + x;
			vertices_tmp[0].y = 3 + 230;

			vertices_tmp[1].u = VOLUME_BAR_SHADOW_X + 12;
			vertices_tmp[1].v = 32;
			vertices_tmp[1].x = 3 + x + 12;
			vertices_tmp[1].y = 3 + 230 + 32;

			vertices_tmp += 2;

			vertices_tmp[0].u = VOLUME_BAR_X;
			vertices_tmp[0].v = 0;
			vertices_tmp[0].x = x;
			vertices_tmp[0].y = 230;

			vertices_tmp[1].u = VOLUME_BAR_X + 12;
			vertices_tmp[1].v = 32;
			vertices_tmp[1].x = x + 12;
			vertices_tmp[1].y = 230 + 32;

			vertices_tmp += 2;

			x += 12;
		}

		for (; i < 30; i++) {
			vertices_tmp[0].u = VOLUME_DOT_SHADOW_X;
			vertices_tmp[0].v = 0;
			vertices_tmp[0].x = 3 + x;
			vertices_tmp[0].y = 3 + 230;

			vertices_tmp[1].u = VOLUME_DOT_SHADOW_X + 12;
			vertices_tmp[1].v = 32;
			vertices_tmp[1].x = 3 + x + 12;
			vertices_tmp[1].y = 3 + 230 + 32;

			vertices_tmp += 2;

			vertices_tmp[0].u = VOLUME_DOT_X;
			vertices_tmp[0].v = 0;
			vertices_tmp[0].x = x;
			vertices_tmp[0].y = 230;

			vertices_tmp[1].u = VOLUME_DOT_X + 12;
			vertices_tmp[1].v = 32;
			vertices_tmp[1].x = x + 12;
			vertices_tmp[1].y = 230 + 32;

			vertices_tmp += 2;

			x += 12;
		}

		sceGuDrawArray(	GU_SPRITES,
						GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
						2 * 31 * 2, NULL, vertices);
	}

	sceGuDisable(GU_BLEND);
	
	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
}

/*------------------------------------------------------
	メインボリューム表示
------------------------------------------------------*/

int draw_volume_status(UINT8 draw)
{
	if (devkit_version >= 0x03050210) {
		static u64 disp_end = 0;
		u64 disp_current;
		SceCtrlData pad;
		
		int volume = kuImposeGetParam(PSP_IMPOSE_MAIN_VOLUME);
		
		if (volume < 0 || volume > 30) return 0;
		
		kuCtrlPeekBufferPositive(&pad, 1);
		
		if (pad.Buttons & (PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN | PSP_CTRL_NOTE)) {
			sceRtcGetCurrentTick(&disp_end);
			disp_end += 2000000;
			draw = 1;
		}

		if (disp_end != 0) {
			sceRtcGetCurrentTick(&disp_current);
			
			if (disp_current < disp_end) {
				if (draw) draw_volume(volume);
			} else {
				disp_end = 0;
			}
		}
	}
	
	return 0;
}

#endif // ENABLE_PRX

