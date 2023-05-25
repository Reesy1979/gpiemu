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
 
#include <png.h>
#include <SDL.h>
#include <stdio.h>
#include "hardware.h"
#include "memstream.h"

#define WINDOW_TITLE_TEXT EMU_DISPLAY_NAME " v" EMU_VERSION

using namespace hw;

///////////////////////////////////////////////////////////////////////////////

static uint32_t oneFrameTime;

#include "hardware.video.inc"
#include "hardware.sound.inc"
#include "hardware.input.inc"

///////////////////////////////////////////////////////////////////////////////

void system::init()
{
	if((SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)) {
		printf("Could not initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
	SDL_WM_SetCaption(WINDOW_TITLE_TEXT, NULL);

	init_frames_counter(60);
	video::init(VM_FAST);
	input::init();
}

void system::close()
{
	sound::close();
	video::close();

	SDL_Quit();
}

void system::init_frames_counter(int framesPerSecond)
{
	oneFrameTime = 1000 / framesPerSecond;
}

uint32_t system::get_frames_count()
{
	return SDL_GetTicks() / oneFrameTime;
}

void system::set_cpu_speed(int)
{
	//	DO NOTHING
}

bool system::is_in_tv_mode()
{
	return false;
}
