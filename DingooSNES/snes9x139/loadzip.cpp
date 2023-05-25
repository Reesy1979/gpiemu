/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
/**********************************************************************************************/
/* Loadzip.CPP                                                                                */
/* This file contains a function for loading a SNES ROM image from a zip file		      */
/**********************************************************************************************/

#include <string.h>
#include <ctype.h>

#ifndef NO_INLINE_SET_GET
#define NO_INLINE_SET_GET
#endif

#include "snes9x.h"
#include "memmap.h"

#include "unzip.h"

#ifdef _DINGOO
#define _SNESPPC
#endif

#ifndef _SNESPPC
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#ifdef _DINGOO
#include "ips.patcher.h"
#endif

bool8_32 LoadZip(const char* zipname,
	      int32 *TotalFileSize,
	      int32 *headers)
{
	*TotalFileSize = 0;
    *headers = 0;
    
    unzFile file = unzOpen(zipname);
    if(file == NULL)
	return (FALSE);

    // find largest file in zip file (under MAX_ROM_SIZE)
    // or a file with extension .1
    char filename[132];
    int filesize = 0;
    int port = unzGoToFirstFile(file);
    unz_file_info info;
    while(port == UNZ_OK)
    {
	char name[132];
	unzGetCurrentFileInfo(file, &info, name,128, NULL,0, NULL,0);

#if 0
	int calc_size = info.uncompressed_size / 0x2000;
	calc_size *= 0x2000;
	if(!(info.uncompressed_size - calc_size == 512 || info.uncompressed_size == calc_size))
	{
	    port = unzGoToNextFile(file);
	    continue;
	}
#endif

	if(info.uncompressed_size > (CMemory::MAX_ROM_SIZE + 512))
	{
	    port = unzGoToNextFile(file);
	    continue;
	}
	
	if ((int) info.uncompressed_size > filesize)
	{
	    strcpy(filename,name);
	    filesize = info.uncompressed_size;
	}
	int len = strlen(name);
	if(name[len-2] == '.' && name[len-1] == '1')
	{
	    strcpy(filename,name);
	    filesize = info.uncompressed_size;
	    break;
	}
	port = unzGoToNextFile(file);
    }
    if( !(port == UNZ_END_OF_LIST_OF_FILE || port == UNZ_OK) || filesize == 0)
    {
#ifdef _SNESPPC
	unzClose(file);
#else
	assert( unzClose(file) == UNZ_OK );
#endif
	return (FALSE);
    }

    // Find extension
    char tmp[2];
    tmp[0] = tmp[1] = 0;
    char *ext = strrchr(filename,'.');
    if(ext) ext++;
    else ext = tmp;
    
    uint8 *ptr = Memory.ROM;
    bool8 more = FALSE;

    unzLocateFile(file,filename,1);
    unzGetCurrentFileInfo(file, &info, filename,128, NULL,0, NULL,0);
    
    if( unzOpenCurrentFile(file) != UNZ_OK )
    {
	unzClose(file);
	return (FALSE);
    }

    do
    {
#ifndef _SNESPPC
	assert(info.uncompressed_size <= CMemory::MAX_ROM_SIZE + 512);
#endif
	int FileSize = info.uncompressed_size;
	
	int calc_size = FileSize / 0x2000;
	calc_size *= 0x2000;
	
#ifdef _DINGOO
	ips_onopen(file, "r");
	z_off_t pos = unztell(file);
	int l = unzReadCurrentFile(file, ptr, FileSize);
	ips_onread(ptr, pos, FileSize, file);
	ips_onclose(file);
#else
	int l = unzReadCurrentFile(file,ptr,FileSize);
#endif

	if(unzCloseCurrentFile(file) == UNZ_CRCERROR)
	{
	    unzClose(file);
	    return (FALSE);
	}
	
	if(l <= 0 || l != FileSize)
	{
	    unzClose(file);
	    switch(l)
	    {
		case UNZ_ERRNO:
		    break;
		case UNZ_EOF:
		    break;
		case UNZ_PARAMERROR:
		    break;
		case UNZ_BADZIPFILE:
		    break;
		case UNZ_INTERNALERROR:
		    break;
		case UNZ_CRCERROR:
		    break;
	    }
	    return (FALSE);
	}

	if ((FileSize - calc_size == 512 && !Settings.ForceNoHeader) ||
	    Settings.ForceHeader)
	{
	    memmove (ptr, ptr + 512, calc_size);
	    (*headers)++;
	    FileSize -= 512;
	}
	ptr += FileSize;
	(*TotalFileSize) += FileSize;

	int len;
	if (ptr - Memory.ROM < CMemory::MAX_ROM_SIZE + 0x200 &&
	    (isdigit (ext [0]) && ext [1] == 0 && ext [0] < '9'))
	{
	    more = TRUE;
	    ext [0]++;
	}
	else if (ptr - Memory.ROM < CMemory::MAX_ROM_SIZE + 0x200 &&
		 (((len = strlen (filename)) == 7 || len == 8) &&
		  strncasecmp (filename, "sf", 2) == 0 &&
		  isdigit (filename [2]) && isdigit (filename [3]) && isdigit (filename [4]) &&
		  isdigit (filename [5]) && isalpha (filename [len - 1])))
	{
	    more = TRUE;
	    filename [len - 1]++;
	}
	else
	    more = FALSE;
	
	if(more)
	{
	    if( unzLocateFile(file,filename,1) != UNZ_OK ||
		unzGetCurrentFileInfo(file, &info, filename,128, NULL,0, NULL,0) != UNZ_OK ||
		unzOpenCurrentFile(file) != UNZ_OK)
		break;
	}
	
    } while(more);
    
    unzClose(file);
    return (TRUE);
}

#ifdef _DINGOO
#undef _SNESPPC
#endif
