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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ips.patcher.h"

#define DEBUG_SAVE_RESULT

typedef struct Record_s {
	uint32_t  start;
	uint32_t  end;
	uint8_t*  data;
	uint8_t   byte;
	Record_s* next;
} Record_t;

static char      ipsFilePath[256];
static FILE*     streamToPatch;
static Record_t* recordsList;
static Record_t* firstRecord;
static uint32_t  maxPatchOffset;

#ifdef DEBUG_SAVE_RESULT
static FILE* 	 resultStream;
#endif

////////////////////////////////////////////////////////////////////////////////

static uint32_t MAX(uint32_t a, uint32_t b) { return a > b ? a : b; }
static uint32_t MIN(uint32_t a, uint32_t b) { return a < b ? a : b; }
static uint32_t threeBytesValue(uint8_t* b) { return uint32_t(b[0] << 16 | b[1] << 8 | b[2]); }
static uint32_t twoBytesValue(uint8_t* b) {	return uint32_t(b[0] << 8 | b[1]); }
static void resetSearch();

static bool loadIPS()
{
	FILE* ipsStream;
	recordsList = NULL;
	maxPatchOffset = 0;

	//	try to open IPS patch stream
	if(strlen(ipsFilePath) == 0 || !(ipsStream = fopen(ipsFilePath, "rb"))) 
		return false;

	//	check IPS file format
	uint8_t b[5]; 
	fread(b, 1, 5, ipsStream);
	if(b[0] != 'P' || b[1] != 'A' || b[2] != 'T' || b[3] != 'C' || b[4] != 'H') {
		fclose(ipsStream);
		return false;
	}

	uint32_t offset; size_t size; bool isRLE; uint8_t rleValue;
	while(true)
	{
		//	read record offset and check for EOF
		fread(b, 1, 3, ipsStream);
		if(b[0] == 'E' && b[1] == 'O' && b[2] == 'F') break;
		offset = threeBytesValue(b);

		//	read record size
		fread(b, 1, 2, ipsStream);
		size = twoBytesValue(b);
		if(isRLE = !size) {

			//	read record RLE size and RLE value 
			fread(b, 1, 2, ipsStream);
			size = twoBytesValue(b);
			fread(b, 1, 1, ipsStream);
			rleValue = b[0];
		}

		//	create record instance and clear its data
		Record_t* record = (Record_t*)malloc(sizeof(Record_t));
		memset(record, 0, sizeof(Record_t));

		//	set up record instance data
		record->start = offset;
		record->end = offset + size;
		if(isRLE) {
			record->byte = rleValue;
		} else {
			record->data = (uint8_t*)malloc(size);
			fread(record->data, 1, size, ipsStream);
		}

		//	determine max patch offset
		maxPatchOffset = MAX(maxPatchOffset, record->end);

		//	append record instance to list
		if(!recordsList) {
			recordsList = record;
		} else {
			firstRecord = recordsList;
			while(firstRecord->next) firstRecord = firstRecord->next;
			firstRecord->next = record;
		}
	}

	//	everything OK, close IPS stream
	fclose(ipsStream);
	resetSearch();
	return true;
}

static void openIPS(void* stream, const char* mode)
{
	if(stream && mode[0] == 'r' && loadIPS()) {
		streamToPatch = (FILE*)stream;
#ifdef DEBUG_SAVE_RESULT
		char filepath[256];
		strcpy(filepath, ipsFilePath);
		strcat(filepath, ".result");
		resultStream = fopen(filepath, "wb");
#endif
	} else {
		streamToPatch = NULL;
	}
}

static void closeIPS()
{
	Record_t* currRecord = recordsList;
	Record_t* nextRecord;

	while(currRecord) 
	{
		nextRecord = currRecord->next;
		if(currRecord->data) free(currRecord->data);
		free(currRecord);
		currRecord = nextRecord;
	}

	recordsList = NULL;
	ipsFilePath[0] = 0;
#ifdef DEBUG_SAVE_RESULT
	fclose(resultStream);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static Record_t* searchRecord(uint32_t readStart, uint32_t readEnd, uint32_t& patchStart, uint32_t& patchEnd)
{
	Record_t* record = firstRecord;
	while(record)
	{
		int chk0 = readEnd - record->start;
		int chk1 = record->end - readStart;
		if(chk0 > 0 && chk1 > 0)
		{
			patchStart = MAX(readStart, record->start);
			patchEnd   = MIN(readEnd, record->end);
			
			if(patchStart == record->start && patchEnd == record->end) {
				firstRecord = record->next;
			} else {
				firstRecord = record;
			}
			return record;
		}

		record = record->next;
		if(record->start >= readEnd) break;
	}
	return NULL;
}

static void resetSearch()
{
	firstRecord = recordsList;
}

static void patchMemory(void* ptr, uint32_t readStart, uint32_t readEnd)
{
	Record_t* record; uint32_t patchStart, patchEnd;
	while(record = searchRecord(readStart, readEnd, patchStart, patchEnd))
	{			
		void*  memAddr = (uint8_t*)(ptr) + (patchStart - readStart);
		size_t memSize = patchEnd - patchStart;

		if(record->data) {
			uint8_t* data = record->data + (patchStart - record->start);
			memcpy(memAddr, data, memSize);
		} else {
			memset(memAddr, record->byte, memSize);
		}
	}
#ifdef DEBUG_SAVE_RESULT
	fwrite(ptr, 1, readEnd - readStart, resultStream);
#endif
}

//	TODO: implement DEBUG_SAVE_RESULT
static int patchValue(int character, uint32_t readStart)
{
	uint32_t patchStart, patchEnd;
	Record_t* record = searchRecord(readStart, readStart + 1, patchStart, patchEnd);

	if(record)
	{
		if(record->data) {
			return record->data[patchStart - record->start];
		} else {
			return record->byte;
		}
	}
	return character;
}

////////////////////////////////////////////////////////////////////////////////

void ips_define(const char* ipsname)
{
	if(ipsname) strcpy(ipsFilePath, ipsname); else ipsFilePath[0] = 0;
}

FILE* ips_fopen(const char* filename, const char* mode)
{
	FILE* stream = fopen(filename, mode);
	openIPS(stream, mode);
	return stream;
}

//	TODO: ftell outside the file but in patchable area
size_t ips_fread(void* ptr, size_t size, size_t count, FILE* stream)
{
	if(stream == streamToPatch) {
		uint32_t start = ftell(stream);
		count = fread(ptr, size, count, stream);
		count = MAX(count, maxPatchOffset - start);
		patchMemory(ptr, start, start + count);
		return count;
	}
	return fread(ptr, size, count, stream);
}

//	TODO: ftell outside the file but in patchable area
int ips_fgetc(FILE* stream)
{
	if(stream == streamToPatch) {
		uint32_t start = ftell(stream);
		int  character = fgetc(stream);
		return patchValue(character, start);
	}
	return fgetc(stream);
}

int ips_fseek(FILE* stream, long int offset, int origin)
{
	if(stream == streamToPatch) resetSearch();
	return fseek(stream, offset, origin);
}

int ips_fclose(FILE* stream)
{
	if(stream == streamToPatch)	closeIPS();
	return fclose(stream);
}

////////////////////////////////////////////////////////////////////////////////

void ips_onopen(void* stream, const char* mode)
{
	openIPS(stream, mode);
}

void ips_onread(void* ptr, uint32_t position, size_t count, void* stream)
{
	if(stream == streamToPatch) {
		count = MAX(count, maxPatchOffset - position);
		patchMemory(ptr, position, position + count);
	}
}

//	TODO: implement patching that add data to original file
int ips_ongetc(int character, uint32_t position, void* stream)
{
	if(stream == streamToPatch)
		return patchValue(character, position);
	return character;
}

void ips_onseek(void* stream)
{
	if(stream == streamToPatch)
		resetSearch();
}

void ips_onclose(void* stream)
{
	if(stream == streamToPatch)
		closeIPS();
}

#endif
