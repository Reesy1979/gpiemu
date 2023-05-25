/* DingooSNES - Super Nintendo Entertainment System Emulator for Dingoo A320
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

enum {
  AM_ROM_SELECT,
  AM_0,
	AM_VIDEO_SETS,
	AM_SOUND_SETS,
	AM_INPUT_SETS,
	AM_1,
	AM_CPU_SPEED,
	AM_FPS,
	AM_2,
	AM_SAVE_GLOBAL_SETTINGS,
	AM_SAVE_GAME_SETTINGS,
	AM_ABOUT,
	AM_BACK,
	AM_count
};

enum {
	VS_BUFFERING,
	VS_SCALING,
	VS_VER_STRETCH,
	VS_RENDERING_MODE,
	VS_BACK,
	VS_count
};

enum {
	SS_SAMPLE_RATE,
	SS_OUTPUT_MODE,
	SS_BACK,
	SS_count
};

enum {
	IS_DPAD_DIAGONALS,
	IS_0,
	IS_KEY_0,
	IS_KEY_1,
	IS_KEY_2,
	IS_KEY_3,
	IS_KEY_4,
	IS_KEY_5,
	IS_BACK,
	IS_count
};


///////////////////////////////////////////////////////////////////////////////

static int 	mCpuSpeedLookup[]  = { 300, 336, 370, 400, 430, 450 };
static int	mAudioRateLookup[] = { 11025, 22050, 32000, 44100 };

static void	Menu_VideoSets();
static void	Menu_SoundSets();
static void	Menu_InputSets();
static void	Menu_About();

gfx_texture* emul::menu_load_logo()
{
	return video::tex_load(png_logo, png_logo_size);
}

gfx_texture* emul::menu_load_gamepad()
{
	return video::tex_load(png_gamepad, png_gamepad_size);
}

void emul::menu_lock_sets()
{
	if(system::is_in_tv_mode()) {
		settings.verStretch = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//	Advanced menu
///////////////////////////////////////////////////////////////////////////////

static void Menu_Advanced_Texts()
{
	char* showFPSValues[3] = { STR_OFF, "RENDERED", "EMULATED" };

	menu::set_title(MENU_TITLE_TEXT);
	menu::set_item_text  (AM_ROM_SELECT,           "Rom Selector");
	menu::set_separator  (AM_0);
	menu::set_item_text  (AM_VIDEO_SETS,           "Video settings");
	menu::set_item_text  (AM_SOUND_SETS,           "Sound settings");
	menu::set_item_text  (AM_INPUT_SETS,           "Input settings");
	menu::set_separator  (AM_1);
	menu::set_item_optint(AM_CPU_SPEED,            "CPU speed, MHz:      %d", settings.cpuSpeed);
	menu::set_item_optstr(AM_FPS,                  "Show FPS mode:       %s", showFPSValues[settings.showFps]);
	menu::set_separator  (AM_2);
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
					case AM_SOUND_SETS:
						Menu_SoundSets();
						break;
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
	menu::set_item_optstr(VS_BUFFERING,		  "Double buffering:    %s", settings.bufferingMode ? "SLOW" : "FAST");
	menu::set_item_optstr(VS_SCALING,         "Fullscreen scaling:  %s", settings.scalingMode ? "HARDWARE" : "SOFTWARE");
	menu::set_item_optstr(VS_VER_STRETCH,     "Vertical stretch:    %s", settings.verStretch ? STR_ON : STR_OFF);
	menu::set_item_optstr(VS_RENDERING_MODE,  "Rendering mode:      %s", settings.renderingMode ? "ACCURATE" : "FAST");
	menu::set_item_text  (VS_BACK,            "Back");
	
	menu::set_item_enable(VS_BUFFERING, !settings.fullScreen);
	menu::set_item_enable(VS_SCALING, settings.fullScreen);
	menu::set_item_enable(VS_VER_STRETCH, settings.fullScreen && !system::is_in_tv_mode());
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
					case VS_BUFFERING:
						settings.bufferingMode ^= true;
						break;
						
					case VS_SCALING:
						settings.scalingMode ^= true;
						break;
						
					case VS_VER_STRETCH:
						settings.verStretch ^= true;
						break;

					case VS_RENDERING_MODE:
						settings.renderingMode ^= true;
						break;
				}
				Menu_VideoSets_Texts();
				break;

			case MA_INCREASE:
				switch(menufocus)
				{
					case VS_BUFFERING:
						settings.bufferingMode ^= true;
						break;
				
					case VS_SCALING:
						settings.scalingMode ^= true;
						break;
						
					case VS_VER_STRETCH:
						settings.verStretch ^= true;
						break;

					case VS_RENDERING_MODE:
						settings.renderingMode ^= true;
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
	menu::set_item_optstr(SS_OUTPUT_MODE, "Output mode:         %s", settings.soundStereo ? "STEREO" : "MONO");
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
						settings.soundRate = menu::prev_val(mAudioRateLookup, 4, settings.soundRate);
						break;
						
					case SS_OUTPUT_MODE:
						settings.soundStereo ^= true;	
						break;
				}
				Menu_SoundSets_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case SS_SAMPLE_RATE:
						settings.soundRate = menu::next_val(mAudioRateLookup, 4, settings.soundRate);
						break;
						
					case SS_OUTPUT_MODE:
						settings.soundStereo ^= true;	
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
	
	menu::set_title("Input settings");
	menu::set_item_optstr(IS_DPAD_DIAGONALS,  "Diagonals helper:    %s", (settings.dpadDiagonals ? STR_ON : STR_OFF));
	menu::set_separator  (IS_0);
	menu::set_item_optstr(IS_KEY_0,	          "L shift:             %s", dingooButtons[settings.pad_config[0]]);
	menu::set_item_optstr(IS_KEY_1,	          "R shift:             %s", dingooButtons[settings.pad_config[1]]);
	menu::set_item_optstr(IS_KEY_2,	          "Button A:            %s", dingooButtons[settings.pad_config[2]]);
	menu::set_item_optstr(IS_KEY_3,	          "Button B:            %s", dingooButtons[settings.pad_config[3]]);
	menu::set_item_optstr(IS_KEY_4,	          "Button X:            %s", dingooButtons[settings.pad_config[4]]);	
	menu::set_item_optstr(IS_KEY_5,	          "Button Y:            %s", dingooButtons[settings.pad_config[5]]);
	menu::set_item_text  (IS_BACK,            "Back");
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
					case IS_DPAD_DIAGONALS:
						settings.dpadDiagonals ^= true;
						break;

					case IS_KEY_0: case IS_KEY_1: case IS_KEY_2: case IS_KEY_3:
					case IS_KEY_4: case IS_KEY_5:
						if(--settings.pad_config[menufocus - IS_KEY_0] > 12) settings.pad_config[menufocus - IS_KEY_0] = 12;
						break;
				}
				Menu_InputSets_Texts();
				break;
				
			case MA_INCREASE:
				switch(menufocus)
				{
					case IS_DPAD_DIAGONALS:
						settings.dpadDiagonals ^= true;
						break;
										
					case IS_KEY_0: case IS_KEY_1: case IS_KEY_2: case IS_KEY_3:
					case IS_KEY_4: case IS_KEY_5:
						if(++settings.pad_config[menufocus - IS_KEY_0] > 12) settings.pad_config[menufocus - IS_KEY_0] = 0;
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
#if VER == 143
#define CORE_VERSION "accurate"
#else
#define CORE_VERSION "fast"
#endif

static void Menu_About()
{
	int menuExit	= 0;
	int menufocus	= 0;
	int menuCount   = 0;
	
	menu::open();
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "       "   MENU_TITLE_TEXT    "       ");
	menu::set_item_text(menuCount++, "             by lion_rsm              ");
	menu::set_item_text(menuCount++, "======================================");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Super Nintendo Enterteinment System   ");
	menu::set_item_text(menuCount++, "emulator for Dingoo A320 native OS.   ");
	menu::set_item_text(menuCount++, "Used " CORE_VERSION " emulation core. ");
	menu::set_item_text(menuCount++, "$");
	menu::set_item_text(menuCount++, "Start+Select - enter menu             ");
	menu::set_item_text(menuCount++, "R+Select     - save quick state       ");
	menu::set_item_text(menuCount++, "L+Start      - load quick state       ");
	menu::set_item_text(menuCount++, "L+R+Up       - increase sound volume  ");
	menu::set_item_text(menuCount++, "L+R+Down     - decrease sound volume  ");
	menu::set_item_text(menuCount++, "L+R+Right    - fast forward           ");
	menu::set_item_text(menuCount++, "L+R+Left     - switch rendering mode  ");
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
