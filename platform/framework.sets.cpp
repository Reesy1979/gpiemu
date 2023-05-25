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

#include "framework.h"

using namespace fw;

#define SETTINGS_FILENAME	EMU_FILE_NAME
#define SETTINGS_EXT		"cfg"
#define VERSION             (uint16_t)(EMU_VERSION[0] | EMU_VERSION[2] << 8)

Sets_t fw::settings;

///////////////////////////////////////////////////////////////////////////////

static bool _sets_load(bool romSpecific)
{
	size_t size = 0;
	cstr_t filePath;

	if(romSpecific) {
		filePath = fsys::getGameRelatedFilePath(SETTINGS_EXT);
	} else {
		filePath = fsys::getEmulRelatedFilePath(SETTINGS_FILENAME, SETTINGS_EXT);
	}

	if(!fsys::load(filePath, (uint8_t *)&settings, sizeof(Sets_t), &size)) return false;
	if(settings.version != VERSION) return false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////

void emul::sets_default()
{
	settings.version = VERSION;

#define USE_SETS_DEFAULTS
#include "sets.h"
#undef  USE_SETS_DEFAULTS

}

void emul::sets_load()
{
	//	Try to load settings for game
	if(_sets_load(true)) return;

	//	Try to load global settings
	if(_sets_load(false)) return;

	//	Set default options
	sets_default();
}

bool emul::sets_save(bool romSpecific)
{
	cstr_t filePath;
	if(romSpecific) {
		filePath = fsys::getGameRelatedFilePath(SETTINGS_EXT);
	} else {
		filePath = fsys::getEmulRelatedFilePath(SETTINGS_FILENAME, SETTINGS_EXT);
	}
	
	menu::msg_box("Save settings...", MMB_MESSAGE);
	return fsys::save(filePath, (uint8_t *)&settings, sizeof(Sets_t));
}
