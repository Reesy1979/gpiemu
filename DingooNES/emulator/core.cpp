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

#include <ctype.h>

#include "ips.patcher.h"

#include "driver.h"
#include "fceu.h"
#include "fds.h"
#include "vsuni.h"
#include "cheat.h"
#include "framework.h"

using namespace fw;
using namespace hw;

////////////////////////////////////////////////////////////////////////////////

//	NES Screen defines
#define NES_SCR_W		256
#define NES_SCR_H		240
#define NES_SCR_CX		NES_SCR_W / 2
#define NES_SCR_CY		NES_SCR_H / 2

//	NES Gamepad defines
#define GAMEPAD_U		(1 << 4)
#define GAMEPAD_D		(1 << 5)
#define GAMEPAD_L		(1 << 6)
#define GAMEPAD_R		(1 << 7)
#define GAMEPAD_ST		(1 << 3)
#define GAMEPAD_SL		(1 << 2)
#define GAMEPAD_A		(1 << 0)
#define GAMEPAD_B		(1 << 1)

//	NES Zapper defines
#define nesZapperX		nesZapper[0]
#define nesZapperY		nesZapper[1]
#define nesZapperB		nesZapper[2]
#define NZ_MAX_SPEED	5.00
#define NZ_SPEED_MUL	1.05

////////////////////////////////////////////////////////////////////////////////

//	FCEUX externals used in Core
extern bool compressSavestates;
extern bool	backupSavestates;
extern int	rapidAlternator;

//	NES Gamepad implementation variables
       bool			nesGamepadAvailable;
static uint32_t 	nesGamepad;

//	
static gfx_color	nes_pal[256];
static bool			nes_pal_dirty;
static uint8_t*		nes_gfx;
static int*			nes_snd;
static int			nes_snd_count;

//
static int 			nes_gfx_fl;
static int 			nes_gfx_ll;
static int 			nes_gfx_h;
static int			nes_gfx_w;
static int 			nes_fps;

//	NES Zapper implementation variables
       bool			nesZapperAvailable;
static float		nesZapperMove;
static uint32_t		nesZapper[3];

//
static int			VBuffOffset;
static uint32_t		lastDPad;
static int 			fdsChangeStep;

uint8_t autoFirePattern[15] = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x21, 0x22, 0x23, 0x24, 0x31, 0x32, 0x33, 0x41, 0x42, 0x51 };

//	Forward declarations
static void ApplyEmulationSettings();
static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

////////////////////////////////////////////////////////////////////////////////
//	Helper routines
////////////////////////////////////////////////////////////////////////////////

void ShowMessage(char *format, int disppos = 0, ...)
{
	char msg[41];

	va_list ap;
	va_start(ap, disppos);
	vsnprintf(msg, sizeof(msg) - 1, format, ap);
	va_end(ap);

	video::set_osd_msg(msg, 60);
}

static bool IsFilenameStateFile(const char *fn)
{
	const char *p = strstr(fn, SAVESTATE_EXT);
	return (p && *(p - 1) == '.');
}

static bool FindTarget(bool change, bool moving, int& boundX0, int& boundX1, int& boundY0, int& boundY1)
{
	bool isFound = false;
	extern uint8 SPRAM[0x100];	
	static uint8 spram[0x100];
	
	static bool isReverse = true;
	if(change) isReverse ^= true;
	
	extern uint8 PPU[4];
	int h = (PPU[0] & 0x20) ? 16 : 8;
	
	int i, n, x, y, dx, dy, fdx = 0, fdy = 0, adx, ady;
	for(n = 0; n < 256; n += 4)
	{
		if(!(i = isReverse ? 256 - n - 4 : n)) continue;
		
		x = (int)(SPRAM[i + 3] + 0);
		y = (int)(SPRAM[i + 0] + 1);
		
		if(y < 240)
		{
			if(moving)
			{
				dx = x - (int)spram[i + 3];
				dy = y - (int)spram[i + 0];
				
				adx = dx < 0 ? -dx : dx;
				ady = dy < 0 ? -dy : dy;
				
				if((adx && adx <= 32) || (ady && ady <= 32))
				{
					if(isFound)
					{
						if(dx == fdx && dy == fdy)
						{
							if(x >= boundX0 - 8 && x <= boundX1 && y >= boundY0 - h && y <= boundY1)
							{
								if(x < boundX0) boundX0 = x;
								if(y < boundY0) boundY0 = y;
								if(x + 8 > boundX1) boundX1 = x + 8;
								if(y + h > boundY1) boundY1 = y + h;
							}
						}
					} else 
					{
						fdx = dx;
						fdy = dy;
						boundX0 = x;
						boundY0 = y;
						boundX1 = x + 8;
						boundY1 = y + h;
						isFound = true;
					}
				}
			} else
			{
				if(isFound)
				{
					if(x >= boundX0 - 8 && x <= boundX1 && y >= boundY0 - h && y <= boundY1)
					{
						if(x < boundX0) boundX0 = x;
						if(y < boundY0) boundY0 = y;
						if(x + 8 > boundX1) boundX1 = x + 8;
						if(y + h > boundY1) boundY1 = y + h;
					}
				} else 
				{
					boundX0 = x;
					boundY0 = y;
					boundX1 = x + 8;
					boundY1 = y + h;
					isFound = true;
				}
			}
		}
		spram[i + 3] = x;
		spram[i + 0] = y;
	}
	
	return isFound;
}

////////////////////////////////////////////////////////////////////////////////
//	Emulation core implementation
////////////////////////////////////////////////////////////////////////////////

bool core::init()
{
	//	initialize FCEUX core
	compressSavestates = false;
	backupSavestates   = false;

	if(!FCEUI_Initialize()) return false;
	
	//	configure working pathes
	FCEUI_SetBaseDirectory(fsys::getGamePath());
	FCEUI_SetDirOverride(FCEUIOD_NV,     (char *)fsys::getGamePath());
	FCEUI_SetDirOverride(FCEUIOD_CHEATS, (char *)fsys::getGamePath());
	FCEUI_SetDirOverride(FCEUIOD_FDSROM, (char *)fsys::getEmulPath());
	return true;
}

void core::close()
{
	FCEUI_CloseGame();
	FCEUI_Kill();
}

bool core::load_rom()
{
	ips_define(settings.applyIPSpatch ? fsys::getGameRelatedFilePath("ips") : NULL);
	if(settings.countryRegion) {
		FCEUI_SetVidSystem(settings.countryRegion == 2 ? 1 : 0);
		return FCEUI_LoadGame(fsys::getGameFilePath(), 0);
	} else {
		FCEUI_SetVidSystem(0);
		return FCEUI_LoadGame(fsys::getGameFilePath(), 1);
	}
}

void core::hard_reset()
{
	//	reset input variables
	nesGamepadAvailable = true;
	nesZapperAvailable  = false;
	nesGamepad = 0;
	nesZapperX = NES_SCR_CX;
	nesZapperY = NES_SCR_CY;
	nesZapperB = 0;

	//	configure port #1
	if(GameInfo->input[0] == SI_ZAPPER) 
	{
		FCEUI_SetInput(0, SI_ZAPPER, &nesZapper, 0);
		nesGamepadAvailable = false;
		nesZapperAvailable  = true;
	} else {
		FCEUI_SetInput(0, SI_GAMEPAD, &nesGamepad, 0);
	}

	//	configure port #2
	if(!nesZapperAvailable && GameInfo->input[1] == SI_ZAPPER) 
	{
		FCEUI_SetInput(1, SI_ZAPPER, &nesZapper, 0);
		nesZapperAvailable = true;
	} else {
		FCEUI_SetInput(1, SI_NONE, 0, 0);
	}
	
	//	configure video
	FCEUI_DisableSpriteLimitation(0);
	FCEUI_SetRenderedLines(8, 231, 0, 239);
	
	//	configure sound
	FCEUI_SetSoundQuality(0);
}

void core::soft_reset()
{
	nesGamepad = 0;
	nesZapperX = NES_SCR_CX;
	nesZapperY = NES_SCR_CY;
	nesZapperB = 0;
	FCEUI_ResetNES();
}

uint32_t core::system_fps()
{
	return nes_fps;
}

void core::start_emulation(bool withSound)
{
	ApplyEmulationSettings();
	InitVideoSystem();
	InitSoundSystem(withSound);
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{
	UpdateInput();
	if(fdsChangeStep) {
		if(fdsChangeStep == 99) FCEU_FDSInsert();	//	eject disk
		if(fdsChangeStep == 59) FCEU_FDSSelect();	//	switch disk side
		if(fdsChangeStep == 19) FCEU_FDSInsert();	//	insert disk back
		fdsChangeStep--;
	}
	
	FCEUI_Emulate(&nes_gfx, (int32**)&nes_snd, (int32*)&nes_snd_count, !withRendering);
	if(nes_gfx) {
		uint8_t*   src = nes_gfx + nes_gfx_fl * nes_gfx_w;
		gfx_color* dst = video::get_framebuffer() + VBuffOffset;
		video::indexed_to_rgb(nes_pal, nes_pal_dirty, src, nes_gfx_w, nes_gfx_h, dst);
		nes_pal_dirty = false;
		withRendering = true;
	} else
		withRendering = false;

	//if(soundBuffer >= 0 && nes_snd_count)
	//	sound::fill_s32(soundBuffer, nes_snd, nes_snd_count);
		
	if(soundBuffer >= 0)
	{
		sound::fill_s32(soundBuffer, nes_snd, nes_snd_count);
	}
		
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	FCEUI_Emulate(&nes_gfx, (int32**)&nes_snd, (int32*)&nes_snd_count, 0);
	video::indexed_to_rgb(nes_pal, 1, nes_gfx + nes_gfx_fl * nes_gfx_w, nes_gfx_w, nes_gfx_h, video::get_framebuffer());
	console_scr_w = nes_gfx_w;
	console_scr_h = nes_gfx_h;
}

void core::stop_emulation(bool withSound)
{
	if(withSound) sound::close();
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)	//	TODO: error checking
{
	if(state::useFile(filePath))
	{
		if(load) 
		{
			FCEUI_LoadState(filePath);
		}
		else
		{
			FCEUI_SaveState(filePath);
		}
	} 
	else
	{
		if(state::useMemory(memBuffer)) 
		{
			char fakename[] = "state." SAVESTATE_EXT "0";
			if(load) FCEUI_LoadState(fakename); else FCEUI_SaveState(fakename);
		}
	}
	return true;
}

int core::get_cheat_count()
{
	return cheatCodeCount;
}

cstr_t core::get_cheat_name(int cheatIndex)
{
	char *name;
	int index = cheatCodeMap[cheatIndex] >> 8;
	FCEUI_GetCheat(index, &name, NULL, NULL, NULL, NULL, NULL);
	return name;
}

bool core::get_cheat_active(int cheatIndex)
{
	int status;
	int index = cheatCodeMap[cheatIndex] >> 8;
	FCEUI_GetCheat(index, NULL, NULL, NULL, NULL, &status, NULL);
	return bool(status);
}

void core::set_cheat_active(int cheatIndex, bool enable)
{
	int index = cheatCodeMap[cheatIndex] >> 8;
	int count = cheatCodeMap[cheatIndex] & 0xFF;
	while(count-- > 0) 
		FCEUI_ToggleCheat(index++, int(enable));
}

////////////////////////////////////////////////////////////////////////////////
//	Emulation core private functions
////////////////////////////////////////////////////////////////////////////////

static void ApplyEmulationSettings()
{
	//	set Turbo signal pattern
	int onframes  = (autoFirePattern[settings.turboSignal] & 0xF0) >> 4;
	int offframes = (autoFirePattern[settings.turboSignal] & 0x0F);
	SetAutoFirePattern(onframes, offframes);
}

static void InitVideoSystem()
{
	nes_fps = FCEUI_GetCurrentVidSystem(&nes_gfx_fl, &nes_gfx_ll) ? 50 : 60;
	nes_gfx_h = nes_gfx_ll - nes_gfx_fl + 1;
	nes_gfx_w = NES_SCR_W;

	if(settings.fullScreen) {
		video::set_mode(settings.scalingMode ? VM_SCALE_HW : VM_SCALE_SW, nes_gfx_w, nes_gfx_h, settings.verStretch);
		VBuffOffset = 0;
	} else {
		video::set_mode(VM_FAST, nes_gfx_w, nes_gfx_h, settings.verStretch);
		VBuffOffset = ((HW_SCREEN_WIDTH - nes_gfx_w) >> 1) + ((HW_SCREEN_HEIGHT - nes_gfx_h) >> 1) * HW_SCREEN_WIDTH;
	}

	nes_pal_dirty  = true;
}

static void InitSoundSystem(bool hasSound)
{
	if (hasSound) {
		FCEUI_Sound(settings.soundRate);
		sound::init(settings.soundRate, false, core::system_fps());
	} else  {
		FCEUI_Sound(0);
	}
}

static void UpdateInput()
{
	#define GAMEPAD nesGamepad
	uint32_t controls = emul::handle_input();
	GAMEPAD = 0;

	//	switch FDS disk side
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_SELECT))
	{
		if(isFDS && !fdsChangeStep) 
		{
			video::set_osd_msg("Try to switch disk side...", 30);
			fdsChangeStep = 99;
		}
		return;
	}

	//	insert coin in VS game
	if (controls == (HW_INPUT_L | HW_INPUT_R | HW_INPUT_START))
	{
		if (!video::has_osd_msg())
		{
			video::set_osd_msg("Insert coin", 60);
			FCEU_VSUniCoin();
		}
		return;
	}

	//	set up NES Gamepad state
	if(controls)
	{
		if (controls & HW_INPUT_UP   ) GAMEPAD |= GAMEPAD_U;
		if (controls & HW_INPUT_DOWN ) GAMEPAD |= GAMEPAD_D;
		if (controls & HW_INPUT_LEFT ) GAMEPAD |= GAMEPAD_L;
		if (controls & HW_INPUT_RIGHT) GAMEPAD |= GAMEPAD_R;
		if (controls & input::get_mask(settings.pad_config[0])) GAMEPAD |= GAMEPAD_ST;
		if (controls & input::get_mask(settings.pad_config[1])) GAMEPAD |= GAMEPAD_SL;
		if (controls & input::get_mask(settings.pad_config[2])) GAMEPAD |= GAMEPAD_A;
		if (controls & input::get_mask(settings.pad_config[3])) GAMEPAD |= GAMEPAD_B;
		if (rapidAlternator) 
		{
			if (controls & input::get_mask(settings.pad_config[4])) GAMEPAD |= GAMEPAD_A;
			if (controls & input::get_mask(settings.pad_config[5])) GAMEPAD |= GAMEPAD_B;
		}
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
	
	//	set up NES Zapper state
	if(nesZapperAvailable && settings.zapperEnable)
	{
		nesZapperB = 0;

		//	NES Zapper auto aim algorithm
		if(settings.zapperAutoAim)
		{
			int boundX0, boundX1;
			int boundY0, boundY1;
			static bool  changeT;
			static bool  isFound;
			if(GAMEPAD & (GAMEPAD_A | GAMEPAD_B)) {
				if(changeT) nesZapperMove = NZ_MAX_SPEED;
				isFound = FindTarget(changeT, true, boundX0, boundX1, boundY0, boundY1);
				changeT = false;
			} else {
				isFound = FindTarget(false, true, boundX0, boundX1, boundY0, boundY1);
				changeT = true;
			}
			if(!isFound) {
				isFound = FindTarget(false, false, boundX0, boundX1, boundY0, boundY1);
			}
			if(isFound) {
				int cx = (boundX0 + boundX1) / 2;
				int cy = (boundY0 + boundY1) / 2;
				if(int(nesZapperX) < cx - 2 && !(GAMEPAD & GAMEPAD_L)) GAMEPAD |= GAMEPAD_R;
				if(int(nesZapperX) > cx + 2 && !(GAMEPAD & GAMEPAD_R)) GAMEPAD |= GAMEPAD_L;
				if(int(nesZapperY) < cy - 2 && !(GAMEPAD & GAMEPAD_U)) GAMEPAD |= GAMEPAD_D;
				if(int(nesZapperY) > cy + 2 && !(GAMEPAD & GAMEPAD_D)) GAMEPAD |= GAMEPAD_U;
			}
		}
		
		//	NES Zapper manual aim algorithm
		if(GAMEPAD & GAMEPAD_L) if((nesZapperX -= int(nesZapperMove)) >= NES_SCR_W) nesZapperX = NES_SCR_W - 1; 
		if(GAMEPAD & GAMEPAD_R) if((nesZapperX += int(nesZapperMove)) >= NES_SCR_W) nesZapperX = 0; 
		if(GAMEPAD & GAMEPAD_U) if((nesZapperY -= int(nesZapperMove)) >= NES_SCR_H) nesZapperY = NES_SCR_H - 1; 
		if(GAMEPAD & GAMEPAD_D) if((nesZapperY += int(nesZapperMove)) >= NES_SCR_H) nesZapperY = 0;
		if(GAMEPAD & GAMEPAD_A) nesZapperB = 1;
		
		if(GAMEPAD & (GAMEPAD_L | GAMEPAD_R | GAMEPAD_U | GAMEPAD_D)) {
			if((nesZapperMove *= NZ_SPEED_MUL) > NZ_MAX_SPEED) nesZapperMove = NZ_MAX_SPEED;
		} else {
			nesZapperMove = 1.0;
		}
		GAMEPAD &= (GAMEPAD_ST | GAMEPAD_SL);
	}
}

////////////////////////////////////////////////////////////////////////////////
//	Support code for external emulation engine
////////////////////////////////////////////////////////////////////////////////

class STATE_FILE : public EMUFILE_FILE 
{
	void open(const char* fname, const char* mode)
	{
		fp = (FILE*)state::fopen(mode);
		if(!fp)	failbit = true;
		this->fname = fname;
		strcpy(this->mode,mode);
	}

public:

	STATE_FILE(const std::string& fname, const char* mode)
		: EMUFILE_FILE("fuck", "rb")
	{
		open(fname.c_str(), mode);
	}
	
	STATE_FILE(const char* fname, const char* mode)
		: EMUFILE_FILE("fuck", "rb")
	{
		open(fname, mode);
	}

	virtual ~STATE_FILE() 
	{
		if(fp) state::fclose();
		fp = NULL;
	}

	virtual int fgetc() 
	{
		return state::fgetc();
	}
	
	virtual int fputc(int c) 
	{
		return state::fputc(c);
	}

	virtual size_t _fread(const void *ptr, size_t bytes)
	{
		size_t ret = state::fread((void*)ptr, bytes);
		if(ret < bytes)	failbit = true;
		return ret;
	}

	virtual void fwrite(const void *ptr, size_t bytes)
	{
		size_t ret = state::fwrite((void*)ptr, bytes);
		if(ret < bytes)	failbit = true;
	}

	virtual int fseek(int offset, int origin) 
	{ 
		return state::fseek(offset, origin);
	}

	virtual int ftell()
	{
		return state::ftell();
	}

	virtual int size() 
	{ 
		int oldpos = ftell();
		fseek(0,SEEK_END);
		int len = ftell();
		fseek(oldpos,SEEK_SET);
		return len;
	}

	virtual void fflush() {}
};

////////////////////////////////////////////////////////////////////////////////
bool	turbo = 0;
int		closeFinishedMovie = 0;

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_GetPalette(uint8, uint8*, uint8*, uint8*) {}
void 	FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b)
{
	nes_pal[index] = gfx_color_rgb(r, g, b);
	nes_pal_dirty  = true;
}

////////////////////////////////////////////////////////////////////////////////
FILE*	FCEUD_UTF8fopen(const char *fn, const char *mode) 
{
	return fopen(fn, mode);
}

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *fn, const char *m) 
{
	EMUFILE_FILE *f = IsFilenameStateFile(fn) 
		? new STATE_FILE(fn, m)
		: new EMUFILE_FILE(fn, m);
	if(!f->is_open()) {
		delete f;
		return NULL;
	} else
		return f;
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord&, std::string&, int) 
{ 
	return NULL; 
}

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord&, std::string&, std::string*) 
{ 
	return NULL; 
}

ArchiveScanRecord FCEUD_ScanArchive(std::string) 
{ 
	return ArchiveScanRecord(); 
}

////////////////////////////////////////////////////////////////////////////////
const char* FCEUD_GetCompilerString() { return ""; }

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_PrintError(const char*) {}
void 	FCEUD_Message(const char*) {}

////////////////////////////////////////////////////////////////////////////////
int 	FCEUD_SendData(void*, uint32) { return 1; }
int 	FCEUD_RecvData(void*, uint32) { return 1; }
void 	FCEUD_NetplayText(uint8*) {}
void 	FCEUD_NetworkClose() {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUI_UseInputPreset(int) {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_SoundToggle() {}
void 	FCEUD_SoundVolumeAdjust(int) {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_SaveStateAs() {}
void 	FCEUD_LoadStateFrom() {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_SetInput(bool, bool, ESI, ESI, ESIFC) {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_MovieRecordTo() {}
void 	FCEUD_MovieReplayFrom() {}

////////////////////////////////////////////////////////////////////////////////
bool 	FCEUD_ShouldDrawInputAids() { return false; }

////////////////////////////////////////////////////////////////////////////////
void 	FCEUI_AviVideoUpdate(const unsigned char*) {}
bool 	FCEUI_AviIsRecording() { return false; }
bool 	FCEUI_AviDisableMovieMessages() { return true; }
void 	FCEUD_AviRecordTo() {}
void 	FCEUD_AviStop() {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_SetEmulationSpeed(int) {}
void 	FCEUD_TurboOn() {}
void 	FCEUD_TurboOff() {}
void 	FCEUD_TurboToggle() {}

////////////////////////////////////////////////////////////////////////////////
int		FCEUD_ShowStatusIcon() { return 0; }
void 	FCEUD_ToggleStatusIcon() {}
void 	FCEUD_HideMenuToggle() {}

////////////////////////////////////////////////////////////////////////////////
void 	FCEUD_DebugBreakpoint() {}
bool 	FCEUD_PauseAfterPlayback() { return 0; }
void 	FCEUD_VideoChanged() {}

//	hack for netplay.cpp ///////////////////////////////////////////////////////
int		FCEUnetplay = 0;
void	NetplayUpdate(uint8*) {}
int		FCEUNET_SendCommand(uint8, uint32) { return 0;}

//	hack for nsf.cpp ///////////////////////////////////////////////////////////
#include   "nsf.h"
NSF_HEADER NSFHeader;
int		NSFLoad(const char*, FCEUFILE*) { return 0; }
void	DoNSFFrame() {}
void	DrawNSF(uint8*) {}

// hack for drawing.cpp ////////////////////////////////////////////////////////
void	DrawTextLineBG(uint8 *) {}
void	DrawMessage(bool) {}
void	FCEU_DrawRecordingStatus(uint8*) {}
void	FCEU_DrawNumberRow(uint8*, int*, int) {}
void	DrawTextTrans(uint8*, uint32, uint8*, uint8) {}
void	DrawTextTransWH(uint8*, uint32, uint8*, uint8, int, int, int) {}

// hack for debug.cpp //////////////////////////////////////////////////////////
volatile int   datacount;
volatile int   undefinedcount;
unsigned char* cdloggerdata;
int		GetPRGAddress(int) { return 0; }
