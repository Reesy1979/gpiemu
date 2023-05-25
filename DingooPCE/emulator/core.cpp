/* DingooPCE - PC Engine, TurboGrafx-16 Emulator for Dingoo A320
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

#include <mednafen.h>
#include <general.h>
#include <state-driver.h>
#include <pce_fast/pce.h>
#include <pce_fast/vdc.h>

#include <string>
#include <ctype.h>
#include "framework.h"

using namespace fw;
using namespace hw;


//	NES Gamepad defines
#define GAMEPAD_U		(1 << 4)
#define GAMEPAD_D		(1 << 6)
#define GAMEPAD_L		(1 << 7)
#define GAMEPAD_R		(1 << 5)
#define GAMEPAD_RUN		(1 << 3)
#define GAMEPAD_SEL		(1 << 2)
#define GAMEPAD_I		(1 << 0)	//	??
#define GAMEPAD_II		(1 << 1)	//	??

///////////////////////////////////////////////////////////////////////////////

static const MDFN_PixelFormat mPixFmtRGB565 = { 16, MDFN_COLORSPACE_RGB, { 11 }, { 5 }, { 0 }, 16, 5, 6, 5, 8 };

namespace PCE_Fast
{
	void applyVideoFormat(EmulateSpecStruct *espec);
	void applySoundFormat(EmulateSpecStruct *espec);
}

static uint16_t		pceGamepad[5] = { 0 };
static int			VBuffOffset;
static uint32_t		lastDPad;

extern MDFNGI EmulatedPCE_Fast;
static MDFNGI *emuSys = &EmulatedPCE_Fast;
MDFNGI *MDFNGameInfo  = &EmulatedPCE_Fast;

static EmulateSpecStruct espec;
static MDFN_Surface		 mSurface;
static MDFN_Rect 		 currRect;

///////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings();
static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

///////////////////////////////////////////////////////////////////////////////

bool core::init()
{
	//
	memset(&espec, 0, sizeof(espec));

	mSurface.Init(0, HW_SCREEN_WIDTH, HW_SCREEN_HEIGHT, HW_SCREEN_WIDTH, mPixFmtRGB565);
	//
	emuSys->soundchan = 0;
	emuSys->soundrate = 0;
	emuSys->rotated   = 0;
	//
	return true;
}

void core::close()
{
	espec.surface->pixels16 = NULL;
	emuSys->CloseGame();
}

bool core::load_rom()
{
	MDFNFILE fp;
	static const FileExtensionSpecStruct ext[] = {{".pce",0},{".sgx",0},{0,0}};
	if(fp.Open(fsys::getGameFilePath(), ext, 0)) return emuSys->Load(0, &fp);
	return false;
}

void core::hard_reset()
{
	// input
	for(int i = 0; i < 5; i++) emuSys->SetInput(i, "gamepad", &pceGamepad[i]);

	// video
	if(!espec.surface)
	{
		espec.LineWidths = 0;	//lineWidth;
		espec.NeedRewind = 0;

		espec.surface = &mSurface;
		
		PCE_Fast::applyVideoFormat(&espec);
		currRect = (MDFN_Rect){0, 4, 256, 232};
	}
	
}

void core::soft_reset()
{
	PCE_Fast::PCE_Power();
}

uint32_t core::system_fps()
{
	return 60;
}

void core::start_emulation(bool withSound)
{	
	ApplyEmulationSettings();
	InitVideoSystem();
	InitSoundSystem(withSound);
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{
	video::clear(COLOR_RED);

	UpdateInput();
	
	espec.skip = !withRendering;
	espec.surface->pixels16 = video::get_framebuffer() + VBuffOffset;

	if(soundBuffer >= 0) {
		espec.SoundBuf = sound::get_buffer(soundBuffer);
		emuSys->Emulate(&espec);
		sound::set_sample_count(soundBuffer, espec.SoundBufSize << 1);
	} else {
		espec.SoundBuf = 0;
		emuSys->Emulate(&espec);
	}

	if(withRendering && (espec.DisplayRect.w != currRect.w || espec.DisplayRect.h != currRect.h)) {
		currRect = espec.DisplayRect;
		hw::video::clear_all(false);
		InitVideoSystem();
		printf("x: %d y: %d w: %d h: %d\n", currRect.x, currRect.y, currRect.w, currRect.h);
	}
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	espec.skip = 0;
	espec.SoundBuf = 0;
	espec.surface->pixels16 = video::get_framebuffer();
	emuSys->Emulate(&espec);
	console_scr_w = currRect.w;
	console_scr_h = currRect.h;
}

void core::stop_emulation(bool withSound)
{
	if(withSound) sound::close();
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{
	if(state::useFile(filePath)) 
	{
		if(load) MDFNI_LoadState(filePath, 0); else MDFNI_SaveState(filePath, 0, 0, 0, 0);
	} else
	if(state::useMemory(memBuffer)) {
		char fakename[] = "state." SAVESTATE_EXT "0";
		if(load) MDFNI_LoadState(fakename, 0); else MDFNI_SaveState(fakename, 0, 0, 0, 0);
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings()
{
	//
}

static void InitVideoSystem()
{
//	int W = currRect.w > 256 ? 320 : 256;
	int W = currRect.w > HW_SCREEN_WIDTH ? HW_SCREEN_WIDTH : currRect.w;
	int H = currRect.h;

	if(settings.fullScreen && (W != HW_SCREEN_WIDTH || settings.verStretch)) {
		video::set_mode(settings.scalingMode ? VM_SCALE_HW : VM_SCALE_SW, W, H, settings.verStretch);
		VBuffOffset = 0;
	} else {
		video::set_mode(VM_FAST, W, H, settings.verStretch);
		VBuffOffset = ((HW_SCREEN_WIDTH - W) >> 1) + ((HW_SCREEN_HEIGHT - H) >> 1) * HW_SCREEN_WIDTH;
	}
}

static void InitSoundSystem(bool hasSound)
{
	if(hasSound) {
		sound::init(settings.soundRate, true, core::system_fps());
		espec.SoundBufMaxSize = sound::get_sample_count(0);
		espec.SoundRate = settings.soundRate;
	} else {
		espec.SoundBufMaxSize = 0;
		espec.SoundRate = 0;
	}
	PCE_Fast::applySoundFormat(&espec);
}

static void UpdateInput()
{
	#define GAMEPAD pceGamepad[0]
	uint32_t controls = emul::handle_input();
	GAMEPAD = 0;

	//	set up PCE Gamepad state
	if(controls)
	{
		if (controls & HW_INPUT_UP   ) GAMEPAD |= GAMEPAD_U;
		if (controls & HW_INPUT_DOWN ) GAMEPAD |= GAMEPAD_D;
		if (controls & HW_INPUT_LEFT ) GAMEPAD |= GAMEPAD_L;
		if (controls & HW_INPUT_RIGHT) GAMEPAD |= GAMEPAD_R;
		if (controls & input::get_mask(settings.pad_config[0])) GAMEPAD |= GAMEPAD_RUN;
		if (controls & input::get_mask(settings.pad_config[1])) GAMEPAD |= GAMEPAD_SEL;
		if (controls & input::get_mask(settings.pad_config[2])) GAMEPAD |= GAMEPAD_I;
		if (controls & input::get_mask(settings.pad_config[3])) GAMEPAD |= GAMEPAD_II;
	}
		
	//	D-Pad diagonals helper algorithm
	if(settings.dpadDiagonals)
	{
		const int GAMEPAD_UD = (GAMEPAD_U | GAMEPAD_D);
		const int GAMEPAD_LR = (GAMEPAD_L | GAMEPAD_R);
		if(((lastDPad & GAMEPAD_UD) && (GAMEPAD & GAMEPAD_LR)) || ((lastDPad & GAMEPAD_LR) && (GAMEPAD & GAMEPAD_UD))) {
			GAMEPAD |= lastDPad;
		} else {
			lastDPad = GAMEPAD & (GAMEPAD_UD | GAMEPAD_LR);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

int MDFNnetplay = 0;

namespace PCE_Fast
{
	//	dummy HES functions
	int   PCE_HESLoad(const uint8 *buf, uint32 size) { return 0; };
	void  HES_Draw(MDFN_Surface *surface, MDFN_Rect *DisplayRect, int16 *SoundBuf, int32 SoundBufSize) { }
	void  HES_Close(void) { }
	void  HES_Reset(void) { }
	uint8 ReadIBP(unsigned int A) { return 0; }
};

void ErrnoHolder::SetErrno(int the_errno) {}
void MDFND_SetMovieStatus(StateStatusStruct *status) { }
void MDFN_DoSimpleCommand(int cmd) {}
void MDFN_indent(int indent) {}
void MDFND_SetStateStatus(StateStatusStruct *status) {}
int  MDFNNET_SendState(void) { return 0; }
int  MDFN_RawInputStateAction(StateMem *sm, int load, int data_only) { return 1; }
void NetplaySendState(void) { }
void MDFND_NetplayText(const uint8 *text, bool NetEcho) { }
void MDFN_ResetMessages(void) { }

///////////////////////////////////////////////////////////////////////////////

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	if(type == MDFNMKF_SAV) return fsys::getGameRelatedFilePath(SRAM_FILE_EXT);
	return "";
}

bool MDFN_GetSettingB(const char *name)
{
	if(!strcmp("cheats",						name)) return 0;	// OK
	if(!strcmp("filesys.disablesavegz",			name)) return 1;	// OK
	if(!strcmp("pce_fast.arcadecard",			name)) return 1;	// ???
	if(!strcmp("pce_fast.forcesgx",				name)) return 0;	// OK
	if(!strcmp("pce_fast.nospritelimit",		name)) return 1;	// OK
	if(!strcmp("pce_fast.forcemono",			name)) return 0;	// UNUSED
	if(!strcmp("pce_fast.disable_softreset",	name)) return 0;	// ???
	if(!strcmp("pce_fast.adpcmlp",				name)) return 0;	// ???
	if(!strcmp("pce_fast.correct_aspect",		name)) return 1;	// UNUSED
	if(!strcmp("cdrom.lec_eval", 				name)) return 1;	// UNUSED
	//
	return 0;
}

std::string MDFN_GetSettingS(const char *name)
{
	if(!strcmp("pce_fast.cdbios",	name)) return std::string("");			// UNUSED
	if(!strcmp("srwcompressor",		name)) return std::string("quicklz");	// ???
	//
	return 0;
}

uint64 MDFN_GetSettingUI(const char *name)
{
	if(!strcmp("pce_fast.ocmultiplier",		name)) return 1;		// OK
	if(!strcmp("pce_fast.cdspeed",			name)) return 2;		// OK
	if(!strcmp("pce_fast.cdpsgvolume",		name)) return 100;		// OK
	if(!strcmp("pce_fast.cddavolume",		name)) return 100;		// OK
	if(!strcmp("pce_fast.adpcmvolume", 		name)) return 100;		// OK
	if(!strcmp("pce_fast.slstart",			name)) return 4;		// OK
	if(!strcmp("pce_fast.slend",			name)) return 235;		// OK
	if(!strcmp("srwframes",					name)) return 0;		// ???
	//
	return 0;
}

float MDFN_GetSettingF(const char *name)
{
	if(!strcmp("pce_fast.mouse_sensitivity", name)) return 0.50;	// OK
	//
	return 0;
}
