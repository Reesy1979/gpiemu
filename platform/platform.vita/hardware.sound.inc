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

#define MAX_SOUND_LEN 	(((48000/50)*2)*SOUND_BUFFER_COUNT)
#define SOUND_BUFFER_COUNT 	10

static short mSoundBuffer[MAX_SOUND_LEN];
static short *mSoundBuffers[SOUND_BUFFER_COUNT];
static unsigned int mStereo=0;
static int sound_ch_handle=-1;
static int thread_handle=-1;
static volatile int sound_stop;
static int mVitaSoundSampleBytes;
static int mSoundSampleCount;
static unsigned int mSoundBufferSize=0;
static short mVitaSoundBuffer1[48000];
static short mVitaSoundBuffer2[48000];
static int mVitaSoundPos=0;
static volatile unsigned int mSb=0;
static int channel_thread(unsigned int args, void *argp)
{
  int vols[2]={0x8000,0x8000};
  unsigned int sbIn = (unsigned int)&mSoundBuffer[0];
  unsigned int sbOut = (unsigned int)&mVitaSoundBuffer1[0];
  unsigned int sbSize = (mSoundBufferSize*SOUND_BUFFER_COUNT);
  unsigned int bufidx = 0;
  sceAudioOutSetVolume(sound_ch_handle,(SceAudioOutChannelFlag)(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH),vols);
  
  while (!sound_stop)
  {	
    if(bufidx==0)
    {
        sbOut = (unsigned int)&mVitaSoundBuffer1[0];
        bufidx = 1;
    }
    else
    {
        sbOut = (unsigned int)&mVitaSoundBuffer2[0];
        bufidx = 0;
    }
    
		mSb=mVitaSoundPos/mSoundBufferSize;
    
    if(mVitaSoundPos+mVitaSoundSampleBytes>=sbSize)
    {
        unsigned int endPart=sbSize-mVitaSoundPos;
        memcpy((void*)sbOut,(const void*)sbIn+mVitaSoundPos,endPart);
        memcpy((void*)sbOut+endPart,(const void*)sbIn,mVitaSoundSampleBytes-endPart);
        mVitaSoundPos=mVitaSoundSampleBytes-endPart;
    }
    else
    {
        memcpy((void*)sbOut,(const void*)sbIn+mVitaSoundPos,mVitaSoundSampleBytes);
        mVitaSoundPos+=mVitaSoundSampleBytes;
    }
    
    sceAudioOutOutput(sound_ch_handle, (const void*)sbOut);
  }

  sceKernelExitThread(0);
  return 0;
}

void sound::init(int rate, bool stereo, int Hz)
{
	int x;
	mStereo=stereo+1;
	mSoundSampleCount=(rate/Hz) * mStereo;
	mSoundBufferSize=mSoundSampleCount*2;
	sound_stop = 0;
	for(x=0;x<SOUND_BUFFER_COUNT;x++)
	{
		mSoundBuffers[x] = &mSoundBuffer[0]+(x*mSoundSampleCount);
	}

  //Vita sound APIs expect sample counts to be aligned
  //to 64
  int vitaSampleCount = ((((rate/Hz)/4) + 63) & ~63);
  mVitaSoundSampleBytes = (vitaSampleCount<<1)*mStereo;
	//Clear all sound banks

	
  sound_ch_handle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                      vitaSampleCount, rate,
                      (stereo)
                        ? SCE_AUDIO_OUT_MODE_STEREO
                        : SCE_AUDIO_OUT_MODE_MONO);
  if(sound_ch_handle <0)
  {
     return;
  }    

  thread_handle =
  sceKernelCreateThread("audtiotx0", &channel_thread, 0x10000100, 0x10000, 0, 0, NULL);
  
  if (thread_handle < 0)
  {
    thread_handle = -1;
    return;
  }

  if (sceKernelStartThread(thread_handle,NULL, NULL) != 0)
  {
    return;
  }
                   
}

void sound::close()
{
    sound_stop = 1;
    
    if (thread_handle != -1)
    {
      sceKernelDeleteThread(thread_handle);
    }
    
    if (sound_ch_handle != -1)
    {
      sceAudioOutReleasePort(sound_ch_handle);
      sound_ch_handle = -1;
    }
}

void sound::set_volume(int volume) 
{ 
	//soundVolume = volume / 10;
} 

//inline
int sound::curr_buffer_index()
{
	return mSb;
}

//inline
int sound::next_buffer_index(int index)
{	
	index += 1;
	if (index >= SOUND_BUFFER_COUNT) index = 0;
	return index;
}

//inline
int sound::prev_buffer_index()
{
	int index = mSb;
	index -= 1;
	if (index < 0) index = SOUND_BUFFER_COUNT -1;
	return index;
}

short* sound::get_buffer(int index)
{
	if(index >= 0 && index < SOUND_BUFFER_COUNT) return mSoundBuffers[index];
	return NULL;
}

int sound::get_sample_count(int index)
{
	return mSoundSampleCount;
}

void sound::set_sample_count(int index, int count)
{

}

void sound::fill_s16(int index, short *src, int count)
{
	//sampleCount[index] = count;
	short *dst = mSoundBuffers[index];
	while(count-- > 0) *dst++ = *src++;
}

void sound::fill_s16(int index, short **src, int count)
{
	//sampleCount[index] = count << 1;
	short *dst = mSoundBuffers[index];
	for(int i = 0; i < count; i++) {
		*dst++ = src[0][i];
		*dst++ = src[1][i];
	}
}

void sound::fill_s32(int index, int *src, int count)
{
	//sampleCount[index] = count;
	//short *dst = sampleBuffer[index];
	short *dst = mSoundBuffers[index];
	while(count-- > 0) *dst++ = (short)*src++;
}

