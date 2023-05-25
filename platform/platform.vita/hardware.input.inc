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
 
static uint32_t inputRepeat;
static uint32_t inputHeld;
static uint32_t inputIgnore;
static uint32_t inputFirst;
static uint32_t inputRepeatTimer[32];

////////////////////////////////////////////////////////////////////////////////

void input::init()
{
	inputRepeat	= 0;
	inputHeld	= 0;
	inputIgnore	= 0;
	inputFirst 	= 0;
	memset(inputRepeatTimer, 0, sizeof(inputRepeatTimer));

	printf("INPUT KEY MAPPING\n");
	printf("Dingoo UP     : Up\n");
	printf("Dingoo DOWN   : Down\n");
	printf("Dingoo LEFT   : Left\n");
	printf("Dingoo RIGHT  : Right\n");
	printf("Dingoo A      : X\n");
	printf("Dingoo B      : Z\n");
	printf("Dingoo X      : S\n");
	printf("Dingoo Y      : A\n");
	printf("Dingoo L      : Left Shift\n");
	printf("Dingoo R      : Rigth Shift\n");
	printf("Dingoo SELECT : Space\n");
	printf("Dingoo START  : Enter\n");
	printf("Dingoo POWER  : Escape\n");
}

uint32_t input::get_repeat()
{
	return inputRepeat;
}

uint32_t input::get_held()
{
	return inputHeld;
}

void input::wait_for_release()
{	
	int i, keys = 0;
	for(i = 0; i < 100; i++)
	{
		while(1)
		{     
			keys = poll();
			if(keys == 0) break;
		}
	}
}

void input::wait_for_press()
{	
	int i, keys = 0;
	for(i = 0; i < 100; i++)
	{
		while(1)
		{     
			keys = poll();
			if(keys != 0) break;
		}
	}
}

void input::ignore()
{
	inputIgnore = 1;
}

uint32_t input::read()
{
	SceCtrlData pad;
  
  sceCtrlPeekBufferPositive(0, &pad, 1);
	return pad.buttons;
}

uint32_t input::poll()
{
	uint32_t i, inputHeld, timer;
  	inputHeld  = read();
	inputHeld &= (HW_INPUT_UP	| 
	              HW_INPUT_SELECT	|
                HW_INPUT_L3	|
                HW_INPUT_R3	|
                HW_INPUT_START	|
                HW_INPUT_UP	|
                HW_INPUT_RIGHT	|
                HW_INPUT_DOWN	|
                HW_INPUT_LEFT		|
                HW_INPUT_L		|
                HW_INPUT_R	|
                HW_INPUT_L1	|
                HW_INPUT_R1	|
                HW_INPUT_TRIANGLE	|
                HW_INPUT_CIRCLE	|
                HW_INPUT_CROSS	|
                HW_INPUT_SQUARE	|
                HW_INPUT_VOL_UP		|
                HW_INPUT_VOL_DOWN		|
                HW_INPUT_POWER);

	timer = system::get_frames_count();
	for (i = 0; i < 32; i++)
	{
		if (inputHeld & (1 << i)) 
		{
			if(inputFirst & (1 << i))
			{
				if (inputRepeatTimer[i] < timer)
				{
					inputRepeat |= 1 << i;
					inputRepeatTimer[i] = timer + 10;
				} else
				{
					inputRepeat &= ~(1 << i);
				}
			} else {
				inputFirst  |= 1 << i;
				inputRepeat |= 1 << i;
				inputRepeatTimer[i] = timer + 50;
			}
		} else	{
			inputRepeatTimer[i] = timer - 10;
			inputRepeat &= ~(1 << i);
			inputFirst  &= ~(1 << i);
		}
	}
	if(inputIgnore)
	{
		if (inputHeld == 0)
		{
			inputIgnore = 0;
		}
		inputHeld = 0;
		inputRepeat = 0;
	}
	return inputHeld;
}

uint32_t input::get_mask(int index)
{
	uint32_t maskArray[21] = {
		0,					//	HW_INPUT_INDEX_NONE
		HW_INPUT_UP,
	              HW_INPUT_SELECT,
                HW_INPUT_L3,
                HW_INPUT_R3,
                HW_INPUT_START,
                HW_INPUT_UP,
                HW_INPUT_RIGHT,
                HW_INPUT_DOWN,
                HW_INPUT_LEFT,
                HW_INPUT_L,
                HW_INPUT_R,
                HW_INPUT_L1,
                HW_INPUT_R1,
                HW_INPUT_TRIANGLE,
                HW_INPUT_CIRCLE,
                HW_INPUT_CROSS,
                HW_INPUT_SQUARE,
                HW_INPUT_VOL_UP,
                HW_INPUT_VOL_DOWN,
                HW_INPUT_POWER
	};
	if(index >= 0 && index <= 20) return maskArray[index];
	return 0;
}
