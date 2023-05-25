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
 
#ifndef _SYSTYPE_
#define _SYSTYPE_

typedef enum {
	SYS_UNKNOWN,
	SYS_SMS,
	SYS_GG,
	SYS_COLECO
} Sys_t;

static Sys_t getSystemType()
{
	static Sys_t sysType = SYS_UNKNOWN;
	if(sysType == SYS_UNKNOWN)
	{
		cstr_t ext = strrchr(fw::fsys::getGameFilePath(), '.');
		if(!strcmp(ext, ".gg") || !strcmp(ext, ".GG")) {
	        sysType = SYS_GG;
	    } else
		if(!strcmp(ext, ".rom") || !strcmp(ext, ".ROM") || !strcmp(ext, ".col") || !strcmp(ext, ".COL")) {
	    	sysType = SYS_COLECO;
	    } else {
	    	sysType = SYS_SMS;
	    }
	}
	return sysType;
}

static bool isSMS()    { return getSystemType() == SYS_SMS;    }
static bool isGG()     { return getSystemType() == SYS_GG;     }
static bool isColeco() { return getSystemType() == SYS_COLECO; }

#endif //_SYSTYPE_
