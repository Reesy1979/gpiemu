/* DingooNES - Nintendo Entertainment System, Famicom, Dendy Emulator for Dingoo A320
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
 
#include "png_logo.h"
#include "png_gamepad.h"
#include "framework.h"

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
	VS_VER_STRETCH,
	VS_BACK,
	VS_count
};

enum {
	SS_SAMPLE_RATE,
	SS_BACK,
	SS_count
};

enum {
	IS_ZAPPER_ENABLE,
	IS_ZAPPER_AUTO_AIM,
	IS_DPAD_DIAGONALS,
	IS_TURBO_SIGNAL,
	IS_0,
	IS_KEY_ST,
	IS_KEY_SL,
	IS_KEY_A,
	IS_KEY_B,
	IS_KEY_TA,
	IS_KEY_TB,
	IS_BACK,
	IS_count
};

///////////////////////////////////////////////////////////////////////////////

static int 	mCpuSpeedLookup[]  = { 300, 336, 370, 400, 430, 450 };
static int 	mAudioRateLookup[] = { 11025, 22050, 44100 };

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
	settings.verStretch = false;
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
	
	char mode[] = "0:0"; extern uint8_t autoFirePattern[15];
	mode[0] += (autoFirePattern[settings.turboSignal] & 0xF0) >> 4;
	mode[2] += (autoFirePattern[settings.turboSignal] & 0x0F);
	
	menu::set_title("Input settings");
	menu::set_item_optstr(IS_ZAPPER_ENABLE,	  "NES Zapper enable:   %s", (settings.zapperEnable  ? STR_ON : STR_OFF));
	menu::set_item_optstr(IS_ZAPPER_AUTO_AIM, "NES Zapper auto aim: %s", (settings.zapperAutoAim ? STR_ON : STR_OFF));
	menu::set_item_optstr(IS_DPAD_DIAGONALS,  "Diagonals helper:    %s", (settings.dpadDiagonals ? STR_ON : STR_OFF));
	menu::set_item_optstr(IS_TURBO_SIGNAL,    "Turbo signal mode:   %s", mode);
	menu::set_separator  (IS_0);
	menu::set_item_optstr(IS_KEY_ST,          "Button Start:        %s", dingooButtons[settings.pad_config[0]]);
	menu::set_item_optstr(IS_KEY_SL,          "Button Select:       %s", dingooButtons[settings.pad_config[1]]);
	menu::set_item_optstr(IS_KEY_A,           "Button A:            %s", dingooButtons[settings.pad_config[2]]);
	menu::set_item_optstr(IS_KEY_B,           "Button B:            %s", dingooButtons[settings.pad_config[3]]);
	menu::set_item_optstr(IS_KEY_TA,          "Button Turbo A:      %s", dingooButtons[settings.pad_config[4]]);
	menu::set_item_optstr(IS_KEY_TB,          "Button Turbo B:      %s", dingooButtons[settings.pad_config[5]]);
	menu::set_item_text  (IS_BACK,            "Back");
	
	extern bool nesZapperAvailable;
	for(int i = IS_ZAPPER_ENABLE; i <= IS_ZAPPER_AUTO_AIM; i++)
		menu::set_item_enable(i, nesZapperAvailable);

	extern bool	nesGamepadAvailable;
	for(int i = IS_KEY_ST; i <= IS_KEY_TB; i++)
		menu::set_item_enable(i, nesGamepadAvailable);
}

static void Menu_InputSets()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	
	menu::open();
	Menu_InputSets_Texts();
	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_menu(IS_count, menufocus);
		video::term_framebuffer();
		video::flip(0);
		
		switch(menu::update(IS_count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;

			case MA_DECREASE:
				switch(menufocus)
				{
					case IS_ZAPPER_ENABLE:
						settings.zapperEnable ^= true;
						break;
						
					case IS_ZAPPER_AUTO_AIM:
						settings.zapperAutoAim ^= true;
						break;
						
					case IS_DPAD_DIAGONALS:
						settings.dpadDiagonals ^= true;
						break;
						
					case IS_TURBO_SIGNAL:
						if(--settings.turboSignal > 14) settings.turboSignal = 14;
						break;

					case IS_KEY_ST: case IS_KEY_SL: case IS_KEY_A: case IS_KEY_B: case IS_KEY_TA: case IS_KEY_TB:
						if(--settings.pad_config[menufocus - IS_KEY_ST] > 12) settings.pad_config[menufocus - IS_KEY_ST] = 12;
						break;
				}
				Menu_InputSets_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case IS_ZAPPER_ENABLE:
						settings.zapperEnable ^= true;
						break;
						
					case IS_ZAPPER_AUTO_AIM:
						settings.zapperAutoAim ^= true;
						break;
						
					case IS_DPAD_DIAGONALS:
						settings.dpadDiagonals ^= true;
						break;
						
					case IS_TURBO_SIGNAL:
						if(++settings.turboSignal > 14) settings.turboSignal = 0;
						break;
				
					case IS_KEY_ST: case IS_KEY_SL: case IS_KEY_A: case IS_KEY_B: case IS_KEY_TA: case IS_KEY_TB:
						if(++settings.pad_config[menufocus - IS_KEY_ST] > 12) settings.pad_config[menufocus - IS_KEY_ST] = 0;
						break;
				}
				Menu_InputSets_Texts();
				break;
				
			case MA_SELECT:
				switch(menufocus)
				{	
					case IS_BACK:
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
	menu::set_item_text(menuCount++, "       "   MENU_TITLE_TEXT    "       ");
	menu::set_item_text(menuCount++, "             by lion_rsm              ");
	menu::set_item_text(menuCount++, "      Ported to the GPI by Reesy      ");
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Nintendo Enterteinment System, Famicom");
	menu::set_item_text(menuCount++, "and Dendy emulator for GPI bare metal ");
	menu::set_item_text(menuCount++, "OS.                                   ");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Start+Select - enter menu             ");
	menu::set_item_text(menuCount++, "R+Select     - save quick state       ");
	menu::set_item_text(menuCount++, "L+Start      - load quick state       ");
	menu::set_item_text(menuCount++, "L+R+Right    - fast forward           ");
	menu::set_item_text(menuCount++, "L+R+Select   - switch disk side       ");
	menu::set_item_text(menuCount++, "L+R+Start    - insert coin            ");
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
