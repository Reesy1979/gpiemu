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

#include "cppdefs.h"
#include "framework.h"
#include "ips.patcher.h"

START_EXTERN_C
#include "common/emu.h"
#include "common/shared.h"
#include "pico/PicoInt.h"
#include "pico/Patch.h"
END_EXTERN_C

using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

#define GAMEPAD_U 1 << 0
#define GAMEPAD_D 1 << 1
#define GAMEPAD_L 1 << 2
#define GAMEPAD_R 1 << 3

#define GAMEPAD_B 1 << 4
#define GAMEPAD_C 1 << 5
#define GAMEPAD_A 1 << 6
#define GAMEPAD_S 1 << 7

#define GAMEPAD_Z 1 << 8
#define GAMEPAD_Y 1 << 9
#define GAMEPAD_X 1 << 10
#define GAMEPAD_M 1 << 11

///////////////////////////////////////////////////////////////////////////////

unsigned char* 	PicoDraw2FB;
unsigned short	rgb565pal[256];
static int 		VMode;;
static int		VBuffOffset;

static int  	SBuffIndex;
static short	SBuffer[2 * 44100 / 50];

static unsigned int	lastDPad;

#define IS_H240 (Pico.video.reg[ 1] & 8) 
#define IS_W320 (Pico.video.reg[12] & 1) 

///////////////////////////////////////////////////////////////////////////////

static int EmuScan16(unsigned int, void *sdata)
{
	DrawLineDest = (unsigned short *)sdata + HW_SCREEN_WIDTH; 
	return 0;
}

static void UpdateSound(int samples)
{
	if(SBuffIndex >= 0)
	{
		if(PicoOpt & PicoOpt_stereo_sound) samples *= 2;
		sound::fill_s16(SBuffIndex, SBuffer, samples);
	}
}

static void ApplyEmulationSettings();
static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

static void ResetVideoRendering();
static void CheckVideoModeChanges();
static void BlitFrame();

bool is_segacd_mode() {	return (PicoMCD & 1); }
extern void KernelLog(char *msg);
///////////////////////////////////////////////////////////////////////////////

bool core::init()
{
	//	set default settings
	KernelLog("core::init 1\n");
	PicoOpt  = PicoOpt_draw_no_32col_border | PicoOpt_enable_sn76496 | PicoOpt_external_ym2612 | PicoOpt_enable_ym2612_dac;
	PicoOpt |= PicoOpt_enable_cd_ramcart;
	PicoOpt |= PicoOpt_enable_cd_pcm;
#ifndef D_DISABLE_MP3_SUPPORT
	PicoOpt |= PicoOpt_enable_cd_cdda;
#endif
	PicoCDBuffers = 64;
	KernelLog("core::init 2\n");
	//	set region/country settings
	switch(settings.countryRegion)
	{
		case  1: PicoRegionOverride = 4; break;	//	US
		case  2: PicoRegionOverride = 8; break;	//	EU
		case  3: PicoRegionOverride = 1; break;	//	JP (NTSC)
		default: PicoRegionOverride = 0; break; //	AUTO
	}
	PicoAutoRgnOrder = 0x184;

	//	create buffer for alt fast rendering
	PicoDraw2FB = (unsigned char*) malloc((8 + 320) * (8 + 224 + 8));
	memset(PicoDraw2FB, 0, (8 + 320) * (8 + 224 + 8));
	KernelLog("core::init 3\n");
	//	init emulation core
	PicoInit();
	PicoMessage = NULL;
	PicoMCDopenTray = NULL;
	PicoMCDcloseTray = NULL;
	KernelLog("core::init 4\n");
	return true;
}

void core::close()
{
	//	save SRAM
	if(SRam.changed) 
	{
		emu_SaveLoadSRAM(0);
		SRam.changed = 0;
	}

	//	deinit emulation core
	PicoExit();

	//	free buffer for alt fast rendering
	free(PicoDraw2FB);
}

bool core::load_rom()
{
	if(settings.applyIPSpatch) {
		strncpy(ipsFileName, fsys::getGameRelatedFilePath("ips"), 256);
		ipsFileName[255] = 0;
	} else {
		ipsFileName[0] = 0;
	}
	strncpy(romFileName, fsys::getGameFilePath(), 256);
	romFileName[255] = 0;
	return emu_ReloadRom();
}

void core::hard_reset()
{
	//	do nothing for now
}

void core::soft_reset()
{
	PicoReset(0);
}

uint32_t core::system_fps()
{
	return Pico.m.pal ? 50 : 60;
}

void core::start_emulation(bool withSound)
{
	ApplyEmulationSettings();
	ResetVideoRendering();
	InitSoundSystem(withSound);

	//	prepare CD buffer
	if(is_segacd_mode()) PicoCDBufferInit();
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{
	SBuffIndex = soundBuffer;
	CheckVideoModeChanges();
	UpdateInput();
	if(withRendering) {
		PicoFrame();
		BlitFrame();
	} else {
		PicoSkipFrame = 1;
		PicoFrame();
		PicoSkipFrame = 0;
	}
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	VMode = (IS_W320 << 2) | IS_H240;	//	TODO: ???
	VBuffOffset = 0;
	CheckVideoModeChanges();
	PicoFrame();
	BlitFrame();
	console_scr_w = IS_W320 ? 320 : 256;
	console_scr_h = IS_H240 ? 240 : 224;
}

void core::stop_emulation(bool withSound)
{
	//	free CD buffer
	if(is_segacd_mode()) PicoCDBufferFree();

	//	close sound device
	if(withSound) sound::close();
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{
	if(state::useFile(filePath) || state::useMemory(memBuffer))
	{
		return !emu_SaveLoadState(filePath, load);
	}
	return false;
}

int core::get_cheat_count()
{
	return PicoPatches ? cheatCodeCount : 0;
}

cstr_t core::get_cheat_name(int cheatIndex)
{
	int index = cheatCodeMap[cheatIndex] >> 8;
	return PicoPatches[index].name;
}

bool core::get_cheat_active(int cheatIndex)
{
	int index = cheatCodeMap[cheatIndex] >> 8;
	return bool(PicoPatches[index].active);
}

void core::set_cheat_active(int cheatIndex, bool enable)
{
	int index = cheatCodeMap[cheatIndex] >> 8;
	int count = cheatCodeMap[cheatIndex] & 0xFF;
	while(count-- > 0) 
		PicoPatches[index++].active = int(enable);
}

///////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings()
{
	//	sound settings
	PsndRate = settings.soundRate;
	if(settings.soundStereo       ) PicoOpt |= PicoOpt_stereo_sound;     else PicoOpt &= ~PicoOpt_stereo_sound;

	//	video settings
	if(settings.renderingMode == 0) PicoOpt |= PicoOpt_alt_renderer;     else PicoOpt &= ~PicoOpt_alt_renderer;
	if(settings.renderingMode == 2) PicoOpt |= PicoOpt_accurate_sprites; else PicoOpt &= ~PicoOpt_accurate_sprites;
	
	//	input settings
	if(settings.sixBtnGamepad     ) PicoOpt |= PicoOpt_6button_gamepad;  else PicoOpt &= ~PicoOpt_6button_gamepad;

	//	other settings
	if(settings.accurateTiming    ) PicoOpt |= PicoOpt_accurate_timing;  else PicoOpt &= ~PicoOpt_accurate_timing;
	if(settings.emulateZ80cpu     ) PicoOpt |= PicoOpt_enable_z80;       else PicoOpt &= ~PicoOpt_enable_z80;
	if(settings.cddaAudio         ) PicoOpt |= PicoOpt_enable_cd_cdda;	 else PicoOpt &= ~PicoOpt_enable_cd_cdda;

	//	activated/deactivated cheats
	if(core::get_cheat_count()) PicoPatchApply();
}

static void InitVideoSystem()
{
	int W = IS_W320 ? 320 : 256;
	int H = IS_H240 ? 240 : 224;

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
	if (hasSound) {
		static int PsndRate_ = 0;
		static int PicoOpt_  = 0;
		static int pal_      = 0;
		
		const int mask = PicoOpt_external_ym2612 | PicoOpt_enable_ym2612_dac | PicoOpt_enable_sn76496 | PicoOpt_stereo_sound;
		if (PsndRate != PsndRate_ || (PicoOpt & mask) != (PicoOpt_ & mask) || Pico.m.pal != pal_) PsndRerate(PsndRate_ ? 1 : 0);
		
		sound::init(PsndRate, !!(PicoOpt & PicoOpt_stereo_sound), core::system_fps());
		PicoWriteSound = UpdateSound;
		PsndOut = SBuffer;
		
		PsndRate_ = PsndRate;
		PicoOpt_  = PicoOpt;
		pal_      = Pico.m.pal;
	} else  {
		PsndOut = NULL;
	}
}

static void UpdateInput()
{
	#define GAMEPAD PicoPad[0]
	unsigned int controls = emul::handle_input();
	GAMEPAD = 0;

	//	switch rendering mode
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_LEFT))
	{
		if (!video::has_osd_msg())
		{
			if(++settings.renderingMode > 2) settings.renderingMode = 0;
			
			switch(settings.renderingMode)
			{
				case 0: video::set_osd_msg("Rendering mode: FAST", 60); break;
				case 1: video::set_osd_msg("Rendering mode: ACCURATE", 60); break;
				case 2: video::set_osd_msg("Rendering mode: BEST", 60); break;
			}

			if(settings.renderingMode == 0) PicoOpt |= PicoOpt_alt_renderer;     else PicoOpt &= ~PicoOpt_alt_renderer;
			if(settings.renderingMode == 2) PicoOpt |= PicoOpt_accurate_sprites; else PicoOpt &= ~PicoOpt_accurate_sprites;

			ResetVideoRendering();
			VMode = (IS_W320 << 2) | IS_H240;	//	TODO: ???
			CheckVideoModeChanges();
		}
		return;
	}
	
	//	set up SMD Gamepad state
	if(controls)
	{
		if (controls & HW_INPUT_UP   ) GAMEPAD |= GAMEPAD_U;
		if (controls & HW_INPUT_DOWN ) GAMEPAD |= GAMEPAD_D;
		if (controls & HW_INPUT_LEFT ) GAMEPAD |= GAMEPAD_L;
		if (controls & HW_INPUT_RIGHT) GAMEPAD |= GAMEPAD_R;

		if (controls & input::get_mask(settings.pad_config[0])) GAMEPAD |= GAMEPAD_S;
		if (controls & input::get_mask(settings.pad_config[1])) GAMEPAD |= GAMEPAD_A;
		if (controls & input::get_mask(settings.pad_config[2])) GAMEPAD |= GAMEPAD_B;
		if (controls & input::get_mask(settings.pad_config[3])) GAMEPAD |= GAMEPAD_C;

		if (controls & input::get_mask(settings.pad_config[4])) GAMEPAD |= GAMEPAD_M;
		if (controls & input::get_mask(settings.pad_config[5])) GAMEPAD |= GAMEPAD_X;
		if (controls & input::get_mask(settings.pad_config[6])) GAMEPAD |= GAMEPAD_Y;
		if (controls & input::get_mask(settings.pad_config[7])) GAMEPAD |= GAMEPAD_Z;	
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

////////////////////////////////////////////////////////////////////////////////

static void ResetVideoRendering()
{
	if (PicoOpt & PicoOpt_alt_renderer) {
//@@		rgb565pal[0xc0] = 0x0000c000; // MCD LEDs
//@@		rgb565pal[0xd0] = 0x00c00000;
//@@		rgb565pal[0xe0] = 0x00000000; // reserved pixels for OSD
//@@		rgb565pal[0xf0] = 0x00ffffff;
	} else {		
		PicoDrawSetColorFormat(PicoVideo_RGB555);
		PicoScan = EmuScan16;	
	}
	InitVideoSystem();
	Pico.m.dirtyPal = 1;
	VMode = (IS_W320 << 2) ^ 0x0C;	//	TODO: ???
}

static void CheckVideoModeChanges()
{
	int currVMode = (IS_W320 << 2) | IS_H240;
	if (currVMode != VMode) 
	{
		VMode = currVMode;
		video::clear_all(false);
		InitVideoSystem();
	}
	if (!(PicoOpt & PicoOpt_alt_renderer)) 
	{
		DrawLineDest = (unsigned short *) video::get_framebuffer() + VBuffOffset; 
	}
}

static void BlitFrame()
{
	if (PicoOpt & PicoOpt_alt_renderer) 
	{
		int i, u;

		if (Pico.m.dirtyPal)
		{
			unsigned int *spal = (unsigned int *)Pico.cram;
			unsigned int *dpal = (unsigned int *)rgb565pal;
			for (i = 0x3f / 2; i >= 0; i--)
				dpal[i] = ((spal[i] & 0x000f000f) << 12) | ((spal[i] & 0x00f000f0) << 3) | ((spal[i] & 0x0f000f00) >> 7);
			Pico.m.dirtyPal = 0;
		}

		unsigned short *dst = (unsigned short *)video::get_framebuffer() + VBuffOffset;
		unsigned char  *src = (unsigned char  *)PicoDraw2FB + (320 + 8) * 8 + 8;
		if(IS_W320) {
			for (i = 0; i < 224; i++, src += 8) {
				for (u = 0; u < 320; u++) *dst++ = rgb565pal[*src++];
			}
		} else {
			for (i = 0; i < 224; i++, src += (8 + 64), dst += 64) {
				for (u = 0; u < 256; u++) *dst++ = rgb565pal[*src++];
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

extern "C" int check_zip(const char *filePath)
{
	return zip::check(filePath);
}

#ifdef _DEBUG
extern "C" void lprintf(const char *fmt, ...) 
{
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);

	debug::out(buf);
}
#else
extern "C" void lprintf(const char *, ...) {} 
#endif

extern "C" void menu_romload_prepare(const char *)
{
	PicoCartLoadProgressCB = NULL; 
	PicoCDLoadProgressCB = NULL;
}

extern "C" void menu_romload_end(void)
{
	PicoCartLoadProgressCB = NULL; 
	PicoCDLoadProgressCB = NULL;
}

extern "C" void emu_noticeMsgUpdated(void)
{
	//	do nothing for now
}

extern "C" void emu_getMainDir(char *dst, int len)
{
	strncpy(dst, fsys::getEmulPath(), len);
	strcat (dst, DIR_SEP);
}

