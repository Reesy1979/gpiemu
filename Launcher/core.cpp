#include "png_backdrop.h"
#include "framework.h"



using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

static int			VBuffOffset;
static uint32_t		lastDPad;
static gfx_texture* backdrop;

static bool keyboardKeyVisible;
static int  keyboardKeyIndex;


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

extern "C" int core_load_rom(char *filename, void **data, int *len)
{

}
   	
bool core::load_rom()
{

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
	
}

void core::emulate_frame(bool& withRendering, int soundBuffer)
{

}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{

}

void core::stop_emulation(bool withSound)
{

}

bool core::save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load)
{

}

///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem()
{

}




static void InitSoundSystem(bool hasSound)
{

}

static void printSelectedKey()
{

}

static void UpdateInput()
{

}

///////////////////////////////////////////////////////////////////////////////

void system_manage_sram(uint8_t *sram, int slot, int mode)	//	TODO: need checking
{

}


