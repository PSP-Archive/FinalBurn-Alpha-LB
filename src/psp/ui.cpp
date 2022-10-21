#include "psp.h"
#include "burner.h"

#define find_rom_list_cnt (10)

static int find_rom_count = 0;
static int find_rom_select = 0;
static int find_rom_top = 0;
char ui_current_path[MAX_PATH];

static UINT32 nPrevGame = ~0U;

static int ui_mainmenu_select = 0;
static int ui_process_pos = 0;

int DrvInitCallback()
{
	return DrvInit(nBurnDrvSelect, false);
}

static UINT16 set_ui_main_menu_item_color(int i)
{
	if (ui_mainmenu_select == i) return UI_BGCOLOR;	
	return UI_COLOR;
}

static struct {
	int cpu, bus; 
} cpu_speeds[] = { { 222, 111 }, { 266, 133 }, { 300, 150 }, { 333, 166 } };

static int cpu_speeds_select = 3;

UINT8 state_slot = 0;

UINT8 game_reset = 0;

#define ROTATE_FLAG  (BDF_ORIENTATION_VERTICAL)
int screen_rotate = ROTATE_FLAG;

const char *frameskip_str[] = { " AUTO ", "MANUAL", " OFF  " };

static const char *ui_main_menu[] = {
	"Select ROM",
	"Load State : %1u",
	"Save State : %1u",
	"Reset Game",
	"Frame Skip Type : %6s",
	"Frame Skip Value : %2u",
	"Rotate vertically aligned games : %3s",
	"CPU Speed %3uMHz",
	"Return to Game",
	"Exit FinaBurn Alpha",
};

#define DRAW_UI_STRING(text)                                                  \
{                                                                             \
  drawString( text, GUC_FRAME_ADDR(work_frame),                               \
              240 - getDrawStringLength(text) / 2, 44 + i * 18, fc);          \
}                                                                             \

void draw_ui_main()
{
	char buf[320];
	
	drawRect(GUC_FRAME_ADDR(work_frame), 0, 0, 480, 272, UI_BGCOLOR);
	drawString("FinalBurn Alpha for PSP", GUC_FRAME_ADDR(work_frame), 10, 10, UI_COLOR);

	drawRect(GUC_FRAME_ADDR(work_frame), 8, 28, 464, 1, UI_COLOR);
	drawRect(GUC_FRAME_ADDR(work_frame), 10, 40 + ui_mainmenu_select * 18, 460, 18, UI_COLOR);
	
	for(int i = 0; i < 10; i++)  {
		UINT16 fc = set_ui_main_menu_item_color(i);
		
		switch ( i ) {
		case 1:
			sprintf( buf, ui_main_menu[i], state_slot );
			DRAW_UI_STRING(buf);
			break;
		case 2:
			sprintf( buf, ui_main_menu[i], state_slot );
			DRAW_UI_STRING(buf);
			break;
		case 4:
			sprintf( buf, ui_main_menu[i], frameskip_str[option_frameskip_type] );
			DRAW_UI_STRING(buf);
			break;
		case 5:
			sprintf( buf, ui_main_menu[i], option_frameskip_value );
			DRAW_UI_STRING(buf);
			break;
		case 6:
			if (screen_rotate) {
				sprintf( buf, ui_main_menu[i], "ON " );
			} else {
				sprintf( buf, ui_main_menu[i], "OFF" );
			}
			DRAW_UI_STRING(buf);
			break;
		case 7:
			sprintf( buf, ui_main_menu[i], cpu_speeds[cpu_speeds_select].cpu );
			DRAW_UI_STRING(buf);
			break;
		default:
			DRAW_UI_STRING(ui_main_menu[i]);
		}
	}
	
	drawRect(GUC_FRAME_ADDR(work_frame), 8, 230, 464, 1, UI_COLOR);
	drawString("FB Alpha contains parts of MAME & Final Burn. (C) 2004, Team FB Alpha.", GUC_FRAME_ADDR(work_frame), 10, 238, UI_COLOR);
	drawString("FinalBurn Alpha for PSP (C) 2008, OopsWare.", GUC_FRAME_ADDR(work_frame), 10, 255, UI_COLOR);
}

void draw_ui_browse(bool rebuiltlist)
{
	UINT32 bds = nBurnDrvSelect;
	char buf[1024];
	drawRect(GUC_FRAME_ADDR(work_frame), 0, 0, 480, 272, UI_BGCOLOR);

	find_rom_count = findRomsInDir( rebuiltlist );

	strcpy(buf, "PATH: ");
	strcat(buf, ui_current_path);
	
	drawString(buf, GUC_FRAME_ADDR(work_frame), 10, 10, UI_COLOR, 460);
    drawRect(GUC_FRAME_ADDR(work_frame), 8, 28, 464, 1, UI_COLOR);
	
	for(int i = 0; i < find_rom_list_cnt; i++) {
		char *p = getRomsFileName(i+find_rom_top);
		UINT16 fc, bc;
		
		if ((i + find_rom_top) == find_rom_select) {
			bc = UI_COLOR;
			fc = UI_BGCOLOR;
		} else {
			bc = UI_BGCOLOR;
			fc = UI_COLOR;
		}
		
		drawRect(GUC_FRAME_ADDR(work_frame), 10, 40+i*18, 230, 18, bc);
		if (p) {
			switch( getRomsFileStat(i + find_rom_top) ) {
			case -2: // unsupport
			case -3: // not working
				drawString(p, GUC_FRAME_ADDR(work_frame), 12, 44 + i * 18, R8G8B8_to_B5G6R5(0x808080), 180);
				break;
			case -1: // directry
				drawString("<DIR>", GUC_FRAME_ADDR(work_frame), 194, 44 + i * 18, fc);
			default:
				drawString(p, GUC_FRAME_ADDR(work_frame), 12, 44 + i * 18, fc, 180);
			}
		}
		
		if (find_rom_count > find_rom_list_cnt) {
			drawRect(GUC_FRAME_ADDR(work_frame), 242, 40, 5, 18 * find_rom_list_cnt, R8G8B8_to_B5G6R5(0x807060));
		
			drawRect(GUC_FRAME_ADDR(work_frame), 242, 
					40 + find_rom_top * 18 * find_rom_list_cnt / find_rom_count , 5, 
					find_rom_list_cnt * 18 * find_rom_list_cnt / find_rom_count, UI_COLOR);
		} else
			drawRect(GUC_FRAME_ADDR(work_frame), 242, 40, 5, 18 * find_rom_list_cnt, UI_COLOR);

	}
	
    drawRect(GUC_FRAME_ADDR(work_frame), 8, 230, 464, 1, UI_COLOR);

	nBurnDrvSelect = getRomsFileStat(find_rom_select);

	strcpy(buf, "Game Info: ");
	if (nBurnDrvSelect < nBurnDrvCount)
		strcat(buf, BurnDrvGetTextA( DRV_FULLNAME ) );
    drawString(buf, GUC_FRAME_ADDR(work_frame), 10, 238, UI_COLOR, 460);

	strcpy(buf, "Released by: ");
	if (nBurnDrvSelect < nBurnDrvCount) {
		strcat(buf, BurnDrvGetTextA( DRV_MANUFACTURER ));
		strcat(buf, " (");
		strcat(buf, BurnDrvGetTextA( DRV_DATE ));
		strcat(buf, ", ");
		strcat(buf, BurnDrvGetTextA( DRV_SYSTEM ));
		strcat(buf, " hardware)");
	}
    drawString(buf, GUC_FRAME_ADDR(work_frame), 10, 255, UI_COLOR, 460);
   
    nBurnDrvSelect = bds;
}

static void return_to_game()
{
	if (nPrevGame < nBurnDrvCount) {
		screen_initialise();
		SetClockFrequency(cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].bus );
		nGameStage = 0;
	}
}

static void process_key( int key, int down, int repeat )
{
	if (!down) return ;
	switch( nGameStage ) {
	/* ---------------------------- Main Menu ---------------------------- */
	case 1:		
		//ui_mainmenu_select
		switch( key ) {
		case PSP_CTRL_UP:
			if (ui_mainmenu_select <= 0)
				ui_mainmenu_select = 9;
			else
				ui_mainmenu_select--;
			draw_ui_main();
			break;
		case PSP_CTRL_DOWN:
			if (ui_mainmenu_select >=9)
				ui_mainmenu_select = 0;
			else
				ui_mainmenu_select++;
			draw_ui_main();
			break;

		case PSP_CTRL_LEFT:
			switch(ui_mainmenu_select) {
			case 1:
			case 2:
				state_slot--;
				if (state_slot < 0) state_slot = 9;
				draw_ui_main();
				break;
			case 4:
				if (option_frameskip_type > 0) {
					option_frameskip_type--;
					draw_ui_main();
				}
				break;
			case 5:
				if (option_frameskip_value > 0) {
					option_frameskip_value--;
					draw_ui_main();
				}
				break;
			case 6:
				screen_rotate ^= ROTATE_FLAG;
				if (nPrevGame != ~0U) {
					DoInputBlank(0);
					get_screen_size();
				}
				draw_ui_main();
				break;
			case 7:
				if (cpu_speeds_select > 0) {
					cpu_speeds_select--;
					draw_ui_main();
				}
				break;
			}
			break;
		case PSP_CTRL_RIGHT:
			switch(ui_mainmenu_select) {
			case 1:
			case 2:
				state_slot++;
				if(state_slot > 9) state_slot = 0;
				draw_ui_main();
				break;
			case 4:
				if (option_frameskip_type < 2) {
					option_frameskip_type++;
					draw_ui_main();
				}
				break;
			case 5:
				if (option_frameskip_value < 30) {
					option_frameskip_value++;
					draw_ui_main();
				}
				break;
			case 6:
				screen_rotate ^= ROTATE_FLAG;
				if (nPrevGame != ~0U) {
					DoInputBlank(0);
					get_screen_size();
				}
				draw_ui_main();
				break;
			case 7:
				if (cpu_speeds_select < 3) {
					cpu_speeds_select++;
					draw_ui_main();
				}
				break;
			}
			break;
			
		case PSP_CTRL_CIRCLE:
			switch( ui_mainmenu_select ) {
			case 0:
				nGameStage = 2;
				strcpy(ui_current_path, szAppRomPath);
				//ui_current_path[strlen(ui_current_path)-1] = 0;
				draw_ui_browse(true);
				break;
			case 1:
				if (nPrevGame != ~0U) {
					sprintf(ui_current_path, "%s%s_%1u.sav", szAppStatePath, BurnDrvGetTextA(DRV_NAME), state_slot);
					if(!BurnStateLoad(ui_current_path, 1, &DrvInitCallback)) {
						screen_initialise();
						SetClockFrequency(cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].bus );
						nGameStage = 0;
					}
				}
				break;
			case 2:
				if (nPrevGame != ~0U) {
					sprintf(ui_current_path, "%s%s_%1u.sav", szAppStatePath, BurnDrvGetTextA(DRV_NAME), state_slot);
					BurnStateSave(ui_current_path, 1);
				}
				break;
			case 3: // Reset Game
				if (nPrevGame != ~0U) {
					game_reset = 1;
					return_to_game();
				}
				break;	
			case 8: // Return to Game
				return_to_game();
				break;	
			case 9:	// Exit
				if (nPrevGame != ~0U) {
					nPrevGame = ~0U;
					DrvExit();
					InpExit();
				}
				bGameRunning = 0;
				break;
				
			}
			break;
			
		case PSP_CTRL_CROSS:
			return_to_game();
			break;
		}
		break;
	/* ---------------------------- Rom Browse ---------------------------- */
	case 2:		
		switch( key ) {
		case PSP_CTRL_UP:
			if (find_rom_select == 0) break;
			if (find_rom_top >= find_rom_select) find_rom_top--;
			find_rom_select--;
			draw_ui_browse(false);
			break;
		case PSP_CTRL_DOWN:
			if ((find_rom_select+1) >= find_rom_count) break;
			find_rom_select++;
			if ((find_rom_top + find_rom_list_cnt) <= find_rom_select) find_rom_top++;
			draw_ui_browse(false);
			break;
		case PSP_CTRL_CIRCLE:
			switch( getRomsFileStat(find_rom_select) ) {
			case -1:	// directry
				{		// printf("change dir %s\n", getRomsFileName(find_rom_select) );
					char * pn = getRomsFileName(find_rom_select);
					if ( strcmp("..", pn) ) {
						strcat(ui_current_path, getRomsFileName(find_rom_select));
						strcat(ui_current_path, "/");
					} else {
						if (strlen(strstr(ui_current_path, ":/")) == 2) break;	// "ROOT:/"
						for(int l = strlen(ui_current_path)-1; l>1; l-- ) {
							ui_current_path[l] = 0;
							if (ui_current_path[l-1] == '/') break;
						}
					}
					//printf("change dir to %s\n", ui_current_path );
					find_rom_count = 0;
					find_rom_select = 0;
					find_rom_top = 0;
					draw_ui_browse(true);
				}
				break;
			default: // rom zip file
				{
					nBurnDrvSelect = (UINT32)getRomsFileStat( find_rom_select );

					if (nPrevGame == nBurnDrvSelect) {
						// same game, reture to it
						return_to_game();
						break;
					}
					
					if ( nPrevGame < nBurnDrvCount ) {
						nBurnDrvSelect = nPrevGame;
						nPrevGame = ~0U;
						DrvExit();
						InpExit();
					}
					
					nBurnDrvSelect = (UINT32)getRomsFileStat( find_rom_select );
					if (nBurnDrvSelect <= nBurnDrvCount && BurnDrvIsWorking() ) {
						
						nGameStage = 3;
						ui_process_pos = 0;
						SetClockFrequency(333, 333, 166);

						if ( DrvInit( nBurnDrvSelect, false ) == 0 ) {
							
							BurnRecalcPal();
							
							screen_initialise();
							SetClockFrequency(cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].cpu, cpu_speeds[cpu_speeds_select].bus );
							nGameStage = 0;
							
						} else {
							nBurnDrvSelect = ~0U; 
							nGameStage = 2;
							SetClockFrequency(222, 222, 111);
						}

					} else
						nBurnDrvSelect = ~0U; 

					nPrevGame = nBurnDrvSelect;
											
					//if (nBurnDrvSelect == ~0U) {
					//	bprintf(0, "unkown rom %s", getRomsFileName(find_rom_select));
					//}
				}
				
				
				
			}
			break;
		case PSP_CTRL_CROSS:	// cancel
			nGameStage = 1;
			draw_ui_main();
			break;
		}
		break;
	/* ---------------------------- Loading Game ---------------------------- */
	case 3:		
		
		break;

	}
}

int do_ui_key(UINT32 key)
{
	// mask keys
	key &= PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS;
	
	static int prvkey = 0;
	static int repeat = 0;
	static int repeat_time = 0;
	
	if (key != prvkey) {
		int def = key ^ prvkey;
		repeat = 0;
		repeat_time = 0;
		process_key( def, def & key, 0 );
		if (def & key) {
			// auto repeat up / down only
			repeat = def & (PSP_CTRL_UP | PSP_CTRL_DOWN);
		} else repeat = 0;
		prvkey = key;
	} else {
		if ( repeat ) {
			repeat_time++;
			if ((repeat_time >= 32) && ((repeat_time & 0x3) == 0))
				process_key( repeat, repeat, repeat_time );
		}
	}
	return 0;
}


void ui_update_progress(float size, char * txt)
{
	drawRect( GUC_FRAME_ADDR(work_frame), 10, 238, 460, 30, UI_BGCOLOR );
	
	drawRect( GUC_FRAME_ADDR(work_frame), 10, 238, 460, 12, R8G8B8_to_B5G6R5(0x807060) );
	drawRect( GUC_FRAME_ADDR(work_frame), 10, 238, ui_process_pos, 12, R8G8B8_to_B5G6R5(0xffc090) );

	int sz = (int)(460 * size + 0.5);
	if (sz + ui_process_pos > 460) sz = 460 - ui_process_pos;
	drawRect( GUC_FRAME_ADDR(work_frame), 10 + ui_process_pos, 238, sz, 12, R8G8B8_to_B5G6R5(0xc09878) );
	drawString(txt, GUC_FRAME_ADDR(work_frame), 10, 255, UI_COLOR, 460);
	
	ui_process_pos += sz;
	if (ui_process_pos > 460) ui_process_pos = 460;

	update_gui();
	show_frame = draw_frame;
	draw_frame = sceGuSwapBuffers();
}

void ui_update_progress2(float size, const char * txt)
{
	static int ui_process_pos2 = 0;

	int sz = (int)(460.0 * size + 0.5);
	if ( txt ) ui_process_pos2 = sz;
	else ui_process_pos2 += sz;
	if ( ui_process_pos2 > 460 ) ui_process_pos2 = 460;
	drawRect( GUC_FRAME_ADDR(work_frame), 10, 245, ui_process_pos2, 3, R8G8B8_to_B5G6R5(0xf06050) );
	
	if ( txt ) {
		drawRect( GUC_FRAME_ADDR(work_frame), 10, 255, 460, 13, UI_BGCOLOR );
		drawString(txt, GUC_FRAME_ADDR(work_frame), 10, 255, UI_COLOR, 460);	
	}

	update_gui();
	show_frame = draw_frame;
	draw_frame = sceGuSwapBuffers();
}


char batt_str[80];

static void update_status_str(char *batt_str)
{
	char batt_life_str[10];
	
	int batt_life_per = scePowerGetBatteryLifePercent();
	
	if(batt_life_per < 0) {
		strcpy(batt_str, "BATT. --%");
	} else {
		sprintf(batt_str, "BATT.%3d%%", batt_life_per);
	}
	
	if(scePowerIsPowerOnline()) {
		strcpy(batt_life_str, "[DC IN]");
	} else {
		int batt_life_time = scePowerGetBatteryLifeTime();
		int batt_life_hour = (batt_life_time / 60) % 100;
		int batt_life_min = batt_life_time % 60;
		
		if(batt_life_time < 0) {
			strcpy(batt_life_str, "[--:--]");
		} else {
			sprintf(batt_life_str, "[%2d:%02d]", batt_life_hour, batt_life_min);
		}
	}
	strcat(batt_str, batt_life_str);
}

void draw_status(void)
{
	static int status_counter = 0;
	
	if((status_counter % 30) == 0) {
		update_status_str(batt_str);
	}
	status_counter++;
	
	drawString(batt_str, GU_FRAME_ADDR(draw_frame), 355, 10, UI_COLOR);
}

