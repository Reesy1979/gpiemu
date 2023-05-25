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
 
#ifndef _MEMSTREAM_H_
#define _MEMSTREAM_H_

#include <stdint.h>
#include <stddef.h>
#include <cstdlib>

class MemBuffer
{
	uint8_t* buffAddr;	//	pointer to buffer in memory
	size_t   buffSize;	//	allocated buffer size, may be larger than numBytes
	size_t   numBytes;	//	actual valid bytes in buffer
	friend class MemStream;

public:
	MemBuffer() : buffAddr(NULL), buffSize(0), numBytes(0) {}
	~MemBuffer() {  }

	uint8_t* getBuff() { return buffAddr; }
	size_t   getSize() { return numBytes; }
	void     free()    { ::free(buffAddr); buffAddr = NULL; buffSize = numBytes = 0; }
};

class MemStream
{
	static const size_t DEFAULT_BUFF_SIZE = 8192;

	MemBuffer* memBuffer;	//	used for reading/writting any MemBuffer objects
	MemBuffer* memInternal;	//	used for reading/writting memory chunks with fixed size
	uintptr_t  buffOffset;	//	offset in working MemBuffer
	bool       isDynamic;	//	working MemBuffer uses internal dynamic memory allocation

public:
	MemStream();
	~MemStream();

	bool 	  open   (MemBuffer* memBuffer = NULL);
	bool 	  open   (uint8_t* addr, size_t size);
	size_t 	  write  (const uint8_t* src, size_t count);
	size_t 	  read   (uint8_t* dst, size_t count);
	bool  	  seek   (int offset, int origin);
	uintptr_t tell   () { return buffOffset; }
	uint8_t*  buffer () { return memBuffer->getBuff(); }
	size_t 	  size   () { return memBuffer->getSize(); }
	void  	  close  ();

private:
	bool ensureCapacity(size_t writeSize);
};

#endif //_MEMSTREAM_H_
