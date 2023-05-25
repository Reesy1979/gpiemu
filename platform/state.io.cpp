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
 
///////////////////////////////////////////////////////////////////////////////
//	Game State IO module
///////////////////////////////////////////////////////////////////////////////

#include "framework.h"
#include "state.io.h"
#include "unzip.h"

enum IO_MODE 
{
	IO_MODE_NONE,
	IO_MODE_MEMORY,
	IO_MODE_FILE
};

enum RW_MODE
{
	RW_MODE_NONE,
	RW_MODE_READ,
	RW_MODE_WRITE
};

///////////////////////////////////////////////////////////////////////////////

static IO_MODE		ioMode = IO_MODE_NONE;
static RW_MODE  	rwMode = RW_MODE_NONE;

static void*		container;
static MemStream 	dataStream;
static char			zipFileName[HW_MAX_PATH];

///////////////////////////////////////////////////////////////////////////////
//	Read ZIP file data
///////////////////////////////////////////////////////////////////////////////

static bool load_zip_file(cstr_t filePath)
{
	bool result = false;
	unz_file_info info;
	
	unzFile fd = unzOpen(filePath);	
	if(fd) 
	{	
		if(unzGoToFirstFile(fd) == UNZ_OK && unzGetCurrentFileInfo(fd, &info, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK)
		{
			dataStream.open();
			dataStream.seek(info.uncompressed_size, SEEK_SET);

			if(dataStream.size() == info.uncompressed_size)
			{
				if(unzOpenCurrentFile(fd) == UNZ_OK)
				{
					if((uLong)unzReadCurrentFile(fd, dataStream.buffer(), dataStream.size()) == info.uncompressed_size)
					{						
						result = true;
					}
					unzCloseCurrentFile(fd);
				}
			}
			if(!result) dataStream.close();
		}
		unzClose(fd);
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
//	Select mode of usage
///////////////////////////////////////////////////////////////////////////////

bool state_use_memory(MemBuffer* memBuffer)
{
	if(ioMode == IO_MODE_NONE && memBuffer)	{
		container = (void*)memBuffer;
		ioMode = IO_MODE_MEMORY;
		return true; 
	}
	return false;
}

bool state_use_file(cstr_t filePath)
{
	if(ioMode == IO_MODE_NONE && filePath)	{
		container = (void*)filePath;
		ioMode = IO_MODE_FILE;
		return true; 
	}
	return false;
}

bool state_is_correct(cstr_t filePath)
{
	if(filePath && fw::zip::check(filePath))
	{
		char fileInZip[16];
		fw::zip::fileNameInZip(filePath, fileInZip, false);
		return strcmp(fileInZip, EMU_FILE_NAME) == 0;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//	Standart IO functions
///////////////////////////////////////////////////////////////////////////////

bool state_fopen(cstr_t mode)
{
	bool result = false;
	if(ioMode == IO_MODE_MEMORY) 
	{
		MemBuffer* memBuffer = static_cast<MemBuffer*>(container);
		if(mode[0] == 'r')
		{
			if(dataStream.open(memBuffer))
			{
				rwMode = RW_MODE_READ;
				result = true;
			}
		} else
		if(mode[0] == 'w')
		{
			memBuffer->free();
			if(dataStream.open(memBuffer))
			{
				rwMode = RW_MODE_WRITE;
				result = true;
			}
		}
	} else 
	if(ioMode == IO_MODE_FILE)
	{
		cstr_t filePath = static_cast<cstr_t>(container);
		if(mode[0]=='r' && state_is_correct(filePath) && load_zip_file(filePath))
		{
			dataStream.seek(0, SEEK_SET);
			rwMode = RW_MODE_READ;
			result = true;
		} else
		if(mode[0] == 'w' && dataStream.open())
		{
			strcpy(zipFileName, filePath);
			rwMode = RW_MODE_WRITE;
			result = true;
		}

	}
	if(!result)
	{
		ioMode = IO_MODE_NONE;
		rwMode = RW_MODE_NONE;
	}
	return result;
}

int state_fread(void* ptr, size_t count)
{
	if(rwMode == RW_MODE_READ && ioMode != IO_MODE_NONE)
	{
		return dataStream.read(static_cast<uint8_t*>(ptr), count);
	}
	return EOF;
}

int state_fwrite(const void* ptr, size_t count)
{
	if(rwMode == RW_MODE_WRITE && ioMode != IO_MODE_NONE)
	{
		return dataStream.write(static_cast<const uint8_t*>(ptr), count);
	}
	return EOF;
}

int state_fgetc() 
{
	uint8_t temp;
	return state_fread(&temp, 1) < 1 ? EOF : temp;
}
	
int state_fputc(int ch)
{
	uint8_t temp = (uint8_t)ch;
	return state_fwrite(&temp, 1) < 1 ? EOF : ch;
}

long int state_fseek(long int offset, int origin)
{
	if(ioMode != IO_MODE_NONE)
	{
		if(dataStream.seek(int(offset), origin)) return 0;
	}
	return -1;
}

long int state_ftell()
{
	if(ioMode != IO_MODE_NONE)
	{
		return dataStream.tell();
	}
	return -1;
}

int state_fclose()
{
	if(ioMode == IO_MODE_FILE && rwMode == RW_MODE_WRITE)
	{
		fw::zip::save(zipFileName, EMU_FILE_NAME, dataStream.buffer(), dataStream.size());
		zipFileName[0] = 0;
	}

	dataStream.close();
	
	ioMode = IO_MODE_NONE;
	rwMode = RW_MODE_NONE;
	return 0;
}
