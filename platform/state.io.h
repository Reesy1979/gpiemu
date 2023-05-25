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
 
#ifndef _STATE_IO_H_
#define _STATE_IO_H_

#include <stdint.h>
#include <stddef.h>
#include "cppdefs.h"
#include "memstream.h"

typedef const char* cstr_t;

START_EXTERN_C

bool     state_use_memory(MemBuffer* memBuffer);
bool     state_use_file  (cstr_t filePath);
bool 	 state_is_correct(cstr_t filePath);

bool     state_fopen (cstr_t mode);
int	     state_fread (void* ptr, size_t count);
int      state_fwrite(const void* ptr, size_t count);
int	     state_fgetc ();
int	     state_fputc (int ch);
long int state_fseek (long int offset, int origin);
long int state_ftell ();
int      state_fclose();

END_EXTERN_C

#endif //_STATE_IO_H_
