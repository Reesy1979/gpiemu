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
 
#include <unistd.h>
#include "hardware.h"

extern int entry_point(int argc, char *argv[]);

int main(int argc, char *argv[])
{	
	if(argc < 2) return 1;
	char cwd[HW_MAX_PATH];
    getcwd(cwd, HW_MAX_PATH - 1);
	char* args[2] = { cwd, argv[1] };
	return ::entry_point(2, args);
}
