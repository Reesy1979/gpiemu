#include "framework.h"
#include "systype.h"
#include "fb.h"
#include "emu.h"
#include "rc.h"
#include "pcm.h"
#include "unzip.h"
#include "gbhw.h"
#include "sound.h"
#include "save.h"
#include "exports.h"


using namespace fw;
using namespace hw;

///////////////////////////////////////////////////////////////////////////////

static int			VBuffOffset;
static uint32_t		lastDPad;

static bool keyboardKeyVisible;
static int  keyboardKeyIndex;

struct fb fb;

///////////////////////////////////////////////////////////////////////////////

static void InitVideoSystem();
static void InitSoundSystem(bool hasSound);
static void UpdateInput();

///////////////////////////////////////////////////////////////////////////////
struct pcm pcm;

extern "C" void Debug(char *msg, ...)
{
	//va_list ap;
	//va_start(ap, msg);
	//vprintf(msg, ap);
	//va_end(ap);
}

extern "C" void ev_poll(int wait)
{
}

extern "C" void vid_setpal(int i, int r, int g, int b)
{

}

static char loader_error[1024];

extern "C" char* loader_get_error(void)
{
	return loader_error;
}

extern "C" void loader_set_error(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(loader_error, sizeof loader_error, fmt, ap);
	va_end(ap);
}

extern "C" void sys_sanitize(char *rom)
{

}

extern "C" void vid_begin()
{
	
}

extern "C" void vid_end()
{
	
}

extern "C" void vid_settitle(char *title)
{

}

extern "C" void pcm_init()
{

}

static byte sampleBuffer[48000];

extern "C" int pcm_submit()
{

}

extern "C" void pcm_close()
{

}

extern "C" void pcm_pause(int dopause)
{
	
}

extern "C" void sys_sleep(int us)
{

}

char savedir[HW_MAX_PATH];
char *psavedir=(char*)&savedir;
bool core::init()
{
	init_exports();
	strcpy(savedir, fsys::getEmulPath());
	rc_setvar("savedir", 1, &psavedir);
	return true;
}

void core::close()
{
	//system_poweroff();
}

extern "C" int core_load_rom(char *filename, void **data, int *len)
{
	debug::printf("core_load_rom start\n");
	if(fw::zip::check(filename))
    	{
    		int result = 0;
		unz_file_info info;
		
		unzFile fd = unzOpen(filename);	
		if(fd) 
		{	
			if(unzGoToFirstFile(fd) == UNZ_OK && unzGetCurrentFileInfo(fd, &info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
			{
				*data = (byte *)malloc(info.uncompressed_size);
				if(*data)
				{
					*len = (int)info.uncompressed_size;
					if(*len < 0x4000) *len = 0x4000;

					if(unzOpenCurrentFile(fd) == UNZ_OK)
					{
						if(unzReadCurrentFile(fd, *data, *len) == info.uncompressed_size)
						{					
							result = 1;//all good
						}
						unzCloseCurrentFile(fd);
					}
				}
			}
			unzClose(fd);
		}
		debug::printf("core_load_rom exit:%d\n", result);
		return result;  	
    	}
    	else
    	{
		/* Read uncompressed file */
		FILE *fd = fopen(filename, "rb");
		if(!fd) return 0;

		/* Seek to end of file, and get size */
		fseek(fd, 0, SEEK_END);
		*len =  ftell(fd);
		fseek(fd, 0, SEEK_SET);

		if (*len < 0x4000) *len = 0x4000;
		*data = (byte *) malloc(*len);
		if(!*data) return 0;

		fread(*data, (size_t)*len, 1, fd);
		fclose(fd);
    	}
	return 1;
}
   	
extern "C" int loader_init(char *s);

bool core::load_rom()
{
	debug::printf("File Path: %s\n", fsys::getGameFilePath());
	if(!loader_init((char*)fsys::getGameFilePath())) {
		if(settings.countryRegion) {
			//sms.display   = (settings.countryRegion == 2) ? DISPLAY_PAL : DISPLAY_NTSC;
			//sms.territory = (settings.countryRegion == 3) ? TERRITORY_DOMESTIC : TERRITORY_EXPORT;
		}
		return true;
	}
	return false;
}

extern "C" void emu_reset(void);

void core::hard_reset()
{
	//	init system components
	emu_reset();
}

void core::soft_reset()
{
	emu_reset();
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
	pcm.pos=0;
	UpdateInput();

	fb.ptr = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);

	emu_run();
	
	if(soundBuffer>=0)
	{
		hw::sound::fill_s8(soundBuffer, (char*)sampleBuffer, pcm.pos);
		pcm.pos=0;
	}
}

void core::emulate_for_screenshot(int& console_scr_w, int& console_scr_h)
{
	VBuffOffset = 0;
	fb.ptr = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
        int oldlen=pcm.len;
        pcm.len=0;//ensure sound disabled
	emu_run();
	pcm.len=oldlen;//restore sound
	
	console_scr_w = fb.w;
	console_scr_h = fb.h;
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
			if(load) system_load_state(); else system_save_state();
			state::fclose();
			return false;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////

extern "C" void lcd_reset2(void);

static void InitVideoSystem()
{
	fb.yuv = 0;
	fb.w = 160;
	fb.h = 144;
	fb.pelsize = 2;
	fb.pitch = HW_SCREEN_WIDTH*fb.pelsize;
	fb.indexed = 0;
	fb.delegate_scaling = 1;

	//return (r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3;
	//rrrr rggg gggb bbbb
	fb.cc[0].r = 3;
	fb.cc[0].l = 11;
	fb.cc[1].r = 2;
	fb.cc[1].l = 5;
	fb.cc[2].r = 3;
	fb.cc[2].l = 0;

	fb.enabled = 1;
	fb.dirty = 0;
	
	if(settings.fullScreen) {
		hw::video::set_mode(settings.scalingMode ? hw::VM_SCALE_HW : hw::VM_SCALE_SW, 160, 144, true);
		VBuffOffset = 0;
	} else {
		hw::video::set_mode(hw::VM_FAST, 160, 144, true);
		VBuffOffset = ((HW_SCREEN_WIDTH - 160)>>1) + ((HW_SCREEN_HEIGHT - (144)) >> 1) * HW_SCREEN_WIDTH;

	}
	fb.ptr = (uint8_t*)(hw::video::get_framebuffer() + VBuffOffset);
	lcd_reset2();
}




static void InitSoundSystem(bool hasSound)
{
	if(hasSound)
	{
		pcm.hz = settings.soundRate;
		pcm.stereo = 0;
		pcm.len = settings.soundRate/core::system_fps(); 
		pcm.buf = (byte*)&sampleBuffer;
		pcm.pos = 0;
		memset(pcm.buf, 0, pcm.len);
		hw::sound::init(settings.soundRate, false, core::system_fps());

	}
	else
	{
		pcm.hz = 0;
		pcm.stereo = 0;
		pcm.len = 0; 
		pcm.buf = 0;
		pcm.pos = 0;
	}

	sound_init();
}

static void printSelectedKey()
{
	cstr_t KEYBOARD_KEYS = "0123456789*#";
	char message[15];

	int src = 0, dst = 0; 
	while(src < 12) {
		if(src == keyboardKeyIndex) {
			message[dst++] = '(';
			message[dst++] = KEYBOARD_KEYS[src++];
			message[dst++] = ')';
		} else
			message[dst++] = KEYBOARD_KEYS[src++];
	}
	message[dst] = 0;

	hw::video::set_osd_msg(message, 45);
	keyboardKeyVisible = true;
}

static void UpdateInput()
{
	#define GAMEPAD  gbhw.pad

	uint32_t controls = fw::emul::handle_input();
	
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


