/* DingooNES - Nintendo Entertainment System, Famicom, Dendy Emulator for Dingoo A320
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
#define DEFAULT_SOUND_VOLUME		5
#define DEFAULT_SOUND_SAMPLE_RATE	44100
#define DEFAULT_DPAD_DIAGONALS		false
#define DEFAULT_TURBO_SIGNAL		11
#define DEFAULT_ZAPPER_ENABLE		true
#define DEFAULT_ZAPPER_AUTO_AIM		false
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

	//	sound settings
	uint16_t soundVolume;
	uint16_t soundRate;

	//	input settings
	uint16_t zapperEnable;
	uint16_t zapperAutoAim;
	uint16_t dpadDiagonals;
	uint16_t turboSignal;
	uint16_t pad_config[8];

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

//	sound settings
settings.soundVolume 	 = DEFAULT_SOUND_VOLUME;
settings.soundRate 		 = DEFAULT_SOUND_SAMPLE_RATE;

//	input settings
settings.zapperEnable	 = DEFAULT_ZAPPER_ENABLE;
settings.zapperAutoAim	 = DEFAULT_ZAPPER_AUTO_AIM;
settings.dpadDiagonals 	 = DEFAULT_DPAD_DIAGONALS;
settings.turboSignal	 = DEFAULT_TURBO_SIGNAL;
settings.pad_config[0] 	 = HW_INPUT_INDEX_START;
settings.pad_config[1]	 = HW_INPUT_INDEX_SELECT;
settings.pad_config[2]	 = HW_INPUT_INDEX_A;
settings.pad_config[3]	 = HW_INPUT_INDEX_B;
settings.pad_config[4]	 = HW_INPUT_INDEX_X;
settings.pad_config[5]	 = HW_INPUT_INDEX_Y;

//	common settings
settings.frameSkip 		 = DEFAULT_SKIP_FRAMES;
settings.cpuSpeed 		 = DEFAULT_CPU_SPEED; 
settings.showFps 		 = DEFAULT_SHOW_FPS;

#endif
