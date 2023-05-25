
#include "sal.h"
#include "menu.h"
#include "sn76496.h"
#include "globals.h"

#define INLINE static inline

INLINE int parity(int val)
{
	val^=val>>8;
	val^=val>>4;
	val^=val>>2;
	val^=val>>1;
	return val;
}

void psg_write(int data)
{
#if 0
	if ((gCurrentEmuMode != EMU_MODE_MD) && (gSoundOn))
	{
	    //update sound before updating psg registers
	    int i;
	    int currstage;
	    int length;
	    currstage= gSampleCountLookup[gDrsms.vdp_line];
	    length=currstage-gLastSample;
		if(length>0)
			{
			RenderSound(gSoundBuffer+(gLastSample<<1),length);
			gLastSample=currstage;
	   }
	}
#endif
	//if (snd_enabled)
	{
		if (data & 0x80)
		{
			int r = (data & 0x70) >> 4;
			int c = r/2;

			PSG.sn_LastRegister = r;
			PSG.sn_Register[r] = (PSG.sn_Register[r] & 0x3f0) | (data & 0x0f);
			switch (r)
			{
			case 0:	/* tone 0 : frequency */
			case 2:	/* tone 1 : frequency */
			case 4:	/* tone 2 : frequency */
				PSG.sn_Period[c] = PSG_sn_UPDATESTEP * PSG.sn_Register[r];
				if (PSG.sn_Period[c] == 0) PSG.sn_Period[c] = PSG_sn_UPDATESTEP;
				if (r == 4)
				{
					/* update noise shift frequency */
					if ((PSG.sn_Register[6] & 0x03) == 0x03)
						PSG.sn_Period[3] = 2 * PSG.sn_Period[2];
				}
				break;
			case 1:	/* tone 0 : volume */
			case 3:	/* tone 1 : volume */
			case 5:	/* tone 2 : volume */
			case 7:	/* noise  : volume */
				PSG.sn_Volume[c] = PSG.sn_VolTable[data & 0x0f];
				break;
			case 6:	/* noise  : frequency, mode */
				{
					int n = PSG.sn_Register[6];
					// orig
					PSG.sn_NoiseFB = (n & 4) ? FB_WNOISE : FB_PNOISE;


					//PSG.sn_NoiseFB = ( (n & 4) ? parity((PSG.sn_RNG) & 0x000f) : PSG.sn_RNG & 1 ) << 15;

					n &= 3;
					/* N/512,N/1024,N/2048,Tone #3 output */
					PSG.sn_Period[3] = (n == 3) ? 2 * PSG.sn_Period[2] : (PSG_sn_UPDATESTEP << (5+n));

					/* reset noise shifter */
					PSG.sn_RNG = NG_PRESET;
					PSG.sn_Output[3] = PSG.sn_RNG & 1;
				}
				break;
			}
		}
		else
		{
			int r = PSG.sn_LastRegister;
			int c = r/2;

			switch (r)
			{
			case 0:	/* tone 0 : frequency */
			case 2:	/* tone 1 : frequency */
			case 4:	/* tone 2 : frequency */
				PSG.sn_Register[r] = (PSG.sn_Register[r] & 0x0f) | ((data & 0x3f) << 4);
				PSG.sn_Period[c] = PSG_sn_UPDATESTEP * PSG.sn_Register[r];
				if (PSG.sn_Period[c] == 0) PSG.sn_Period[c] = PSG_sn_UPDATESTEP;
				if (r == 4)
				{
					/* update noise shift frequency */
					if ((PSG.sn_Register[6] & 0x03) == 0x03)
						PSG.sn_Period[3] = 2 * PSG.sn_Period[2];
				}
				break;
			}
		}
	}
}

int SN76496_sh_start(void)
{
	int i;
	double out;

	for (i = 0;i < 4;i++) PSG.sn_Volume[i] = 0;

	PSG.sn_LastRegister = 0;
	for (i = 0;i < 8;i+=2)
	{
		PSG.sn_Register[i] = 0;
		PSG.sn_Register[i + 1] = 0x0f;	/* volume = 0 */
	}

	for (i = 0;i < 4;i++)
	{
		PSG.sn_Output[i] = 0;
		PSG.sn_Period[i] = PSG.sn_Count[i] = PSG_sn_UPDATESTEP;
	}
	PSG.sn_RNG = NG_PRESET;
	PSG.sn_Output[3] = PSG.sn_RNG & 1;

	/* increase max output basing on gain (0.2 dB per step) */
	out = MAX_OUTPUT / 3;

	/* build volume table (2dB per step) */
	for (i = 0;i < 15;i++)
	{
		/* limit volume to avoid clipping */
		if (out > MAX_OUTPUT / 3) PSG.sn_VolTable[i] = MAX_OUTPUT / 3;
		else PSG.sn_VolTable[i] = out;

		out /= 1.258925412;	/* = 10 ^ (2/20) = 2dB */
	}
	PSG.sn_VolTable[15] = 0;

	return 0;
}

void RenderSound(short *buffer, unsigned int length)
{
	long long psg_out=0;
	int i=0;

	/* If the volume is 0, increase the counter */
	for (i = 0;i < 4;i++)
	{
		if (PSG.sn_Volume[i] == 0)
		{
			/* note that I do count += length, NOT count = length + 1. You might think */
			/* it's the same since the volume is 0, but doing the latter could cause */
			/* interferencies when the program is rapidly modulating the volume. */
			if (PSG.sn_Count[i] <= (length<<STEP_BITS))
				PSG.sn_Count[i] += (length<<STEP_BITS);
		}
	}

	for(i=0; i < length ; i++)
	{
		psg_out=0;
		//if((menu_options.sound_on==1)||(menu_options.sound_on==3))
		{
			int vol;

			vol=0;
			if (PSG.sn_Output[0])
				vol = PSG.sn_Count[0];
			PSG.sn_Count[0] -= STEP;

			while (PSG.sn_Count[0] <= 0)
			{
				PSG.sn_Count[0] += PSG.sn_Period[0];
				if (PSG.sn_Count[0] > 0)
				{
					PSG.sn_Output[0] ^= 1;
					if (PSG.sn_Output[0])
						vol += PSG.sn_Period[0];
					break;
				}
				PSG.sn_Count[0] += PSG.sn_Period[0];
				vol += PSG.sn_Period[0];
			}
			if (PSG.sn_Output[0])
				vol -= PSG.sn_Count[0];

			psg_out = vol * PSG.sn_Volume[0];

			vol=0;
			if (PSG.sn_Output[1])
				vol = PSG.sn_Count[1];
			PSG.sn_Count[1] -= STEP;

			while (PSG.sn_Count[1] <= 0)
			{
				PSG.sn_Count[1] += PSG.sn_Period[1];
				if (PSG.sn_Count[1] > 0)
				{
					PSG.sn_Output[1] ^= 1;
					if (PSG.sn_Output[1])
						vol += PSG.sn_Period[1];
					break;
				}
				PSG.sn_Count[1] += PSG.sn_Period[1];
				vol += PSG.sn_Period[1];
			}
			if (PSG.sn_Output[1])
				vol -= PSG.sn_Count[1];

			psg_out += vol * PSG.sn_Volume[1];

			vol=0;
			if (PSG.sn_Output[2])
				vol = PSG.sn_Count[2];
			PSG.sn_Count[2] -= STEP;

			while (PSG.sn_Count[2] <= 0)
			{
				PSG.sn_Count[2] += PSG.sn_Period[2];
				if (PSG.sn_Count[2] > 0)
				{
					PSG.sn_Output[2] ^= 1;
					if (PSG.sn_Output[2])
						vol += PSG.sn_Period[2];
					break;
				}
				PSG.sn_Count[2] += PSG.sn_Period[2];
				vol += PSG.sn_Period[2];
			}
			if (PSG.sn_Output[2])
				vol -= PSG.sn_Count[2];

			psg_out += vol * PSG.sn_Volume[2];

			{
				int left = STEP;

				vol=0;
				do
				{
					int nextevent;

					if (PSG.sn_Count[3] < left)
						nextevent = PSG.sn_Count[3];
					else
						nextevent = left;

					if (PSG.sn_Output[3])
						vol += PSG.sn_Count[3];

					PSG.sn_Count[3] -= nextevent;

					if (PSG.sn_Count[3] <= 0)
					{
						if (PSG.sn_RNG & 1)
							PSG.sn_RNG ^= PSG.sn_NoiseFB;
						PSG.sn_RNG >>= 1;
						PSG.sn_Output[3] = PSG.sn_RNG & 1;
						PSG.sn_Count[3] += PSG.sn_Period[3];
						if (PSG.sn_Output[3])
							vol += PSG.sn_Period[3];
					}
					if (PSG.sn_Output[3])
						vol -= PSG.sn_Count[3];

					left -= nextevent;
				}
				while (left > 0);

				psg_out += vol * PSG.sn_Volume[3];
			}


		}
		psg_out>>=STEP_BITS;
		if (psg_out > (32767L))
			psg_out = 32767L;
		if (psg_out < (-32768L))
			psg_out = -32768L;

		*buffer++ = (short)psg_out;
		*buffer++ = (short)psg_out;
	}
}
