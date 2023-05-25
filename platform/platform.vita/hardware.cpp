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
#include <stdio.h>
#include <cstring>
#include "hardware.h"
#include "memstream.h"
#include <psp2/ctrl.h>
#include <psp2/rtc.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/audioout.h>
#include <psp2/io/fcntl.h>
#include <vita2d.h>

using namespace hw;

///////////////////////////////////////////////////////////////////////////////

static uint32_t oneFrameTime;

#include "hardware.video.inc"
#include "hardware.sound.inc"
#include "hardware.input.inc"

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
  uint64_t t;
  sceRtcGetCurrentTick((SceRtcTick*)&t);
	return t / oneFrameTime;
}

void system::set_cpu_speed(int)
{
	//	DO NOTHING
}

bool system::is_in_tv_mode()
{
	return false;
}
