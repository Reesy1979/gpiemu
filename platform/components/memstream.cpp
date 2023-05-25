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
#include <stdio.h>
#include "memstream.h"
#include "memcpy.h"

#include "framework.h"

MemStream::MemStream()
	: memBuffer(NULL)
	, memInternal(NULL)
	, buffOffset(0)
	, isDynamic(false)
{
	//	TODO
}

MemStream::~MemStream()
{
	close();
}

////////////////////////////////////////////////////////////////////////////////

//	Try to open MemStream on given MemBuffer
bool MemStream::open(MemBuffer* memBuffer)
{
	if(!this->memBuffer) {

		//	if input MemBuffer not defined, try to use internal
		//	MemBuffer with dynamic memory allocation 
		if(!memBuffer) {
			memBuffer = this->memInternal = new MemBuffer();
		}

		//	if MemBuffer has allocated memory, it must be used as
		//	a fixed size memory chunk
		if(this->isDynamic = !memBuffer->buffAddr) {

			//	if given MemBuffer is empty, try to use it as MemBuffer
			//	with dynamic allocation
			if(memBuffer->buffAddr = (uint8_t*)malloc(DEFAULT_BUFF_SIZE)) {

				//	MemBuffer allocated with inital size and is empty
				memBuffer->buffSize = DEFAULT_BUFF_SIZE;
				memBuffer->numBytes = 0;
				this->memBuffer = memBuffer;
				return true;
			}
			return false;
		}
		this->memBuffer = memBuffer;
		return true;
	}
	return false;
}

//	Try o open MemStream on fixed size memory chunk
bool MemStream::open(uint8_t* addr, size_t size)
{
	if(addr && size > 0) {

		//	create internal MemBuffer that will be used as
		//	external MemBuffer with reference to given
		//	fixed size memory chunk
		memInternal = new MemBuffer();
		memInternal->buffAddr = addr;
		memInternal->buffSize = size;
		memInternal->numBytes = size;
		if(open(memInternal)) return true;

		//	oops, something wrong, delete allocated buffer
		delete memInternal;
		memInternal = NULL;
	}
	return false;
}

//	Write bytes to MemStream
size_t MemStream::write(const uint8_t* src, size_t count)
{
	if(!ensureCapacity(count)) 
	{
		return 0;
	}
	memcpy(memBuffer->buffAddr + buffOffset, src, count);
	buffOffset += count;

	if(buffOffset > memBuffer->numBytes) {
		memBuffer->numBytes = buffOffset;
	}
	return count;
}

//	Read bytes from MemStream
size_t MemStream::read(uint8_t* dst, size_t count)
{
	size_t nread = count;
	if(buffOffset + count > memBuffer->numBytes) {
		nread = memBuffer->numBytes - buffOffset;
	}
	memcpy(dst, memBuffer->buffAddr + buffOffset, nread);
	buffOffset += nread;

	return nread;
}

//	Seek on MemStream
bool MemStream::seek(int offset, int origin)
{
	int npos;
	switch (origin) {
	case SEEK_SET:
		npos = offset;
		break;

	case SEEK_CUR:
		npos = buffOffset + offset;
		break;

	case SEEK_END:
		npos = memBuffer->numBytes + offset;
		break;
	}

	if(npos > memBuffer->numBytes) {
		if(!ensureCapacity(npos - memBuffer->numBytes)) return false;
		memBuffer->numBytes = npos;
	}
	if(npos >= 0) buffOffset = npos;
	return true;
}

void MemStream::close()
{
	//	free and delete internal MemBuffer if used
	if(memInternal) {
		if(isDynamic) {
			memInternal->free(); 
		} else {
			memInternal->buffAddr = NULL;
		}
		delete memInternal;
		memInternal = NULL;
	}
	
	memBuffer = NULL;
	buffOffset = 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MemStream::ensureCapacity(size_t writeSize)
{
    size_t needSize = buffOffset + writeSize;
    if(needSize <= memBuffer->buffSize)
	{
		return true;
	}
    if(!isDynamic) return false;

    size_t newSize = memBuffer->buffSize + memBuffer->buffSize / 2;
    if(newSize < needSize) newSize = needSize;
	
    uint8_t* newBuff = (uint8_t*)realloc(memBuffer->buffAddr, newSize);
    if(!newBuff) 
	{
		return false;
	}
    memBuffer->buffAddr = newBuff;
    memBuffer->buffSize = newSize;
    return true;
}
