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

#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "hardware.h"
#include "state.io.h"

////////////////////////////////////////////////////////////////////////////////
//	DEP v1.x color theme (magenta)
////////////////////////////////////////////////////////////////////////////////
// static const gfx_color COLOR_TRANSPARENT 		= gfx_color_rgb(0xFF, 0x00, 0xFF);
// static const gfx_color COLOR_HEADER 				= gfx_color_rgb(0x50, 0x50, 0x50);
// static const gfx_color COLOR_FOOTER 				= gfx_color_rgb(0x50, 0x50, 0x50);
// static const gfx_color COLOR_HF_LINE 			= gfx_color_rgb(0xFF, 0x00, 0x00);
// static const gfx_color COLOR_HEADER_TXT 			= gfx_color_rgb(0xFF, 0xFF, 0xFF);
// static const gfx_color COLOR_SELECTOR_EDGING		= gfx_color_rgb(0x48, 0x00, 0xFF);
// static const gfx_color COLOR_SELECTOR_INSIDE		= gfx_color_rgb(0x48, 0x00, 0xFF);
// static const gfx_color COLOR_MENU_TXT_ENABLE		= gfx_color_rgb(0xFF, 0xFF, 0xFF);
// static const gfx_color COLOR_MENU_TXT_DISABLE	= gfx_color_rgb(0x60, 0x60, 0x60);
// static const gfx_color COLOR_STATESHOT_EDGING	= gfx_color_rgb(0x48, 0x00, 0xFF);
// static const gfx_color COLOR_STATESHOT_INSIDE	= gfx_color_rgb(0x48, 0x00, 0xFF);
// static const gfx_color COLOR_STATESHOT_TXT		= gfx_color_rgb(0xFF, 0xFF, 0xFF);
// static const gfx_color COLOR_MSGBOX_TXT			= gfx_color_rgb(0x00, 0xFF, 0xFF);
// static const gfx_color COLOR_ROM_UNZIPPED_TXT	= gfx_color_rgb(0xFF, 0xFF, 0x00);
// static const gfx_color COLOR_ROM_ZIPPED_TXT		= gfx_color_rgb(0x00, 0xFF, 0x00);
// static const bool		HALF_TRANSP_HEADER		= true;
// static const bool		HALF_TRANSP_FOOTER		= true;
// static const bool		HALF_TRANSP_SELECTOR	= true;
// static const bool		HALF_TRANSP_STATESHOT	= true;

////////////////////////////////////////////////////////////////////////////////
//	DEP v2.x color theme (orange)
////////////////////////////////////////////////////////////////////////////////
static const gfx_color 	COLOR_TRANSPARENT 		= gfx_color_rgb(0xFF, 0x00, 0xFF);
static const gfx_color 	COLOR_HEADER 			= gfx_color_rgb(0x00, 0x00, 0x00);
static const gfx_color 	COLOR_FOOTER 			= gfx_color_rgb(0x00, 0x00, 0x00);
static const gfx_color 	COLOR_HF_LINE 			= gfx_color_rgb(0xE7, 0x65, 0x0D);
static const gfx_color 	COLOR_HEADER_TXT 		= gfx_color_rgb(0xFF, 0xFF, 0xFF);
static const gfx_color 	COLOR_SELECTOR_EDGING	= gfx_color_rgb(0xE7, 0x65, 0x0D);
static const gfx_color 	COLOR_SELECTOR_INSIDE	= gfx_color_rgb(0xE7, 0x65, 0x0D);
static const gfx_color 	COLOR_MENU_TXT_ENABLE	= gfx_color_rgb(0xFF, 0xFF, 0xFF);
static const gfx_color 	COLOR_MENU_TXT_DISABLE	= gfx_color_rgb(0x60, 0x60, 0x60);
static const gfx_color 	COLOR_STATESHOT_EDGING	= gfx_color_rgb(0x30, 0x30, 0x30);
static const gfx_color 	COLOR_STATESHOT_INSIDE	= gfx_color_rgb(0x30, 0x30, 0x30);
static const gfx_color 	COLOR_STATESHOT_TXT		= gfx_color_rgb(0xE7, 0x65, 0x0D);
static const gfx_color 	COLOR_MSGBOX_TXT		= gfx_color_rgb(0x60, 0x60, 0x60);
static const gfx_color 	COLOR_ROM_UNZIPPED_TXT	= gfx_color_rgb(0x60, 0x60, 0x60);
static const gfx_color 	COLOR_ROM_ZIPPED_TXT	= gfx_color_rgb(0xE7, 0x65, 0x0D);
static const bool		HALF_TRANSP_HEADER		= true;
static const bool		HALF_TRANSP_FOOTER		= true;
static const bool		HALF_TRANSP_SELECTOR	= false;
static const bool		HALF_TRANSP_STATESHOT	= false;

////////////////////////////////////////////////////////////////////////////////

#define DIR_SEP				"/"
#define DIR_SEP_CHAR		(char)DIR_SEP[0]

#define SRAM_FILE_EXT		"srm"
#define SAVESTATE_EXT		"sv"

#define CPU_SPEED_IN_MENU	336
#define SCREENSHOT_WIDTH	150
#define SCREENSHOT_HEIGHT	112
#define COVER_MIN_SIZE		136
#define COVER_MAX_SIZE		200
#define MAX_MENU_ITEM_COUNT 50
#define MAX_MENU_ITEM_CHARS	38

#define MENU_TITLE_TEXT		EMU_DISPLAY_NAME " v" EMU_VERSION
#define STR_ON				"ON"
#define STR_OFF				"OFF"

namespace fw
{

enum EVENT {
	EVENT_NONE,
	EVENT_LOAD_ROM,
	EVENT_RESET_ROM,
	EVENT_RUN_ROM,
	EVENT_EXIT_APP,
};

enum MENU_MSG_BOX
{
	MMB_MESSAGE,
	MMB_PAUSE,
	MMB_YESNO,
};

enum MENU_ACTION
{
	MA_NONE = 0,
	MA_DECREASE,
	MA_INCREASE,
	MA_SELECT,
	MA_CANCEL
};

enum MENU_TEXTURE
{
	MT_LOGO,
	MT_GAMEPAD,
	MT_COVER
};

enum  MENU_FILE_TYPE_ENUM
{
	MENU_FILE_TYPE_FILE = 0,
	MENU_FILE_TYPE_DIRECTORY
};

struct MENU_DIR
{
	DIR *dir;
	int needParent;
	char *path[HW_MAX_PATH];
	int drivesRead;
};

struct MENU_DIRECTORY_ENTRY
{
	char filename[HW_MAX_PATH];
	char displayName[HW_MAX_PATH];
	int type;
};

struct MENU_DIRECTORY_LIST
{
	MENU_DIRECTORY_ENTRY **fileList;
	int count;
};

#define USE_SETS_TYPEDEF
#include "sets.h"
#undef USE_SETS_TYPEDEF
extern Sets_t settings;

////////////////////////////////////////////////////////////////////////////////
//	Low-level framework API
////////////////////////////////////////////////////////////////////////////////

namespace menu
{
	void 	 init(cstr_t romName, gfx_color romNameColor);
	void 	 setup_bg_texture(gfx_texture* texture);
	void	 setup_bg_texture(MENU_TEXTURE index, gfx_texture* texture, int x, int y);
	void 	 setup_bg_scrolling();
	void 	 select_bg_texture(bool logo);

	void	 set_title(cstr_t title);
	void 	 set_item_text(int index, cstr_t text);
	void 	 set_separator(int index);
	void 	 set_item_optstr(int index, cstr_t text, cstr_t optStr);
	void 	 set_item_optint(int index, cstr_t text, int optInt);
	void 	 set_item_enable(int index, bool enable);

	int 	 next_val(int* values, int count, int current);
	int 	 prev_val(int* values, int count, int current);

	void 	 open();
MENU_ACTION  update(int count, int& focus);
MENU_ACTION  rom_list_update(int count, int& focus);
	void 	 close();

	void 	 render_tiled_bg();
	void 	 render_text(int x, int y, cstr_t text, gfx_color color);
	void 	 render_rect(int x, int y, int w, int h, gfx_color color);
	void 	 render_bg();
	void     render_title();
	void     render_footer();
	void 	 render_menu(int count, int focus);
	void     render_file_list(MENU_DIRECTORY_LIST *list, int focus, bool render_background);
	void     render_file_list(MENU_DIRECTORY_LIST *list, int focus);
	void 	 render_stateshot(int x, int y, int w, int h, int mode, int slot, gfx_color* stateCurrent, gfx_color* stateSaved);
	
	bool 	 msg_box(cstr_t message, MENU_MSG_BOX mode);
	void     directory_entry_swap(struct MENU_DIRECTORY_ENTRY *from, struct MENU_DIRECTORY_ENTRY *to);
	int      string_compare(char *string1, char *string2);
	MENU_DIRECTORY_LIST *get_file_list(char *path, char **ext);
	MENU_DIRECTORY_LIST *get_directory_list(char *path);
}



namespace fsys
{
	void 	 split(cstr_t filePath, char* path, char* name, char* ext);
	void 	 combine(char* path, cstr_t fileName);
	void 	 combine(char* path, cstr_t name, cstr_t ext);
	void 	 parent(char* filePath);

	void 	 setDirectories(cstr_t emulPath, cstr_t gamePath);
	cstr_t 	 getEmulPath();
	cstr_t 	 getGamePath();
	cstr_t   getGameFilePath();
	void     setGameFilePath(cstr_t gameFilePath);
	cstr_t   getGameRelatedFilePath(cstr_t replaceExt);
	cstr_t 	 getEmulRelatedFilePath(cstr_t name, cstr_t ext);

	bool 	 exists(cstr_t filePath);
	bool 	 size(cstr_t filePath, size_t* fileSize);
	bool 	 load(cstr_t filePath, uint8_t* buffer, size_t bufferSize, size_t* fileSize);
	bool 	 save(cstr_t filePath, uint8_t* buffer, size_t bufferSize);
	bool 	 kill(cstr_t filePath);
	
	bool     directoryItemCount(cstr_t path, int *returnItemCount);
	bool     directoryItemCountType(cstr_t path, int type, int *returnItemCount);
	bool     directoryOpen(char *path, MENU_DIR *d);
	bool     directoryClose(MENU_DIR *d);
	bool     directoryRead(MENU_DIR *d, MENU_DIRECTORY_ENTRY *dir);
};

namespace zip
{
	bool 	 check(cstr_t filePath);
	void 	 fileNameInZip(cstr_t filePath, char* fileNameInZip, bool cropExtension);
	bool 	 fileCrcInZip(cstr_t filePath, int* fileCrcInZip);
	bool 	 load(cstr_t filePath, uint8_t* buffer, size_t bufferSize, size_t* uncompressedSize);
	bool 	 save(cstr_t filePath, cstr_t fileNameInZip, uint8_t* buffer, size_t fileSize);
};

namespace state
{
	bool 	 useMemory(MemBuffer* memBuffer);
	bool 	 useFile(cstr_t filePath);
	bool 	 isCorrect(cstr_t filePath);
	
	bool 	 fopen(cstr_t mode);
	int		 fread(void* ptr, size_t count);
	int 	 fwrite(const void* ptr, size_t count);
	int		 fgetc();
	int		 fputc(int character);
	long int fseek(long int offset, int origin);
	long int ftell();
	int 	 fclose();
};

namespace debug
{
	int 	 add_fps_value(int fps);
	void 	 show_fps_statistic();
	void 	 out(cstr_t msg);
	void     printf(char *fmt, ...);
};

////////////////////////////////////////////////////////////////////////////////
//	Basic high-level emulator implementation
////////////////////////////////////////////////////////////////////////////////

namespace emul
{
	int 	 entry_point(int argc, char *argv[]);
	bool 	 get_screenshot_filename(char *filename);
	uint32_t handle_input();

	void     menu_init();
	void 	 menu_init_game();
gfx_font*    menu_load_font();
gfx_texture* menu_load_tile();
gfx_texture* menu_load_logo();
gfx_texture* menu_load_gamepad();
	void 	 menu_lock_sets();
	EVENT    menu_main();
	void 	 menu_extra();
	void	 menu_advanced();
	void 	 menu_close();

	void	 sets_default();
	void	 sets_load();
	bool	 sets_save(bool romSpecific);
};

////////////////////////////////////////////////////////////////////////////////
//	Emulation core (unique implementation for each emulator)
////////////////////////////////////////////////////////////////////////////////

namespace core
{
	bool 	 init();
	void 	 close();
	bool 	 load_rom();
	void 	 hard_reset();
	void 	 soft_reset();

	uint32_t system_fps();
	void 	 start_emulation(bool withSound);
	void 	 emulate_frame(bool& withRendering, int soundBuffer);
	void 	 emulate_for_screenshot(int& console_scr_w, int& console_scr_h);
	void 	 stop_emulation(bool withSound);
	bool 	 save_load_state(cstr_t filePath, MemBuffer* memBuffer, bool load);

#ifndef DISABLE_CHEATING
	int 	 get_cheat_count();
	cstr_t	 get_cheat_name(int cheatIndex);
	bool	 get_cheat_active(int cheatIndex);
	void 	 set_cheat_active(int cheatIndex, bool enable);
#endif
};

};

#endif //_FRAMEWORK_H_
