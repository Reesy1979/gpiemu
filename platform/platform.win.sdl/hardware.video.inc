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

static SDL_Surface* fb_sys;
static gfx_color*	fb_mem;
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
//	Hardware scale (stub)
///////////////////////////////////////////////////////////////////////////////

static uint32_t hw_srcW;
static uint32_t hw_srcH;

static void hw_scale_init(int srcW, int srcH)
{
	hw_srcW = srcW;
	hw_srcH = srcH;
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

inline
static void _bind_framebuffer(gfx_color* framebuffer)
{
	fb_gfx->address = framebuffer;
}

inline
static void _term_framebuffer()
{
	fb_gfx->address = NULL;
}

static void _clear_framebuffer(gfx_color color) {
	uint32_t tempPixelColor = (color << 16) | color;
	uint32_t* tempLineStart = (uint32_t*)fb_gfx->address;
	uint32_t* tempBlockEnd;
	tempBlockEnd = (uint32_t*)((uintptr_t)fb_gfx->address + (fb_gfx->width * fb_gfx->height * sizeof(gfx_color)));
	while(tempLineStart < tempBlockEnd) {
		*(tempLineStart++) = tempPixelColor;
		*(tempLineStart++) = tempPixelColor;
		*(tempLineStart++) = tempPixelColor;
		*(tempLineStart++) = tempPixelColor;
	}
}

void _draw_h_line(int x0, int x1, int y, gfx_color color) {
	if((y < 0) || (y >= fb_gfx->height))
		return;
	if(x0 == x1)
		return;

	if(x0 > x1) {
		x0 = x0 ^ x1;
		x1 = x0 ^ x1;
		x0 = x0 ^ x1;
	}

	if(x0 >= fb_gfx->width)
		return;
	if(x1 < 0)
		return;

	if(x0 < 0)
		x0 = 0;
	if(x1 > fb_gfx->width)
		x1 = fb_gfx->width;
	x1 -= x0;

	if(((x0 | x1) & 1) == 0) {
		uint32_t* tempStart32 = (uint32_t*)((uintptr_t)fb_gfx->address + (((y * fb_gfx->width) + x0) * sizeof(gfx_color)));
		uint32_t* tempEnd32   = (uint32_t*)((uintptr_t)tempStart32 + (x1 * sizeof(gfx_color)));
		uint32_t  tempColor32 = (color << 16) | color;
		while(tempStart32 < tempEnd32)
			*(tempStart32++) = tempColor32;
		return;
	}

	gfx_color* tempStart = (gfx_color*)((uintptr_t)fb_gfx->address + (((y * fb_gfx->width) + x0) * sizeof(gfx_color)));
	gfx_color* tempEnd   = (gfx_color*)((uintptr_t)tempStart + (x1 * sizeof(gfx_color)));
	while(tempStart < tempEnd)
		*(tempStart++) = color;
}

void _draw_v_line(int x, int y0, int y1, gfx_color color) {
	if((x < 0) || (x >= fb_gfx->width))
		return;
	if(y0 == y1)
		return;

	if(y0 > y1) {
		y0 = y0 ^ y1;
		y1 = y0 ^ y1;
		y0 = y0 ^ y1;
	}

	if(y0 >= fb_gfx->height)
		return;
	if(y1 < 0)
		return;

	if(y0 < 0)
		y0 = 0;
	if(y1 > fb_gfx->height)
		y1 = fb_gfx->height;
	y1 -= y0;

	gfx_color* tempStart = (gfx_color*)((uintptr_t)fb_gfx->address + (((y0 * fb_gfx->width) + x) * sizeof(gfx_color)));
	gfx_color* tempEnd   = (gfx_color*)((uintptr_t)tempStart + ((y1 * fb_gfx->width) * sizeof(gfx_color)));
	while(tempStart < tempEnd) {
		*tempStart = color;
		tempStart += fb_gfx->width;
	}
}

static void _print(int x, int y, gfx_font* font, char* str) {
	if((font == NULL) || (str == NULL) || (font->texture == NULL)) return;

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;
	gfx_color* tFont = (gfx_color*)font->texture->address;
	uint8_t*   tChar;
	int16_t    tX = x, tY = y, ii, jj, xx, yy;

	for(tChar = (uint8_t*)str; *tChar != '\0'; tChar++) {
		if(*tChar == ' ') {
			tX += (font->texture->width >> 4);
			continue;
		}
		if(*tChar == '\t') {
			tX += ((font->texture->width >> 4) << 2);
			continue;
		}
		if(*tChar == '\r') {
			tX = x;
			continue;
		}
		if(*tChar == '\n') {
			tX = x;
			tY += (font->texture->height >> 4);
			continue;
		}

		if (font->colorize) {
			for(jj = ((*tChar >> 4) * (font->texture->height >> 4)), yy = tY; (jj < (((*tChar >> 4) + 1) * (font->texture->height >> 4))) && (yy < fb_gfx->height); jj++, yy++) {
				for(ii = ((*tChar & 0x0F) * (font->texture->width >> 4)), xx = tX; (ii < (((*tChar & 0x0F) + 1) * (font->texture->width >> 4))) && (xx < fb_gfx->width); ii++, xx++) {
					if (tFont[(jj * font->texture->width) + ii] != font->colorKey) {
						tBuff[(yy * fb_gfx->width) + xx] = ~(tFont[(jj * font->texture->width) + ii]) | font->fontColor;
					}
				}
			}
		} else {
			for(jj = ((*tChar >> 4) * (font->texture->height >> 4)), yy = tY; (jj < (((*tChar >> 4) + 1) * (font->texture->height >> 4))) && (yy < fb_gfx->height); jj++, yy++) {
				for(ii = ((*tChar & 0x0F) * (font->texture->width >> 4)), xx = tX; (ii < (((*tChar & 0x0F) + 1) * (font->texture->width >> 4))) && (xx < fb_gfx->width); ii++, xx++) {
					if (tFont[(jj * font->texture->width) + ii] != font->colorKey) {
						tBuff[(yy * fb_gfx->width) + xx] = tFont[(jj * font->texture->width) + ii];
					}
				}
			}
		}
		tX += (font->texture->width >> 4);
	}
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
		video::fill_rect(0, 0, current_font_w << 1, current_font_h, COLOR_BLACK);
		video::print(0, 0, text, COLOR_YELLOW);
		_term_framebuffer();
		messageX = 3 * 8;
	}
	if(video_msg_timer > 0) {
		_bind_framebuffer(framebuffer);
		video::fill_rect(messageX, 0, video::font_w(current_font, video_msg), current_font_h, COLOR_BLACK);
		if(video_msg_timer > 2) {
			video::print(messageX, 0, video_msg, COLOR_GREEN);
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
		fb_sys = SDL_SetVideoMode(HW_SCREEN_WIDTH, HW_SCREEN_HEIGHT, 16, SDL_HWSURFACE);
		if(!fb_sys) {
			printf("Couldn’t initialize screen: %s\n", SDL_GetError());
			exit(1);
		}
		fb_mem = (gfx_color*)malloc(BUFFER_SIZE);

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
	SDL_FreeSurface(fb_sys);
	free(fb_mem);
	free(fb_gfx);
}

void video::set_mode(VIDEO_MODE mode, int srcW, int srcH, bool stretch)
{
	_save_last_mode();
	video_mode = mode;
	dst_offset = !stretch ? ((HW_SCREEN_HEIGHT - srcH) >> 1) * HW_SCREEN_WIDTH : 0;	
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
	if(SDL_MUSTLOCK(fb_sys)) SDL_LockSurface(fb_sys);
	switch(video_mode)
	{
		case VM_FAST:
		case VM_SLOW:
			memcpy(fb_sys->pixels, fb_mem, BUFFER_SIZE);
			_draw_osd(fps, (gfx_color*)fb_sys->pixels);
			break;
		case VM_SCALE_SW:
#ifdef INDEXED_IMAGE
			if(indexed_src) 
			{
				sw_scale_start(indexed_pal, indexed_src, (gfx_color*)fb_sys->pixels);
				_draw_osd(fps, (gfx_color*)fb_sys->pixels);
				indexed_src = 0;
			}
#else
			sw_scale_start(fb_mem, (gfx_color*)fb_sys->pixels);
			_draw_osd(fps, (gfx_color*)fb_sys->pixels);
#endif
			break;
		case VM_SCALE_HW:
			bilinear_scale(
				fb_mem,
				hw_srcW,
				hw_srcH,
				HW_SCREEN_WIDTH,
				(gfx_color*)fb_sys->pixels + dst_offset,
				HW_SCREEN_WIDTH,
				dst_offset ? hw_srcH : HW_SCREEN_HEIGHT,
				HW_SCREEN_WIDTH);
			_draw_osd(fps, (gfx_color*)fb_sys->pixels);
			break;
	}
	if(SDL_MUSTLOCK(fb_sys)) SDL_UnlockSurface(fb_sys);
	SDL_Flip(fb_sys);
}

#ifdef INDEXED_IMAGE
void video::indexed_to_rgb(gfx_color* pal, bool pal_dirty, uint8_t* src, int srcW, int srcH, gfx_color* dst)
{
	switch(video_mode)
	{
		case VM_FAST:
		case VM_SLOW:
		case VM_SCALE_HW:
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
	gfx_texture* texture = (gfx_texture*)malloc(sizeof(gfx_texture) + (w * h * sizeof(gfx_color)));
	if(texture)
	{
		texture->address = (void*)((uintptr_t)texture + sizeof(gfx_texture));
		texture->width  = w;
		texture->height = h;
	}
	return texture;
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
	if(texture)	free(texture);
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
	if(font == NULL || text == NULL || font->texture == NULL) return 0;
	uintptr_t i, tempCur, tempMax;

	for(i = 0, tempCur = 0, tempMax = 0; text[i] != '\0'; i++) {
		if(text[i] == '\t')
			tempCur += 4;
		else 
		if(text[i] == '\r' || text[i] == '\n')
			tempCur = 0;
		else
			tempCur++;
		if(tempCur > tempMax) tempMax = tempCur;
	}
	tempMax *= (font->texture->width >> 4);
	return tempMax;
}

int video::font_h(gfx_font* font)
{
	if(font && font->texture) return font->texture->height >> 4;
	return 0;
}

void video::font_delete(gfx_font* font)
{
	if(font) {
		tex_delete(font->texture);
		free(font);
	}
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

void video::draw_point(int, int, gfx_color)
{
	//	DO NOTHING
}

void video::draw_line(int x0, int y0, int x1, int y1, gfx_color color)
{
	if(y0 == y1) {
		if((y0 >= 0) && (y0 < fb_gfx->height)) 
			_draw_h_line(x0, x1, y0, color);
	} else
	if(x0 == x1) {
		if((x0 >= 0) && (x0 < fb_gfx->width)) 
			_draw_v_line(x0, y0, y1, color);
	}
}

void video::draw_rect(int x, int y, int w, int h, gfx_color color)
{
	if(((x + w) <= 0) || (x >= fb_gfx->width)) return;
	if(((y + h) <= 0) || (y >= fb_gfx->height))	return;

	if(w < 0) {	x += w;	w = -w;	}
	if(h < 0) {	y += h;	h = -h;	}

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;
	uintptr_t i, j;
	for(j = y, i = (x > 0 ? x : 0); (i < fb_gfx->width) && (i < (x + w)); i++)
		tBuff[(j * fb_gfx->width) + i] = color;
	for(j = (y + 1); (j < fb_gfx->height) && (j < (y + h - 1)); j++) {
		if(x >= 0)
			tBuff[(j * fb_gfx->width) + x] = color;
		if((x + w - 1) < fb_gfx->width)
			tBuff[(j * fb_gfx->width) + (x + w - 1)] = color;
	}
	for(j = (y + h - 1), i = (x > 0 ? x : 0); (i < fb_gfx->width) && (i < (x + w)); i++)
		tBuff[(j * fb_gfx->width) + i] = color;
}

void video::fill_rect(int x, int y, int w, int h, gfx_color color)
{
	if(((x + w) <= 0) || (x >= fb_gfx->width)) return;
	if(((y + h) <= 0) || (y >= fb_gfx->height)) return;

	if(w < 0) { x += w;	w = -w;	}
	if(h < 0) {	y += h;	h = -h;	}
	if(x < 0) {	w += x;	x = 0;	}
	if((x + w) > fb_gfx->width)	w = (fb_gfx->width - x);
	if(y < 0) {	h += y;	y = 0;	}
	if((y + h) > fb_gfx->height) h = (fb_gfx->height - y);

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;
	uintptr_t i, j;
	for(j = y; (j < fb_gfx->height) && (j < (y + h)); j++) {
		for(i = x; (i < fb_gfx->width) && (i < (x + w)); i++) {
			tBuff[(j * fb_gfx->width) + i] = color;
		}
	}
}

void video::mult_rect(int x, int y, int w, int h, gfx_color color) 
{
	if(((x + w) <= 0) || (x >= fb_gfx->width )) return;
	if(((y + h) <= 0) || (y >= fb_gfx->height)) return;

	if(w < 0) {	x += w;	w = -w; }
	if(h < 0) {	y += h;	h = -h;	}
	if(x < 0) {	w += x;	x = 0;	}
	if((x + w) > fb_gfx->width ) w = (fb_gfx->width  - x);
	if(y < 0) {	h += y; y = 0;	}
	if((y + h) > fb_gfx->height)	h = (fb_gfx->height - y);

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;
	int i, j;
	for(j = y; (j < fb_gfx->height) && (j < (y + h)); j++) {
		for(i = x; (i < fb_gfx->width) && (i < (x + w)); i++) {
			tBuff[j * fb_gfx->width + i] = AVERAGE(tBuff[j * fb_gfx->width + i], color);
		}
	}
}

void video::draw_rgb(int x, int y, gfx_color* rgb, int w, int h)
{
	if(rgb == NULL) return;

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < h) && (yy < fb_gfx->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < w) && (xx < fb_gfx->width); ii++, xx++) {
			if (xx < 0) continue;
			tBuff[(yy * fb_gfx->width) + xx] = rgb[(jj * w) + ii];
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

	gfx_color* tBuff  = (gfx_color*)fb_gfx->address;
	gfx_color* tempTexture = (gfx_color*)texture->address;
	gfx_color  c;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < texture->height) && (yy < fb_gfx->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < texture->width) && (xx < fb_gfx->width); ii++, xx++) {
			if (xx < 0) continue;
			c = tempTexture[(jj * texture->width) + ii];
			if(c != COLOR_MAGENTA)	tBuff[(yy * fb_gfx->width) + xx] = c;
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
	current_font->colorize = true;
	current_font->fontColor = color;
	_print(x, y, current_font, (char*)text);
}

void video::clear(gfx_color color)
{
	bind_framebuffer();
	_clear_framebuffer(color);
	term_framebuffer();
}

void video::clear_all(bool with_flip)
{
	SDL_LockSurface(fb_sys);
	_bind_framebuffer((gfx_color*)fb_sys->pixels);
	_clear_framebuffer(0);
	_bind_framebuffer(fb_mem);
	_clear_framebuffer(0);
	_term_framebuffer();
	SDL_UnlockSurface(fb_sys);
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

