#ifndef _SHARED_H_
#define _SHARED_H_

enum {
	PicoOpt_enable_ym2612_dac		= 0x00001,		//	ON
	PicoOpt_enable_sn76496			= 0x00002,		//	ON
	PicoOpt_enable_z80				= 0x00004,		//	depends on user settings
	PicoOpt_stereo_sound			= 0x00008,		//	depends on user settings
	PicoOpt_alt_renderer			= 0x00010,		//	depends on user settings
	PicoOpt_6button_gamepad			= 0x00020,		//	depends on user settings
	PicoOpt_accurate_timing			= 0x00040,		//	depends on user settings
	PicoOpt_accurate_sprites		= 0x00080,		//	depends on user settings
	PicoOpt_draw_no_32col_border	= 0x00100,		//	ON
	PicoOpt_external_ym2612			= 0x00200,		//	ON
	PicoOpt_enable_cd_pcm			= 0x00400,		//	ON
	PicoOpt_enable_cd_cdda			= 0x00800,		//	depends on user settings
	PicoOpt_enable_cd_gfx			= 0x01000,		//	OFF
	PicoOpt_cd_perfect_sync			= 0x02000,  	//	OFF
	PicoOpt_enable_cd_ramcart		= 0x08000,		//	ON
	PicoOpt_disable_vdp_fifo		= 0x10000		//	internal
};

enum {
	PicoVideo_UNDEFINED				= -1,
	PicoVideo_BGR444				=  0,
	PicoVideo_RGB555				=  1,
	PicoVideo_8BIT_INDEXED			=  2
};

#endif //_SHARED_H_
