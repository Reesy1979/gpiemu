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

#define SCREENSHOT_FILENAME	EMU_FILE_NAME
#define SCREENSHOT_EXT		"tga"

static bool		mEnterMenu;
static int 		mRenFPS;
static int 		mEmuFPS;
static int      mFPSValue;
static uint32_t mLastTimer;
static int 		mPendingRequest;
static bool		mInFastForward;	   

static MemBuffer quickState;

enum {
	REQUEST_NOTHING,
	REQUEST_SAVE_STATE,
	REQUEST_LOAD_STATE,	
};

////////////////////////////////////////////////////////////////////////////////
//	Private helper routines
////////////////////////////////////////////////////////////////////////////////

static bool loadGame()
{
	debug::printf("Here 3\n");
	menu::msg_box("Loading...", MMB_MESSAGE);
	debug::printf("Here 4\n");
	if (core::load_rom())
	{
		debug::printf("Here 5\n");
		core::hard_reset();
		debug::printf("Here 6\n");
		return true;
	}
	debug::printf("Here 7\n");
	menu::msg_box("Loading of the game is FAILED!", MMB_PAUSE);
	return false;
}

static void loadSaveOldSchoolModeState(bool load)
{
	if(settings.oldSchoolMode) {
		if(!load) menu::msg_box("Save game state...", MMB_MESSAGE);
		cstr_t stateFilePath = fsys::getGameRelatedFilePath(SAVESTATE_EXT);
		core::save_load_state(stateFilePath, NULL, load);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void flipFramebuffer()
{
	uint32_t newTimer;
	if (settings.showFps) {
		mRenFPS++;
		if((newTimer = system::get_frames_count()) - mLastTimer >= core::system_fps()) 
		{
			mLastTimer = newTimer;
			mFPSValue = settings.showFps > 1 ? mEmuFPS : mRenFPS;
			mEmuFPS = mRenFPS = 0;
#ifdef _COUNT_FPS
			if((mFPSValue = debug::add_fps_value(mFPSValue)) == 100)
			{
				debug::show_fps_statistic();
				mEnterMenu = true;
			}
#endif
		}
		video::flip(mFPSValue);
	} else {
		video::flip(0);
	}
}

static void printSoundVolume()
{
	char message[] = "Sound volume:           ";
	for(int i = 0; i < settings.soundVolume; i++) message[13 + i] = 0xDE;
	video::set_osd_msg(message, 60);
}

static void handleQuickRequests()
{
	if(mPendingRequest == REQUEST_SAVE_STATE) core::save_load_state(NULL, &quickState, false);
	if(mPendingRequest == REQUEST_LOAD_STATE) core::save_load_state(NULL, &quickState, true );
	mPendingRequest = REQUEST_NOTHING;
}

///////////////////////////////////////////////////////////////////////////////

static void runFastForward()
{
	bool render;
	for(int i = 0; i < 9; i++) 
	{
		render = false;
		core::emulate_frame(render, -1);
		mEmuFPS++;
	}
}

static void emulateWithSound()
{
  	int  skip		= 0;
	int  done		= 0;
	int  doneLast	= 0;
	int  aim		= 0;
	bool render     = 0;
	
  	while(!mEnterMenu) 
  	{
		for (int i = 0; i < 100; i++)
		{
			aim = sound::prev_buffer_index();
			if (done != aim)
			{
				doneLast = done;
				done = sound::next_buffer_index(done);
				if(settings.frameSkip == 0)
				{
					if(done == aim) {
						render = true;
					} else {         
						render = false;
					}
				} else {
					if(--skip <= 0) {
						skip = settings.frameSkip;
						render = true;
					} else {
						render = false;
					}
				}
		
				if(mInFastForward)
				{
					runFastForward();
					render = true;
				}

				core::emulate_frame(render, doneLast);
				if(render) flipFramebuffer();
				handleQuickRequests();
				mEmuFPS++;
			}
			if (done == aim) break;
			if (mEnterMenu ) break;
		}
		done = aim;
  	}

	mEnterMenu = false;
}

static void emulateWithoutSound()
{
  	int  skip		= 0;
	int  done		= 0;
	int  aim		= 0;
	bool render     = 0;
	
	done = system::get_frames_count() - 1;
  	while(!mEnterMenu) 
  	{
		for (int i = 0; i < 10; i++)
		{
			aim = system::get_frames_count();
			if (done != aim)
			{
				done++;
				if(settings.frameSkip == 0)
				{
					if(done == aim) {
						render = true;
					} else {
						render = false;
					}
				} else {
					if(--skip <= 0) {
						skip = settings.frameSkip;
						render = true;
					} else {
						render = false;
					}
				}
		
				if(mInFastForward)
				{
					runFastForward();
					render = true;
				}

				core::emulate_frame(render, -1);
				if(render) flipFramebuffer();
				handleQuickRequests();
				mEmuFPS++;
			}
			if (done == aim) break;
			if (mEnterMenu ) break;
		}
		done = aim;
  	}

	mEnterMenu = false;
}

////////////////////////////////////////////////////////////////////////////////
//	Main emulator loop
////////////////////////////////////////////////////////////////////////////////
//extern void KernelLog(char *msg);
#if defined(_LAUNCHER)
#include "launcher.h"
int emul::entry_point(int argc, char* argv[])
{
	debug::printf("Here 1\n");
	system::init();
	debug::printf("Here 2\n");
	fsys::setDirectories(argv[0], argv[1]);
	debug::printf("Here 3\n");
	menu_init();
	launcher::open();
	system::close();
  	return 0;
}
#else
int emul::entry_point(int argc, char* argv[])
{

	EVENT event = EVENT_NONE;
	system::init();
	fsys::setDirectories(argv[0], argv[1]);
	
	menu_init();
	//if(input::poll() & HW_INPUT_SELECT) menu_extra();
	if(core::init())
	{
		while(true)
		{
			event = menu_main();
			if(event == EVENT_LOAD_ROM) 
			{
				debug::printf("Here 1\n");
				if(loadGame()) 
				{
					debug::printf("Here 2\n");
					loadSaveOldSchoolModeState(true);
					event = EVENT_RUN_ROM;
				} else 
				{
					settings.compressROM = false;
					break;
				}
			}

			if(event == EVENT_RESET_ROM) 
			{				
				core::soft_reset();
				event = EVENT_RUN_ROM;
			}

			
			
			if(event == EVENT_RUN_ROM) {

				//	set cpu speed and sound volume
				bool emulationWithSound = settings.soundVolume > 0;
				system::set_cpu_speed(settings.cpuSpeed);
				sound::set_volume(settings.soundVolume);

				//	clear all frame buffers and prepare to emulation
				video::clear_all(true);
				core::start_emulation(emulationWithSound);
				system::init_frames_counter(core::system_fps());

				//	emulation loop
				if(emulationWithSound) 
					emulateWithSound(); 
				else 
					emulateWithoutSound();

				//	cleanup after emulation
				core::stop_emulation(emulationWithSound);
				event = EVENT_NONE;
			}

			if(event == EVENT_EXIT_APP) 
			{
				loadSaveOldSchoolModeState(false);
				break;
			}

			
		}
		quickState.free();
		core::close();
	}
	menu_close();
	system::close();
  	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
//	Callbacks section (called from client code)
////////////////////////////////////////////////////////////////////////////////

bool emul::get_screenshot_filename(char *filename)
{	
	for(int i = 1; i <= 9; i++)
	{
		sprintf(filename, "%s%s%s%d.%s", fsys::getEmulPath(), DIR_SEP, SCREENSHOT_FILENAME, i, SCREENSHOT_EXT);
		if(!fsys::exists(filename)) return true;
	}
	return false;
}

uint32_t emul::handle_input()
{
	uint32_t controls = input::read();
	
	//	exit to emulator menu using Start+Select
	if (controls == (HW_INPUT_SELECT | HW_INPUT_START) || controls == HW_INPUT_POWER)	
	{
		mEnterMenu = true;
		return 0;
	}

	//	increase volume
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_UP))
	{
		if(video::has_osd_msg() < 45 && settings.soundVolume)
		{
			if(++settings.soundVolume > 10) settings.soundVolume = 10;			
			sound::set_volume(settings.soundVolume);
			printSoundVolume();
		}
		return 0;
	}

	//	decrease volume
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_DOWN))
	{
		if(video::has_osd_msg() < 45 && settings.soundVolume)
		{
			if(--settings.soundVolume < 1) settings.soundVolume = 1;			
			sound::set_volume(settings.soundVolume);
			printSoundVolume();
		}
		return 0;
	}

	//	fast forward emulation
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_RIGHT))
	{
		if (video::has_osd_msg() < (mInFastForward ? 15 : 1))
		{
			video::set_osd_msg("Fast forward", 60);
			mInFastForward = true;			
		}
		return 0;	
	} else {
		mInFastForward = false;
	}

	//	quick save state
	if (controls == (HW_INPUT_SELECT | HW_INPUT_R))
	{
		if (!video::has_osd_msg() && !settings.oldSchoolMode)
		{
			video::set_osd_msg("Save quick state", 60);
			mPendingRequest = REQUEST_SAVE_STATE;
		}
		return 0;
	}

	//	quick state load
	if (controls == (HW_INPUT_START | HW_INPUT_L))
	{
		if (!video::has_osd_msg() && !settings.oldSchoolMode && quickState.getSize() > 0)
		{
			video::set_osd_msg("Load quick state", 60);
			mPendingRequest = REQUEST_LOAD_STATE;
		}
		return 0;
	}

	return controls;
}
