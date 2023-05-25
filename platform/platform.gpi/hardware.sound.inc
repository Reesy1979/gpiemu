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
 
#define SOUND_BUFFER_SIZE	2000
#define SOUND_BUFFER_NUM	8

static short 	sampleBuffer[SOUND_BUFFER_NUM][SOUND_BUFFER_SIZE];
static int		sampleCount [SOUND_BUFFER_NUM];

static int 		sampleRate;
static int 		channelsCount;

static volatile int soundVolume	= 0;
static volatile int soundBuffer	= 0;
extern void KernelLog(char *msg);
static int _sound_thread_callback(short *dst, int maxSamples)
{
	int count=sampleCount[soundBuffer];
	if(count>maxSamples)
	{
		KernelLog("_sound_thread_callback too much\n");
		count=maxSamples; //limit
	}
	int ret=count;
	short *src=sampleBuffer[soundBuffer];
	while(count-- > 0) 
	{
		*dst++ = (short)*src++;
		*dst++ = 0;
	}
	
	if(soundBuffer+1>=SOUND_BUFFER_NUM) soundBuffer=0;
	else soundBuffer++;
	
	return ret;
}

void sound::init(int rate, bool stereo, int Hz)
{
	sampleRate = rate;
	channelsCount = stereo ? 2 : 1;

	int count = (rate / Hz) * channelsCount;
	for(int sb = 0; sb < SOUND_BUFFER_NUM; sb++) 
	{
		sampleCount[sb] = count;
		for(int sample = 0; sample < SOUND_BUFFER_SIZE; sample++) sampleBuffer[sb][sample] = 0;
	}
	
	KernelAudioInit(rate, Hz, _sound_thread_callback);
}

void sound::close()
{
	KernelAudioClose();
}

void sound::set_volume(int volume) 
{ 
	const uint32_t table[] = { 0, 6, 11, 15, 19, 22, 25, 27, 28, 29, 30 };
	soundVolume = table[volume];
} 

int sound::curr_buffer_index()
{
	return soundBuffer;
}

int sound::next_buffer_index(int index)
{	
	index += 1;
	if(index>=SOUND_BUFFER_NUM) index=0;
	
	return index;
}

int sound::prev_buffer_index()
{
	int index = soundBuffer;
	index -= 1;
	if(index<0) index=SOUND_BUFFER_NUM-1;
	return index;
}

short* sound::get_buffer(int index)
{
	if(index >= 0 && index < SOUND_BUFFER_NUM) return sampleBuffer[index];
	return NULL;
}

int sound::get_sample_count(int index)
{
	if(index >= 0 && index < SOUND_BUFFER_NUM) return sampleCount[index];
	return 0;
}

void sound::set_sample_count(int index, int count)
{
	if(index >= 0 && index < SOUND_BUFFER_NUM) sampleCount[index] = count;
}

void sound::fill_s8(int index, char *src, int count)
{
	sampleCount[index] = count;
	short *dst = sampleBuffer[index];
	while(count-- > 0)
	{
		*dst++ = ( ( ( *src++ ^ 0x80 ) & 0xff ) << 8 );
	}
}

void sound::fill_s16(int index, short *src, int count)
{
	sampleCount[index] = count;
	short *dst = sampleBuffer[index];
	while(count-- > 0) *dst++ = *src++;
}

void sound::fill_s16(int index, short **src, int count)
{
	sampleCount[index] = count << 1;
	short *dst = sampleBuffer[index];
	for(int i = 0; i < count; i++) {
		*dst++ = src[0][i];
		*dst++ = src[1][i];
	}
}

void sound::fill_s32(int index, int *src, int count)
{
	sampleCount[index] = count;
	short *dst = sampleBuffer[index];
	while(count-- > 0) *dst++ = (short)*src++;
}
