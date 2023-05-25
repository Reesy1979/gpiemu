/* DingooSNES - Super Nintendo Entertainment System Emulator for Dingoo A320
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
 
#ifndef _PORT_H_
#define _PORT_H_

//	Includes //////////////////////////////////////////////////////////////////
#include <string.h>
#include <time.h>
#include "pixform.h"

//	Types Defined  ////////////////////////////////////////////////////////////
#ifndef snes9x_types_defined
#define snes9x_types_defined
typedef unsigned char	bool8;
typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef signed char		int8;
typedef short			int16;
typedef int				int32;
typedef long long		int64;
#define bool8_32 		bool8
#define uint8_32 		uint8
#define uint16_32		uint16
#define int8_32			int8
#define int16_32		int16
#define TRUE			true
#define FALSE 			false
#endif

#ifndef ACCEPT_SIZE_T
#define ACCEPT_SIZE_T unsigned int
#endif

//	Compiling  ////////////////////////////////////////////////////////////////
#include "cppdefs.h"

#define INLINE inline
#define STATIC static
#define VOID void
#define FASTCALL

//	Functions  ////////////////////////////////////////////////////////////////
#define strcasecmp strcmp
#define strncasecmp strncmp
#define ZeroMemory(a,b) memset((a),0,(b))
EXTERN_C void S9xGenerateSound ();

//#ifndef _makepath
EXTERN_C void _makepath (char *path, const char *drive, const char *dir, const char *fname, const char *ext);
//#endif
//#ifndef _splitpath
EXTERN_C void _splitpath (const char *path, char *drive, char *dir, char *fname, char *ext);
//#endif

//	Filesystem  ///////////////////////////////////////////////////////////////
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define _MAX_DIR	PATH_MAX
#define _MAX_DRIVE	1
#define _MAX_FNAME	PATH_MAX
#define _MAX_EXT	PATH_MAX
#define _MAX_PATH	PATH_MAX

//	Configuration  ////////////////////////////////////////////////////////////
#define PIXEL_FORMAT RGB565
#define CHECK_SOUND()

#define VAR_CYCLES
#define CPU_SHUTDOWN
#define SPC700_SHUTDOWN
#define SPC700_C
#define UNZIP_SUPPORT
#define LSB_FIRST
#define RIGHTSHIFT_IS_SAR
#define SDD1_DECOMP

//	Optimization  /////////////////////////////////////////////////////////////
#define OPT_Mode7Interpolate
#define OPT_SupportHiRes
#define OPT_AutoSaveDelay
#define OPT_InfoString

//	Undefs  ///////////////////////////////////////////////////////////////////
#undef DEBUGGER
#undef GFX_MULTI_FORMAT
#undef FAST_LSB_WORD_ACCESS
#undef NO_INLINE_SET_GET
#undef SA1_OPCODES
#undef USE_OPENGL
#undef ZLIB

#endif
