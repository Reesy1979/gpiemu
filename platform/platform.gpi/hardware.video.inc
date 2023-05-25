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

static gfx_texture*	fb_gfx;

#ifdef INDEXED_IMAGE
static gfx_color*	indexed_pal;
static uint8_t*		indexed_src;
static bool			i_pal_dirty;
#endif

static gfx_font*	current_font;

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
static void sw_scale_start(gfx_color* pal, uint8_t* src, gfx_color* dst)
#else
static void sw_scale_start(gfx_color* src, gfx_color* dst)
#endif
{
	uint32_t W, H, ix, iy, x, y;
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

void video::font_size(gfx_font* font, char *str, int *height, int *width)
{
	if(font == NULL)
	{
		*height=0;
		*width=0;
		return;
	}

	uint8_t*   tChar;
	int w=0,h=-1,mw=0, mh=0;
	gfx_font_glyph *glyph;
	for(tChar = (uint8_t*)str; *tChar != '\0'; tChar++) {
		if(*tChar == ' ') {
			glyph = font->glyph[*tChar];
			w += glyph->mtx.advanceWidth; //move pen to position for next char
			continue;
		}
		if(*tChar == '\t') {
			w += 16;
			continue;
		}
		if(*tChar == '\r') {
			mw=w;
			w = 0;
			continue;
		}
		if(*tChar == '\n') {
			h += mh;
			continue;
		}
		
  		gfx_font_glyph *glyph = font->glyph[*tChar];
  		
  		w += glyph->mtx.advanceWidth+1; //move pen to position for next char
  		if(mh<glyph->img.height)//+glyph->mtx.yOffset)
  		{
  			mh=glyph->img.height;//+glyph->mtx.yOffset;
  		}
	}
	if(mw>w) w=mw;
	if(h<0) h=mh;
	*width=w;
	*height=h;
}

static gfx_color _alpha_blend(gfx_color color1, gfx_color color2, unsigned char alpha) {
	//rrrr rggg gggb bbbb
	unsigned int rb = ((color1 & 0xF800)<<8) | (color1 & 0x1f);
	unsigned int g  = color1 & 0x07E0;
	rb += (((((color2 & 0xF800)<<8)|(color2 & 0x1F)) - rb) * alpha) >> 8;
	g += (((color2 & 0x07E0) - g) * alpha) >> 8;
	return (rb & 0xF80000)>>8 | (rb & 0x1F) | (g & 0x07E0);
}

static void _print(int x, int y, gfx_font* font, char* str, gfx_color color) 
{
	if((font == NULL) || (str == NULL)) return;
	gfx_color* tBuff = (gfx_color*)fb_gfx->address;
	gfx_font_glyph *glyph;
	unsigned char *tFont;
	uint8_t*   tChar;
	int16_t    tX = x, tY = y + (int)ceil(font->lmtx.ascender
	                          + font->lmtx.descender
							  + font->lmtx.lineGap), ii, jj, xx, yy;

	for(tChar = (uint8_t*)str; *tChar != '\0'; tChar++) {
		if(*tChar == ' ') {
			glyph = font->glyph[*tChar];
			tX += glyph->mtx.advanceWidth; //move pen to position for next char
			continue;
		}
		if(*tChar == '\t') {
			tX += 16;
			continue;
		}
		if(*tChar == '\r') {
			tX = x;
			continue;
		}
		if(*tChar == '\n') {
			tX = x;
			tY += 8;
			continue;
		}
		
  		gfx_font_glyph *glyph = font->glyph[*tChar];
  		tFont=(unsigned char*)glyph->img.pixels;

		for(jj = 0, yy = tY; (jj < glyph->img.height) && (yy < fb_gfx->height+10); jj++, yy++) {
			for(ii = 0, xx = tX; (ii < glyph->img.width) && (xx < fb_gfx->width-1); ii++, xx++) {
				unsigned char fontA=tFont[(jj * glyph->img.width) + ii];

				if (fontA == 255) {
					
					tBuff[((yy + glyph->mtx.yOffset) * fb_gfx->width) + xx + (int)glyph->mtx.leftSideBearing] 
					    = color;
				}
				else if (fontA != 0) {
					
					tBuff[((yy + glyph->mtx.yOffset) * fb_gfx->width) + xx + (int)glyph->mtx.leftSideBearing] 
			   		    = _alpha_blend(tBuff[((yy + glyph->mtx.yOffset) * fb_gfx->width) + xx + (int)glyph->mtx.leftSideBearing]
					                 ,color, fontA);
				}
			}
		}
  		tX += glyph->mtx.advanceWidth+1; //move pen to position for next char
	}
}

static void _print(int x, int y, gfx_font* font, char* str) 
{
	_print(x, y, font, str, font->fontColor); 
}

static void _draw_osd(int fps, gfx_color* framebuffer)
{
	int messageX = 0;
	int height, width;

	if(fps) {
		char text[3] = { 0 };
		if(fps > 99) fps = 99;
		text[0] = '0' + (fps / 10);
		text[1] = '0' + (fps % 10);
		_bind_framebuffer(framebuffer);
		
		video::font_size(current_font, text, &height, &width);
		video::fill_rect(0, 0, width, height, COLOR_BLACK);
		video::print(0, 0, text, COLOR_YELLOW);
		_term_framebuffer();
		messageX = 3 * 8;
	}

	if(video_msg_timer > 0) {
		_bind_framebuffer(framebuffer);
		video::font_size(current_font, video_msg, &height, &width);
		video::fill_rect(messageX, 0, width, height, COLOR_BLACK);
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
	if(!fb_gfx)
	{
		fb_gfx = (gfx_texture*)malloc(sizeof(gfx_texture));
		fb_gfx->width  = HW_SCREEN_WIDTH;
		fb_gfx->height = HW_SCREEN_HEIGHT;
	}
	set_mode(mode, HW_SCREEN_WIDTH, HW_SCREEN_HEIGHT, true);
}

void video::close()
{
	clear_all(true);
	free(fb_gfx);
}

void video::set_mode(VIDEO_MODE mode, int srcW, int srcH, bool stretch)
{
	_save_last_mode();
	video_mode = mode;
	dst_offset = !stretch ? ((HW_SCREEN_HEIGHT - srcH) >> 1) * HW_SCREEN_HEIGHT : 0;	
	if(video_mode == VM_SCALE_SW || video_mode == VM_SCALE_HW) 
	{
		//to do set HVS scaled
		KernelVideoScale(srcW, srcH);
	}
	else
	{
		//to do reset HVS to unscaled
		KernelVideoScale(HW_SCREEN_WIDTH, HW_SCREEN_HEIGHT);
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
	return (gfx_color*)KernelVideoGetBuffer();
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
	_draw_osd(fps, (gfx_color*)KernelVideoGetBuffer());
	KernelVideoFlip(true);
}

#ifdef INDEXED_IMAGE
void video::indexed_to_rgb(gfx_color* pal, bool pal_dirty, uint8_t* src, int srcW, int srcH, gfx_color* dst)
{
	switch(video_mode)
	{
		case VM_FAST:
		case VM_SLOW:
		case VM_SCALE_HW:
		{	int x;
			int y;
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
gfx_texture* _gfx_tex_load_png(png_image image)
{
	png_bytep buffer;

	/* Set the format in which to read the PNG file; this code chooses a
	* simple sRGB format with a non-associated alpha channel, adequate to
	* store most images.
	*/
	image.format = PNG_FORMAT_RGBA;

	/* Now allocate enough memory to hold the image in this format; the
	* PNG_IMAGE_SIZE macro uses the information about the image (width,
	* height and format) stored in 'image'.
	*/

	gfx_texture* texture = video::tex_allocate32(image.width, image.height);
	if(texture == NULL) 
	{
		debug::printf("video::tex_allocate failed\n");
	}

	buffer = (png_bytep)malloc(PNG_IMAGE_SIZE(image));

	/* If enough memory was available, read the image in the desired
	* format, then write the result out to the new file.  'background' is
	* not necessary when reading the image, because the alpha channel is
	* preserved; if it were to be removed, for example if we requested
	* PNG_FORMAT_RGB, then either a solid background color would have to
	* be supplied, or the output buffer would have to be initialized to
	* the actual background of the image.
	*
	* The fourth argument to png_image_finish_read is the 'row_stride' -
	* this is the number of components allocated for the image in each
	* row.  It has to be at least as big as the value returned by
	* PNG_IMAGE_ROW_STRIDE, but if you just allocate space for the
	* default, minimum size, using PNG_IMAGE_SIZE as above, you can pass
	* zero.
	*
	* The final argument is a pointer to a buffer for the colormap;
	* colormaps have exactly the same format as a row of image pixels
	* (so you choose what format to make the colormap by setting
	* image.format).  A colormap is only returned if
	* PNG_FORMAT_FLAG_COLORMAP is also set in image.format, so in this
	* case NULL is passed as the final argument.  If you do want to force
	* all images into an index/color-mapped format, then you can use:
	*
	*    PNG_IMAGE_COLORMAP_SIZE(image)
	*
	* to find the maximum size of the colormap in bytes.
	*/
	if (texture != NULL &&
		png_image_finish_read(&image, NULL/*background*/, buffer,
		0/*row_stride*/, NULL/*colormap*/) != 0)
	{
		uintptr_t h, w;
		unsigned char *src = buffer;
		unsigned char *dst = (unsigned char *)texture->address;
		for (h = 0; h < image.height; h++) 
		{
			for(w = 0; w < image.width; w++) 
			{
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
			}
		}
		free(buffer);
		return texture;
	}
	else
	{
		/* Calling png_image_free is optional unless the simplified API was
		* not run to completion.  In this case, if there wasn't enough
		* memory for 'buffer', we didn't complete the read, so we must
		* free the image:
		*/

		if (buffer != NULL)
			free(buffer);

		if (texture == NULL)
			png_image_free(&image);
		else
			free(texture);
	}

	/* Something went wrong reading or writing the image.  libpng stores a
	* textual message in the 'png_image' structure:
	*/
	debug::printf("pngtopng: error: %s\n", image.message);
	return NULL;
}

gfx_texture* _gfx_tex_load_png_file(char *filename)
{
	png_image image;
	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	/* The first argument is the file to read: */
	if (png_image_begin_read_from_file(&image, filename) != 0)
	{
		return _gfx_tex_load_png(image);
	}
	return NULL;
}

gfx_texture* _gfx_tex_load_png_mem(uint8_t* buffer, size_t size)
{
	png_image image;
	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	if (png_image_begin_read_from_memory(&image, buffer, size) != 0)
	{
		return _gfx_tex_load_png(image);
	}
	return NULL;
}

gfx_texture* video::tex_allocate32(int w, int h)
{
	gfx_texture* texture = (gfx_texture*)malloc(sizeof(gfx_texture) + (w * h * 4));
	if(texture)
	{
		texture->address = (void*)((uintptr_t)texture + sizeof(gfx_texture));
		texture->width  = w;
		texture->height = h;
	}
	return texture;
}

gfx_texture* video::tex_allocate16(int w, int h)
{
	gfx_texture* texture = (gfx_texture*)malloc(sizeof(gfx_texture) + (w * h * 2));
	if(texture)
	{
		texture->address = (void*)((uintptr_t)texture + sizeof(gfx_texture));
		texture->width  = w;
		texture->height = h;
	}
	return texture;
}

gfx_texture* video::tex_allocate8(int w, int h)
{
	gfx_texture* texture = (gfx_texture*)malloc(sizeof(gfx_texture) + (w * h));
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
	gfx_texture* texture = _gfx_tex_load_png_file((char*)path);
	return texture;
}

gfx_texture* video::tex_load(uint8_t* buffer, size_t size)
{
	if(buffer == NULL || size == 0) return NULL;

	gfx_texture* texture = _gfx_tex_load_png_mem(buffer, size);
	return texture;
}

void video::tex_delete(gfx_texture* texture)
{
	if(texture)	free(texture);
}

///////////////////////////////////////////////////////////////////////////////
//	Load font from PNG file and memory
///////////////////////////////////////////////////////////////////////////////
void video::font_delete(gfx_font *font)
{
	//Need to free up glyph pixel buffers 1st
	if(font==NULL) return;

	for(int x=255;x>=0;x--)
	{
		if(font->glyph[x]->img.pixels != NULL)
		{
			free(font->glyph[x]->img.pixels);
			font->glyph[x]->img.pixels=NULL;
		}
	}

	sft_freefont(font->sft.font);
	//Then remaining mem can freed
	free(font);
}

gfx_font* video::font_load(const char* path, gfx_color key, int font_size)
{
	//Want to carry on using ascii but want to convinence of TTF fonts
	//So we will create a TTF glyth for each ascii char
	int fontMemSize = sizeof(gfx_font)
	                + (sizeof(gfx_font_glyph*)*256)
					+ (sizeof(gfx_font_glyph)*256);

	gfx_font* font = (gfx_font*)malloc(fontMemSize);
	memset(font,0,fontMemSize);

	//Now setup pointers to each section item
	char *entryIdxMem=((char*)font)+sizeof(gfx_font);
	char *entryMem=(char*)entryIdxMem+(sizeof(gfx_font_glyph*)*256);

	font->glyph = (gfx_font_glyph**)entryIdxMem;
	for(int x=0;x<256;x++)
	{
		font->glyph[x]=(gfx_font_glyph*)entryMem;
		entryMem+=sizeof(gfx_font_glyph);
	}

	font->fontColor = key;
	font->sft.xScale = font_size;
	font->sft.yScale = font_size;
	font->sft.flags  = SFT_DOWNWARD_Y;
	font->sft.font=NULL;
	font->sft.xOffset=0;
	font->sft.yOffset=0;

	font->sft.font = sft_loadfile(path);
	if (font->sft.font == NULL)
	{
		free(font);
		debug::printf("TTF load failed\n");
		return NULL;	
	}
	
	sft_lmetrics(&font->sft, &font->lmtx);
	
	for(int c=0;c<256;c++)
	{
		//Convert ASCII to UNICODE since TTF lib works with UNICODE
		gfx_font_glyph *glyph=font->glyph[c];
		unsigned int unicode=c;//AsciiToUnicode(c);
		if (sft_lookup(&font->sft, unicode, &glyph->gid) < 0)
		{
			debug::printf("missing\n");
			video::font_delete(font);
			return NULL;
		}

		if (sft_gmetrics(&font->sft, glyph->gid, &glyph->mtx) < 0)
		{
			debug::printf("bad glyph metrics\n");
			video::font_delete(font);
			return NULL;
		}

		glyph->img.width = (glyph->mtx.minWidth + 3) & ~3;
		glyph->img.height = glyph->mtx.minHeight;
		glyph->img.pixels = malloc(glyph->img.width*glyph->img.height);
		
		if (sft_render(&font->sft, glyph->gid, glyph->img) < 0)
		{
			debug::printf("not rendered");
			video::font_delete(font);
			return NULL;
		}
			
	}
	return font;
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

void video::fill_rect(int x, int y, int w, int h, unsigned char *rgba)
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
			tBuff[(j * fb_gfx->width) + i] = _alpha_blend(
				tBuff[(j * fb_gfx->width) + i]
				,gfx_color_rgb(rgba[0], rgba[1], rgba[2])
				,(float)rgba[3]
			);
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

void video::draw_rgba(int x, int y, unsigned char* rgba, int w, int h)
{
	if(rgba == NULL) return;

	gfx_color* tBuff = (gfx_color*)fb_gfx->address;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < h) && (yy < fb_gfx->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < w) && (xx < fb_gfx->width); ii++, xx++) {
			if (xx < 0) continue;
			int offset = ((jj * w)<<2) + ii;
			tBuff[(yy * fb_gfx->width) + xx] = gfx_color_rgb(rgba[offset], rgba[offset +1], rgba[offset +2]);
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
	draw_rgba(x, y, (unsigned char*)texture->address, texture->width, texture->height);
}

void video::draw_tex_transp(int x, int y, gfx_texture* texture)
{
	if(texture == NULL) return;

	gfx_color* tBuff  = (gfx_color*)fb_gfx->address;
	unsigned char* tempTexture = (unsigned char*)texture->address;
	gfx_color  c;

	int xx, yy, ii, jj;
	for(jj = 0, yy = y; (jj < texture->height) && (yy < fb_gfx->height); jj++, yy++) {
		if (yy < 0) continue;
		for(ii = 0, xx = x; (ii < texture->width) && (xx < fb_gfx->width); ii++, xx++) {
			if (xx < 0) continue;
			int offset = ((jj * texture->width) + ii)<<2;
			c = tempTexture[offset + 3];
			if(c == 255)	
			{
				tBuff[(yy * fb_gfx->width) + xx] = gfx_color_rgb(
				                                                tempTexture[offset + 0]
			                                                        ,tempTexture[offset + 1]
			                                                        ,tempTexture[offset + 2]);
			}
			else if(c > 0)	
			{
				tBuff[(yy * fb_gfx->width) + xx] = _alpha_blend( tBuff[(yy * fb_gfx->width) + xx]
				                                                ,gfx_color_rgb(tempTexture[offset + 0]
			                                                        ,tempTexture[offset + 1]
			                                                        ,tempTexture[offset + 2])
			                                                        ,tempTexture[offset + 3]);
			}
		}
	}
}

void video::draw_tex_tiled(int x, int y, gfx_texture* texture, int xscroll, int yscroll)
{
	gfx_color* fbuff = (gfx_color*)fb_gfx->address + HW_SCREEN_WIDTH * y + x;
	unsigned char* tdata = (unsigned char*)texture->address;
	unsigned char* src;
	gfx_color* dst;

	int xx_mask = texture->width - 1;
	int yy_mask = texture->height * texture->width - 1;
	int xx, yy  = yscroll * texture->width;

	for (int i = y; i < HW_SCREEN_HEIGHT; i++) 
	{
		xx  = xscroll;
		src = tdata + (yy<<2);
		dst = fbuff;

		for (int j = x; j < HW_SCREEN_WIDTH; j++) 
		{
			
			

			int offset = xx<<2;
			if(src[offset + 3] > 0)	
			{
				*dst++ = gfx_color_rgb(
				                                                src[offset + 0]
			                                                        ,src[offset + 1]
			                                                        ,src[offset + 2]);
			}
			else
			{
				dst++;
			}
			xx++;
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

void video::print(int x, int y, const char* text, gfx_color color, gfx_font *font)
{
	if(font==NULL) return;
	_print(x, y, font, (char*)text, color);
}

void video::clear(gfx_color color)
{
	bind_framebuffer();
	_clear_framebuffer(color);
	term_framebuffer();
}

void video::clear_all(bool with_flip)
{
	_bind_framebuffer((gfx_color*)KernelVideoGetBuffer());
	_clear_framebuffer(0);
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

void video::bilinear_scale(unsigned char* src, int old_w, int old_h, int old_pitch, unsigned char* dst, int new_w, int new_h, int new_pitch)
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

			*(dst + (y * new_pitch) + x) =  *(src + L0 + C);
		}
	}
}

