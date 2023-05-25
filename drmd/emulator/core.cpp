#include "framework.h"

using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

static int			VBuffOffset;


///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

///////////////////////////////////////////////////////////////////////////////




bool core::init()
{

	return true;
}

void core::close()
{

}

bool core::load_rom()
{
	debug::printf("File Path: %s\n", fsys::getGameFilePath());
#if 0
	if(!loader_init((char*)fsys::getGameFilePath())) {
		if(settings.countryRegion) {
			//sms.display   = (settings.countryRegion == 2) ? DISPLAY_PAL : DISPLAY_NTSC;
			//sms.territory = (settings.countryRegion == 3) ? TERRITORY_DOMESTIC : TERRITORY_EXPORT;
		}
		return true;
	}
#endif
	return false;
}

void core::hard_reset()
{

}

void core::soft_reset()
{

}

uint32_t core::system_fps()
{
	return 60;//snd.fps;
}

void core::start_emulation(bool withSound)
{
	InitVideoSystem();
	InitSoundSystem(withSound);
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{

}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	VBuffOffset = 0;
	//fb.ptr = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
    //    int oldlen=pcm.len;
    //    pcm.len=0;//ensure sound disabled
	//emu_run();
	//pcm.len=oldlen;//restore sound
	
	//console_scr_w = fb.w;
	//console_scr_h = fb.h;
}

void core::stop_emulation(bool withSound)
{
	if(withSound) 
		hw::sound::close();
}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{
	if(state::useFile(filePath) || state::useMemory(memBuffer)) 
	{
		if(state::fopen(load ? "rb" : "wb")) 
		{
#if 0
			if(load) system_load_state(); else system_save_state();
			state::fclose();
			return false;
#endif
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem()
{
	if(settings.fullScreen) {
		hw::video::set_mode(settings.scalingMode ? hw::VM_SCALE_HW : hw::VM_SCALE_SW, 160, 144, true);
		VBuffOffset = 0;
	} else {
		hw::video::set_mode(hw::VM_FAST, 160, 144, true);
		VBuffOffset = ((HW_SCREEN_WIDTH - 160)>>1) + ((HW_SCREEN_HEIGHT - (144)) >> 1) * HW_SCREEN_WIDTH;

	}
	//fb.ptr = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
}

static void InitSoundSystem(bool hasSound)
{
	if(hasSound)
	{
		hw::sound::init(settings.soundRate, false, core::system_fps());

	}
	else
	{

	}

	//sound_init();
}

static void UpdateInput()
{
	uint32_t controls = fw::emul::handle_input();
#if 0	
	GAMEPAD = 0;
	if(controls)
	{
		if(controls & HW_INPUT_UP   ) GAMEPAD |= PAD_UP;
		if(controls & HW_INPUT_DOWN ) GAMEPAD |= PAD_DOWN;
		if(controls & HW_INPUT_LEFT ) GAMEPAD |= PAD_LEFT;
		if(controls & HW_INPUT_RIGHT) GAMEPAD |= PAD_RIGHT;

		if (controls & input::get_mask(settings.pad_config[2])) GAMEPAD |= PAD_START;
		if (controls & input::get_mask(settings.pad_config[3])) GAMEPAD |= PAD_SELECT;
		if (controls & input::get_mask(settings.pad_config[0])) GAMEPAD |= PAD_A;
		if (controls & input::get_mask(settings.pad_config[1])) GAMEPAD |= PAD_B;
	}  
#endif    
}

///////////////////////////////////////////////////////////////////////////////

void system_manage_sram(uint8_t *sram, int slot, int mode)	//	TODO: need checking
{
#if 0
	cstr_t name = fsys::getGameRelatedFilePath(SRAM_FILE_EXT);
    FILE *fd;

    switch(mode) {
    case SRAM_SAVE:
        if(sms.save) {
            fd = fopen(name, "wb");
            if(fd) {
                fwrite(sram, 0x8000, 1, fd);
                fclose(fd);
            }
        }
        break;

    case SRAM_LOAD:
        fd = fopen(name, "rb");
        if(fd) {
            sms.save = 1;
            fread(sram, 0x8000, 1, fd);
            fclose(fd);
        } else {
            memset(sram, 0x00, 0x8000);
        }
        break;
    }
#endif
}


