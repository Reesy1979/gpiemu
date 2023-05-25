/* DingooSMD - Sega Mega Drive, Sega Genesis, Sega CD Emulator for Dingoo A320
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
#define DEFAULT_APPLY_IPS_PATCH		false
#define DEFAULT_COMPRESS_ROM		false
#define DEFAULT_OLD_SCHOOL_MODE		false
#define DEFAULT_FULLSCREEN			true
#define DEFAULT_SCALING_MODE		1
#define DEFAULT_VERTICAL_STRETCH	false
#define DEFAULT_RENDERING_MODE		1
#define DEFAULT_SOUND_VOLUME		5
#define DEFAULT_SOUND_SAMPLE_RATE	22050
#define DEFAULT_SOUND_OUTPUT_MODE	1
#define DEFAULT_DPAD_DIAGONALS		false
#define DEFAULT_SIX_BUTTON_GAMEPAD	false
#define DEFAULT_ACCURATE_TIMING		false
#define DEFAULT_EMULATE_Z80_CPU		true
#define DEFAULT_CDDA_AUDIO			true
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
	uint16_t applyIPSpatch;
	uint16_t compressROM;
	uint16_t oldSchoolMode;

	//	video settings
	uint16_t fullScreen;
	uint16_t scalingMode;
	uint16_t verStretch;
	uint16_t renderingMode;

	//	sound settings
	uint16_t soundVolume;
	uint16_t soundRate;
	uint16_t soundStereo;

	//	input settings
	uint16_t dpadDiagonals;
	uint16_t sixBtnGamepad;
	uint16_t pad_config[8];

	//	core settings
	uint16_t accurateTiming;
	uint16_t emulateZ80cpu;
	uint16_t cddaAudio;

	//	common settings
	uint16_t frameSkip;
	uint16_t cpuSpeed;
	uint16_t showFps;
} Sets_t;

#endif
#ifdef USE_SETS_DEFAULTS

//	extra settings
settings.countryRegion 	 = DEFAULT_COUNTRY_REGION;
settings.applyIPSpatch   = DEFAULT_APPLY_IPS_PATCH;
settings.compressROM 	 = DEFAULT_COMPRESS_ROM;
settings.oldSchoolMode   = DEFAULT_OLD_SCHOOL_MODE;

//	video settings
settings.fullScreen		 = DEFAULT_FULLSCREEN;
settings.scalingMode 	 = DEFAULT_SCALING_MODE;
settings.verStretch		 = DEFAULT_VERTICAL_STRETCH;
settings.renderingMode 	 = DEFAULT_RENDERING_MODE;

//	sound settings
settings.soundVolume 	 = DEFAULT_SOUND_VOLUME;
settings.soundRate 		 = DEFAULT_SOUND_SAMPLE_RATE;
settings.soundStereo 	 = DEFAULT_SOUND_OUTPUT_MODE;

//	input settings
settings.dpadDiagonals 	 = DEFAULT_DPAD_DIAGONALS;
settings.sixBtnGamepad	 = DEFAULT_SIX_BUTTON_GAMEPAD;
settings.pad_config[0] 	 = HW_INPUT_INDEX_START;
settings.pad_config[1] 	 = HW_INPUT_INDEX_Y;
settings.pad_config[2] 	 = HW_INPUT_INDEX_B;
settings.pad_config[3] 	 = HW_INPUT_INDEX_A;
settings.pad_config[4] 	 = HW_INPUT_INDEX_SELECT;
settings.pad_config[5] 	 = HW_INPUT_INDEX_L;
settings.pad_config[6] 	 = HW_INPUT_INDEX_X;
settings.pad_config[7] 	 = HW_INPUT_INDEX_R;

//	core settings
settings.accurateTiming  = DEFAULT_ACCURATE_TIMING;
settings.emulateZ80cpu 	 = DEFAULT_EMULATE_Z80_CPU;
settings.cddaAudio 		 = DEFAULT_CDDA_AUDIO;

//	common settings
settings.frameSkip 		 = DEFAULT_SKIP_FRAMES;
settings.cpuSpeed 		 = DEFAULT_CPU_SPEED; 
settings.showFps 		 = DEFAULT_SHOW_FPS;

#endif
