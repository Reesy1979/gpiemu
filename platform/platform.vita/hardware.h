/* DEP - Dingoo Emulation Pack for Dingoo A320
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
 
#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdint.h>
#include "hardware.defines.inc"

typedef uint16_t gfx_color;

typedef struct {
	void*         address;
	uint16_t      width, height;
} gfx_texture;

typedef struct {
	gfx_texture*  texture;
	gfx_color     colorKey;
	bool          colorize;
	gfx_color     fontColor;
} gfx_font;

static gfx_color gfx_color_rgb(uint8_t r, uint8_t g, uint8_t b) 
{
	return (r & 0xF8) << 8 | (g & 0xFC) << 3 | (b & 0xF8) >> 3;
}

static const gfx_color COLOR_RED     = 0xf800;
static const gfx_color COLOR_GREEN   = 0x07e0;
static const gfx_color COLOR_BLUE    = 0x001f;
static const gfx_color COLOR_YELLOW  = 0xffe0;
static const gfx_color COLOR_MAGENTA = 0xf81f;
static const gfx_color COLOR_TEAL    = 0x07ff;
static const gfx_color COLOR_BLACK   = 0x0000;
static const gfx_color COLOR_WHITE   = 0xffff;

////////////////////////////////////////////////////////////////////////////////

namespace hw
{

enum VIDEO_MODE
{
	VM_FAST,
	VM_SLOW,
	VM_SCALE_SW,
	VM_SCALE_HW
};

namespace video 
{
	//	framebuffer low level
	void 		 init(VIDEO_MODE mode);
	void 		 close();
	void 		 set_mode(VIDEO_MODE mode, int srcW, int srcH, bool stretch);
	VIDEO_MODE	 get_mode();
	void 		 prev_mode();
	gfx_color*	 get_framebuffer();
	void 		 set_osd_msg(const char* message, int timeOut);
	int 		 has_osd_msg();
	void 		 flip(int fps);
#ifdef INDEXED_IMAGE
	void 		 indexed_to_rgb(gfx_color* pal, bool pal_dirty, uint8_t* src, int srcW, int srcH, gfx_color* dst);
#endif

	//	textures
	gfx_texture* tex_allocate(int w, int h);
	gfx_texture* tex_load(const char* path);
	gfx_texture* tex_load(uint8_t* buffer, size_t size);
	void         tex_delete(gfx_texture* texture);

	//	fonts
	gfx_font* 	 font_load(const char* path, gfx_color key);
	gfx_font* 	 font_load(uint8_t* buffer, size_t size, gfx_color key);
	int          font_w(gfx_font* font, const char* text);
	int          font_h(gfx_font* font);
	void         font_delete(gfx_font* font);

	//	framebuffer and current font
	void 		 bind_framebuffer();
	void 		 term_framebuffer();
	void 		 font_set(gfx_font* font);
	gfx_font* 	 font_get();

	//	drawing
	void         draw_point(int x, int y, gfx_color color);
	void         draw_line(int x0, int y0, int x1, int y1, gfx_color color);
	void         draw_rect(int x, int y, int w, int h, gfx_color color);
	void         fill_rect(int x, int y, int w, int h, gfx_color color);
	void         mult_rect(int x, int y, int w, int h, gfx_color color);
	void 		 draw_rgb(int x, int y, gfx_color* rgb, int w, int h);
	void 		 draw_tex(int x, int y, gfx_texture* texture);
	void 		 draw_tex_transp(int x, int y, gfx_texture* texture);
	void 		 draw_tex_tiled(int x, int y, gfx_texture* texture, int xscroll, int yscroll);
	void 		 print(int x, int y, const char* text, gfx_color color);

	//	additional
	void 		 clear(gfx_color color);
	void 		 clear_all(bool with_flip);
	void 		 bilinear_scale(gfx_color* src, int old_w, int old_h, int old_pitch, gfx_color* dst, int new_w, int new_h, int new_pitch);
};

namespace sound 
{
	void 		init(int rate, bool stereo, int Hz);
	void 		close();
	void 		set_volume(int volume);
	int 		curr_buffer_index();
	int 		next_buffer_index(int index);
	int 		prev_buffer_index();
	short* 		get_buffer(int index);
	int 		get_sample_count(int index);
	void 		set_sample_count(int index, int count);
	void 		fill_s16(int index, short *src, int count);
	void 		fill_s16(int index, short **src, int count);
	void 		fill_s32(int index, int *src, int count);
};

namespace input 
{
	void 		init();
	uint32_t 	get_repeat();
	uint32_t 	get_held();
	void 		wait_for_release();
	void 		wait_for_press();
	void 		ignore();
	uint32_t 	read();
	uint32_t 	poll();
	uint32_t 	get_mask(int index);
};

namespace system
{
	void 		init();
	void 		close();
	void 		init_frames_counter(int framesPerSecond);
	uint32_t 	get_frames_count();
	void 		set_cpu_speed(int mhz);
	bool 		is_in_tv_mode();
};

};

#endif //_HARDWARE_H_
