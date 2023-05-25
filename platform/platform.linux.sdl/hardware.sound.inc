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

static SDL_AudioSpec spec;
static SDL_AudioSpec real;

static short 	sampleBuffer[SOUND_BUFFER_NUM][SOUND_BUFFER_SIZE];
static int		sampleCount [SOUND_BUFFER_NUM];

static int 		sampleRate;
static int 		channelsCount;

static volatile int soundVolume	= 100;
static volatile int soundBuffer	= 0;

static void _sound_thread_callback_orig(void*, Uint8* stream, int len)
{
	int buffer_len = sampleCount[soundBuffer] * sizeof(short);
	int actual_len = len > buffer_len ? buffer_len : len;

	debug::printf("buffer_len: %d len: %d\n", buffer_len, len);

	SDL_MixAudio(stream, (const Uint8*)sampleBuffer[soundBuffer], actual_len, SDL_MIX_MAXVOLUME);
	SDL_MixAudio(stream, (const Uint8*)sampleBuffer[soundBuffer], actual_len, SDL_MIX_MAXVOLUME);
		

	if(soundBuffer+1>=SOUND_BUFFER_NUM) soundBuffer=0;
	else soundBuffer++;
}

static int buffer_pos = 0;
static int buffer_len = -1;
static void _sound_thread_callback(void*, Uint8* stream, int len)
{
	if(buffer_len<0)
	{
		buffer_len = sampleCount[soundBuffer] * sizeof(short);
	}
    //debug::printf("bp %d bl %d len %d\n", buffer_pos, buffer_len, len);
	
	//SDL buffer is much smaller, than ours
	//So it will take a few calls from SDL o drain the buffer
	if(buffer_len < len)
	{
		//debug::printf("INNER1 bp %d bl %d len %d\n", buffer_pos, buffer_len, len);
		//Write remainder on current buffer
		SDL_MixAudio(stream, ((const Uint8*)sampleBuffer[soundBuffer])+buffer_pos, buffer_len, SDL_MIX_MAXVOLUME);	
		//reset buffer pos and eat up samples in requested len
		stream+=buffer_len;
		buffer_pos=0;
		len-=buffer_len;
		

		//Need to fill requested samples using multiple buffers..hopefully we have enough
		while(1)
		{
			// Move to next buffer
			if(soundBuffer+1>=SOUND_BUFFER_NUM) soundBuffer=0;
			else soundBuffer++;

			// Get size of new buffer
			buffer_len = sampleCount[soundBuffer] * sizeof(short);

			if(buffer_len > len)
			{
				//New buffer has enough samples - so finish off
				//debug::printf("INNER2 bl %d len %d\n", buffer_len, len);
				SDL_MixAudio(stream, (const Uint8*)sampleBuffer[soundBuffer], len, SDL_MIX_MAXVOLUME);
				buffer_pos=len;
				buffer_len-=len;
				break;
			}
			else
			{
				//Still not enough samples
				//debug::printf("INNER3 bl %d len %d\n", buffer_len, len);
				SDL_MixAudio(stream, (const Uint8*)sampleBuffer[soundBuffer], buffer_len, SDL_MIX_MAXVOLUME);
				stream+=buffer_len;
				buffer_pos=0;
				len-=buffer_len;
			}
		}
	}
	else
	{
		SDL_MixAudio(stream, ((const Uint8*)sampleBuffer[soundBuffer])+buffer_pos, len, SDL_MIX_MAXVOLUME);
		buffer_pos+=len;
		buffer_len-=len;
	}
}

void sound::init(int rate, bool stereo, int Hz)
{
	printf("sound::init\n");
	printf("rate:     %d\n", rate);
	printf("channels: %d\n", 1 + stereo);
	printf("frames:   %d\n", Hz);

	sampleRate = rate;
	channelsCount = stereo ? 2 : 1;

	int count = (rate / Hz) * channelsCount;
	printf("count:    %d\n", count);
	for(int sb = 0; sb < SOUND_BUFFER_NUM; sb++) 
	{
		sampleCount[sb] = count;
		for(int sample = 0; sample < SOUND_BUFFER_SIZE; sample++) sampleBuffer[sb][sample] = 0;
	}

	spec.freq     = sampleRate;
	spec.format   = AUDIO_S16;
	spec.channels = channelsCount;
	spec.samples  = (rate / Hz);
	spec.callback = _sound_thread_callback;
	spec.userdata = NULL;
	
	if (SDL_OpenAudio(&spec, &real) < 0) {
		printf("Couldn't start audio: %s\n", SDL_GetError());
		exit(1);
	}

	printf("real rate:     %d\n", real.freq);
	printf("real channels: %d\n", real.channels);
	printf("real samples:  %d\n", real.samples);
	printf("real format:   %d\n", real.format);

	SDL_PauseAudio(0);
}

void sound::close()
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
}

void sound::set_volume(int volume) 
{ 
	soundVolume = volume * SDL_MIX_MAXVOLUME / 10;
} 

//inline
int sound::curr_buffer_index()
{
	return soundBuffer;
}

//inline
int sound::next_buffer_index(int index)
{	
	index += 1;
	index &= SOUND_BUFFER_NUM - 1;
	return index;
}

//inline
int sound::prev_buffer_index()
{
	int index = soundBuffer;
	index -= 1;
	index &= SOUND_BUFFER_NUM - 1;
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

