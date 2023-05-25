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
 
#ifdef USE_SETS_DEFAULTS

#define DEFAULT_COUNTRY_REGION      0
#define DEFAULT_COMPRESS_ROM		false
#define DEFAULT_OLD_SCHOOL_MODE		false
#define DEFAULT_FULLSCREEN		true
#define DEFAULT_SCALING_MODE		1
#define DEFAULT_SOUND_VOLUME		1
#define DEFAULT_SOUND_SAMPLE_RATE	44100
#define DEFAULT_DPAD_DIAGONALS		false
#define DEFAULT_SKIP_FRAMES			0
#define DEFAULT_CPU_SPEED			336
#define DEFAULT_SHOW_FPS			0

#endif
#ifdef USE_SETS_TYPEDEF

typedef struct {
	//	header
	uint16_t version;

	//	extra settings
	uint16_t countryRegion;
	uint16_t compressROM;
	uint16_t oldSchoolMode;

	//	video settings
	uint16_t fullScreen;
	uint16_t scalingMode;

	//	sound settings
	uint16_t soundVolume;
	uint16_t soundRate;

	//	input settings
	uint16_t dpadDiagonals;
	uint16_t pad_config[14];

	//	common settings
	uint16_t frameSkip;
	uint16_t cpuSpeed;
	uint16_t showFps;
} Sets_t;

#endif
#ifdef USE_SETS_DEFAULTS

//	extra settings
settings.countryRegion 	 = DEFAULT_COUNTRY_REGION;
settings.compressROM 	 = DEFAULT_COMPRESS_ROM;
settings.oldSchoolMode   = DEFAULT_OLD_SCHOOL_MODE;

//	video settings
settings.fullScreen		 = DEFAULT_FULLSCREEN;
settings.scalingMode 	 = DEFAULT_SCALING_MODE;

//	sound settings
settings.soundVolume 	 = DEFAULT_SOUND_VOLUME;
settings.soundRate 		 = DEFAULT_SOUND_SAMPLE_RATE;

//	input settings
// 0					//	HW_INPUT_INDEX_NONE
// 1		HW_INPUT_UP,		//	HW_INPUT_INDEX_UP
// 2		HW_INPUT_DOWN,		//	HW_INPUT_INDEX_DOWN
// 3		HW_INPUT_LEFT,		//	HW_INPUT_INDEX_LEFT
// 4		HW_INPUT_RIGHT,		//	HW_INPUT_INDEX_RIGHT
// 5		HW_INPUT_A,			//	HW_INPUT_INDEX_A
// 6		HW_INPUT_B,			//	HW_INPUT_INDEX_B
// 7		HW_INPUT_X,			//	HW_INPUT_INDEX_X
// 8		HW_INPUT_Y,			//	HW_INPUT_INDEX_Y
// 9		HW_INPUT_L,			//	HW_INPUT_INDEX_L
// 10		HW_INPUT_R,			//	HW_INPUT_INDEX_R
// 11		HW_INPUT_SELECT,	//	HW_INPUT_INDEX_SELECT
// 12		HW_INPUT_START,		//	HW_INPUT_INDEX_START
// 13		HW_INPUT_POWER		//	HW_INPUT_INDEX_POWER
	
settings.dpadDiagonals	 = DEFAULT_DPAD_DIAGONALS;
settings.pad_config[1]	 = 6; //These link to the position of the key in the input array - not the actual phsyical key
settings.pad_config[0]	 = 5;	
settings.pad_config[2]	 = 12;	
settings.pad_config[3]	 = 11;	

//	common settings
settings.frameSkip 		 = DEFAULT_SKIP_FRAMES;
settings.cpuSpeed 		 = DEFAULT_CPU_SPEED; 
settings.showFps 		 = DEFAULT_SHOW_FPS;

#endif
