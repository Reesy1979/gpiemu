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

#include "framework.h"
#include "systype.h"
#include "shared.h"

using namespace fw;

///////////////////////////////////////////////////////////////////////////////

#define GAMEPAD_U  	INPUT_UP
#define GAMEPAD_D  	INPUT_DOWN
#define GAMEPAD_L  	INPUT_LEFT
#define GAMEPAD_R  	INPUT_RIGHT

#define GAMEPAD_1  	INPUT_BUTTON1
#define GAMEPAD_2  	INPUT_BUTTON2
#define GAMEPAD_ST 	INPUT_START
#define GAMEPAD_PA 	INPUT_PAUSE

#define KEYBOARD_1  1
#define KEYBOARD_2  2
#define KEYBOARD_S  10
#define KEYBOARD_P  11

///////////////////////////////////////////////////////////////////////////////

static int			VBuffOffset;
static uint32_t		lastDPad;

static bool keyboardKeyVisible;
static int  keyboardKeyIndex;

///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

///////////////////////////////////////////////////////////////////////////////

bool core::init()
{
	return true;
}

void core::close()
{
	system_poweroff();
}

extern int load_rom(char *filename);

bool core::load_rom()
{
	debug::printf("core::load_rom 1\n");
	if(system_load_rom((char*)fsys::getGameFilePath())) {
		if(settings.countryRegion) {
			sms.display   = (settings.countryRegion == 2) ? DISPLAY_PAL : DISPLAY_NTSC;
			sms.territory = (settings.countryRegion == 3) ? TERRITORY_DOMESTIC : TERRITORY_EXPORT;
		}
		return true;
	}
	debug::printf("core::load_rom 2\n");
	return false;
}

extern "C" void sys_sanitize(char *rom)
{
}
void core::hard_reset()
{
	//	init bitmap
	bitmap.width  = HW_SCREEN_WIDTH;
	bitmap.height = HW_SCREEN_HEIGHT;
	bitmap.depth  = 16;
	bitmap.pitch  = bitmap.width * (bitmap.depth >> 3);

	//	init sound
	sms.use_fm		= 0;
	snd.fm_which 	= SND_YM2413;
	//snd.fm_which 	= SND_EMU2413;
	snd.sample_rate	= settings.soundRate;

	//	init system components
	system_poweron();
}

void core::soft_reset()
{
	system_reset();
}

uint32_t core::system_fps()
{
	return snd.fps;
}

void core::start_emulation(bool withSound)
{
	InitVideoSystem();
	InitSoundSystem(withSound);
}



void core::emulate_frame(bool& withRendering, int soundBuffer)
{
	UpdateInput();

	bitmap.data = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
	
	system_frame(!withRendering);

	if(withRendering && bitmap.viewport.changed) {
		bitmap.viewport.changed = 0;
		hw::video::clear_all(false);
		InitVideoSystem();
	}

	if(soundBuffer >= 0)
	{
		hw::sound::fill_s16(soundBuffer, snd.output, snd.sample_count);
	}
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	VBuffOffset = -(bitmap.viewport.y * HW_SCREEN_WIDTH + bitmap.viewport.x);
	bitmap.data = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
	system_frame(0);
	console_scr_w = bitmap.viewport.w;
	console_scr_h = bitmap.viewport.h;
}

void core::stop_emulation(bool withSound)
{
	debug::printf("stop_emulation start\n");
	if(withSound) 
		hw::sound::close();
	debug::printf("stop_emulation end\n");
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{
	if(state::useFile(filePath) || state::useMemory(memBuffer)) 
	{
		if(state::fopen(load ? "rb" : "wb")) 
		{
			if(load) system_load_state(); else system_save_state();
			state::fclose();
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem()
{
	if(settings.fullScreen) {
		hw::video::set_mode(settings.scalingMode ? hw::VM_SCALE_HW : hw::VM_SCALE_SW, bitmap.viewport.w, bitmap.viewport.h, true);
		VBuffOffset = -(bitmap.viewport.y * HW_SCREEN_WIDTH + bitmap.viewport.x);
	} else {
		hw::video::set_mode(hw::VM_FAST, bitmap.viewport.w, bitmap.viewport.h, true);
		VBuffOffset = ((HW_SCREEN_WIDTH - 256) >> 1) + ((HW_SCREEN_HEIGHT - ((isGG()) ? 192 :  bitmap.viewport.h)) >> 1) * HW_SCREEN_WIDTH;
	}
}

static void InitSoundSystem(bool hasSound)
{
	if(hasSound) 
		hw::sound::init(settings.soundRate, true, core::system_fps());
}

static void printSelectedKey()
{
	cstr_t KEYBOARD_KEYS = "0123456789*#";
	char message[15];

	int src = 0, dst = 0; 
	while(src < 12) {
		if(src == keyboardKeyIndex) {
			message[dst++] = '(';
			message[dst++] = KEYBOARD_KEYS[src++];
			message[dst++] = ')';
		} else
			message[dst++] = KEYBOARD_KEYS[src++];
	}
	message[dst] = 0;

	hw::video::set_osd_msg(message, 45);
	keyboardKeyVisible = true;
}

static void UpdateInput()
{
	#define GAMEPAD  input.pad[0]
	#define BUTTONS  input.system

	uint32_t controls = fw::emul::handle_input();
	GAMEPAD  = 0x00;
	BUTTONS  = 0x00;
	
	if(controls)
	{
        if(controls & HW_INPUT_UP   ) GAMEPAD |= GAMEPAD_U;
		if(controls & HW_INPUT_DOWN ) GAMEPAD |= GAMEPAD_D;
		if(controls & HW_INPUT_LEFT ) GAMEPAD |= GAMEPAD_L;
		if(controls & HW_INPUT_RIGHT) GAMEPAD |= GAMEPAD_R;

		if(isGG()) {
			if (controls & hw::input::get_mask(settings.pad_config[3])) BUTTONS |= GAMEPAD_ST;
			if (controls & hw::input::get_mask(settings.pad_config[4])) GAMEPAD |= GAMEPAD_1;
			if (controls & hw::input::get_mask(settings.pad_config[5])) GAMEPAD |= GAMEPAD_2;
		} else {
			if (controls & hw::input::get_mask(settings.pad_config[0])) GAMEPAD |= GAMEPAD_1;
			if (controls & hw::input::get_mask(settings.pad_config[1])) GAMEPAD |= GAMEPAD_2;
			if (controls & hw::input::get_mask(settings.pad_config[2])) BUTTONS |= GAMEPAD_PA;
		}
        
	}
		
	//	D-Pad diagonals helper algorithm
	if(settings.dpadDiagonals)
	{
		const int GAMEPAD_UD = (GAMEPAD_U | GAMEPAD_D);
		const int GAMEPAD_LR = (GAMEPAD_L | GAMEPAD_R);
		if(((lastDPad & GAMEPAD_UD) && (GAMEPAD & GAMEPAD_LR)) || ((lastDPad & GAMEPAD_LR) && (GAMEPAD & GAMEPAD_UD))) {
			GAMEPAD |= lastDPad;
		} else {
			lastDPad = GAMEPAD & (GAMEPAD_UD | GAMEPAD_LR);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void system_manage_sram(uint8_t *sram, int slot, int mode)	//	TODO: need checking
{
	cstr_t name = fsys::getGameRelatedFilePath(SRAM_FILE_EXT);
    FILE *fd;

    switch(mode) {
    case SRAM_SAVE:
        if(sms.save) {
            fd = fopen(name, "wb");
            if(fd) {
                fwrite(sram, 0x8000, 1, fd);
                fclose(fd);
            }
        }
        break;

    case SRAM_LOAD:
        fd = fopen(name, "rb");
        if(fd) {
            sms.save = 1;
            fread(sram, 0x8000, 1, fd);
            fclose(fd);
        } else {
            memset(sram, 0x00, 0x8000);
        }
        break;
    }
}
