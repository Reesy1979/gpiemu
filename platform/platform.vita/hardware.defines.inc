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

#define HW_INPUT_INDEX_SELECT			0
#define HW_INPUT_INDEX_L3    			1
#define HW_INPUT_INDEX_R3    			2
#define HW_INPUT_INDEX_START			  3
#define HW_INPUT_INDEX_UP			    4
#define HW_INPUT_INDEX_RIGHT			  5
#define HW_INPUT_INDEX_DOWN			  6
#define HW_INPUT_INDEX_LEFT			  7
#define HW_INPUT_INDEX_L			      8
#define HW_INPUT_INDEX_R			      9
#define HW_INPUT_INDEX_L1			    10
#define HW_INPUT_INDEX_R1			    11
#define HW_INPUT_INDEX_TRIANGLE		12
#define HW_INPUT_INDEX_CIRCLE			13
#define HW_INPUT_INDEX_CROSS			  14
#define HW_INPUT_INDEX_SQUARE			15
#define HW_INPUT_INDEX_VOL_UP			19
#define HW_INPUT_INDEX_VOL_DOWN		20
#define HW_INPUT_INDEX_POWER       30

#define HW_INPUT_UP			(1<<HW_INPUT_INDEX_UP)
#define HW_INPUT_SELECT			(1<<HW_INPUT_INDEX_SELECT)
#define HW_INPUT_L3			(1<<HW_INPUT_INDEX_L3)
#define HW_INPUT_R3			(1<<HW_INPUT_INDEX_R3)
#define HW_INPUT_START			(1<<HW_INPUT_INDEX_START)
#define HW_INPUT_UP			(1<<HW_INPUT_INDEX_UP)
#define HW_INPUT_RIGHT			(1<<HW_INPUT_INDEX_RIGHT)
#define HW_INPUT_DOWN			(1<<HW_INPUT_INDEX_DOWN)
#define HW_INPUT_LEFT			(1<<HW_INPUT_INDEX_LEFT)
#define HW_INPUT_L			(1<<HW_INPUT_INDEX_L)
#define HW_INPUT_R			(1<<HW_INPUT_INDEX_R)
#define HW_INPUT_L1			(1<<HW_INPUT_INDEX_L1)
#define HW_INPUT_R1			(1<<HW_INPUT_INDEX_R1)
#define HW_INPUT_TRIANGLE			(1<<HW_INPUT_INDEX_TRIANGLE)
#define HW_INPUT_CIRCLE			(1<<HW_INPUT_INDEX_CIRCLE)
#define HW_INPUT_CROSS			(1<<HW_INPUT_INDEX_CROSS)
#define HW_INPUT_SQUARE			(1<<HW_INPUT_INDEX_SQUARE)
#define HW_INPUT_VOL_UP			(1<<HW_INPUT_INDEX_VOL_UP)
#define HW_INPUT_VOL_DOWN			(1<<HW_INPUT_INDEX_VOL_DOWN)
#define HW_INPUT_POWER			(1<<HW_INPUT_INDEX_POWER)
#define HW_INPUT_A  HW_INPUT_CIRCLE
#define HW_INPUT_B  HW_INPUT_CROSS
#define HW_INPUT_X  HW_INPUT_SQUARE

#define HW_MAX_PATH				512

#endif //_HARDWARE_DEFINES_H_
