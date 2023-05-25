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

#define HW_INPUT_INDEX_NONE		0
#define HW_INPUT_INDEX_UP		1
#define HW_INPUT_INDEX_DOWN		2
#define HW_INPUT_INDEX_LEFT		3
#define HW_INPUT_INDEX_RIGHT	4
#define HW_INPUT_INDEX_A		5
#define HW_INPUT_INDEX_B		6
#define HW_INPUT_INDEX_X		7
#define HW_INPUT_INDEX_Y		8
#define HW_INPUT_INDEX_L		9
#define HW_INPUT_INDEX_R		10
#define HW_INPUT_INDEX_SELECT	11
#define HW_INPUT_INDEX_START	12
#define HW_INPUT_INDEX_POWER	13

#define HW_INPUT_BIT_UP			20
#define HW_INPUT_BIT_DOWN		27
#define HW_INPUT_BIT_LEFT		28
#define HW_INPUT_BIT_RIGHT		18
#define HW_INPUT_BIT_A			31
#define HW_INPUT_BIT_B			21
#define HW_INPUT_BIT_X			16
#define HW_INPUT_BIT_Y			6
#define HW_INPUT_BIT_L			8
#define HW_INPUT_BIT_R			29
#define HW_INPUT_BIT_SELECT		10
#define HW_INPUT_BIT_START		11
#define HW_INPUT_BIT_POWER		7

#define HW_INPUT_UP				(1 << HW_INPUT_BIT_UP)
#define HW_INPUT_DOWN			(1 << HW_INPUT_BIT_DOWN)
#define HW_INPUT_LEFT			(1 << HW_INPUT_BIT_LEFT)
#define HW_INPUT_RIGHT			(1 << HW_INPUT_BIT_RIGHT)
#define HW_INPUT_A				(1 << HW_INPUT_BIT_A)
#define HW_INPUT_B				(1 << HW_INPUT_BIT_B)
#define HW_INPUT_X				(1 << HW_INPUT_BIT_X)
#define HW_INPUT_Y				(1 << HW_INPUT_BIT_Y)
#define HW_INPUT_L				(1 << HW_INPUT_BIT_L)
#define HW_INPUT_R				(1 << HW_INPUT_BIT_R)
#define HW_INPUT_SELECT			(1 << HW_INPUT_BIT_SELECT)
#define HW_INPUT_START			(1 << HW_INPUT_BIT_START)
#define HW_INPUT_POWER			(1 << HW_INPUT_BIT_POWER)

#define HW_MAX_PATH				256

#endif //_HARDWARE_DEFINES_H_
