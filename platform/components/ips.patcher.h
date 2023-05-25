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
 
#ifndef DISABLE_IPS_PATCHING

#ifndef _IPS_PATCHER_
#define _IPS_PATCHER_

#include <stdint.h>
#include <stdio.h>
#include "cppdefs.h"

START_EXTERN_C

void   ips_define (const char* ipsname);
FILE*  ips_fopen  (const char* filename, const char* mode);
size_t ips_fread  (void* ptr, size_t size, size_t count, FILE* stream);
int    ips_fgetc  (FILE* stream);
int    ips_fseek  (FILE* stream, long int offset, int origin);
int    ips_fclose (FILE* stream);

void   ips_onopen (void* stream, const char* mode);
void   ips_onread (void* ptr, uint32_t position, size_t count, void* stream);
int    ips_ongetc (int character, uint32_t position, void* stream);
void   ips_onseek (void* stream);
void   ips_onclose(void* stream);

END_EXTERN_C

#endif //_IPS_PATCHER_

#endif //DISABLE_IPS_PATCHING
