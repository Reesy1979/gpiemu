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

#include <ctype.h>
#include "zip.h"
#include "unzip.h"
#include "framework.h"
#include "state.io.h"
#include <stdarg.h>

using namespace hw;
using namespace fw;

////////////////////////////////////////////////////////////////////////////////
//	Menu module
////////////////////////////////////////////////////////////////////////////////

#define MENU_TILE_SPEED			2

#define INP_BUTTON_MENU_SELECT	HW_INPUT_A
#define INP_BUTTON_MENU_CANCEL	HW_INPUT_B

static gfx_texture* mMenuTile;

static float		mMenutileXscroll = 0;
static float		mMenutileYscroll = 0;
static float		tileXstep = 0.5;
static float        tileYstep = 0.5;


struct DIRECTORY_ENTRY
{
	char filename[255];
	char displayName[255];
	int type;
};

static char     	mMenuTitle   [MAX_MENU_ITEM_CHARS + 1];
static char			mMenuRomName [MAX_MENU_ITEM_CHARS + 1];
static char			mMenuText    [MAX_MENU_ITEM_COUNT][MAX_MENU_ITEM_CHARS + 1];
static bool			mMenuEnable  [MAX_MENU_ITEM_COUNT];
static int 			mMenuInterval;
static int          mMenuFontH;
static float		mMenuSmooth;

static gfx_color 	mRomNameColor;

typedef struct {
	bool		 enable;
	gfx_texture* texture;
	int			 x;
	int			 y;
} gfx_bg_tex;

static gfx_bg_tex bg_texture[3];

////////////////////////////////////////////////////////////////////////////////

static void _update_menu_interval(uint32_t keysHeld, uint32_t keysRepeat)
{
	if(keysHeld || keysRepeat) mMenuInterval = 16;
	if(mMenuInterval < 16) mMenuInterval++;
	if(mMenuInterval > 16) mMenuInterval--;
}

static unsigned long _rand()
{
    static 
    unsigned long t = system::get_frames_count();
    unsigned long k = t / 12773;
    t = 16807 * (t - k * 12773) - 2836 * k;
    return t;
}

static float _get_direction()
{
	char a = _rand();
	return a < -40 ? -0.5 : (a > 40 ? 0.5 : 0);
}

static bool isNotSeparator(char ch) 
{
    return (ch > ' ' && ch != '-' && ch != '/' && ch != ',' && ch != '.');
}

static int nextSeparator(cstr_t str, int width, int &pos)
{
	int len = strlen(str);
	if(pos >= len) return -1;

	int start = pos;
	int index = pos;

	while(true) {
		while(index < len && isNotSeparator(str[index])) index++;
		int w = index - start;
		if(pos == start && w > width) {
			while((--index - start) > width) {}
			return (pos = index);
		}
		if(w <= width) {
			pos = index + 1;
		}
		if(w > width || index >= len || str[index] < ' ') {
			return pos < len ? pos : len;
		}
		index++;
	}
}

static void getMultilineText(cstr_t in, char* out, int w)
{
	int pos = 0, start = 0, index; out[0] = 0;
	while((index = nextSeparator(in, w, pos)) >= 0) 
	{
		strncat(out, in + start, index - start);
		int i = strlen(out); while(--i >= 0) if(isspace(out[i])) out[i] = 0; else break;
		strcat(out, "\n\n");
		start = index;
	}
}

static void waitForNextFrame()
{
	static uint32_t lastFrameCount;
	while(true) {
		if(system::get_frames_count() > lastFrameCount) {
			lastFrameCount = system::get_frames_count();
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void menu::init(const char* romName, gfx_color romNameColor)
{
	strncpy(mMenuRomName, romName, MAX_MENU_ITEM_CHARS);
	mMenuRomName[MAX_MENU_ITEM_CHARS-1] = 0;
	mRomNameColor = romNameColor;
}

void menu::setup_bg_texture(gfx_texture* texture)
{
	mMenuTile = texture;
}

void menu::setup_bg_texture(MENU_TEXTURE index, gfx_texture* texture, int x, int y)
{
	bg_texture[index].texture = texture;
	bg_texture[index].x = x;
	bg_texture[index].y = y;
	select_bg_texture(false);
}

void menu::setup_bg_scrolling()
{
	do {
		tileXstep = _get_direction();
		tileYstep = _get_direction();
	} while(tileXstep + tileYstep == 0); 
}

void menu::select_bg_texture(bool logo)
{
	if(logo)	{
		bg_texture[MT_LOGO   ].enable = bool(bg_texture[MT_LOGO].texture);
		bg_texture[MT_GAMEPAD].enable = false;
		bg_texture[MT_COVER  ].enable = false;
	} else {
		bg_texture[MT_LOGO   ].enable = false;
		bg_texture[MT_GAMEPAD].enable = bool(bg_texture[MT_GAMEPAD].texture) && !bool(bg_texture[MT_COVER].texture);
		bg_texture[MT_COVER  ].enable = bool(bg_texture[MT_COVER].texture);
	}
}

void menu::set_title(const char* title)
{
	strncpy(mMenuTitle, title, MAX_MENU_ITEM_CHARS);
	mMenuTitle[MAX_MENU_ITEM_CHARS] = 0;
}

void menu::set_item_text(int index, const char* text)
{
	strcpy(mMenuText[index], text);
	mMenuEnable[index] = true;
}

void menu::set_separator(int index)
{
	strcpy(mMenuText[index], "$");
	mMenuEnable[index] = false;
}

void menu::set_item_optstr(int index, const char* text, const char* optStr)
{
	sprintf(mMenuText[index], text, optStr);
	mMenuEnable[index] = true;
}

void menu::set_item_optint(int index, const char* text, const int optInt)
{
	sprintf(mMenuText[index], text, optInt);
	mMenuEnable[index] = true;
}

void menu::set_item_enable(int index, bool enable)
{
	mMenuEnable[index] = enable;
}

int menu::next_val(int* values, int count, int current)
{
	int x;
	for (x = 0; x < count; x++) {
		if(values[x] > current) return values[x];
	}
	return values[0];
}

int menu::prev_val(int* values, int count, int current)
{
	int x;
	for (x = count - 1; x >= 0; x--) {
		if(values[x] < current) return values[x];
	}
	return values[count - 1];
}

////////////////////////////////////////////////////////////////////////////////

void menu::open()
{
	mMenuInterval = 1;
	mMenuSmooth  = 0.0;
	int font_w, font_h;
	video::font_size(video::font_get(), "T", &font_h, &font_w);

	mMenuFontH = (int)((float)font_h*1.5);
	input::ignore();
}

MENU_ACTION menu::rom_list_update(int count, int& focus)
{
	waitForNextFrame();

	uint32_t keysHeld   = input::poll();
	uint32_t keysRepeat = input::get_repeat();
	_update_menu_interval(keysHeld, keysRepeat);

	if (keysRepeat & HW_INPUT_UP  ) focus--;
	if (keysRepeat & HW_INPUT_DOWN) focus++;
	
	if (keysRepeat & HW_INPUT_LEFT  ) focus-=10;
	if (keysRepeat & HW_INPUT_RIGHT) focus+=10;
	
	if (focus > count - 1) focus = 0;
	if (focus < 0) focus = count - 1;


	if (keysRepeat & INP_BUTTON_MENU_CANCEL) {
		return MA_CANCEL;
	}

	if (keysRepeat & INP_BUTTON_MENU_SELECT) {
		return MA_SELECT;
	}

	return MA_NONE;
}

MENU_ACTION menu::update(int count, int& focus)
{
	waitForNextFrame();

	uint32_t keysHeld   = input::poll();
	uint32_t keysRepeat = input::get_repeat();
	_update_menu_interval(keysHeld, keysRepeat);

	do {
		if (keysRepeat & HW_INPUT_UP  ) focus--;
		if (keysRepeat & HW_INPUT_DOWN) focus++;
		
		if (focus > count - 1) focus = 0;
		if (focus < 0) focus = count - 1;

	} while(mMenuText[focus][0] == '$');

	if (keysRepeat & INP_BUTTON_MENU_CANCEL) {
		return MA_CANCEL;
	}
	if(mMenuEnable[focus]) {
		if (keysRepeat & HW_INPUT_LEFT) {
			return MA_DECREASE;
		}
		if (keysRepeat & HW_INPUT_RIGHT) {
			return MA_INCREASE;
		}
		if (keysRepeat & INP_BUTTON_MENU_SELECT) {
			return MA_SELECT;
		}
	}
	return MA_NONE;
}

void menu::close()
{
	mMenuInterval = 32;
	input::ignore();
}

////////////////////////////////////////////////////////////////////////////////

void menu::render_tiled_bg()
{
	//	draw tiled background
	video::draw_tex_tiled(0, 0, mMenuTile, int(mMenutileXscroll), int(mMenutileYscroll));

	//	update backgroind tile scroll
	mMenutileXscroll += tileXstep;
	if(mMenutileXscroll < 0) mMenutileXscroll += mMenuTile->width;
	if(mMenutileXscroll >= mMenuTile->width) mMenutileXscroll -= mMenuTile->width;
	mMenutileYscroll += tileYstep;
	if(mMenutileYscroll < 0) mMenutileYscroll += mMenuTile->height;
	if(mMenutileYscroll >= mMenuTile->height) mMenutileYscroll -= mMenuTile->height;

//@@	//	draw additional textures
//@@	if(bg_texture[MT_LOGO].enable)
//@@		video::draw_tex_transp(bg_texture[MT_LOGO].x, bg_texture[MT_LOGO].y, bg_texture[MT_LOGO].texture);
//@@	if(bg_texture[MT_GAMEPAD].enable)
//@@		video::draw_tex_transp(bg_texture[MT_GAMEPAD].x, bg_texture[MT_GAMEPAD].y, bg_texture[MT_GAMEPAD].texture);

//@@	//	draw cover if present
//@@	if(bg_texture[MT_COVER].enable)
//@@		video::draw_tex(bg_texture[MT_COVER].x, bg_texture[MT_COVER].y, bg_texture[MT_COVER].texture);
}

void menu::render_text(int x, int y, const char* text, gfx_color color)
{
	video::print(x + 1, y + 1, text, 0);
	video::print(x, y, text, color);
}

void menu::render_rect(int x, int y, int w, int h, gfx_color color)
{
	video::draw_rect(x, y, w, h, color);
	video::draw_rect(x + 1, y + 1, w - 2, h - 2, color);
}

void menu::render_bg()
{
	//	draw tiled background
	int font_h, font_w;
	int x, y;

	video::font_size(video::font_get(), "T", &font_h, &font_w);
	render_tiled_bg();

	//	draw additional textures
	if(bg_texture[MT_LOGO].enable)
		video::draw_tex_transp(bg_texture[MT_LOGO].x, bg_texture[MT_LOGO].y, bg_texture[MT_LOGO].texture);
	if(bg_texture[MT_GAMEPAD].enable)
		video::draw_tex_transp(bg_texture[MT_GAMEPAD].x, bg_texture[MT_GAMEPAD].y, bg_texture[MT_GAMEPAD].texture);
		
}

void menu::render_footer()
{
	//	draw tiled background
	int x, y;
	int font_h, font_w;

	//	draw rom name
	if(COLOR_FOOTER != COLOR_TRANSPARENT) {
		if(HALF_TRANSP_FOOTER)
			video::mult_rect(0, HW_SCREEN_HEIGHT - mMenuFontH, HW_SCREEN_WIDTH, mMenuFontH, COLOR_FOOTER);
		else
			video::fill_rect(0, HW_SCREEN_HEIGHT - mMenuFontH, HW_SCREEN_WIDTH, mMenuFontH, COLOR_FOOTER);
	}
	if(COLOR_HF_LINE != COLOR_TRANSPARENT) video::fill_rect(0, HW_SCREEN_HEIGHT - mMenuFontH - 2, HW_SCREEN_WIDTH, 2, COLOR_HF_LINE);
	video::font_size(video::font_get(), mMenuRomName, &font_h, &font_w);
	x = (HW_SCREEN_WIDTH - font_w) >> 1;
	y = HW_SCREEN_HEIGHT - mMenuFontH;
	render_text(x, y, mMenuRomName, mRomNameColor);
}

void menu::render_title()
{
	//	draw tiled background
	int font_h, font_w;
	int x, y;

	video::font_size(video::font_get(), "T", &font_h, &font_w);

	//	draw title
	if(COLOR_HEADER != COLOR_TRANSPARENT) {
		if(HALF_TRANSP_HEADER)
			video::mult_rect(0, 0, HW_SCREEN_WIDTH, 16, COLOR_HEADER);
		else
			video::fill_rect(0, 0, HW_SCREEN_WIDTH, 16, COLOR_HEADER);
	}
	if(COLOR_HF_LINE != COLOR_TRANSPARENT) video::fill_rect(0, 16, HW_SCREEN_WIDTH, 2, COLOR_HF_LINE);

	video::font_size(video::font_get(), mMenuTitle, &font_h, &font_w);
	x = (HW_SCREEN_WIDTH - font_w) >> 1;
	y = (16 - font_h) >> 1;
	render_text(x, y, mMenuTitle, COLOR_HEADER_TXT);
}

void menu::render_file_list(MENU_DIRECTORY_LIST *list, int focus)
{
	render_file_list(list, focus, true);
}

void menu::render_file_list(MENU_DIRECTORY_LIST *list, int focus, bool render_background)
{
	mMenuSmooth = (mMenuSmooth * 7) + (float)focus ;
	mMenuSmooth /= 8;

	//	draw background
	if(render_background) render_bg();
	
	

	//	draw selector
	int y = 112;
	int x = 0;
	int w = HW_SCREEN_WIDTH;

	if(COLOR_SELECTOR_INSIDE != COLOR_TRANSPARENT) {
		if(HALF_TRANSP_SELECTOR)
			video::mult_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_INSIDE);
		else
			video::fill_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_INSIDE);
	}
	if(COLOR_SELECTOR_EDGING != COLOR_TRANSPARENT) render_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_EDGING);

	//	save background area for future clipping
	//gfx_color clipping[2][HW_SCREEN_WIDTH * 12];
	//gfx_color *p = (gfx_color*)video::get_framebuffer();
	//memcpy(clipping[0], p + (16 + 2) * HW_SCREEN_WIDTH, sizeof clipping[0]);
	//memcpy(clipping[1], p + (HW_SCREEN_HEIGHT - 16 - 2 - 12) * HW_SCREEN_WIDTH, sizeof clipping[1]);

	//	draw menu items
	int renderc = 0;
	for (int i = 0; i < list->count; i++)
	{
		y = 112 + 4 + (i * mMenuFontH) - (int)(mMenuSmooth * mMenuFontH);
		if (y <= 16 + 2 || y >= HW_SCREEN_HEIGHT - (mMenuFontH * 2)) continue;
		render_text(8, y, (const char*)list->fileList[i]->displayName, COLOR_MENU_TXT_ENABLE);
		renderc++;
	}
	//	draw menu clipping
	//video::draw_rgb(0, 16 + 2, clipping[0], HW_SCREEN_WIDTH, 12);
	//video::draw_rgb(0, HW_SCREEN_HEIGHT - 16 - 2 - 12, clipping[1], HW_SCREEN_WIDTH, 12);

	render_title();
	render_footer();
}


void menu::render_menu(int count, int focus)
{
	mMenuSmooth = mMenuSmooth * 7 + (focus);
	mMenuSmooth /= 8;

	//	draw background
	render_bg();
	

	//	draw selector
	int y = 112;
	int x = 0;
	int w = HW_SCREEN_WIDTH;
	if(COLOR_SELECTOR_INSIDE != COLOR_TRANSPARENT) {
		if(HALF_TRANSP_SELECTOR)
			video::mult_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_INSIDE);
		else
			video::fill_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_INSIDE);
	}

	if(COLOR_SELECTOR_EDGING != COLOR_TRANSPARENT) render_rect(x, y, w, mMenuFontH, COLOR_SELECTOR_EDGING);

	//	draw cover if present
	if(bg_texture[MT_COVER].enable) {
		if(COLOR_STATESHOT_EDGING != COLOR_TRANSPARENT) {
			render_rect(
				bg_texture[MT_COVER].x - 2,
				bg_texture[MT_COVER].y - 2,
				bg_texture[MT_COVER].texture->width + 4,
				bg_texture[MT_COVER].texture->height + 4,
				COLOR_STATESHOT_EDGING);
		}
		video::draw_tex(bg_texture[MT_COVER].x, bg_texture[MT_COVER].y, bg_texture[MT_COVER].texture);
	}

	//	draw menu items
	int font_h, font_w;
	video::font_size(video::font_get(),"T", &font_h, &font_w);
	for (int i = 0; i < count; i++)
	{
		y = 112 + 4 + (i * mMenuFontH) - (int)(mMenuSmooth * mMenuFontH);
		if (y <= 16 + 2 || y >= HW_SCREEN_HEIGHT - (mMenuFontH * 2)) continue;
		if (mMenuText[i][0] == '$') continue;
		render_text(8, y, mMenuText[i], mMenuEnable[i] ? COLOR_MENU_TXT_ENABLE : COLOR_MENU_TXT_DISABLE);
	}

	render_title();
	render_footer();
}

void menu::render_stateshot(int x, int y, int w, int h, int mode, int slot, gfx_color* stateCurrent, gfx_color* stateSaved)
{
	static bool white_noise = true;

	if(COLOR_STATESHOT_INSIDE != COLOR_TRANSPARENT) {
		if(HALF_TRANSP_STATESHOT)
			video::mult_rect (x - 2, y - 2, w + 4, 16 + 4, COLOR_STATESHOT_INSIDE);
		else
			video::fill_rect (x - 2, y - 2, w + 4, 16 + 4, COLOR_STATESHOT_INSIDE);
	}
	if(COLOR_STATESHOT_EDGING != COLOR_TRANSPARENT) {
		render_rect(x - 2, y, w + 4, 16 + h + 2, COLOR_STATESHOT_EDGING);
		video::fill_rect (x - 2, y + 16 - 2, w + 4, 2, COLOR_STATESHOT_EDGING);
	}
	
	char slotText[16] = "Current";
	if(mode == 0) {
		video::draw_rgb(x, y + 16, stateCurrent, w, h);
	} else {
		sprintf(slotText, "Slot %d", slot);
		if(mode == 1 && white_noise) {
			unsigned char r;
			for(int i = w * h - 1; i >= 0; i--) {
				r = _rand(); stateSaved[i] = gfx_color_rgb(r, r, r);
			}			
		}
		video::draw_rgb(x, y + 16, stateSaved, w, h);
		white_noise ^= true;
	}
	int font_h, font_w;
	video::font_size(video::font_get(), slotText, &font_h, &font_w);
	x += (w  - font_w) >> 1;
	y += (16 - font_h) >> 1;
	render_text(x, y, slotText, COLOR_STATESHOT_TXT);
}

////////////////////////////////////////////////////////////////////////////////
//	Message box
////////////////////////////////////////////////////////////////////////////////

bool menu::msg_box(cstr_t message, MENU_MSG_BOX mode)
{
	int  menuExit   = 0;
	int  menuCount  = 0;
	int  menufocus  = 0;
	bool action     = 0;

	uint32_t  keysRepeat = 0;
	uint32_t  keysHeld   = 0;

	open();
	if(mode == MMB_YESNO) {
		set_item_text(0, "YES");
		set_item_text(1, "NO");
		menuCount = 2;
	} else
	if(mode == MMB_PAUSE) {
		set_item_text(0, "Press A or B to continue");
		menuCount = 1;
	}

	char msg[256];				
	getMultilineText(message, msg, MAX_MENU_ITEM_CHARS);
	while (!menuExit)
	{
		video::bind_framebuffer();
		if(mode != MMB_MESSAGE) {
			render_menu(menuCount, menufocus);
		} else {
			render_bg();
			render_title();
			render_footer();
		}
		render_text(8, 48, msg, COLOR_MSGBOX_TXT);
		video::term_framebuffer();
		video::flip(0);

		waitForNextFrame();
		if(mode != MMB_MESSAGE) {
			keysHeld   = input::poll();
			keysRepeat = input::get_repeat();
			_update_menu_interval(keysHeld, keysRepeat);
			if (keysRepeat & HW_INPUT_UP  ) menufocus--;
			if (keysRepeat & HW_INPUT_DOWN) menufocus++;
			if (keysHeld & INP_BUTTON_MENU_SELECT)
			{
				action = (menufocus == 0);	
				menuExit = 1;
			}
			if (keysHeld & INP_BUTTON_MENU_CANCEL)
			{
				action = false;	
				menuExit = 1;
			}
			if (menufocus > menuCount - 1) {
				menufocus  = 0;
				mMenuSmooth = menufocus -1;
			} else 
			if (menufocus < 0) {
				menufocus  = menuCount - 1;
				mMenuSmooth = menufocus - 1;
			}
		} else {
			action = true;	
			menuExit = 1;
		}
	}
	close();
	return action;
}

////////////////////////////////////////////////////////////////////////////////
//	File system support
////////////////////////////////////////////////////////////////////////////////

//	Naming conventions:
//	filePath	=> A:\Game\Emulator.sim
//	fileName 	=> Emulator.sim
//	path 		=> A:\Game
//	name 		=> Emulator
//	ext 		=> sim

static char emulPath[HW_MAX_PATH];
static char gamePath[HW_MAX_PATH];
static char gameFilePath[HW_MAX_PATH];

////////////////////////////////////////////////////////////////////////////////

void fsys::split(cstr_t filePath, char* path, char* name, char* ext)
{	
	int len = strlen(filePath), i = 0, dot = -1, slash = -1;

	path[0] = 0;
	name[0] = 0;
	ext [0] = 0;
	
	if (len <= 0) return;
	
	for(i = len - 2; i > 0; i--)
	{
		if (filePath[i] == '.' && dot < 0) dot = i;
		if (filePath[i] == DIR_SEP_CHAR && slash < 0) { slash = i; break; }
	}
	
	/* /ppppppp/ppppppp/ffffff.eeeee */
	/*                 S      D      */

	if (slash >= 0)
	{
		memcpy(path, filePath, slash);
		path[slash] = 0;
	}
	
	if (dot >= 0)
	{
		memcpy(name, filePath + slash + 1, dot - slash - 1);
		name[dot - slash - 1] = 0;
		memcpy(ext, filePath + dot + 1, len - dot);
		ext[len - dot + 1] = 0;
	} else
	{
		memcpy(name, filePath + slash + 1, len - slash);
		name[len - slash] = 0;
	}
}

void fsys::combine(char* path, cstr_t fileName)
{
	int len = strlen(path), i = 0;
	if (len > 0)
	{
		if((path[len - 1] != DIR_SEP_CHAR))
		{
			path[len] = DIR_SEP_CHAR;
			len++;
		}
	}
	while((path[len++] = fileName[i++]) != 0);
}

void fsys::combine(char* path, cstr_t name, cstr_t ext)
{
	char fileName[HW_MAX_PATH];
	sprintf(fileName, "%s.%s", name, ext);
	combine(path, fileName);
}

void fsys::parent(char* filePath)
{
	int len = strlen(filePath), lstSep = -1, fstSep = -1, i = 0;
	for(i = 0; i < len; i++)
	{
		if (filePath[i] == DIR_SEP_CHAR)
		{
			if(lstSep == -1) fstSep = i;
			if(i + 1 != len) lstSep = i;
		}
	}
	if (lstSep == fstSep) lstSep++; 
	if (lstSep >= 0) 
	{
		for(i = lstSep; i < len; i++) filePath[i] = 0;
	} else
	{
		filePath[0] = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

void fsys::setDirectories(cstr_t emulPath, cstr_t gamePath)
{
	strcpy(::emulPath, emulPath);
	strcpy(::gamePath, gamePath);
}

cstr_t fsys::getEmulPath()
{
	return emulPath;
}

cstr_t fsys::getGamePath()
{
	return gamePath;
}

cstr_t fsys::getGameFilePath()
{
	return gameFilePath;
}

void fsys::setGameFilePath(cstr_t gameFilePath)
{
	strcpy(::gameFilePath,gameFilePath);
}

cstr_t fsys::getGameRelatedFilePath(cstr_t replaceExt)
{	
	static char path[HW_MAX_PATH];
	static char name[HW_MAX_PATH];
	static char ext [HW_MAX_PATH];
	split(gameFilePath, path, name, ext);
	strcpy(path, emulPath);
	combine(path, name, replaceExt);
	return path;
}

cstr_t fsys::getEmulRelatedFilePath(cstr_t name, cstr_t ext)
{	
	static char path[HW_MAX_PATH];
	strcpy(path, emulPath);
	combine(path, name, ext);
	return path;
}

////////////////////////////////////////////////////////////////////////////////

bool fsys::exists(cstr_t filePath)
{
	FILE* fd = fopen(filePath, "rb");
	if(fd) 
	{
		fclose(fd);
		return true;	
	}
	return false;
}

bool fsys::size(cstr_t filePath, size_t* fileSize)
{
	FILE* fd = fopen(filePath, "rb");
	if(fd)
	{		
		fseek(fd, 0, SEEK_END);
		*fileSize = ftell(fd);
		fclose(fd);
		return true;
	}
	*fileSize = 0;
	return false;
}

bool fsys::load(cstr_t filePath, uint8_t* buffer, size_t bufferSize, size_t* fileSize)
{
	size_t size;
	FILE* fd = fopen(filePath, "rb");
	if(fd)
	{		
		fseek(fd, 0, SEEK_END);
		size = (size = ftell(fd)) > bufferSize ? bufferSize : size;
		fseek(fd, 0, SEEK_SET);
		fread(buffer, 1, size, fd);
		fclose(fd);
		
		*fileSize = size;
		return true;
	}
	*fileSize = 0;
	return false;
}

bool fsys::save(cstr_t filePath, uint8_t* buffer, size_t bufferSize)
{
	FILE* fd = fopen(filePath, "wb");
	if(fd)
	{
		fwrite(buffer, 1, bufferSize, fd);
		fclose(fd);
		return true;
	}
	return false;
}

bool fsys::kill(cstr_t filePath)
{
	if(exists(filePath))
	{
		remove(filePath);
		return true;
	}
	return false;
}

bool fsys::directoryItemCount(cstr_t path, int *returnItemCount)
{
	int count=1;
	DIR *d;
	struct dirent *de;
	bool exists=false;

	d = opendir((const char*)path);

	if (d)
	{
		while ((de = readdir(d)))
		{
			count++;
		}
		closedir(d);
		d=NULL;
		exists=true;
	}
	
	*returnItemCount=count;
	return exists;
}

bool fsys::directoryItemCountType(cstr_t path, int type, int *returnItemCount)
{
	int count=0;
	DIR *d;
	struct dirent *de;

	d = opendir((const char*)path);
	if (d)
	{
		while ((de = readdir(d)))
		{
			if(type==MENU_FILE_TYPE_DIRECTORY)
			{
				if (de->d_type&FSYS_DIR_FLAG) // Directory
				{
					count++;
				}		
			}
			else
			{
				if (de->d_type&FSYS_DIR_FLAG) // Directory
				{
					
				}
				else
				{
					count++;
				}
			}
		}
		closedir(d);	
	}
	d=NULL;
	
	*returnItemCount=count;
	return true;
}

bool fsys::directoryOpen(char *path, MENU_DIR *d)
{
	if(path[0] == 0)
	{
		strcpy((char*)d->path,(char*)path);
		d->needParent=0;
	}
	else
	{
		strcpy((char*)d->path,(char*)path);
#ifdef FSYS_NEED_PARENT
		d->needParent=1;
#else
		d->needParent=0;
#endif
	}
	
	d->dir=opendir((const char*)path);

	if(!d->dir) return false;
	
	return true;
}

bool fsys::directoryClose(struct MENU_DIR *d)
{
	if(d)
	{
		if(d->dir)
		{
			closedir(d->dir);
			d->dir=NULL;
			d->path[0]=0;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool fsys::directoryRead(MENU_DIR *d, MENU_DIRECTORY_ENTRY *dir)
{
	dirent *de=NULL;

	if(d)
	{
		if(d->needParent)
		{
			strcpy((char*)dir->filename,(char*)"..");
			strcpy((char*)dir->displayName,(char*)"..");
			dir->type=MENU_FILE_TYPE_DIRECTORY;
			d->needParent=0;
			return true;
		}
		else
		{
			if(dir)
			{
				de=readdir(d->dir);
				if(de)
				{
					strcpy((char*)dir->filename,(char*)de->d_name);
					strcpy((char*)dir->displayName,(char*)de->d_name);

					if (de->d_type&FSYS_DIR_FLAG) // Directory
					{
						dir->type=MENU_FILE_TYPE_DIRECTORY;
					}
					else
					{
						dir->type=MENU_FILE_TYPE_FILE;
					}
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}
}
////////////////////////////////////////////////////////////////////////////////
//	ZIP files support
////////////////////////////////////////////////////////////////////////////////

bool zip::check(cstr_t filePath)
{
    uint8_t buf[2];
    FILE* fd = (FILE*)fopen(filePath, "rb");
    if(fd) 
	{
		fread(buf, 1, 2, fd);
		fclose(fd);
		if(memcmp(buf, "PK", 2) == 0) return true;
	}
    return false;
}

void zip::fileNameInZip(cstr_t filePath, char* fileNameInZip, bool cropExtension)
{
	uint8_t nameLen = 0;
	FILE* fd = fopen(filePath, "rb");
	if(fd)
	{   
		fseek(fd, 26, SEEK_SET);
		if(fread(&nameLen, 1, 1, fd) == 1)
		{			
			fseek(fd, 30, SEEK_SET);
			fread(fileNameInZip, 1, nameLen, fd);

			if(cropExtension && fileNameInZip[nameLen - 4] == '.')
			{
				fileNameInZip[nameLen - 1] = 0;
				fileNameInZip[nameLen - 2] = 0;
				fileNameInZip[nameLen - 3] = 0;
				fileNameInZip[nameLen - 4] = 0;
			}
		}
		fclose(fd);
	}
	fileNameInZip[nameLen] = 0;
}

bool zip::fileCrcInZip(cstr_t filePath, int* fileCrcInZip)
{
	int result = true;
	FILE* fd = (FILE*)fopen(filePath, "rb");
	if(fd)
	{    
		fseek(fd, 14, SEEK_SET);
		if(fread((char*)fileCrcInZip, 1, 4, fd) != 4)
		{
			result = false;			
			*fileCrcInZip = 0;
		}
		fclose(fd);
	}
	return result;
}

bool zip::load(cstr_t filePath, uint8_t* buffer, size_t bufferSize, size_t* uncompressedSize)
{
	bool result = false;
	unz_file_info info;
	
	//	attempt to open the archive	
	unzFile fd = unzOpen(filePath);	
	if(fd) 
	{	
		//	go to first file in archive and get file info
		if(unzGoToFirstFile(fd) == UNZ_OK && unzGetCurrentFileInfo(fd, &info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
		{
			//	open first file for reading
			if(unzOpenCurrentFile(fd) == UNZ_OK && info.uncompressed_size <= bufferSize)
			{
				//	read (decompress) the file
				if((uLong)unzReadCurrentFile(fd, buffer, info.uncompressed_size) == info.uncompressed_size)
				{						
					//	update file size and return pointer to file data
					*uncompressedSize = info.uncompressed_size;
					result = true;
				}
				
				//	close the current file 
				unzCloseCurrentFile(fd);
			}
		}
		
		//	close the archive
		unzClose(fd);
	}
	return result;
}

bool zip::save(cstr_t filePath, cstr_t fileNameInZip, uint8_t* buffer, size_t fileSize)
{
	bool result = false;
	
	zipFile fd = zipOpen(filePath, 0);
	if(fd)
	{
		if(zipOpenNewFileInZip(fd, fileNameInZip, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION) == ZIP_OK)
		{
			if(zipWriteInFileInZip(fd, buffer, fileSize) == ZIP_OK)
			{
				result = true;
			}
			zipCloseFileInZip(fd);
		}
		zipClose(fd, NULL); 
	}
	return result;
}

////////////////////////////////////////////////////////////////////////////////
//	Save/Load game state support
////////////////////////////////////////////////////////////////////////////////

bool state::useMemory(MemBuffer* memBuffer)
{
	return state_use_memory(memBuffer);
}

bool state::useFile(cstr_t filePath)
{
	return state_use_file(filePath);
}

bool state::isCorrect(cstr_t filePath)
{
	return state_is_correct(filePath);
}

bool state::fopen(cstr_t mode)
{
	return state_fopen(mode);
}

int	state::fread(void* ptr, size_t count)
{
	return state_fread(ptr, count);
}

int state::fwrite(const void* ptr, size_t count)
{
	return state_fwrite(ptr, count);
}

int	state::fgetc()
{
	return state_fgetc();
}

int	state::fputc(int character)
{
	return state_fputc(character);
}

long int state::fseek(long int offset, int origin)
{
	return state_fseek(offset, origin);
}

long int state::ftell()
{
	return state_ftell();
}

int state::fclose()
{
	return state_fclose();
}

////////////////////////////////////////////////////////////////////////////////
//	Debug support
////////////////////////////////////////////////////////////////////////////////

#ifdef _COUNT_FPS

#define MAX_COUNT 300

static int fpsStat[MAX_COUNT];
static int statIndex = 0;
static int min_fps;
static int avg_fps;

#endif

int debug::add_fps_value(int fps)
{
#ifdef _COUNT_FPS
	fpsStat[statIndex++] = fps;
	if(statIndex >= MAX_COUNT)
	{
		min_fps = 100, avg_fps = 0;
		while(--statIndex >= 0)
		{
			if(fpsStat[statIndex] > 15 && fpsStat[statIndex] < min_fps) 
			min_fps  = fpsStat[statIndex];
			avg_fps += fpsStat[statIndex];
		}
		avg_fps  /= MAX_COUNT;
		statIndex = 0;
		return 100;
	} else {
		return statIndex * 100 / MAX_COUNT;
	}
#else
	return fps;
#endif
}

void debug::show_fps_statistic()
{
#ifdef _COUNT_FPS

	//	bind framebuffer for drawing
	video::init(VM_FAST);
	video::bind_framebuffer();

	//	print fps values
	char msg[40]; 
	sprintf(msg, "Average FPS: %d\nMinimal FPS: %d", avg_fps, min_fps);
	video::print(0, 0, msg, COLOR_GREEN);

	//	draw graphics
	int x0 = (320 - MAX_COUNT) / 2;
	video::draw_line(x0, 120 - 30 * 2, x0 + MAX_COUNT - 1, 120 - 30 * 2, COLOR_RED);
	video::draw_line(x0, 120,          x0 + MAX_COUNT - 1, 120,          COLOR_RED);
	video::draw_line(x0, 120 + 30 * 2, x0 + MAX_COUNT - 1, 120 + 30 * 2, COLOR_RED);
	int y0 = 120 + (30 - fpsStat[0]) * 2, y1;
	for(int i = 1; i < MAX_COUNT; i++)
	{
		y1 = 120 + (30 - fpsStat[i]) * 2;
		video::draw_line(x0 + i - 1, y0, x0 + i, y1, COLOR_YELLOW);
		y0 = y1;
	}
	y0 = y1 = 120 + (30 - avg_fps) * 2;
	video::draw_line(x0, y0, x0 + MAX_COUNT - 1, y1, COLOR_TEAL);

	//	flip and release framebuffer
	video::term_framebuffer();
	video::flip(0);
	
	//	wait input and restore video mode
	input::wait_for_press();
	input::wait_for_release();
	video::prev_mode();
#endif
}

#ifdef _DEBUG
#include "debug.font.inc"

void debug::out(cstr_t msg)
{
	//	on first call init system and load debug font
	static gfx_font* dbg_font;
	if (!dbg_font) {
		system::init();
		dbg_font = video::font_load(debug_font, debug_font_size, COLOR_BLACK);
	}

	//	convert message text to multiline format
	char txt[1024];
	getMultilineText(msg, txt, MAX_MENU_ITEM_CHARS);

	//	set debug font and print message
	gfx_font* old_font = video::font_get();
	video::font_set(dbg_font);
	video::init(VM_FAST);
	video::bind_framebuffer();
	video::print(0, 0, txt, COLOR_WHITE);
	video::term_framebuffer();
	video::flip(0);
	
	//	wait for any key press
	input::wait_for_press();
	input::wait_for_release();
	
	//	restore video mode and previous font
	video::prev_mode();
	video::font_set(old_font);
}
#else
void debug::out(const char *) {}
#endif

////////////////////////////////////////////////////////////////////////////////

int entry_point(int argc, char *argv[])
{
	return fw::emul::entry_point(argc, argv);
}

bool get_screenshot_filename(char *filePath)
{
	return fw::emul::get_screenshot_filename(filePath);
}

char logItText[256];
#if defined(_GPI)
void debug::printf (char *fmt, ...) 
{

}
#else
void debug::printf (char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#endif
