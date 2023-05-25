/* Gnuboy - A Nintendo Gameboy emulator
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
#include "systype.h"

using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

#define MENU_FONT_FILENAME   "font.ttf"
#define MENU_IMG_LOGO_FILENAME  "logo.png"
#define MENU_IMG_GAMEPAD_FILENAME  "gamepad.png"
#define MENU_IMG_TILE_FILENAME  "tile.png"

enum {
	AM_SOUND_SETS,
	AM_INPUT_SETS,
	AM_0,
	AM_FPS,
	AM_1,
	AM_SAVE_GLOBAL_SETTINGS,
	AM_SAVE_GAME_SETTINGS,
	AM_ABOUT,
	AM_BACK,
	AM_count
};

enum {
	VS_SCALING,
	VS_BACK,
	VS_count
};

enum {
	SS_SAMPLE_RATE,
	SS_BACK,
	SS_count
};

//enum {
//	IS_DPAD_DIAGONALS,
//	IS_0,
//	IS_KEY_0,
//	IS_KEY_1,
//	IS_KEY_2,
//	IS_BACK,
//	IS_count
//};

///////////////////////////////////////////////////////////////////////////////

static int 	mCpuSpeedLookup[]  = { 300, 336, 370, 400, 430, 450 };
static int 	mAudioRateLookup[] = { 11025, 22050, 44100 };

static void	Menu_VideoSets();
static void	Menu_SoundSets();
static void	Menu_InputSets();
static void	Menu_About();

gfx_font* emul::menu_load_font()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_FONT_FILENAME);
	debug::printf("menu font:%s\n", path);
	return video::font_load(path, COLOR_BLACK, 16);
}

gfx_texture* emul::menu_load_logo()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_LOGO_FILENAME);
	return video::tex_load(path);
}

gfx_texture* emul::menu_load_gamepad()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_GAMEPAD_FILENAME);
	return video::tex_load(path);
}

gfx_texture* emul::menu_load_tile()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_TILE_FILENAME);
	return video::tex_load(path);
}

void emul::menu_lock_sets()
{
	//
}

///////////////////////////////////////////////////////////////////////////////
//	Advanced menu
///////////////////////////////////////////////////////////////////////////////
static void Menu_Advanced_Texts()
{
	char* showFPSValues[3] = { STR_OFF, "RENDERED", "EMULATED" };

	menu::set_title(MENU_TITLE_TEXT);
	menu::set_item_text  (AM_SOUND_SETS,           "Sound settings");
	menu::set_item_text  (AM_INPUT_SETS,           "Input settings");
	menu::set_separator  (AM_0);
	menu::set_item_optstr(AM_FPS,                  "Show FPS mode:       %s", showFPSValues[settings.showFps]);
	menu::set_separator  (AM_1);
	menu::set_item_text  (AM_SAVE_GLOBAL_SETTINGS, "Save global settings");
	menu::set_item_text  (AM_SAVE_GAME_SETTINGS,   "Save game settings");
	menu::set_item_text  (AM_ABOUT,                "About emulator");
	menu::set_item_text  (AM_BACK,                 "Back");
	
}

void emul::menu_advanced()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	char *loadedGame=(char*)fsys::getGameFilePath();
	menu::open();
	Menu_Advanced_Texts();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(AM_count, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(AM_count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;

			case MA_DECREASE:
				switch(menufocus)
				{
					case AM_FPS:
						if(--settings.showFps > 2) settings.showFps = 2;
						break;
				}
				Menu_Advanced_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case AM_FPS:
						if(++settings.showFps > 2) settings.showFps = 0;
						break;
				}
				Menu_Advanced_Texts();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{	
					case AM_SOUND_SETS:
						Menu_SoundSets();
						break;
					case AM_INPUT_SETS:
						Menu_InputSets();
						break;
					case AM_SAVE_GLOBAL_SETTINGS:
						emul::sets_save(false);
						break;
					case AM_SAVE_GAME_SETTINGS:
						if(loadedGame[0]!=0)
						{
							emul::sets_save(true);
						}						
						break;
					case AM_ABOUT:
						Menu_About();
						break;
					case AM_BACK:
						menuExit = 1;
						break;
				}
				Menu_Advanced_Texts();
				break;

			case MA_NONE:
				break;
		}
	}
	menu::close();
}

///////////////////////////////////////////////////////////////////////////////
//	Video settings menu
///////////////////////////////////////////////////////////////////////////////

static void Menu_VideoSets_Texts()
{
	menu::set_title("Video settings");
	menu::set_item_optstr(VS_SCALING,         "Fullscreen scaling:  %s", settings.scalingMode ? "HARDWARE" : "SOFTWARE");
	menu::set_item_text  (VS_BACK,            "Back");
	
	menu::set_item_enable(VS_SCALING, settings.fullScreen);
}

static void Menu_VideoSets()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	
	menu::open();
	Menu_VideoSets_Texts();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(VS_count, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(VS_count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;
				
			case MA_DECREASE:
				switch(menufocus)
				{
					case VS_SCALING:
						settings.scalingMode ^= true;
						break;
				}
				Menu_VideoSets_Texts();
				break;

			case MA_INCREASE:
				switch(menufocus)
				{
					case VS_SCALING:
						settings.scalingMode ^= true;
						break;
				}
				Menu_VideoSets_Texts();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{	
					case VS_BACK:
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
//	Sound settings menu
///////////////////////////////////////////////////////////////////////////////
static void Menu_SoundSets_Texts()
{
	menu::set_title("Sound settings");
	menu::set_item_optint(SS_SAMPLE_RATE, "Sample rate:         %d", settings.soundRate);
	menu::set_item_text  (SS_BACK,        "Back");
}

static void Menu_SoundSets()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	
	menu::open();
	Menu_SoundSets_Texts();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(SS_count, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(SS_count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;
				
			case MA_DECREASE:
				switch(menufocus)
				{
					case SS_SAMPLE_RATE:
						settings.soundRate = menu::prev_val(mAudioRateLookup, 3, settings.soundRate);
						break;
				}
				Menu_SoundSets_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case SS_SAMPLE_RATE:
						settings.soundRate = menu::next_val(mAudioRateLookup, 3, settings.soundRate);
						break;
				}
				Menu_SoundSets_Texts();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{	
					case SS_BACK:
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
//	Input settings menu
///////////////////////////////////////////////////////////////////////////////

static void Menu_InputSets_Texts()
{
	char dingooButtons[][8] = {	"?", "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "X", "Y", "L", "R", "SELECT", "START" };
	
	int menuItem = 0;
	menu::set_title("Input settings");
	menu::set_item_optstr(menuItem++, "Button A:      %s", dingooButtons[settings.pad_config[0]]);
	menu::set_item_optstr(menuItem++, "Button B:            %s", dingooButtons[settings.pad_config[1]]);
	menu::set_item_optstr(menuItem++, "Button Start:        %s", dingooButtons[settings.pad_config[2]]);
	menu::set_item_optstr(menuItem++, "Button Select:        %s", dingooButtons[settings.pad_config[3]]);
	menu::set_item_text(menuItem++,       "Back");
}

static void Menu_InputSets()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	int menuCount   = 5;
	
	menu::open();
	Menu_InputSets_Texts();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(menuCount, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(menuCount, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;

			case MA_DECREASE:
				if(--settings.pad_config[menufocus] > 13) settings.pad_config[menufocus] = 13;

				Menu_InputSets_Texts();
				break;
				
			case MA_INCREASE:
				if(++settings.pad_config[menufocus] > 13) settings.pad_config[menufocus] = 0;
				Menu_InputSets_Texts();
				break;
				
			case MA_SELECT:
				if(menufocus == menuCount - 1) menuExit = 1;
				break;

			case MA_NONE:
				break;
		}
	}
	menu::close();
}

///////////////////////////////////////////////////////////////////////////////
//	About menu
///////////////////////////////////////////////////////////////////////////////
static void Menu_About()
{
	int menuExit	= 0;
	int menufocus	= 0;
	int menuCount   = 0;
	
	menu::open();
	menu::set_title("About emulator");
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "  "        MENU_TITLE_TEXT         "  ");
	menu::set_item_text(menuCount++, "        ported to GPi by Reesy        ");
	menu::set_item_text(menuCount++, "        menu system by lion_rsm       ");
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "GnuBOY port for Retroflag Gpi bare    ");
	menu::set_item_text(menuCount++, "metal os provided by Circle lib.      ");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "GnuBoy is a Gameboy and Gameboy color ");
	menu::set_item_text(menuCount++, "emulator.                             ");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Start+Select - enter menu             ");
	menu::set_item_text(menuCount++, "R+Select     - save quick state       ");
	menu::set_item_text(menuCount++, "L+Start      - load quick state       ");
	menu::set_item_text(menuCount++, "L+R+Right    - fast forward           ");
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(menuCount, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		if(menu::update(menuCount, menufocus) == MA_CANCEL) menuExit = 1;
	}
  	menu::close();
}
