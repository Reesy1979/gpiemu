/******************************************************************************
 *  Sega Master System / GameGear Emulator
 *  Copyright (C) 1998-2007  Charles MacDonald
 *
 *  additionnal code by Eke-Eke (SMS Plus GX)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   VDP rendering core
 *
 ******************************************************************************/

#include "shared.h"
#include "framework.h"

/*** Vertical Counter Tables ***/
extern uint8 *vc_table[2][3];

struct
{
    uint16 yrange;
    uint16 xpos;
    uint16 attr;
} object_info[64];

/* Background drawing function */
void (*render_bg)(int line) = NULL;
void (*render_obj)(int line) = NULL;

/* Pointer to output buffer */
uint8 *linebuf;

/* Pixel 8-bit color tables */
uint8 sms_cram_expand_tableR[4];
uint8 sms_cram_expand_tableG[4];
uint8 sms_cram_expand_tableB[4];

uint8 gg_cram_expand_tableR[16];
uint8 gg_cram_expand_tableG[16];
uint8 gg_cram_expand_tableB[16];

/* Dirty pattern info */
uint8 bg_name_dirty[0x400];     /* 1= This pattern is dirty */
uint16 bg_name_list[0x400];     /* List of modified pattern indices */
uint16 bg_list_index;           /* # of modified patterns in list */

/* Internal buffer for drawing non 8-bit displays */
static uint8 internal_buffer[0x4000];

/* Precalculated pixel table */
static uint16 pixel_rgb[PALETTE_SIZE];
static uint16 pixel_3d1[PALETTE_SIZE];
static uint16 pixel_3d2[PALETTE_SIZE];

static uint8 bg_pattern_cache[0x40000];/* Cached and flipped patterns */

/* Pixel look-up table */
static uint8 lut[0x20000];

static uint8 object_index_count;

/* Top Border area height */
static uint8 active_border[2][3] =
{
    { 24, 8,  0  }, /* NTSC VDP */
    { 48, 32, 24 }  /*  PAL VDP */
};

/* Active Scan Area height */
static uint16 active_range[2] =
{
    243, /* NTSC VDP */
    294  /*  PAL VDP */
};

/* CRAM palette in TMS compatibility mode */
static const uint8 tms_crom[] =
{
    0x00, 0x00, 0x08, 0x0C,
    0x10, 0x30, 0x01, 0x3C,
    0x02, 0x03, 0x05, 0x0F,
    0x04, 0x33, 0x15, 0x3F
};

/* original TMS palette for SG-1000 & Colecovision */
static uint8 tms_palette[16][3] =
{
    /* from Richard F. Drushel (http://users.stargate.net/~drushel/pub/coleco/twwmca/wk961118.html) */
    {   0 >> 3,  0 >> 2,  0 >> 3 },
    {   0 >> 3,  0 >> 2,  0 >> 3 },
    {  71 >> 3,183 >> 2, 59 >> 3 },
    { 124 >> 3,207 >> 2,111 >> 3 },
    {  93 >> 3, 78 >> 2,255 >> 3 },
    { 128 >> 3,114 >> 2,255 >> 3 },
    { 182 >> 3, 98 >> 2, 71 >> 3 },
    { 93  >> 3,200 >> 2,237 >> 3 },
    { 215 >> 3,107 >> 2, 72 >> 3 },
    { 251 >> 3,143 >> 2,108 >> 3 },
    { 195 >> 3,205 >> 2, 65 >> 3 },
    { 211 >> 3,218 >> 2,118 >> 3 },
    {  62 >> 3,159 >> 2, 47 >> 3 },
    { 182 >> 3,100 >> 2,199 >> 3 },
    { 204 >> 3,204 >> 2,204 >> 3 },
    { 255 >> 3,255 >> 2,255 >> 3 }
};

/* Attribute expansion table */
static const uint32 atex[4] =
{
    0x00000000,
    0x10101010,
    0x20202020,
    0x30303030,
};

/* Bitplane to packed pixel LUT */
static uint32 bp_lut[0x20000];

static void parse_satb(int line);
static void update_bg_pattern_cache(void);

/* Macros to access memory 32-bits at a time (from MAME's drawgfx.c) */

inline uint32 read_dword(unsigned char *address)
{
    if ((intptr_t)address & 3)
	{
        return ( *address++ |
                (*address++ << 8)  |
                (*address++ << 16) |
                (*address << 24) );
	}
	else
        return *(uint32 *) address;
}


inline void write_dword(unsigned char *address, uint32 data)
{
    if ((intptr_t)address & 3)
	{
            *address++ = (uint8) data;
            *address++ = (uint8) (data >> 8);
            *address++ = (uint8) (data >> 16);
            *address = (uint8) (data >> 24);
		return;
  	}
  	else
        *(uint32 *) address = data;
}

/****************************************************************************/


void render_shutdown(void)
{
}

/* Initialize the rendering data */
void render_init(void)
{
    int i, j;
    int bx, sx, b, s, bp, bf, sf, c;

    make_tms_tables();
   
    /* Generate 64k of data for the look up table */
    for(bx = 0; bx < 0x100; bx++)
    {
        for(sx = 0; sx < 0x100; sx++)
        {
            /* Background pixel */
            b  = (bx & 0x0F);

            /* Background priority */
            bp = (bx & 0x20) ? 1 : 0;

            /* Full background pixel + priority + sprite marker */
            bf = (bx & 0x7F);

            /* Sprite pixel */
            s  = (sx & 0x0F);

            /* Full sprite pixel, w/ palette and marker bits added */
            sf = (sx & 0x0F) | 0x10 | 0x40;

            /* Overwriting a sprite pixel ? */
            if(bx & 0x40)
            {
                /* Return the input */
                c = bf;
            }
            else
            {
                /* Work out priority and transparency for both pixels */
                if(bp)
                {
                    /* Underlying pixel is high priority */
                    if(b)
                    {
                        c = bf | 0x40;
                    }
                    else
                    {
                        if(s)
                        {
                            c = sf;
                        }
                        else
                        {
                            c = bf;
                        }
                    }
                }
                else
                {
                    /* Underlying pixel is low priority */
                    if(s)
                    {
                        c = sf;
                    }
                    else
                    {
                        c = bf;
                    }
                }
            }

            /* Store result */
            lut[(bx << 8) | (sx)] = c;
        }
    }

    /* Make bitplane to pixel lookup table */
    for(i = 0; i < 0x100; i++)
    {
        for(j = 0; j < 0x100; j++)
        {
            int x;
            uint32 out = 0;
            for(x = 0; x < 8; x++)
            {
                out |= (j & (0x80 >> x)) ? (uint32) (8 << (x << 2)) : 0;
                out |= (i & (0x80 >> x)) ? (uint32) (4 << (x << 2)) : 0;
            }
            bp_lut[(j << 8) | (i)] = out;
        }
    }

    render_reset();
}

/* Set the palettes data */
static void palette_init(void)
{
    int i;

    for(i = 0; i < 4; i++)
    {
        uint8 c = i << 6 | i << 4 | i << 2 | i;
        sms_cram_expand_tableR[i] = c >> 3;
        sms_cram_expand_tableG[i] = c >> 2;
        sms_cram_expand_tableB[i] = c >> 3;
    }
    for(i = 0; i < 16; i++)
    {
        uint8 c = i << 4 | i;
        gg_cram_expand_tableR[i] = c >> 3;
        gg_cram_expand_tableG[i] = c >> 2;
        gg_cram_expand_tableB[i] = c >> 3;
    }
    for(i = 0; i < REAL_PALETTE_SIZE; i++)
    {
        palette_sync(i);
    }
}

/* Reset the rendering data */
void render_reset(void)
{
    int i;

//@@    /* Clear display bitmap */
//@@    memset(bitmap.data, 0, bitmap.pitch * bitmap.height);

    palette_init();

    /* Invalidate pattern cache */
    memset(bg_name_dirty, 0, sizeof(bg_name_dirty));
    memset(bg_name_list, 0, sizeof(bg_name_list));
    bg_list_index = 0;
    memset(bg_pattern_cache, 0, sizeof(bg_pattern_cache));

    /* Pick default render routine */
    if (vdp.reg[0] & 4)
    {
        render_bg = render_bg_sms;
        render_obj = render_obj_sms;
    }
    else
    {
        render_bg = render_bg_tms;
        render_obj = render_obj_tms;
    }
}

static int prev_line = -1;

/* Draw a line of the display */
void render_line(int line)
{
	int view = 1;
    /* ensure we have not already rendered this line */
    if (prev_line == line) return;
    prev_line = line;

    /* Ensure we're within the VDP active area (incl. overscan) */
    int top_border = active_border[sms.display][vdp.extended];
    int vline = (line + top_border) % vdp.lpf;
    if (vline >= active_range[sms.display]) return;

	/* adjust for Game Gear screen */
	top_border = top_border + (vdp.height - bitmap.viewport.h) / 2;

    /* Point to current line in output buffer */
    linebuf = &internal_buffer[0];

    /* Sprite limit flag is set at the beginning of the line */
    if (vdp.spr_ovr)
    {
        vdp.spr_ovr = 0;
        vdp.status |= 0x40;
    }

    /* Vertical borders */
	if ((vline < top_border) || (vline >= (bitmap.viewport.h + top_border)))
    {
        /* Sprites are still processed offscreen */
        if ((vdp.mode > 7) && (vdp.reg[1] & 0x40)) 
            render_obj(line);

		view = 0;
    }

    /* Active display */
    else
    {
        /* Display enabled ? */
        if (vdp.reg[1] & 0x40)
        {
            /* Update pattern cache */
            update_bg_pattern_cache();

            /* Draw background */
            render_bg(line);

            /* Draw sprites */
            render_obj(line);

            /* Blank leftmost column of display */
            if((vdp.reg[0] & 0x20) && (IS_SMS)) 
                memset(linebuf, BACKDROP_COLOR, 8);
        }
        else 
        { 
            /* Background color */
            memset(linebuf, BACKDROP_COLOR, bitmap.viewport.w + 2 * bitmap.viewport.x); 
        }
    }

    /* Parse Sprites for next line */
    if (vdp.mode > 7) 
        parse_satb(line);
    else
        parse_line(line);

    /* LightGun mark */
    if (sms.device[0] == DEVICE_LIGHTGUN)
    {
        int dy = vdp.line - input.analog[0][1];

        if (abs(dy) < 6)
        {
            int i;
            int start = input.analog[0][0] - 4;
            int end = input.analog[0][0] + 4;
            if (start < 0) start = 0;
            if (end > 255) end = 255;
            for (i=start; i<end+1; i++)
            {
                linebuf[i] = 0xFF;
            }
        }
    }

	/* Only draw lines within the video output range ! */
	if (view)
	{
        uint16 *dst = ((uint16*)&bitmap.data[line * bitmap.pitch]) + bitmap.viewport.x;
        uint8  *src = linebuf + bitmap.viewport.x;
#if 0
        if(sms.glasses_3d) {
            uint16 *pixel = (sms.wram[0x1ffb] & 1) ? pixel_3d1 : pixel_3d2;
            if(anaglyph_merge_frames) {
                for(int i = bitmap.viewport.w; --i >= 0; dst++, src++) *dst = (pixel[*src] + *dst);
            } else {
                for(int i = bitmap.viewport.w; --i >= 0;) *dst++ = pixel[*src++];
            }
        } else {
            for(int i = bitmap.viewport.w; --i >= 0;) *dst++ = pixel_rgb[*src++];
        }
#endif
        for(int i = bitmap.viewport.w; --i >= 0;) *dst++ = pixel_rgb[*src++];
	}
}

/* Draw the Master System background */
void render_bg_sms(int line)
{
  int locked = 0;
  int yscroll_mask = (vdp.extended) ? 256 : 224;
  int v_line = (line + vdp.vscroll) % yscroll_mask;
  int v_row  = (v_line & 7) << 3;
  int hscroll = ((vdp.reg[0] & 0x40) && (line < 0x10) && (sms.console != CONSOLE_GG)) ? 0 : (0x100 - vdp.reg[8]);
  int column = 0;
  uint16 attr;
  uint16 nt_addr = (vdp.ntab + ((v_line >> 3) << 6)) & (((sms.console == CONSOLE_SMS) && !(vdp.reg[2] & 1)) ? ~0x400 :0xFFFF);
  uint16 *nt = (uint16 *)&vdp.vram[nt_addr];
  int nt_scroll = (hscroll >> 3);
  int shift = (hscroll & 7);
  uint32 atex_mask;
  uint32 *cache_ptr;
  uint32 *linebuf_ptr = (uint32 *)&linebuf[0-shift];
  
  /* Draw first column (clipped) */
  if(shift)
  {
    int x;

    for(x = shift; x < 8; x++)
      linebuf[(0 - shift) + (x)] = 0;

    column++;
  }

  /* Draw a line of the background */
  for(; column < 32; column++)
  {
    /* Stop vertical scrolling for leftmost eight columns */
    if((vdp.reg[0] & 0x80) && (!locked) && (column >= 24))
    {
      locked = 1;
      v_row = (line & 7) << 3;
      nt = (uint16 *)&vdp.vram[nt_addr];
    }

    /* Get name table attribute word */
    attr = nt[(column + nt_scroll) & 0x1F];

    /* Expand priority and palette bits */
    atex_mask = atex[(attr >> 11) & 3];

    /* Point to a line of pattern data in cache */
    cache_ptr = (uint32 *)&bg_pattern_cache[((attr & 0x7FF) << 6) | (v_row)];
    
    /* Copy the left half, adding the attribute bits in */
    write_dword((uint8 *) &linebuf_ptr[(column << 1)], read_dword((uint8 *) &cache_ptr[0]) | (atex_mask));
    //write_dword((uint8 *) &linebuf_ptr[(column)], read_dword((uint8 *) &cache_ptr[0]) | (atex_mask));

    /* Copy the right half, adding the attribute bits in */
    write_dword((uint8 *) &linebuf_ptr[(column << 1) | (1)], read_dword((uint8 *) &cache_ptr[1]) | (atex_mask));
    //write_dword((uint8 *) &linebuf_ptr[(column) | (1)], read_dword((uint8 *) &cache_ptr[1]) | (atex_mask));

  }

  /* Draw last column (clipped) */
  if(shift)
  {
    int x, c, a;

    uint8 *p = &linebuf[(0 - shift)+(column << 3)];

    attr = nt[(column + nt_scroll) & 0x1F];

    a = (attr >> 7) & 0x30;

    for(x = 0; x < shift; x++)
    {
      c = bg_pattern_cache[((attr & 0x7FF) << 6) | (v_row) | (x)];
      p[x] = ((c) | (a));
    }
  }
}

/* Draw sprites */
void render_obj_sms(int line)
{
  int i,x,start,end,xp,yp,n;
  uint8 sp,bg;
  uint8 *linebuf_ptr;
  uint8 *cache_ptr;

  int width = 8;

  /* Adjust dimensions for double size sprites */
  if(vdp.reg[1] & 0x01)
    width *= 2;

  /* Draw sprites in front-to-back order */
  for(i = 0; i < object_index_count; i++)
  {
    /* Width of sprite */
    start = 0;
    end = width;

    /* Sprite X position */
    xp = object_info[i].xpos;

    /* Sprite Y range */
    yp = object_info[i].yrange;

    /* Pattern name */
    n = object_info[i].attr;

    /* X position shift */
    if(vdp.reg[0] & 0x08) xp -= 8;

    /* Add MSB of pattern name */
    if(vdp.reg[6] & 0x04) n |= 0x0100;

    /* Mask LSB for 8x16 sprites */
    if(vdp.reg[1] & 0x02) n &= 0x01FE;

    /* Point to offset in line buffer */
    linebuf_ptr = (uint8 *)&linebuf[xp];

    /* Clip sprites on left edge */
    if(xp < 0)
      start = (0 - xp);

    /* Clip sprites on right edge */
    if((xp + width) > 256)
      end = (256 - xp);

    /* Draw double size sprite */
    if(vdp.reg[1] & 0x01)
    {
      /* Retrieve tile data from cached nametable */
      cache_ptr = (uint8 *)&bg_pattern_cache[(n << 6) | ((yp >> 1) << 3)];

      /* Draw sprite line (at 1/2 dot rate) */
      for(x = start; x < end; x+=2)
      {
        /* Source pixel from cache */
        sp = cache_ptr[(x >> 1)];

        /* Only draw opaque sprite pixels */
        if(sp)
        {
          /* Background pixel from line buffer */
          bg = linebuf_ptr[x];

          /* Look up result */
          linebuf_ptr[x] = linebuf_ptr[x+1] = lut[(bg << 8) | (sp)];

          /* Check sprite collision */
          if ((bg & 0x40) && !(vdp.status & 0x20))
          {
            /* pixel-accurate SPR_COL flag */
            vdp.status |= 0x20;
            vdp.spr_col = (line << 8) | ((xp + x + 13) >> 1);
          }
        }
      }
    }
    else /* Regular size sprite (8x8 / 8x16) */
    {
      /* Retrieve tile data from cached nametable */
      cache_ptr = (uint8 *)&bg_pattern_cache[(n << 6) | (yp << 3)];

      /* Draw sprite line */
      for(x = start; x < end; x++)
      {
        /* Source pixel from cache */
        sp = cache_ptr[x];

        /* Only draw opaque sprite pixels */
        if(sp)
        {
          /* Background pixel from line buffer */
          bg = linebuf_ptr[x];

          /* Look up result */
          linebuf_ptr[x] = lut[(bg << 8) | (sp)];

          /* Check sprite collision */
          if ((bg & 0x40) && !(vdp.status & 0x20))
          {
            /* pixel-accurate SPR_COL flag */
            vdp.status |= 0x20;
            vdp.spr_col = (line << 8) | ((xp + x + 13) >> 1);
          }
        }
      }
    }
  }
}

/* Generate RGB for 3D red/cyan glasses */
static void adjust_rgb(int& r, int& g, int& b, int filter)
{
    if(filter == 1) { g = 0; b = 0; }
    if(filter == 2) { r = 0;        }
}

static void save_rgb_to_palette(uint16 *pixel, int r, int g, int b, int index)
{
    pixel[index] = MAKE_PIXEL(r, g, b);
    pixel[index + 0x20] = pixel[index];
    pixel[index + 0x40] = pixel[index];
    pixel[index + 0x60] = pixel[index];
    pixel[index + 0x80] = pixel[index];
    pixel[index + 0xa0] = pixel[index];
    pixel[index + 0xc0] = pixel[index];
    pixel[index + 0xe0] = pixel[index];
}

/* Update a palette entry */
void palette_sync(int index)
{
	int r, g, b;

	/* VDP Mode */
    if ((vdp.reg[0] & 4) || IS_GG)
    {
        if(IS_GG)
        {
			/* GG palette       */
            /* ----BBBBGGGGRRRR */
            r = (vdp.cram[(index << 1) | 0] >> 0) & 0x0F;
            g = (vdp.cram[(index << 1) | 0] >> 4) & 0x0F;
            b = (vdp.cram[(index << 1) | 1] >> 0) & 0x0F;
        
            r = gg_cram_expand_tableR[r];
            g = gg_cram_expand_tableG[g];
            b = gg_cram_expand_tableB[b];
        }
        else
        {
			/* SMS palette */
            /* --BBGGRR    */
            r = (vdp.cram[index] >> 0) & 3;
            g = (vdp.cram[index] >> 2) & 3;
            b = (vdp.cram[index] >> 4) & 3;
        
            r = sms_cram_expand_tableR[r];
            g = sms_cram_expand_tableG[g];
            b = sms_cram_expand_tableB[b];
        }
    }
    else
    {
        /* TMS Mode (16 colors only) */
        int color = index & 0x0F;

        if (sms.console < CONSOLE_SMS)
        {
            r = tms_palette[color][0];
            g = tms_palette[color][1];
            b = tms_palette[color][2];
        }
        else
        {
            /* fixed CRAM palette in TMS mode */ 
            r = (tms_crom[color] >> 0) & 3;
            g = (tms_crom[color] >> 2) & 3;
            b = (tms_crom[color] >> 4) & 3;

            r = sms_cram_expand_tableR[r];
            g = sms_cram_expand_tableG[g];
            b = sms_cram_expand_tableB[b];
        }
    }

    if (sms.glasses_3d) {
        int r1 = r, r2 = r, g1 = g, g2 = g, b1 = b, b2 = b;
        adjust_rgb(r1, g1, b1, 1);
        adjust_rgb(r2, g2, b2, 2);
        save_rgb_to_palette(pixel_3d1, r1, g1, b1, index);
        save_rgb_to_palette(pixel_3d2, r2, g2, b2, index);
    } else {
        save_rgb_to_palette(pixel_rgb, r, g, b, index);
    }
}

static void parse_satb(int line)
{
  /* Pointer to sprite attribute table */
  uint8 *st = (uint8 *)&vdp.vram[vdp.satb];

  /* Sprite counter (64 max.) */
  int i = 0;

  /* Line counter value */
  int vc = vc_table[sms.display][vdp.extended][line];

  /* Sprite height (8x8 by default) */
  int yp;
  int height = 8;
  
  /* Adjust height for 8x16 sprites */
  if(vdp.reg[1] & 0x02) 
    height <<= 1;

  /* Adjust height for zoomed sprites */
  if(vdp.reg[1] & 0x01)
    height <<= 1;

  /* Sprite count for current line (8 max.) */
  object_index_count = 0;

  for(i = 0; i < 64; i++)
  {
    /* Sprite Y position */
    yp = st[i];

    /* Found end of sprite list marker for non-extended modes? */
    if(vdp.extended == 0 && yp == 208)
      return;

    /* Wrap Y coordinate for sprites > 240 */
    if(yp > 240) yp -= 256;

    /* Compare sprite position with current line counter */
    yp = vc - yp;

    /* Sprite is within vertical range? */
    if((yp >= 0) && (yp < height))
    {
      /* Sprite limit reached? */
      if (object_index_count == 8)
      {
        /* Flag is set only during active area */
        if (line < vdp.height)
          vdp.spr_ovr = 1;

        /* End of sprite parsing */
        //if (option.spritelimit)
          return;
      }

      /* Store sprite attributes for later processing */
      object_info[object_index_count].yrange = yp;
      object_info[object_index_count].xpos = st[0x80 + (i << 1)];
      object_info[object_index_count].attr = st[0x81 + (i << 1)];

      /* Increment Sprite count for current line */
      ++object_index_count;
    }
  }
}

static void update_bg_pattern_cache(void)
{
    int i;
    uint8 x, y;
    uint16 name;
    uint8 y3;
    uint8 y7;
    uint8 *dst;
    uint8 dirty;
    uint8 *dst0000;
    uint8 *dst8000;
    uint8 *dst10000;
    uint8 *dst18000;

    if(!bg_list_index) return;

    for(i = 0; i < bg_list_index; i++)
    {
        name = bg_name_list[i];
        bg_name_list[i] = 0;
        dirty = bg_name_dirty[name];

        if(dirty)
        {
            dst = &bg_pattern_cache[name << 6];
            for(y = 0; y < 8; y++)
            {
                /* Test bits */
                if(dirty & (1 << y))
                {
                    y3 = y << 3;
                    y7 = (y ^ 7) << 3;
                    uint16 bp01 = *(uint16 *) &vdp.vram[(name << 5) | (y << 2)];
                    uint16 bp23 = *(uint16 *) &vdp.vram[(name << 5) | (y << 2) | 2];
                    uint32 temp = (bp_lut[bp01] >> 2) | (bp_lut[bp23]);
                    dst0000 = dst + y3;
                    dst10000 = dst + 0x10000 + y7;
                    dst8000 = (dst + 0x8000 + y3) + 7;
                    dst18000 = (dst + 0x18000 + y7) + 7;
                    for(x = 0; x < 8; x++)
                    {
                        uint8 c = (uint8) ((temp >> (x << 2)) & 0x0F);
                        *dst0000++ = c;
                        *dst10000++ = c;

                        *dst8000-- = c;
                        *dst18000-- = c;
                    }
                }
            }
            bg_name_dirty[name] = 0;
        }
    }
    bg_list_index = 0;
}

