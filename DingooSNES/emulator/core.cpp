/* DingooSNES - Super Nintendo Entertainment System Emulator for Dingoo A320
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

#include "cppdefs.h"
#include "framework.h"
#include "ips.patcher.h"

#include "snes9x.h"
#include "memmap.h"
#include "apu.h"
#include "soundux.h"
#include "snapshot.h"
#include "gfx.h"
#include "cheats.h"

using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

#define GAMEPAD_U  	SNES_UP_MASK
#define GAMEPAD_D  	SNES_DOWN_MASK
#define GAMEPAD_L  	SNES_LEFT_MASK
#define GAMEPAD_R  	SNES_RIGHT_MASK

#define GAMEPAD_ST 	SNES_START_MASK
#define GAMEPAD_SL 	SNES_SELECT_MASK
#define GAMEPAD_TL 	SNES_TL_MASK
#define GAMEPAD_TR 	SNES_TR_MASK

#define GAMEPAD_A  	SNES_A_MASK
#define GAMEPAD_B  	SNES_B_MASK
#define GAMEPAD_X  	SNES_X_MASK
#define GAMEPAD_Y  	SNES_Y_MASK

///////////////////////////////////////////////////////////////////////////////

static bool 		enableS9xUpdates;
static int			screenH;
static int			VBuffOffset;
static uint32_t		lastDPad;

///////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings();
static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);

EXTERN_C const char* S9xGetFilename(const char* extension);

///////////////////////////////////////////////////////////////////////////////

bool core::init()
{
	ZeroMemory (&Settings, sizeof (Settings));

///////////////////////////////////////////////////////////////////////////////
////Settings.APUEnabled;
    Settings.Shutdown				= true;
    Settings.SoundSkipMethod 		= 0;
    Settings.H_Max					= SNES_CYCLES_PER_SCANLINE;
    Settings.HBlankStart			= (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
    Settings.CyclesPercentage 		= 100;
    Settings.DisableIRQ 			= false;
////Settings.Paused;
////Settings.ForcedPause;
////Settings.StopEmulation;

///////////////////////////////////////////////////////////////////////////////
////Settings.TraceDMA;
////Settings.TraceHDMA;
////Settings.TraceVRAM;
////Settings.TraceUnknownRegisters;
////Settings.TraceDSP;

///////////////////////////////////////////////////////////////////////////////
    Settings.SwapJoypads 			= false;

///////////////////////////////////////////////////////////////////////////////
////Settings.ForcePAL;
////Settings.ForceNTSC;
////Settings.PAL;
    Settings.FrameTimePAL			= 20000;
    Settings.FrameTimeNTSC			= 16667;
////Settings.FrameTime				= Settings.FrameTimeNTSC;

///////////////////////////////////////////////////////////////////////////////
////Settings.ForceLoROM;
////Settings.ForceHiROM;
////Settings.ForceHeader;
////Settings.ForceNoHeader;
////Settings.ForceInterleaved;
////Settings.ForceInterleaved2;
////Settings.ForceNotInterleaved;

///////////////////////////////////////////////////////////////////////////////
////Settings.ForceSuperFX;
////Settings.ForceNoSuperFX;
////Settings.ForceDSP1;
////Settings.ForceNoDSP1;
////Settings.ForceSA1;
////Settings.ForceNoSA1;
////Settings.ForceC4;
////Settings.ForceNoC4;
////Settings.ForceSDD1;
////Settings.ForceNoSDD1;
    Settings.MultiPlayer5			= false;
    Settings.Mouse					= false;
    Settings.SuperScope				= false;
////Settings.SRTC;
    Settings.ControllerOption		= 0;
    Settings.ShutdownMaster			= true;
////Settings.MultiPlayer5Master;
////Settings.SuperScopeMaster;
////Settings.MouseMaster;
////Settings.SuperFX;
////Settings.DSP1Master;
////Settings.SA1;
////Settings.C4;
////Settings.SDD1;
////Settings.SPC7110;
////Settings.SPC7110RTC;
////Settings.OBC1;
	
///////////////////////////////////////////////////////////////////////////////
////Settings.SoundPlaybackRate;
////Settings.TraceSoundDSP;
////Settings.Stereo;
////Settings.ReverseStereo;
    Settings.SixteenBitSound 		= true;
////Settings.SoundBufferSize;
////Settings.SoundMixInterval;
////Settings.SoundEnvelopeHeightReading;
    Settings.DisableSoundEcho		= false;
    Settings.DisableSampleCaching	= false;
    Settings.DisableMasterVolume	= false;
    Settings.SoundSync				= false;
////Settings.InterpolatedSound;
    Settings.NextAPUEnabled 		= true;
////Settings.AltSampleDecode;
////Settings.FixFrequency;
    
///////////////////////////////////////////////////////////////////////////////
    Settings.SixteenBit 			= true;
    Settings.Transparency			= false;
////Settings.SupportHiRes;
////Settings.Mode7Interpolate;

///////////////////////////////////////////////////////////////////////////////
////Settings.BGLayering;
////Settings.DisableGraphicWindows;
////Settings.DisableHDMA;

///////////////////////////////////////////////////////////////////////////////
    Settings.NetPlay				= false;
////Settings.NetPlayServer;
    Settings.ServerName[0] 			= 0;
////Settings.Port;
////Settings.OpenGLEnable;
////Settings.AutoSaveDelay;
    Settings.ApplyCheats			= true;
    
///////////////////////////////////////////////////////////////////////////////
////Settings.StarfoxHack;
////Settings.WinterGold;
////Settings.BS;
////Settings.DaffyDuck;
////Settings.APURAMInitialValue;
////Settings.SampleCatchup;
////Settings.JustifierMaster 		= true;
////Settings.Justifier;
////Settings.SecondJustifier;
////Settings.SETA;
////Settings.TakeScreenshot;
////Settings.StretchScreenshots;
////Settings.DisplayColor;
////Settings.SoundDriver;
////Settings.AIDOShmId;
#if VER == 143
	Settings.SDD1Pack				= true;
	Settings.NoPatch				= true;
#endif
////Settings.ForceInterleaveGD24;

	GFX.Pitch 		= 320 * 2;
	GFX.RealPitch 	= 320 * 2;
	GFX.Screen 		= NULL;		//	will be set later
	
	GFX.SubScreen	= (uint8*)malloc(GFX.RealPitch * 480 * 2); 
	GFX.ZBuffer 	= (uint8*)malloc(GFX.RealPitch * 480 * 2); 
	GFX.SubZBuffer 	= (uint8*)malloc(GFX.RealPitch * 480 * 2);
	GFX.Delta 		= (GFX.SubScreen - GFX.Screen) >> 1;
	GFX.PPL 		= GFX.Pitch >> 1;
	GFX.PPLx2 		= GFX.Pitch;
	GFX.ZPitch 		= GFX.Pitch >> 1;
	
	if (!Memory.Init() || !S9xGraphicsInit() || !S9xInitAPU())
	{
		S9xMessage (0, 0, "Failed to init SNES machine!");
		return false;
	}
	
	Settings.NextAPUEnabled = true;
	S9xInitSound (0, 0, 0);
	return true;
}

void core::close()
{
	if(CPU.SRAMModified) Memory.SaveSRAM(S9xGetFilename(".srm"));
	
	S9xGraphicsDeinit();
	S9xDeinitAPU();
	Memory.Deinit();
	free(GFX.SubZBuffer);
	free(GFX.ZBuffer);
	free(GFX.SubScreen);
	GFX.SubZBuffer = NULL;
	GFX.ZBuffer    = NULL;
	GFX.SubScreen  = NULL;
}

bool core::load_rom()
{
	ips_define(settings.applyIPSpatch ? fsys::getGameRelatedFilePath("ips") : NULL);
	Settings.ForcePAL  = (settings.countryRegion == 2);
	Settings.ForceNTSC = (settings.countryRegion == 1 || settings.countryRegion  == 3);

	if(Memory.LoadROM(fsys::getGameFilePath())) {
		Memory.LoadSRAM(S9xGetFilename(".srm"));
		return true;
	}
	return false;
}

void core::hard_reset()	//	TODO: ???
{
	screenH = SNES_HEIGHT;
}

void core::soft_reset()	//	TODO: ???
{
	S9xReset();
}

uint32_t core::system_fps()
{
	return Memory.ROMFramesPerSecond;
}

void core::start_emulation(bool withSound)
{
	ApplyEmulationSettings();
	InitVideoSystem();
	InitSoundSystem(withSound);
	enableS9xUpdates = true;
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{
	IPPU.RenderThisFrame = withRendering;
	S9xMainLoop();
	if(soundBuffer >= 0) {
		S9xMixSamples((uint8*)sound::get_buffer(soundBuffer), sound::get_sample_count(soundBuffer));
	}
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	enableS9xUpdates = false;
	VBuffOffset = 0;

	IPPU.RenderThisFrame = true;
	S9xSetSoundMute(true);
	S9xMainLoop();
	S9xMainLoop();	

	console_scr_w = SNES_WIDTH;
	console_scr_h = screenH;
}

void core::stop_emulation(bool withSound)
{
	if(withSound) sound::close();
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{
	if(state::useFile(filePath)) {
		return load ? S9xUnfreezeGame(filePath) : S9xFreezeGame(filePath);
	} else
	if(state::useMemory(memBuffer)) {
		char fakename[] = "state." SAVESTATE_EXT "0";
		return load ? S9xUnfreezeGame(fakename) : S9xFreezeGame(fakename);
	}
	return false;
}

int core::get_cheat_count()
{
	return 0;
}

cstr_t core::get_cheat_name(int cheatIndex)
{
	return NULL;//int index = cheatCodeMap[cheatIndex] >> 8;
	//return S9xGetCheatName(index);
}

bool core::get_cheat_active(int cheatIndex)
{
	return false;//int index = cheatCodeMap[cheatIndex] >> 8;
	//return bool(S9xGetCheatEnable(index));
}

void core::set_cheat_active(int cheatIndex, bool enable)
{
	//int index = cheatCodeMap[cheatIndex] >> 8;
	//int count = cheatCodeMap[cheatIndex] & 0xFF;
	//while(count-- > 0)
	//	if(enable) S9xEnableCheat(index++); else S9xDisableCheat(index++);
}

///////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings()
{
	Settings.Transparency = settings.renderingMode;
	Settings.Stereo = settings.soundStereo;
	Settings.SoundPlaybackRate = settings.soundRate;
}

static void InitVideoSystem()
{
	if(settings.fullScreen) {
		video::set_mode(settings.scalingMode ? VM_SCALE_HW : VM_SCALE_SW, SNES_WIDTH, screenH, settings.verStretch);
		VBuffOffset = 0;
	} else {
		video::set_mode(settings.bufferingMode ? VM_SLOW : VM_FAST, SNES_WIDTH, screenH, settings.verStretch);
		VBuffOffset = ((HW_SCREEN_WIDTH - SNES_WIDTH) >> 1) + ((HW_SCREEN_HEIGHT - screenH) >> 1) * HW_SCREEN_WIDTH;
	}
}

static void InitSoundSystem(bool hasSound)
{
	if (hasSound) {
		sound::init(Settings.SoundPlaybackRate, Settings.Stereo, core::system_fps());
		S9xSetStereo(Settings.Stereo);
		S9xSetPlaybackRate(Settings.SoundPlaybackRate);
		S9xResetSound(false);
		S9xSetSoundMute(false);
	} else  {
		S9xResetSound(false);
		S9xSetSoundMute(true);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool8 S9xInitUpdate()
{
	GFX.Screen = (uint8*)(video::get_framebuffer() + VBuffOffset);
	return true;
}

bool8 S9xDeinitUpdate(int Width, int Height, bool8)
{
	if(enableS9xUpdates) {
		if(Height != screenH) {
			screenH = Height;
			video::clear_all(false);
			InitVideoSystem();
		}
	}
}

EXTERN_C uint32 S9xReadJoypad(int which1)
{
	uint32 GAMEPAD = 0x80000000;
	if (enableS9xUpdates && which1 == 0)
	{
		uint32_t controls = emul::handle_input();
	
		//	switch rendering mode
		if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_LEFT))
		{
			if (!video::has_osd_msg())
			{
				settings.renderingMode ^= true;
				if(settings.renderingMode) {
					video::set_osd_msg("Rendering mode: ACCURATE", 60);
				} else {
					video::set_osd_msg("Rendering mode: FAST", 60);
				}
				Settings.Transparency = settings.renderingMode;
			}
			return GAMEPAD;
		}

		//	set up NES Gamepad state
		if(controls)
		{
			if (controls & HW_INPUT_UP    ) GAMEPAD |= GAMEPAD_U;
			if (controls & HW_INPUT_DOWN  ) GAMEPAD |= GAMEPAD_D;
			if (controls & HW_INPUT_LEFT  ) GAMEPAD |= GAMEPAD_L;
			if (controls & HW_INPUT_RIGHT ) GAMEPAD |= GAMEPAD_R;
			if (controls & HW_INPUT_START ) GAMEPAD |= GAMEPAD_ST;
			if (controls & HW_INPUT_SELECT) GAMEPAD |= GAMEPAD_SL;
			if (controls & input::get_mask(settings.pad_config[0])) GAMEPAD |= GAMEPAD_TL;
			if (controls & input::get_mask(settings.pad_config[1])) GAMEPAD |= GAMEPAD_TR;
			if (controls & input::get_mask(settings.pad_config[2])) GAMEPAD |= GAMEPAD_A;
			if (controls & input::get_mask(settings.pad_config[3])) GAMEPAD |= GAMEPAD_B;
			if (controls & input::get_mask(settings.pad_config[4])) GAMEPAD |= GAMEPAD_X;
			if (controls & input::get_mask(settings.pad_config[5])) GAMEPAD |= GAMEPAD_Y;
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
	return GAMEPAD;
}

///////////////////////////////////////////////////////////////////////////////

START_EXTERN_C

void S9xMessage (int type, int number, const char* message)
{
//	if(message)
//	{
//		char info[40];
//		sprintf(info, "type:%d number: %d", type, number);
//		sal_MenuMessageBox("Snes9x message:", (char*)message, info, MENU_MESSAGE_BOX_MODE_PAUSE);
//	}
}

const char* S9xGetFilename(const char* extension)
{
//@@	static
//@@	char path[HW_MAX_PATH];
//@@	char file[HW_MAX_PATH];
//@@	char ext [HW_MAX_PATH];
//@@	fsys::split(Memory.ROMFilename, path, file, ext);
//@@	fsys::combine(path, file);
//@@    strcat(path, extension);
//@@    return path;
	return fsys::getGameRelatedFilePath(extension);
}

const char* S9xBasename (const char* f)
{
      const char* p;
      if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL) return (p + 1);
      return (f);
}

char* osd_GetPackDir()	//	TODO: ???
{
	/*
	static char	filename[PATH_MAX + 1];
	strcpy(filename, gamePath);

	if(!strncmp((char*)&Memory.ROM [0xffc0], "SUPER POWER LEAG 4   ", 21))
	{
		strcat(filename, "/SPL4-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "MOMOTETSU HAPPY      ",21))
	{
		strcat(filename, "/SMHT-SP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21))
	{
		strcat(filename, "/FEOEZSP7");
	}
	else if(!strncmp((char*)&Memory.ROM [0xffc0], "JUMP TENGAIMAKYO ZERO",21))
	{
		strcat(filename, "/SJUMPSP7");
	}
	else
	{
		strcat(filename, "/MISC-SP7");
	}
	return filename;
	*/
	S9xMessage (0, 0, "get pack dir");
	return NULL;
}

void S9xExit() {}
void S9xSetPalette() {}
void S9xLoadSDD1Data() {}
bool8 S9xReadSuperScopePosition(int& x, int& y, uint32& buttons) { return false; }
bool8 S9xReadMousePosition(int which1, int& x, int& y, uint32& buttons) { return false; }

#if VER == 143
const char *S9xGetFilenameInc(const char *e) { return e; }
const char *S9xGetSnapshotDirectory() { return NULL; }
#endif

//	TODO: use framework functions
void _makepath (char *path, const char *, const char *dir, const char *fname, const char *ext)
{
	if (dir && *dir)
	{
		strcpy (path, dir);
		strcat (path, "/");
	}
	else
	*path = 0;
	strcat (path, fname);
	if (ext && *ext)
	{
		strcat (path, ".");
		strcat (path, ext);
	}
}

//	TODO: use framework functions
void _splitpath (const char *path, char *drive, char *dir, char *fname,	char *ext)
{
	*drive = 0;

	char *slash = strrchr ((char*)path, '/');
	if (!slash)
		slash = strrchr ((char*)path, '\\');

	char *dot = strrchr ((char*)path, '.');

	if (dot && slash && dot < slash)
		dot = NULL;

	if (!slash)
	{
		strcpy (dir, "");
		strcpy (fname, path);
		if (dot)
		{
			*(fname + (dot - path)) = 0;
			strcpy (ext, dot + 1);
		}
		else
			strcpy (ext, "");
	}
	else
	{
		strcpy (dir, path);
		*(dir + (slash - path)) = 0;
		strcpy (fname, slash + 1);
		if (dot)
		{
			*(fname + (dot - slash) - 1) = 0;
			strcpy (ext, dot + 1);
		}
		else
			strcpy (ext, "");
	}
}

END_EXTERN_C

#if VER == 143
bool JustifierOffscreen() { return false; }
void JustifierButtons(uint32& justifiers) {}
#endif
