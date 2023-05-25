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

#ifndef _HARDWARE_DEFINES_H_
#define _HARDWARE_DEFINES_H_

#define HW_SCREEN_WIDTH			320
#define HW_SCREEN_HEIGHT		240

#define HW_INPUT_INDEX_UP			15
#define HW_INPUT_INDEX_DOWN			17
#define HW_INPUT_INDEX_LEFT			18
#define HW_INPUT_INDEX_RIGHT			16
#define HW_INPUT_INDEX_A			 9
#define HW_INPUT_INDEX_B			 8
#define HW_INPUT_INDEX_X			 10
#define HW_INPUT_INDEX_Y			 7
#define HW_INPUT_INDEX_L			 5
#define HW_INPUT_INDEX_R			 6
#define HW_INPUT_INDEX_START			14
#define HW_INPUT_INDEX_SELECT			11
#define HW_INPUT_INDEX_POWER			0

#define HW_INPUT_UP			(1<<HW_INPUT_INDEX_UP)
#define HW_INPUT_DOWN			(1<<HW_INPUT_INDEX_DOWN)
#define HW_INPUT_LEFT			(1<<HW_INPUT_INDEX_LEFT)
#define HW_INPUT_RIGHT			(1<<HW_INPUT_INDEX_RIGHT)
#define HW_INPUT_A			(1<<HW_INPUT_INDEX_A)
#define HW_INPUT_B			(1<<HW_INPUT_INDEX_B)
#define HW_INPUT_X			(1<<HW_INPUT_INDEX_X)
#define HW_INPUT_Y			(1<<HW_INPUT_INDEX_Y)
#define HW_INPUT_L			(1<<HW_INPUT_INDEX_L)
#define HW_INPUT_R			(1<<HW_INPUT_INDEX_R)
#define HW_INPUT_START			(1<<HW_INPUT_INDEX_START)
#define HW_INPUT_SELECT			(1<<HW_INPUT_INDEX_SELECT)
#define HW_INPUT_POWER			(1<<HW_INPUT_INDEX_POWER)

#define HW_MAX_PATH				256

#endif //_HARDWARE_DEFINES_H_
