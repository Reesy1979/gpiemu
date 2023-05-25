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

#include <stdlib.h>
#include <stdio.h>
#include "hardware.h"
#include "memstream.h"

using namespace hw;

//	Dingoo A320 OS specific functions
extern "C" void  	_lcd_set_frame();
extern "C" void* 	_lcd_get_frame();
extern "C" uint32_t kbd_get_key();
extern "C" uint32_t clock();
extern "C" void 	cpu_clock_set(uint32_t);

///////////////////////////////////////////////////////////////////////////////

static uint32_t 	oneFrameTime;
static void 		_check_for_save_screenshot(uint32_t input);
extern bool 		get_screenshot_filename(char *filename);

#include "hardware.video.inc"
#include "hardware.sound.inc"
#include "hardware.input.inc"

static void _check_for_save_screenshot(uint32_t input)
{
	static bool isChecked = false;
	if((input & HW_INPUT_L) && (input & HW_INPUT_R) && (input & HW_INPUT_POWER)) {
		if(!isChecked) {
			//	prepare path and file name for screenshot
			char filename[HW_MAX_PATH];
			if(!::get_screenshot_filename(filename)) return;
			
			//	bind framebuffer and save to TGA file
			_bind_framebuffer((gfx_color*)_lcd_get_frame());
			gfx_tex_save_tga(filename, fb_gfx);
			
			//	term framebuffer and setup checked flag
			_term_framebuffer();
			isChecked = true;
		}
	} else {
		isChecked = false;
	}
}

///////////////////////////////////////////////////////////////////////////////

void system::init()
{
	init_frames_counter(60);
	video::init(VM_FAST);
	input::init();
}

void system::close()
{
	sound::close();
	video::close();
}

void system::init_frames_counter(int framesPerSecond)
{
	oneFrameTime = 1000000 / framesPerSecond;
}

uint32_t system::get_frames_count()
{
	return clock() / oneFrameTime;
}

void system::set_cpu_speed(int mhz)
{
	cpu_clock_set(uint32_t(mhz * 1000000));
}

bool system::is_in_tv_mode()
{
	return bool(isTVON());
}
