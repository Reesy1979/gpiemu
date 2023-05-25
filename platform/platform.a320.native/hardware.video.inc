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
 
///////////////////////////////////////////////////////////////////////////////
//	Video module
///////////////////////////////////////////////////////////////////////////////

static VIDEO_MODE	video_mode;
static char     	video_msg[41];
static int			video_msg_timer;
static int			dst_offset;

static VIDEO_MODE 	sv_video_mode;
static int 			sv_srcW;
static int 			sv_srcH;
static int 			sv_stretch;

static gfx_color**	fb_lcd_ptr;
static gfx_color*	fb_sys = 0;
static gfx_color*	fb_mem = 0;
static gfx_color*	fb_ipu = 0;
static gfx_texture*	fb_gfx;

#ifdef INDEXED_IMAGE
static gfx_color*	indexed_pal;
static uint8_t*		indexed_src;
static bool			i_pal_dirty;
#endif

static gfx_font*	current_font;
static uint32_t		current_font_w;
static uint32_t		current_font_h;

#define AVERAGE(a, b) (((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)))
#define BUFFER_SIZE (HW_SCREEN_WIDTH * HW_SCREEN_HEIGHT * sizeof(gfx_color))

///////////////////////////////////////////////////////////////////////////////
//	Hardware scale using IPU
///////////////////////////////////////////////////////////////////////////////

#include "ipu.scaler.h"

static uint32_t hw_srcW;
static uint32_t hw_srcH;
static uint8_t* source_y = 0;
static uint8_t* source_u = 0;
static uint8_t* source_v = 0;

static uint8_t* rgb_to_y = 0;
static uint8_t* rgb_to_u = 0;
static uint8_t* rgb_to_v = 0;

#define RGB_TO_YUV()																\
	r = rgb >> 8 & 0xF8;															\
	g = rgb >> 3 & 0xFC;															\
	b = rgb << 3 & 0xF8;															\
	y = ((r *  2104 + g *  4130 + b *  802 + 4096 +    8192) & 0x7fffffff) >> 13;	\
	u = ((r * -1214 + g * -2384 + b * 3598 + 4096 + 1048576) & 0x7fffffff) >> 13;	\
	v = ((r *  3598 + g * -3013 + b * -585 + 4096 + 1048576) & 0x7fffffff) >> 13;
	
#define SRC_TO_YUV()			\
	c = *src++;					\
	*dst_y++ = *(rgb_y + c);	\
	*dst_u++ = *(rgb_u + c);	\
	*dst_v++ = *(rgb_v + c);

static void hw_scale_init(int srcW, int srcH)
{
	hw_srcW = srcW;
	hw_srcH = srcH;

	source_y = (uint8_t*)malloc(srcW * srcH);
	source_u = (uint8_t*)malloc(srcW * srcH);
	source_v = (uint8_t*)malloc(srcW * srcH);
	
	int dstH = dst_offset ? hw_srcH : HW_SCREEN_HEIGHT;
	int dstW = HW_SCREEN_WIDTH;

	ipu_init(srcW, srcH, dstW, dstH, dstW << 1, dstW << 1, dstH << 1);

#ifdef INDEXED_IMAGE
	rgb_to_y = (uint8_t*)malloc(0x100);
	rgb_to_u = (uint8_t*)malloc(0x100);
	rgb_to_v = (uint8_t*)malloc(0x100);
#else
	uint32_t rgb;
	uint8_t  r, g, b, y, u, v;

	rgb_to_y = (uint8_t*)malloc(0x10000);
	rgb_to_u = (uint8_t*)malloc(0x10000);
	rgb_to_v = (uint8_t*)malloc(0x10000);

	for(rgb = 0; rgb < 0x10000; rgb++) {
		RGB_TO_YUV()
		rgb_to_y[rgb] = y;
		rgb_to_u[rgb] = u;
		rgb_to_v[rgb] = v;
	}
#endif
}

static void hw_scale_deinit()
{
	if(source_y) { free(source_y); source_y = 0; }
	if(source_u) { free(source_u); source_u = 0; }
	if(source_v) { free(source_v); source_v = 0; }

	if(rgb_to_y) { free(rgb_to_y); rgb_to_y = 0; }
	if(rgb_to_u) { free(rgb_to_u); rgb_to_u = 0; }
	if(rgb_to_v) { free(rgb_to_v); rgb_to_v = 0; }
}

#ifdef INDEXED_IMAGE
static void hw_scale_indexed_pal_2_yuv(register gfx_color* pal)
{
	register uint32_t i, rgb;
	register uint8_t r, g, b, y, u, v;
	for(i = 0; i < 0x100; i++) {
		rgb = pal[i];
		RGB_TO_YUV()
		rgb_to_y[i] = y;
		rgb_to_u[i] = u;
		rgb_to_v[i] = v;
	}
}
#endif

#ifdef INDEXED_IMAGE
static void hw_scale_indexed_src_2_yuv(register uint8_t* src)
#else
static void hw_scale_rgb_2_yuv(register gfx_color* src)
#endif
{
	register gfx_color c;
	register uint8_t* dst_y = &source_y[0];
	register uint8_t* dst_u = &source_u[0];
	register uint8_t* dst_v = &source_v[0];

	const uint8_t* rgb_y = &rgb_to_y[0];
	const uint8_t* rgb_u = &rgb_to_u[0];
	const uint8_t* rgb_v = &rgb_to_v[0];

	register int x, y;

	for(y = hw_srcH - 1; y >= 0; y--)
	{
		for(x = hw_srcW - 1; x >= 0; x -= 16)
		{
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
			SRC_TO_YUV() SRC_TO_YUV()
		}
#ifndef INDEXED_IMAGE
		src += (HW_SCREEN_WIDTH - hw_srcW);
#endif
	}
}

static void hw_scale_start_ipu(gfx_color* dst)
{
	ipu_blit(source_y, source_u, source_v, dst + dst_offset);
}

///////////////////////////////////////////////////////////////////////////////
//	Software scale
///////////////////////////////////////////////////////////////////////////////

static uint32_t	sw_srcW;
static uint32_t sw_srcH;

static void sw_scale_init(int srcW, int srcH)
{
	sw_srcW = srcW;
	sw_srcH = srcH;
}

#ifdef INDEXED_IMAGE
static void sw_scale_start(register gfx_color* pal, register uint8_t* src, register gfx_color* dst)
#else
static void sw_scale_start(register gfx_color* src, register gfx_color* dst)
#endif
{
	register uint32_t W, H, ix, iy, x, y;
	x = 0;
	y = 0;
	W = HW_SCREEN_WIDTH;
	if(dst_offset) {
		dst += dst_offset;
		H = sw_srcH;
	} else {
		H = HW_SCREEN_HEIGHT;
	}
	ix = (sw_srcW << 16) / W;
	iy = (sw_srcH << 16) / H;
	
	do {
#ifdef INDEXED_IMAGE
		uint8_t* buffer = &src[(y >> 16) * sw_srcW];
#else
		gfx_color* buffer = &src[(y >> 16) * HW_SCREEN_WIDTH];
#endif
		W = HW_SCREEN_WIDTH;
		x = 0;
		do {
#ifdef INDEXED_IMAGE
			*dst++ = pal[buffer[x >> 16]];
#else
			*dst++ = buffer[x >> 16];
#endif
			x += ix;
		} while (--W);
		y += iy;
	} while (--H);
}

///////////////////////////////////////////////////////////////////////////////
//	Internal functions
///////////////////////////////////////////////////////////////////////////////

static gfx_color** _get_framebuffer_pointer()
{
	uint32_t *LP, LV;
	short *SP;
	LP = (uint32_t*)_lcd_get_frame;
	LV = *LP;
	LV = ((LV & 0x03FFFFFF) << 2) | 0x80000000;
	LP = (uint32_t*)LV;
	LV = LP[2];
	LV = ((LV & 0x03FFFFFF) << 2) | 0x80000000;
	LP = (uint32_t*)LV;
	LV = LP[10];
	LV = ((LV & 0x03FFFFFF) << 2) | 0x80000000;
	LP = (uint32_t*)LV;
	SP = (short*)LV;
	LV = (LP[0] << 16) + ((int)(SP[4]));
	return ((gfx_color**)LV);
}

inline
static void _bind_framebuffer(gfx_color* framebuffer)
{
	fb_gfx->address = framebuffer;
	gfx_render_target_set(fb_gfx);
}

inline
static void _term_framebuffer()
{
	fb_gfx->address = NULL;
	gfx_render_target_set(NULL);
}

static void _draw_osd(int fps, gfx_color* framebuffer)
{
	int messageX = 0;
	if(fps) {
		char text[3] = { 0 };
		if(fps > 99) fps = 99;
		text[0] = '0' + (fps / 10);
		text[1] = '0' + (fps % 10);
		_bind_framebuffer(framebuffer);
		gfx_rect_fill_draw(0, 0, current_font_w << 1, current_font_h, COLOR_BLACK);
		gfx_font_color_set(current_font, COLOR_YELLOW);
		gfx_font_print(0, 0, current_font, text);
		_term_framebuffer();
		messageX = 3 * 8;
	}
	if(video_msg_timer > 0) {
		_bind_framebuffer(framebuffer);
		gfx_rect_fill_draw(messageX, 0, gfx_font_width(current_font, video_msg), current_font_h, COLOR_BLACK);
		if(video_msg_timer > 2) {
			gfx_font_color_set(current_font, COLOR_GREEN);
			gfx_font_print(messageX, 0, current_font, video_msg);
		}
		_term_framebuffer();
		video_msg_timer--;
	}
}

static void _save_last_mode()
{
	sv_video_mode = video_mode;
	sv_srcW = (video_mode == VM_SCALE_HW) ? hw_srcW : ((video_mode == VM_SCALE_SW) ? sw_srcW : 0);
	sv_srcH = (video_mode == VM_SCALE_HW) ? hw_srcH : ((video_mode == VM_SCALE_SW) ? sw_srcH : 0);
	sv_stretch = !!dst_offset;
}

///////////////////////////////////////////////////////////////////////////////
//	General video functions
///////////////////////////////////////////////////////////////////////////////

void video::init(VIDEO_MODE mode)
{
	if(!fb_sys)
	{
		fb_lcd_ptr = _get_framebuffer_pointer();
		fb_sys = *fb_lcd_ptr;
		fb_mem = (gfx_color*)malloc(BUFFER_SIZE);
		fb_ipu = (gfx_color*)malloc(BUFFER_SIZE);

		fb_gfx = (gfx_texture*)malloc(sizeof(gfx_texture));
		fb_gfx->width  = HW_SCREEN_WIDTH;
		fb_gfx->height = HW_SCREEN_HEIGHT;
	}
	set_mode(mode, HW_SCREEN_WIDTH, HW_SCREEN_HEIGHT, true);
	clear_all(true);
}

void video::close()
{
	clear_all(true);
	*fb_lcd_ptr = fb_sys;
	_lcd_set_frame();						//	may be not need
	free(fb_mem);
	free(fb_ipu);
	free(fb_gfx);
}

void video::set_mode(VIDEO_MODE mode, int srcW, int srcH, bool stretch)
{
	_save_last_mode();
	video_mode = mode;
	dst_offset = !stretch ? ((HW_SCREEN_HEIGHT - srcH) >> 1) * HW_SCREEN_WIDTH : 0;	
	hw_scale_deinit();
	if(video_mode == VM_SCALE_SW || video_mode == VM_SCALE_HW) {
		if (srcW == HW_SCREEN_WIDTH && (srcH == HW_SCREEN_HEIGHT || !stretch)) {
			video_mode = VM_FAST;
		} else {
			if(video_mode == VM_SCALE_HW) 
				hw_scale_init(srcW, srcH); 
			else 
				sw_scale_init(srcW, srcH);
		}
	}
	if(video_mode != VM_FAST) {
		*fb_lcd_ptr = (video_mode == VM_SCALE_HW) ? fb_ipu : fb_sys;
	}
}

VIDEO_MODE video::get_mode()
{
	return video_mode;
}

void video::prev_mode()
{
	set_mode(sv_video_mode, sv_srcW, sv_srcH, sv_stretch);
}

gfx_color* video::get_framebuffer()
{
	if(video_mode == VM_FAST)
	{
		return *fb_lcd_ptr;
	}
	return fb_mem;
}

void video::set_osd_msg(const char* message, int timeOut)
{
	strcpy(video_msg, message);
	video_msg_timer = timeOut;
}

int video::has_osd_msg()
{
	return video_msg_timer;
}

void video::flip(int fps)
{
	switch(video_mode)
	{
		case VM_FAST:
			_draw_osd(fps, *fb_lcd_ptr);
			_lcd_set_frame();
			*fb_lcd_ptr = (*fb_lcd_ptr == fb_sys) ? fb_mem : fb_sys;
			break;
			
		case VM_SLOW:
			memcpy(fb_sys, fb_mem, BUFFER_SIZE);
			_draw_osd(fps, fb_sys);
			_lcd_set_frame();
			break;
			
		case VM_SCALE_SW:
#ifdef INDEXED_IMAGE
			if(indexed_src) 
			{
				sw_scale_start(indexed_pal, indexed_src, fb_sys);
				_draw_osd(fps, fb_sys);
				indexed_src = 0;
			}
#else
			sw_scale_start(fb_mem, fb_sys);
			_draw_osd(fps, fb_sys);
#endif
			_lcd_set_frame();
			break;
			
		case VM_SCALE_HW:
#ifdef INDEXED_IMAGE
			if(indexed_src) 
			{
				if(i_pal_dirty)
				hw_scale_indexed_pal_2_yuv(indexed_pal);
				hw_scale_indexed_src_2_yuv(indexed_src);
				hw_scale_start_ipu(*fb_lcd_ptr);
				*fb_lcd_ptr = (*fb_lcd_ptr == fb_sys) ? fb_ipu : fb_sys;
				_draw_osd(fps, *fb_lcd_ptr);
				indexed_src = 0;
			}
#else
			hw_scale_rgb_2_yuv(fb_mem);
			hw_scale_start_ipu(*fb_lcd_ptr);
			*fb_lcd_ptr = (*fb_lcd_ptr == fb_sys) ? fb_ipu : fb_sys;
			_draw_osd(fps, *fb_lcd_ptr);
#endif
			_lcd_set_frame();
			break;
	}
}

#ifdef INDEXED_IMAGE
void video::indexed_to_rgb(gfx_color* pal, bool pal_dirty, uint8_t* src, int srcW, int srcH, gfx_color* dst)
{
	switch(video_mode)
	{
		case VM_FAST:
		case VM_SLOW:
		{	register int x;
			register int y;
			for(y = srcH - 1; y >= 0; y--)
			{
				for(x = srcW - 1; x >= 0; x -= 8)
				{
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
					*dst++ = pal[*src++];
				}
				dst += (HW_SCREEN_WIDTH - srcW);
			}
		}	break;
		case VM_SCALE_SW:
		case VM_SCALE_HW:
		{	indexed_pal = pal;
			indexed_src = src;
			i_pal_dirty = pal_dirty;
		}	break;
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
//	Load PNG from file and memory
///////////////////////////////////////////////////////////////////////////////

static gfx_texture* _gfx_tex_load_png(void* source, void (*reader)(png_structp, png_bytep, png_size_t))
{
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return NULL;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
		return NULL;
	}

	png_set_read_fn(png_ptr, (png_voidp)source, reader);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING, NULL);
	png_bytepp row_ptr = png_get_rows(png_ptr, info_ptr);
	if (row_ptr == NULL || info_ptr->pixel_depth != 24) {
//	if (row_ptr == NULL || png_get_bit_depth(png_ptr, info_ptr) != 24) {
		png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
		return NULL;
	}

	uint16_t png_w = info_ptr->width;
//	uint16_t png_w = png_get_image_width(png_ptr, info_ptr);
	uint16_t png_h = info_ptr->height;
//	uint16_t png_h = png_get_image_height(png_ptr, info_ptr);
	gfx_texture* texture = video::tex_allocate(png_w, png_h);
	if(texture == NULL) return NULL;

	uintptr_t h, w;
	gfx_color* dst = (gfx_color *)texture->address;
	for (h = 0; h < png_h; h++) {
		unsigned char *src = row_ptr[h];
		for(w = 0; w < png_w; w++) {
			*dst++ = gfx_color_rgb(src[0], src[1], src[2]);
			src += 3;
		}
	}

	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : NULL, (png_infopp)NULL);
	return texture;
}

static void readDataFromFile(png_structp png_ptr, png_bytep data, png_size_t length)
{
	FILE* source = static_cast<FILE*>(png_get_io_ptr(png_ptr));
	fread(data, 1, length, source);
}

static void readDataFromMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
	MemStream* source = static_cast<MemStream*>(png_get_io_ptr(png_ptr));
	source->read(data, length);
}

gfx_texture* video::tex_allocate(int w, int h)
{
	return gfx_tex_allocate(w, h);
}

gfx_texture* video::tex_load(const char* path)
{
	if(path == NULL) return NULL;

	FILE* source = fopen(path, "rb");
	if(source)
	{
		gfx_texture* texture = _gfx_tex_load_png(source, readDataFromFile);
		fclose(source);
		return texture;
	}
	return NULL;
}

gfx_texture* video::tex_load(uint8_t* buffer, size_t size)
{
	if(buffer == NULL || size == 0) return NULL;

	MemStream source; 
	if(source.open(buffer, size))
	{
		gfx_texture* texture = _gfx_tex_load_png(&source, readDataFromMemory);
		source.close();
		return texture;
	}
	return NULL;
}

void video::tex_delete(gfx_texture* texture)
{
	gfx_tex_delete(texture);
}

///////////////////////////////////////////////////////////////////////////////
//	Load font from PNG file and memory
///////////////////////////////////////////////////////////////////////////////

gfx_font* video::font_load(const char* path, gfx_color key)
{
	gfx_texture* texture = tex_load(path);
	if(texture)	{
		gfx_font* font = (gfx_font*)malloc(sizeof(gfx_font));
		if(font) {
			font->texture  = texture;
			font->colorKey = key;
			font->colorize = false;
			return font;
		}
		tex_delete(texture);
	}	
	return NULL;
}

gfx_font* video::font_load(uint8_t* buffer, size_t size, gfx_color key)
{
	gfx_texture* texture = tex_load(buffer, size);
	if(texture) {
		gfx_font* font = (gfx_font*)malloc(sizeof(gfx_font));
		if(font) {
			font->texture  = texture;
			font->colorKey = key;
			font->colorize = false;
			return font;
		}
		tex_delete(texture);
	}	
	return NULL;
}

int video::font_w(gfx_font* font, const char* text)
{
	return gfx_font_width(font, (char*)text);
}

int video::font_h(gfx_font* font)
{
	return gfx_font_height(font);
}

void video::font_delete(gfx_font* font)
{
	gfx_font_delete(font);
}

///////////////////////////////////////////////////////////////////////////////
//	Drawing functions
///////////////////////////////////////////////////////////////////////////////

void video::bind_framebuffer()
{
	_bind_framebuffer(get_framebuffer());
}

void video::term_framebuffer()
{
	_term_framebuffer();
}

void video::font_set(gfx_font* font)
{
	current_font   = font;
	current_font_w = font_w(current_font, " ");
	current_font_h = font_h(current_font);
}

gfx_font* video::font_get()
{
	return current_font;
}

void video::draw_point(int x, int y, gfx_color color)
{
	 gfx_point_draw(x, y, color);
}

void video::draw_line(int x0, int y0, int x1, int y1, gfx_color color)
{
	gfx_line_draw(x0, y0, x1, y1, color);
}

void video::draw_rect(int x, int y, int w, int h, gfx_color color)
{
	gfx_rect_draw(x, y, w, h, color);
}

void video::fill_rect(int x, int y, int w, int h, gfx_color color)
{
	gfx_rect_fill_draw(x, y, w, h, color);
}

void video::mult_rect(int x, int y, int w, int h, gfx_color color) 
{
	if(((x + w) <= 0) || (x >= gfx_render_target->width )) return;
	if(((y + h) <= 0) || (y >= gfx_render_target->height)) return;

	if(w < 0) {	x += w;	w = -w; }
	if(h < 0) {	y += h;	h = -h;	}
	if(x < 0) {	w += x;	x = 0;	}
	if((x + w) > gfx_render_target->width ) w = (gfx_render_target->width  - x);
	if(y < 0) {	h += y; y = 0;	}
	if((y + h) > gfx_render_target->height)	h = (gfx_render_target->height - y);

	gfx_color* tempBuffer = (gfx_color*)gfx_render_target->address;
	int i, j;
	for(j = y; (j < gfx_render_target->height) && (j < (y + h)); j++) {
		for(i = x; (i < gfx_render_target->width) && (i < (x + w)); i++) {
			tempBuffer[j * gfx_render_target->width + i] = AVERAGE(tempBuffer[j * gfx_render_target->width + i], color);
		}
	}
}

void video::draw_rgb(int x, int y, gfx_color* rgb, int w, int h)
{
	if(rgb == NULL) return;

	gfx_color* tempBuffer = (gfx_color*)gfx_render_target->address;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < h) && (yy < gfx_render_target->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < w) && (xx < gfx_render_target->width); ii++, xx++) {
			if (xx < 0) continue;
			tempBuffer[(yy * gfx_render_target->width) + xx] = rgb[(jj * w) + ii];
		}
	}
}

void video::draw_tex(int x, int y, gfx_texture* texture)
{
	draw_rgb(x, y, (gfx_color*)texture->address, texture->width, texture->height);
}

void video::draw_tex_transp(int x, int y, gfx_texture* texture)
{
	if(texture == NULL) return;

	gfx_color* tempBuffer  = (gfx_color*)gfx_render_target->address;
	gfx_color* tempTexture = (gfx_color*)texture->address;
	gfx_color  c;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < texture->height) && (yy < gfx_render_target->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < texture->width) && (xx < gfx_render_target->width); ii++, xx++) {
			if (xx < 0) continue;
			c = tempTexture[(jj * texture->width) + ii];
			if(c != COLOR_MAGENTA)	tempBuffer[(yy * gfx_render_target->width) + xx] = c;
		}
	}
}

void video::draw_tex_tiled(int x, int y, gfx_texture* texture, int xscroll, int yscroll)
{
	gfx_color* fbuff = (gfx_color*)fb_gfx->address + HW_SCREEN_WIDTH * y + x;
	gfx_color* tdata = (gfx_color*)texture->address;
	gfx_color* src;
	gfx_color* dst;

	int xx_mask = texture->width - 1;
	int yy_mask = texture->height * texture->width - 1;
	int xx, yy  = yscroll * texture->width;

	for (int i = y; i < HW_SCREEN_HEIGHT; i++) 
	{
		xx  = xscroll;
		src = tdata + yy;
		dst = fbuff;

		for (int j = x; j < HW_SCREEN_WIDTH; j++) 
		{
			*dst++ = src[xx++];
			xx &= xx_mask;
		}

		fbuff += HW_SCREEN_WIDTH;
		yy += texture->width;
		yy &= yy_mask;
	}
}

void video::print(int x, int y, const char* text, gfx_color color)
{
	gfx_font_color_set(current_font, color);
	gfx_font_print(x, y, current_font, (char*)text);
}

void video::clear(gfx_color color)
{
	bind_framebuffer();
	gfx_render_target_clear(color);
	term_framebuffer();
}

void video::clear_all(bool with_flip)
{
	_bind_framebuffer(fb_sys);
	gfx_render_target_clear(0);
	_bind_framebuffer(fb_mem);
	gfx_render_target_clear(0);
	_bind_framebuffer(fb_ipu);
	gfx_render_target_clear(0);
	_term_framebuffer();
	if(with_flip) flip(video_msg_timer = 0);
}

///////////////////////////////////////////////////////////////////////////////
//	Software bilinear scaler
///////////////////////////////////////////////////////////////////////////////

void video::bilinear_scale(gfx_color* src, int old_w, int old_h, int old_pitch, gfx_color* dst, int new_w, int new_h, int new_pitch)
{
	float 		t, u, temp, t1, u1;
	int   		y, x, L, C, L0, L1;
	gfx_color	rgb00, rgb01, rgb10, rgb11;
	uint8_t		R, G, B;

	int old_w1 = old_w - 1;
	int old_h1 = old_h - 1;

	for (y = 0; y < new_h; y++) {

		temp = (float) (y) / (float) (new_h - 1) * old_h1;
		L = (int)(temp < 0 ? temp - 0.5f : temp);
		L = L < 0 ? 0 : (L >= old_h1 ? old_h1 - 1 : L);
		u1 = 1 - (u = temp - L);

		L0 = L * old_pitch;
		L1 = (L + 1) * old_pitch;

		for (x = 0; x < new_w; x++) {

			temp = (float) (x) / (float) (new_w - 1) * old_w1;
			C = (int)(temp < 0 ? temp - 0.5f : temp);
			C = C < 0 ? 0 : (C >= old_w1 ? old_w1 - 1 : C);
			t1 = 1 - (t = temp - C);

			rgb00 = *(src + L0 + C);
			rgb01 = *(src + L0 + C + 1);
			rgb10 = *(src + L1 + C);
			rgb11 = *(src + L1 + C + 1);

			R = uint8_t(
				t1 * u1 * (rgb00 >> 11) +
				t  * u1 * (rgb01 >> 11) +
				t1 * u  * (rgb10 >> 11) +
				t  * u  * (rgb11 >> 11));
			G = uint8_t(
				t1 * u1 * (rgb00 >> 5 & 0x3F) +
				t  * u1 * (rgb01 >> 5 & 0x3F) +
				t1 * u  * (rgb10 >> 5 & 0x3F) +
				t  * u  * (rgb11 >> 5 & 0x3F));
			B = uint8_t(
				t1 * u1 * (rgb00 & 0x1F) +
				t  * u1 * (rgb01 & 0x1F) +
				t1 * u  * (rgb10 & 0x1F) +
				t  * u  * (rgb11 & 0x1F));

			*(dst + (y * new_pitch) + x) =  (R << 11) | (G << 5) | B;
		}
	}
}

