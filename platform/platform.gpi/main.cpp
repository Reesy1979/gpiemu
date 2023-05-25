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
#include "kernel.h"
#include "framework.h"
#include <circle/chainboot.h>

extern int entry_point(int argc, char *argv[]);
extern int KernelInit();
extern int KernelReset();
extern void KernelVideoFlip();
extern void KernelLog(char *msg);

int main()
{	
	KernelInit();

	char cwd[HW_MAX_PATH];
    	strcpy(cwd,"/");
	char* args[2] = { EMU_SYS_PATH, EMU_ROM_PATH };

	::entry_point(2, args);
	
	u32 filesize=0;
	// Get filesize
	fw::fsys::size("kernel.img", &filesize);
	// Malloc memory
	s8 *execBuffer=(s8*)malloc(filesize+31);
	// Align memory to 32bit and created pointer
	u8 *execAddr=(u8 *)(((uintptr) execBuffer + 31) & ~31);
	
#if !defined(_LAUNCHER)	
	//load file into the memory
	if(fw::fsys::load("kernel.img", execAddr, filesize, &filesize))
	{
		EnableChainBoot((void*)execAddr,filesize);
	}
#endif
	KernelReset();
	return 1;
}
