/* DingooSMS - Sega Master System, Sega Game Gear, ColecoVision Emulator for Dingoo A320
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
 
#include "png_logo.h"
#include "png_gamepad.h"
#include "framework.h"
#include "launcher.h"

using namespace fw;
using namespace hw;

#define MENU_FONT_FILENAME   "font.ttf"
#define MENU_IMG_LOGO_FILENAME  "logo.png"
#define MENU_IMG_GAMEPAD_FILENAME  "gamepad.png"
#define MENU_IMG_TILE_FILENAME  "tile.png"

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

gfx_font* emul::menu_load_font()
{
	return NULL;
}

gfx_texture* emul::menu_load_logo()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_LOGO_FILENAME);
	return video::tex_load(path);
}

gfx_texture* emul::menu_load_gamepad()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_GAMEPAD_FILENAME);
	return video::tex_load(path);
}

gfx_texture* emul::menu_load_tile()
{
	char path[HW_MAX_PATH];
	strcpy(path, fsys::getEmulPath());
	fsys::combine(path, MENU_IMG_TILE_FILENAME);
	return video::tex_load(path);
}

void emul::menu_lock_sets()
{
	//
}

void launcher::menu_wallpaper()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	char *fileext[] = {"png",""};
	char *wallpapersPath;
	char errorMsg[100];
	gfx_texture *wallpaper=NULL;
	menu::open();
	menu::set_title("Select your wallpaper");
	wallpapersPath = launcher::GetWallpapersPath();
	MENU_DIRECTORY_LIST *list = menu::get_file_list((char*)wallpapersPath, (char**)&fileext);
	if(list==NULL)
	{
		sprintf(errorMsg, "Failed to open wallpaper directory\n%s", wallpapersPath);
		menu::msg_box(errorMsg, MMB_PAUSE);
		return;
	}

	if(list->count==0)
	{
		sprintf(errorMsg, "No wallpapers installed in\n%s", wallpapersPath);
		menu::msg_box(errorMsg, MMB_PAUSE);
		return;
	}

	char path[HW_MAX_PATH];
	strcpy(path, wallpapersPath);
	fsys::combine(path, list->fileList[0]->filename);
	debug::printf((char*)"Wallpaper:%s\n",path);
	wallpaper = video::tex_load(path);

	while (!menuExit)
	{
		video::bind_framebuffer();
		video::draw_tex_transp(0, 0, wallpaper);
		menu::render_file_list(list, menufocus, false);
		video::term_framebuffer();
		video::flip(0);
		int lastmenufocus=menufocus;
		switch(menu::rom_list_update(list->count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;

			case MA_DECREASE:
				
				break;
				
			case MA_INCREASE:

				break;
				
			case MA_SELECT:
				launcher::SetWallpaper(list->fileList[menufocus]->filename);
				menuExit = 1;
				break;

			case MA_NONE:
				if(menufocus!=lastmenufocus)
				{
					lastmenufocus=menufocus;
					if(wallpaper!=NULL) free(wallpaper);
					wallpaper=NULL;
					strcpy(path, wallpapersPath);
					fsys::combine(path, list->fileList[menufocus]->filename);
					debug::printf((char*)"Wallpaper:%s\n",path);
					wallpaper = video::tex_load(path);
				}
				
				break;
		}
	}
	if(wallpaper!=NULL) free(wallpaper);
	wallpaper=NULL;
	if(list!=NULL) free(list);
	list=NULL;
	menu::close();
}

void launcher::menu_skin()
{
	int menuExit 	= 0;
	int menufocus 	= 0;
	char *skinsPath;
	char errorMsg[100];
	menu::open();
	menu::set_title("Select your skin");
	skinsPath = launcher::GetSkinsPath();
	MENU_DIRECTORY_LIST *list = menu::get_directory_list((char*)skinsPath);
	if(list==NULL)
	{
		sprintf(errorMsg, "Failed to open skin directory\n%s", skinsPath);
		menu::msg_box(errorMsg, MMB_PAUSE);
		return;
	}

	if(list->count==0)
	{
		sprintf(errorMsg, "No Skins installed in\n%s", skinsPath);
		menu::msg_box(errorMsg, MMB_PAUSE);
		return;
	}

	while (!menuExit)
	{
		video::bind_framebuffer();
		menu::render_file_list(list, menufocus);
		video::term_framebuffer();
		video::flip(0);
		switch(menu::rom_list_update(list->count, menufocus))
		{
			case MA_CANCEL:
				menuExit = 1;
				break;

			case MA_DECREASE:
				
				break;
				
			case MA_INCREASE:

				break;
				
			case MA_SELECT:
				launcher::SetSkin(list->fileList[menufocus]->filename);
				menuExit = 1;
				break;

			case MA_NONE:
				
				break;
		}
	}
	if(list!=NULL) free(list);
	list=NULL;
	menu::close();
}

void emul::menu_advanced()
{

}
