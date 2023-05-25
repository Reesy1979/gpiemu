/* DingooSMS - Sega Master System, Sega Game Gear, ColecoVision Emulator for Dingoo A320
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
 
#include "png_logo_sms.h"
#include "png_logo_gg.h"
#include "png_logo_coleco.h"
#include "png_gamepad_sms.h"
#include "png_gamepad_gg.h"
#include "png_gamepad_coleco.h"
#include "framework.h"
#include "systype.h"
#include "shared.h"

using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

#define MENU_FONT_FILENAME   "font.ttf"
#define MENU_IMG_LOGO_FILENAME  "logo.png"
#define MENU_IMG_GAMEPAD_FILENAME  "gamepad.png"
#define MENU_IMG_TILE_FILENAME  "tile.png"

enum {
	AM_VIDEO_SETS,
//	AM_SOUND_SETS,
	AM_INPUT_SETS,
	AM_0,
	AM_CPU_SPEED,
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
	menu::set_item_text  (AM_VIDEO_SETS,           "Video settings");
//	menu::set_item_text  (AM_SOUND_SETS,           "Sound settings");
	menu::set_item_text  (AM_INPUT_SETS,           "Input settings");
	menu::set_separator  (AM_0);
	menu::set_item_optint(AM_CPU_SPEED,            "CPU speed, MHz:      %d", settings.cpuSpeed);
	menu::set_item_optstr(AM_FPS,                  "Show FPS mode:       %s", showFPSValues[settings.showFps]);
	menu::set_separator  (AM_1);
	menu::set_item_text  (AM_SAVE_GLOBAL_SETTINGS, "Save global settings");
	menu::set_item_text  (AM_SAVE_GAME_SETTINGS,   "Save game settings");
	menu::set_item_text  (AM_ABOUT,                "About emulator");
	menu::set_item_text  (AM_BACK,                 "Back");
	
	menu::set_item_enable(AM_CPU_SPEED, !system::is_in_tv_mode());
}

void emul::menu_advanced()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	
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
					case AM_CPU_SPEED:
						settings.cpuSpeed = menu::prev_val(mCpuSpeedLookup, 6, settings.cpuSpeed);
						break;
					case AM_FPS:
						if(--settings.showFps > 2) settings.showFps = 2;
						break;
				}
				Menu_Advanced_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case AM_CPU_SPEED:
						settings.cpuSpeed = menu::next_val(mCpuSpeedLookup, 6, settings.cpuSpeed);
						break;
					case AM_FPS:
						if(++settings.showFps > 2) settings.showFps = 0;
						break;
				}
				Menu_Advanced_Texts();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{	
					case AM_VIDEO_SETS:
						Menu_VideoSets();
						break;
//					case AM_SOUND_SETS:
//						Menu_SoundSets();
//						break;
					case AM_INPUT_SETS:
						Menu_InputSets();
						break;
					case AM_CPU_SPEED:
						if(++settings.cpuSpeed > 450) settings.cpuSpeed = 450;
						break;
					case AM_SAVE_GLOBAL_SETTINGS:
						emul::sets_save(false);
						break;
					case AM_SAVE_GAME_SETTINGS:
						emul::sets_save(true);						
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
	char dingooButtons[][8] = {	"?", "\x18", "\x19", "\x1A", "\x1B", "A", "B", "X", "Y", "\x1C\x1D", "\x1E\x1F", "\x15\x16\x17", "\x12\x13\x14" };
	
	int menuItem = 0;
	menu::set_title("Input settings");
	menu::set_item_optstr(menuItem++,     "Diagonals helper:    %s", (settings.dpadDiagonals ? STR_ON : STR_OFF));
	menu::set_separator(menuItem++);

	if(isColeco()) {
		menu::set_item_optstr(menuItem++, "Button Fire:         %s", dingooButtons[settings.pad_config[6]]);
		menu::set_item_optstr(menuItem++, "Button Arm (Fire 2): %s", dingooButtons[settings.pad_config[7]]);
		menu::set_item_optstr(menuItem++, "Numpad Key 1:        %s", dingooButtons[settings.pad_config[8]]);
		menu::set_item_optstr(menuItem++, "Numpad Key 2:        %s", dingooButtons[settings.pad_config[9]]);
		menu::set_item_optstr(menuItem++, "Numpad Key *:        %s", dingooButtons[settings.pad_config[10]]);
		menu::set_item_optstr(menuItem++, "Numpad Key #:        %s", dingooButtons[settings.pad_config[11]]);
		menu::set_item_optstr(menuItem++, "Numpad previous Key: %s", dingooButtons[settings.pad_config[12]]);
		menu::set_item_optstr(menuItem++, "Numpad next Key:     %s", dingooButtons[settings.pad_config[13]]);
	} else
	if(isGG()) {
		menu::set_item_optstr(menuItem++, "Button Start:        %s", dingooButtons[settings.pad_config[3]]);
		menu::set_item_optstr(menuItem++, "Button 1:            %s", dingooButtons[settings.pad_config[4]]);
		menu::set_item_optstr(menuItem++, "Button 2:            %s", dingooButtons[settings.pad_config[5]]);
	} else {
		menu::set_item_optstr(menuItem++, "Button 1 Start:      %s", dingooButtons[settings.pad_config[0]]);
		menu::set_item_optstr(menuItem++, "Button 2:            %s", dingooButtons[settings.pad_config[1]]);
		menu::set_item_optstr(menuItem++, "Button Pause:        %s", dingooButtons[settings.pad_config[2]]);
	}	
	menu::set_item_text(menuItem++,       "Back");
}

static void Menu_InputSets()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	int menuCount   = isColeco() ? 11 : 6;
	
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
				if(menufocus == 0) {
					settings.dpadDiagonals ^= true;
				} else {
					int index = menufocus - 2 + (isColeco() ? 6 : (isGG() ? 3 : 0));
					if(--settings.pad_config[index] > 12) settings.pad_config[index] = 12;
				}
				Menu_InputSets_Texts();
				break;
				
			case MA_INCREASE:
				if(menufocus == 0) {
					settings.dpadDiagonals ^= true;
				} else {
					int index = menufocus - 2 + (isColeco() ? 6 : (isGG() ? 3 : 0));
					if(++settings.pad_config[index] > 12) settings.pad_config[index] = 0;
				}
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
	menu::set_item_text(menuCount++, "             by lion_rsm              ");
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Sega Master System, Sega Game Gear and");
	menu::set_item_text(menuCount++, "ColecoVision emulator for Dingoo A320 ");
	menu::set_item_text(menuCount++, "native OS.                            ");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Start+Select - enter menu             ");
	menu::set_item_text(menuCount++, "R+Select     - save quick state       ");
	menu::set_item_text(menuCount++, "L+Start      - load quick state       ");
	menu::set_item_text(menuCount++, "L+R+Up       - increase sound volume  ");
	menu::set_item_text(menuCount++, "L+R+Down     - decrease sound volume  ");
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
