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
 
#include <stdlib.h>
#include <string.h>

#define CURRENT_WORKING_DIRECTORY "A:\\GAME"

extern int entry_point(int argc, char *argv[]);

/* file extension name */
extern "C" int GetFileType(char* pname) {
	if(pname != NULL)
	{
		//	Emulator ROM extensions (use "EXT|EXT|EXT" for several file-type associations,
		//	not more than five)
		strcpy(pname, EMU_FILE_TYPES);
	}
	return 0;
}

/* to get default path */
extern "C" int GetDefaultPath(char* path) {
	if(path != NULL) {
		strcpy(path, CURRENT_WORKING_DIRECTORY);
	}
	return 0;
}

/* module description, optional */
extern "C" int GetModuleName(char* name, int code_page) {
	if((name != NULL) && (code_page == 0))
	{
		//	Your emulator file name
		strcpy(name, EMU_FILE_NAME ".sim");
	}
	return 0;
}

/* main entry point */
extern "C" int main(int, char *argv[])
{	
	char* args[2] = { CURRENT_WORKING_DIRECTORY, argv[0] };
	return ::entry_point(2, args);
}
