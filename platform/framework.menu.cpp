/* DEP - Dingoo Emulation Pack for Dingoo A320
 *
 * Copyright (C) 2012-2013 lion_rsm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
 
#include "framework.h"

using namespace fw;
using namespace hw;

#define GAME_COVER_EXT  "png"

///////////////////////////////////////////////////////////////////////////////

enum {
#ifndef DISABLE_REGION_SELECT
	ES_COUNTRY_REGION,
#endif
#ifndef DISABLE_IPS_PATCHING
	ES_APPLY_IPS_PATCH,
#endif
	ES_COMPRESS_ROM,
	ES_OLDSCHOOL_MODE,
	ES_0,
	ES_DEFAULT_SETS,
	ES_CONTINUE,
	ES_count
};

enum {
	MM_ROM_SELECT,
	MM_RESUME,
	MM_SAVE_STATE,
	MM_LOAD_STATE,
#ifndef DISABLE_CHEATING
	MM_CHEATING,
#endif
	MM_SKIP_FRAMES,
	MM_FULLSCREEN,
	MM_SOUND_VOLUME,
	MM_ADVANCED_SETS,
	MM_RESET,
	MM_EXIT,
	MM_count
};

///////////////////////////////////////////////////////////////////////////////

typedef struct {
	char filePath[HW_MAX_PATH];
	bool inUse;
} State_t;

static gfx_texture* mTile;
static gfx_texture* mLogo;
static gfx_texture* mGamepad;
static gfx_texture* mCover;

static bool mNeedSaveStateScan[10] = {false,false,false,false,false,false,false,false,false,false};
static State_t   mSaveState[10];
static int       slotNumber = -1;
static int       mLastRomIndex = -1;
static gfx_color stateCurrent[SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT];
static gfx_color stateSaved  [SCREENSHOT_WIDTH * SCREENSHOT_HEIGHT];

static void menu_cheating();
static int menu_rom_list();

///////////////////////////////////////////////////////////////////////////////
//  Saved states
///////////////////////////////////////////////////////////////////////////////

static void _scan_for_saved_states()
{
	char stateExt[strlen(SAVESTATE_EXT) + 2];
	for(int i = 0; i < 10; i++)
	{
		sprintf(stateExt, "%s%d", SAVESTATE_EXT, i);
		strcpy(mSaveState[i].filePath, fsys::getGameRelatedFilePath(stateExt));
		mSaveState[i].inUse = state::isCorrect(mSaveState[i].filePath);
	}
}

static void _scan_for_saved_states(int index)
{
	char stateExt[strlen(SAVESTATE_EXT) + 2];

	sprintf(stateExt, "%s%d", SAVESTATE_EXT, index);
	strcpy(mSaveState[index].filePath, fsys::getGameRelatedFilePath(stateExt));
	mSaveState[index].inUse = state::isCorrect(mSaveState[index].filePath);
}

static void _get_state_screenshot(gfx_color* screenshot)
{
	int console_scr_w;
	int console_scr_h;
	core::emulate_for_screenshot(console_scr_w, console_scr_h);
	video::bilinear_scale(
		video::get_framebuffer(),
		console_scr_w, console_scr_h, HW_SCREEN_WIDTH,
		screenshot,
		SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, SCREENSHOT_WIDTH);
}

///////////////////////////////////////////////////////////////////////////////
//  Menu init/deinit
///////////////////////////////////////////////////////////////////////////////
void emul::menu_init()
{
	menu::set_title(MENU_TITLE_TEXT);
	
	

	//  Calculate auxiliary values
	int area_y0 = 32 + 16 + SCREENSHOT_HEIGHT + 2;
	int area_y1 = HW_SCREEN_HEIGHT - 16 - 2;
	int area_h  = area_y1 - area_y0;
	int area_x0 = HW_SCREEN_WIDTH - 8 - SCREENSHOT_WIDTH - 2;
	int area_x1 = HW_SCREEN_WIDTH - 8 + 2;
	int area_w  = area_x1 - area_x0;
	//  Load resources
	debug::printf("Before load FONT\n");
	video::font_set(emul::menu_load_font());	//	TODO: destroy font ?
	debug::printf("After load FONT\n");
	mTile    = emul::menu_load_tile();
	mLogo    = emul::menu_load_logo();
	mGamepad = emul::menu_load_gamepad();
	menu::setup_bg_texture(mTile);
	menu::setup_bg_texture(MT_LOGO, mLogo, area_x0 + (area_w - mLogo->width) / 2, area_y0 + (area_h - mLogo->height) / 2);
	menu::setup_bg_texture(MT_GAMEPAD, mGamepad, HW_SCREEN_WIDTH - mGamepad->width, area_y1 - 2 - mGamepad->height);

	//  Load settings
	sets_load();

	//  Lock some settings to specific values
	menu_lock_sets();
	if(system::is_in_tv_mode()) {
		settings.fullScreen = true;
		settings.cpuSpeed = CPU_SPEED_IN_MENU;
	}
}

void emul::menu_init_game()
{
	char path[HW_MAX_PATH];
	char name[HW_MAX_PATH];
	char ext [HW_MAX_PATH];
	fsys::split(fsys::getGameFilePath(), path, name, ext);
	//  Init menu engine with game name
	char gameFileName[HW_MAX_PATH];
	sprintf(gameFileName, "%s.%s", name, ext);  //  TODO
	bool isROMCompressed = zip::check(fsys::getGameFilePath());
	menu::init(gameFileName, isROMCompressed ? COLOR_ROM_ZIPPED_TXT : COLOR_ROM_UNZIPPED_TXT);
	menu::set_title(MENU_TITLE_TEXT);
	//  Calculate auxiliary values
	int area_y0 = 32 + 16 + SCREENSHOT_HEIGHT + 2;
	int area_y1 = HW_SCREEN_HEIGHT - 16 - 2;
	int area_h  = area_y1 - area_y0;
	int area_x0 = HW_SCREEN_WIDTH - 8 - SCREENSHOT_WIDTH - 2;
	int area_x1 = HW_SCREEN_WIDTH - 8 + 2;
	int area_w  = area_x1 - area_x0;

#if 0
	//  Try to load game cover
	if(mCover = video::tex_load(fsys::getGameRelatedFilePath(GAME_COVER_EXT))) 
	{
		int w = mCover->width;
		int h = mCover->height;
		bool portr = bool(h > w && h == COVER_MAX_SIZE && (w >= COVER_MIN_SIZE && w <= COVER_MAX_SIZE));
		bool album = bool(h < w && w == COVER_MAX_SIZE && (h >= COVER_MIN_SIZE && h <= COVER_MAX_SIZE));
		if(portr || album) {
			menu::setup_bg_texture(MT_COVER, mCover, HW_SCREEN_WIDTH - 8 - w, (HW_SCREEN_HEIGHT - h) / 2);
		} else {
			video::tex_delete(mCover);
			mCover = NULL;
		}
	}
#endif
	//  Load settings
	sets_load();

	//  Lock some settings to specific values
	menu_lock_sets();
	if(system::is_in_tv_mode()) {
		settings.fullScreen = true;
		settings.cpuSpeed = CPU_SPEED_IN_MENU;
	}
	//  Scan saved states on disk
	//_scan_for_saved_states();
}

void emul::menu_close()
{
	video::font_delete(video::font_get());
	video::tex_delete (mTile);
	video::tex_delete (mLogo);
	video::tex_delete (mGamepad);
}

///////////////////////////////////////////////////////////////////////////////
//  Main menu
///////////////////////////////////////////////////////////////////////////////

static void _update_text_menu_main()
{
	menu::set_title(MENU_TITLE_TEXT);
	menu::set_item_text  (MM_ROM_SELECT,       "Rom Select");
	menu::set_item_text  (MM_RESUME,           "Resume");
	menu::set_item_text  (MM_SAVE_STATE,       "Save state");
	menu::set_item_text  (MM_LOAD_STATE,       "Load state");
#ifndef DISABLE_CHEATING
	menu::set_item_text  (MM_CHEATING,         "Cheating");
#endif
	if(settings.frameSkip) {
		menu::set_item_optint(MM_SKIP_FRAMES,  "Skip frames:  %d", settings.frameSkip - 1);
	} else {
		menu::set_item_text  (MM_SKIP_FRAMES,  "Skip frames:  AUTO");
	}
	menu::set_item_optstr(MM_FULLSCREEN,       "Full screen:  %s", (settings.fullScreen ? STR_ON : STR_OFF));
	if(settings.soundVolume) {
		menu::set_item_text  (MM_SOUND_VOLUME, "Sound: " STR_ON);
	} else {
		menu::set_item_text  (MM_SOUND_VOLUME, "Sound: " STR_OFF);
	}
	menu::set_item_text  (MM_ADVANCED_SETS,    "Advanced sets");
	menu::set_item_text  (MM_RESET,            "Reset");
	menu::set_item_text  (MM_EXIT,             "Exit");
	
	menu::set_item_enable(MM_FULLSCREEN, !system::is_in_tv_mode());
	menu::set_item_enable(MM_SAVE_STATE, !settings.oldSchoolMode);
	menu::set_item_enable(MM_LOAD_STATE, !settings.oldSchoolMode);
#ifndef DISABLE_CHEATING
	menu::set_item_enable(MM_CHEATING, !settings.oldSchoolMode && core::get_cheat_count());
#endif
}

EVENT emul::menu_main()
{
	int menuExit = 0;
	static int menufocus = 0;
	
	EVENT action = EVENT_NONE;
	system::set_cpu_speed(CPU_SPEED_IN_MENU);
	system::init_frames_counter(100);
	video::init(VM_FAST);
	menu::setup_bg_scrolling();
	menu::msg_box("Please wait...", MMB_MESSAGE);
	//  Prepare states screenshots
	MemBuffer currState;
	char *loadedGame=(char*)fsys::getGameFilePath();
	if(loadedGame[0]!=0)
	{
		debug::printf("menu_main savestate 1\n");
		core::save_load_state(NULL, &currState, false);                             //  save current state to memory
		debug::printf("menu_main savestate 2\n");
		_get_state_screenshot(stateCurrent);                                        //  get screenshot for current state
		debug::printf("menu_main savestate 3\n");
		if(slotNumber < 0)
		{
			slotNumber = 0;
			_scan_for_saved_states(0);
			mNeedSaveStateScan[0]=false;
			core::save_load_state(mSaveState[slotNumber].filePath, NULL, true); 	//  load selected slot state
			_get_state_screenshot(stateSaved);                                      //  get screenshot for selected slot state

			core::save_load_state(NULL, &currState, true);                          //  restore current state
		}
		debug::printf("menu_main savestate end\n");
	}
	//  Begin menu loop
	menu::open();
	_update_text_menu_main();
	while (!menuExit)
	{
		// Draw screen:
		video::bind_framebuffer();

		menu::select_bg_texture(true);

		menu::render_menu(MM_count, menufocus);
		bool showSavedState = (menufocus == MM_SAVE_STATE || menufocus == MM_LOAD_STATE) && !settings.oldSchoolMode;
		
		if((menufocus == MM_SAVE_STATE || menufocus == MM_LOAD_STATE) && mNeedSaveStateScan[slotNumber])
		{
			_scan_for_saved_states(slotNumber);
			mNeedSaveStateScan[slotNumber]=false;
		}
		
		menu::render_stateshot(
			HW_SCREEN_WIDTH - 8 - SCREENSHOT_WIDTH, 32, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT,
			showSavedState ? (mSaveState[slotNumber].inUse ? 2 : 1) : 0, slotNumber, stateCurrent, stateSaved);
		menu::select_bg_texture(false);
		video::term_framebuffer();
		video::flip(0);

		switch(menu::update(MM_count, menufocus))
		{
			case MA_CANCEL:
				if(loadedGame[0]!=0)
				{
					action = EVENT_RUN_ROM;
					menuExit = 1;
				}
				break;

			case MA_DECREASE:
				switch(menufocus)
				{
					case MM_SAVE_STATE:
					case MM_LOAD_STATE:
						if(--slotNumber < 0) slotNumber = 9;
						if(mSaveState[slotNumber].inUse) {
							core::save_load_state(mSaveState[slotNumber].filePath, NULL, true);
							_get_state_screenshot(stateSaved);
							core::save_load_state(NULL, &currState, true);
						}
						break;
					case MM_SKIP_FRAMES:
						if(--settings.frameSkip > 6) settings.frameSkip = 6;
						break;
					case MM_FULLSCREEN:
						settings.fullScreen ^= 1;
						break;
					case MM_SOUND_VOLUME:
						settings.soundVolume ^= 1;
						break;
				}
				_update_text_menu_main();
				break;

			case MA_INCREASE:
				switch(menufocus)
				{
					case MM_SAVE_STATE:
					case MM_LOAD_STATE:
						if(++slotNumber > 9) slotNumber = 0;
						if(mSaveState[slotNumber].inUse) {
							core::save_load_state(mSaveState[slotNumber].filePath, NULL, true);
							_get_state_screenshot(stateSaved);
							core::save_load_state(NULL, &currState, true);
						}
						break;
					case MM_SKIP_FRAMES:
						if(++settings.frameSkip > 6) settings.frameSkip = 0;
						break;
					case MM_FULLSCREEN:
						settings.fullScreen ^= 1;
						break;
					case MM_SOUND_VOLUME:
						settings.soundVolume ^= 1;
						break;
				}
				_update_text_menu_main();
				break;

			case MA_SELECT:
				switch(menufocus)
				{
					case MM_ROM_SELECT:
						if(menu_rom_list())
						{
							action = EVENT_LOAD_ROM;
							for(int i=0;i<10;i++)
							{
								mNeedSaveStateScan[i]=true;
							}
							slotNumber = -1; //reset displayed state
							menuExit = 1;
						}

						break;
					case MM_RESUME:
						if(loadedGame[0]!=0)
						{
							action = EVENT_RUN_ROM;
							menuExit = 1;
						}
						break;
					case MM_SAVE_STATE:
						if(loadedGame[0]!=0)
						{
							menu::msg_box("Save game state...", MMB_MESSAGE);
							core::save_load_state(mSaveState[slotNumber].filePath, NULL, false);
							mSaveState[slotNumber].inUse = true;
							memcpy(stateSaved, stateCurrent, sizeof(stateCurrent));
						}
						break;
					case MM_LOAD_STATE:
						if(loadedGame[0]!=0)
						{
							core::save_load_state(mSaveState[slotNumber].filePath, NULL, true);
							action = EVENT_RUN_ROM;
							menuExit = 1;
						}
						break;
#ifndef DISABLE_CHEATING
					case MM_CHEATING:
						menu_cheating();
						break;
#endif 
					case MM_SKIP_FRAMES:
						settings.frameSkip = 0;
						break;
					case MM_SOUND_VOLUME:
						settings.soundVolume ^= 1;
						break;
					case MM_ADVANCED_SETS:
						menu_advanced();
						break;
					case MM_RESET:
						if(loadedGame[0]!=0)
						{
							if(menu::msg_box("Are you sure want to reset the game?", MMB_YESNO))
							{
								action = EVENT_RESET_ROM;
								menuExit = 1;
							}
						}
						break;
					case MM_EXIT:
						action = EVENT_EXIT_APP;
						menuExit = 1;
						break;
				}
				_update_text_menu_main();
				break;
			
			case MA_NONE:
				if(menufocus == MM_SAVE_STATE || menufocus == MM_LOAD_STATE)
				{
					if((input::poll() & HW_INPUT_X) && mSaveState[slotNumber].inUse)
					{
						char buf[256]; sprintf(buf, "Are you really want to delete the game state from the \"Slot %d\"?", slotNumber);
						if(menu::msg_box(buf, MMB_YESNO))
						{
							fsys::kill(mSaveState[slotNumber].filePath);
							mSaveState[slotNumber].inUse = false;                           
						}
						_update_text_menu_main();
					}
				}
				break;
	
		}

	}
	menu::close();
	//  Free used heap memory
  	currState.free();
	return action;
}

int menu::string_compare(char *string1, char *string2)
{
	int i=0;
	char c1=0,c2=0;
	while(1)
	{
		c1=string1[i];
		c2=string2[i];
		// check for string end
		
		if ((c1 == 0) && (c2 == 0)) return 0;
		if (c1 == 0) return -1;
		if (c2 == 0) return 1;
		
		if ((c1 >= 0x61)&&(c1<=0x7A)) c1-=0x20;
		if ((c2 >= 0x61)&&(c2<=0x7A)) c2-=0x20;
		if (c1>c2)
			return 1;
		else if (c1<c2)
			return -1;
		i++;
	}

}

void menu::directory_entry_swap(struct MENU_DIRECTORY_ENTRY *from, struct MENU_DIRECTORY_ENTRY *to)
{
	MENU_DIRECTORY_ENTRY temp;

	//Copy salFrom to temp entry
	strcpy((char*)temp.displayName, (char*)from->displayName);
	strcpy((char*)temp.filename, (char*)from->filename);
	temp.type = from->type;

	//Copy salTo to salFrom
	strcpy((char*)from->displayName, (char*)to->displayName);
	strcpy((char*)from->filename,(char*)to->filename);
	from->type=to->type;

	//Copy temp entry to salTo
	strcpy((char*)to->displayName, (char*)temp.displayName);
	strcpy((char*)to->filename, (char*)temp.filename);
	to->type=temp.type;
		
}

MENU_DIRECTORY_LIST *get_file_list_internal(char *path, char **ext, int type)
{
	int itemCount=0, fileCount=0;
	int x,a,b;
	char filename[HW_MAX_PATH];
	char filepath[HW_MAX_PATH];
	char fileext[HW_MAX_PATH];
	MENU_DIR d;
	MENU_DIRECTORY_LIST *list = NULL;
	
	if(!fsys::directoryItemCount((char*)path,&itemCount))
	{
		menu::msg_box("Failed to open directory", MMB_PAUSE);
		return NULL;
	}

	if (itemCount>0)
	{
		list=(MENU_DIRECTORY_LIST*)malloc(sizeof(MENU_DIRECTORY_LIST)
		                                 +(sizeof(MENU_DIRECTORY_ENTRY*)*itemCount)
		                                 +(sizeof(MENU_DIRECTORY_ENTRY)*itemCount));
		
		//was there enough memory?
		if(list == 0)
		{
			menu::msg_box("Could not allocate memory", MMB_PAUSE);
			return list;
		}
		
		//Zero object
		memset(list, 0, sizeof(MENU_DIRECTORY_LIST)
		                                 +(sizeof(MENU_DIRECTORY_ENTRY*)*itemCount)
		                                 +(sizeof(MENU_DIRECTORY_ENTRY)*itemCount));

		//Setup points to each ENTRY item
		char *entryIdxMem=((char*)list)+sizeof(MENU_DIRECTORY_LIST);
		char *entryMem=(char*)entryIdxMem+(sizeof(MENU_DIRECTORY_ENTRY*)*itemCount);

		list->fileList=(MENU_DIRECTORY_ENTRY**)entryIdxMem;
		for(x=0;x<itemCount;x++)
		{
			list->fileList[x]=(MENU_DIRECTORY_ENTRY*)entryMem;
			entryMem+=sizeof(MENU_DIRECTORY_ENTRY);
		}

		if (fsys::directoryOpen(path, &d))
		{
			x=0;
			//Dir opened, now stream out details
			while(fsys::directoryRead(&d, list->fileList[x]))
			{
				MENU_DIRECTORY_ENTRY *entry=list->fileList[x];
				//Dir entry read
				if (entry->type == MENU_FILE_TYPE_FILE
				    && type == MENU_FILE_TYPE_FILE)
				{
					fsys::split((char*)entry->filename,(char*)filepath,(char*)filename,(char*)fileext);
					//loop through ext
					int c=0;
					while(ext[c][0] != 0)
					{
						if(menu::string_compare((char*)fileext,(char*)ext[c]) == 0)
						{
							list->count++;
							x++;
							break;
						}
						c++;
					}
				}
				else if(entry->type == MENU_FILE_TYPE_DIRECTORY 
				        && type == MENU_FILE_TYPE_DIRECTORY)
				{
					if(menu::string_compare((char*)entry->filename,(char*)".") == 0
					|| menu::string_compare((char*)entry->filename,(char*)"..") == 0)
					{
						//Ignore linux navs . and ..
					}
					else
					{
						list->count++;
						x++;
					}
					
				}
			}
			fsys::directoryClose(&d);
		}
		else
		{
			//Failed to open dir - display error
			menu::msg_box("Failed to open rom directory", MMB_PAUSE);
			list->count=0;
			return list;
		}
		int lowIndex=0;

		//Sort file entries
		for(a=0;a<list->count;a++)
		{
			lowIndex=a;		
			for(b=a+1;b<list->count;b++)
			{
				if (menu::string_compare(list->fileList[b]->displayName, list->fileList[lowIndex]->displayName) < 0)
				{
					//this index is lower
					lowIndex=b;
				}
			}
			//lowIndex should index next lowest value
			menu::directory_entry_swap(list->fileList[lowIndex],list->fileList[a]);
		}
	}		
	return list;
}

MENU_DIRECTORY_LIST *menu::get_file_list(char *path, char **ext)
{
	return get_file_list_internal(path, ext, MENU_FILE_TYPE_FILE);
}

MENU_DIRECTORY_LIST *menu::get_directory_list(char *path)
{
	return get_file_list_internal(path, NULL, MENU_FILE_TYPE_DIRECTORY);
}

static int menu_rom_list()
{
	int menuExit    = 0;
	int menufocus   = 0;
	int menuRet     = 0;
	char *fileext[] = {"zip",""};
	system::set_cpu_speed(CPU_SPEED_IN_MENU);
	system::init_frames_counter(100);
	video::init(VM_FAST);
	
	
	menu::open();
	MENU_DIRECTORY_LIST *list = menu::get_file_list((char*)fsys::getGamePath(), (char**)&fileext);
	
	//Jump to last position in rom list
	if(mLastRomIndex>=0 && mLastRomIndex < list->count)
	{
		menufocus=mLastRomIndex;
	}
	
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_file_list(list, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::rom_list_update(list->count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;
							
			case MA_SELECT:
				//Load rom
				char fullFilename[HW_MAX_PATH];
				strcpy(fullFilename,fsys::getGamePath());
				fsys::combine(fullFilename, (char*)list->fileList[menufocus]->filename);
				fsys::setGameFilePath(fullFilename);
				emul::menu_init_game();
				menuRet = 1;
				menuExit = 1;
				break;

			case MA_NONE:
				break;
		}
		mLastRomIndex = menufocus;
	}
	menu::close();
	
	if(list != 0)
	{
		free(list);
	}

	return menuRet;
}

///////////////////////////////////////////////////////////////////////////////
//  Extra settings menu
///////////////////////////////////////////////////////////////////////////////

static void _update_text_menu_extra()
{
	menu::set_title(MENU_TITLE_TEXT);
#ifndef DISABLE_REGION_SELECT
	char regions[][8] = { "AUTO", "US", "EU", "JP" };
	menu::set_item_optstr(ES_COUNTRY_REGION,  "Country/Region:      %s", regions[settings.countryRegion]);
#endif
#ifndef DISABLE_IPS_PATCHING
	menu::set_item_optstr(ES_APPLY_IPS_PATCH, "Apply IPS patch:     %s", settings.applyIPSpatch ? STR_ON : STR_OFF);
#endif
	menu::set_item_optstr(ES_COMPRESS_ROM,    "Auto compress ROM:   %s", settings.compressROM ? STR_ON : STR_OFF);
	menu::set_item_optstr(ES_OLDSCHOOL_MODE,  "Old school mode:     %s", settings.oldSchoolMode ? STR_ON : STR_OFF);
	menu::set_separator  (ES_0);
	menu::set_item_text  (ES_DEFAULT_SETS,    "Default settings");
	menu::set_item_text  (ES_CONTINUE,        "Continue");
}

void emul::menu_extra()
{
	int menuExit    = 0;
	int menufocus   = 0;

	system::set_cpu_speed(CPU_SPEED_IN_MENU);
	system::init_frames_counter(100);
	video::init(VM_FAST);
	
	menu::open();
	_update_text_menu_extra();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(ES_count, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(ES_count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;
				
			case MA_DECREASE:
				switch(menufocus)
				{
#ifndef DISABLE_REGION_SELECT
					case ES_COUNTRY_REGION:
						if(--settings.countryRegion > 3) settings.countryRegion = 3;
						break;
#endif
#ifndef DISABLE_IPS_PATCHING
					case ES_APPLY_IPS_PATCH:
						settings.applyIPSpatch ^= true;
						break;
#endif
					case ES_COMPRESS_ROM:
						settings.compressROM ^= true;
						break;
					case ES_OLDSCHOOL_MODE:
						settings.oldSchoolMode ^= true;
						break;
				}
				_update_text_menu_extra();
				break;

			case MA_INCREASE:
				switch(menufocus)
				{
#ifndef DISABLE_REGION_SELECT
					case ES_COUNTRY_REGION:
						if(++settings.countryRegion > 3) settings.countryRegion = 0;
						break;
#endif
#ifndef DISABLE_IPS_PATCHING
					case ES_APPLY_IPS_PATCH:
						settings.applyIPSpatch ^= true;
						break;
#endif
					case ES_COMPRESS_ROM:
						settings.compressROM ^= true;
						break;
					case ES_OLDSCHOOL_MODE:
						settings.oldSchoolMode ^= true;
						break;
				}
				_update_text_menu_extra();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{   
					case ES_DEFAULT_SETS:
						if(menu::msg_box("Are you sure want to set the settings to default?", MMB_YESNO))
						{
							sets_default();
						}
						_update_text_menu_extra();
						break;
					case ES_CONTINUE:
						menuExit = 1;
						break;
				}
				break;

			case MA_NONE:
				break;
		}
	}
	menu::close();
}

///////////////////////////////////////////////////////////////////////////////
//  Cheating menu
///////////////////////////////////////////////////////////////////////////////
#ifndef DISABLE_CHEATING
static int _get_cheat_count()
{
	int cheatsCount = core::get_cheat_count();
	if (cheatsCount > MAX_MENU_ITEM_COUNT - 1)
		cheatsCount = MAX_MENU_ITEM_COUNT - 1;
	return cheatsCount;
}

static void _update_text_menu_cheating()
{
	menu::set_title("Cheating");
	int cheatsCount = _get_cheat_count();

	//  find max name length
	int max_len = 0;
	for(int i = 0; i < cheatsCount; i++) {
		int str_len = strlen(core::get_cheat_name(i));
		if(str_len > max_len) max_len = str_len;
	}
	if (max_len > MAX_MENU_ITEM_CHARS - 5)
		max_len = MAX_MENU_ITEM_CHARS - 5;

	//  generate menu items
	char item[MAX_MENU_ITEM_CHARS + 1];
	for(int i = 0; i < cheatsCount; i++) {
		item[0] = 0;
		strncat(item, core::get_cheat_name(i), max_len);
		int sp = max_len - strlen(item);
		strcat(item, ": "); while(--sp >= 0) strcat(item, " ");
		strcat(item, core::get_cheat_active(i) ? STR_ON : STR_OFF);
		menu::set_item_text(i, item);
	}

	//  add back menu item
	menu::set_item_text(cheatsCount, "Back");
}

static void menu_cheating()
{
	int menuExit    = 0;
	int menufocus   = 0;
	int cheatsCount = _get_cheat_count();
	
	menu::open();
	_update_text_menu_cheating();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(cheatsCount + 1, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(cheatsCount + 1, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;
				
			case MA_DECREASE:
			case MA_INCREASE:
				if(menufocus < cheatsCount) {
					core::set_cheat_active(menufocus, !core::get_cheat_active(menufocus));
					_update_text_menu_cheating();
				}
				break;
				
			case MA_SELECT:
				if(menufocus < cheatsCount) {
					menu::set_title("Cheat description");
					menu::msg_box(core::get_cheat_name(menufocus), MMB_PAUSE);
					_update_text_menu_cheating();
				} else
					menuExit = 1;
				break;

			case MA_NONE:
				break;
		}
	}
	menu::close();
}

#endif
