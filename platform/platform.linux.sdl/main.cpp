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
#include "stdio.h"
#include "framework.h"
#include "hardware.h"

extern int entry_point(int argc, char *argv[]);

int main(int argc, char *argv[])
{	
	char* args[2] = { EMU_SYS_PATH, EMU_ROM_PATH };
	fw::debug::printf("here:%s\n", EMU_SYS_PATH);
	return ::entry_point(2, args);
}
