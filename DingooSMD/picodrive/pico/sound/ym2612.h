/*
  header file for software emulation for FM sound generator

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

/* compiler dependence */
#ifndef UINT8
typedef unsigned char	UINT8;   /* unsigned  8bit */
typedef unsigned short	UINT16;  /* unsigned 16bit */
typedef unsigned int	UINT32;  /* unsigned 32bit */
#endif
#ifndef INT8
typedef signed char		INT8;    /* signed  8bit   */
typedef signed short	INT16;   /* signed 16bit   */
typedef signed int		INT32;   /* signed 32bit   */
#endif

#if 1
/* struct describing a single operator (SLOT) */
typedef struct
{
	INT32	*DT;		/* #0x00 detune          :dt_tab[DT] */
	UINT8	ar;			/* #0x04 attack rate  */
	UINT8	d1r;		/* #0x05 decay rate   */
	UINT8	d2r;		/* #0x06 sustain rate */
	UINT8	rr;			/* #0x07 release rate */
	UINT32	mul;		/* #0x08 multiple        :ML_TABLE[ML] */

	/* Phase Generator */
	UINT32	phase;		/* #0x0c phase counter */
	UINT32	Incr;		/* #0x10 phase step */

	UINT8	KSR;		/* #0x14 key scale rate  :3-KSR */
	UINT8	ksr;		/* #0x15 key scale rate  :kcode>>(3-KSR) */

	UINT8	key;		/* #0x16 0=last key was KEY OFF, 1=KEY ON */

	/* Envelope Generator */
	UINT8	state;		/* #0x17 phase type: EG_OFF=0, EG_REL, EG_SUS, EG_DEC, EG_ATT */
	UINT16	tl;			/* #0x18 total level: TL << 3 */
	INT16	volume;		/* #0x1a envelope counter */
	UINT32	sl;			/* #0x1c sustain level:sl_table[SL] */

	UINT32	eg_pack_ar;		/* #0x20 (attack state) */
	UINT32	eg_pack_d1r;	/* #0x24 (decay state) */
	UINT32	eg_pack_d2r;	/* #0x28 (sustain state) */
	UINT32	eg_pack_rr;		/* #0x2c (release state) */
} FM_SLOT;


typedef struct
{
	FM_SLOT	SLOT[4];	/* four SLOTs (operators) */

	UINT8	ALGO;		/* algorithm */
	UINT8	FB;			/* feedback shift */
	INT32	op1_out;	/* op1 output for feedback */

	INT32	mem_value;	/* delayed sample (MEM) value */

	INT32	pms;		/* channel PMS */
	UINT8	ams;		/* channel AMS */

	UINT8	kcode;		/* key code:                        */
	UINT32	fc;			/* fnum,blk:adjusted to sample rate */
	UINT32	block_fnum;	/* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */

	/* LFO */
	UINT8	AMmasks;	/* AM enable flag */

} FM_CH;

typedef struct
{
	int		clock;		/* master clock  (Hz)   */
	int		rate;		/* sampling rate (Hz)   */
	double	freqbase;	/* 08 frequency base       */
	UINT8	address;	/* 10 address register     */
	UINT8	status;		/* 11 status flag          */
	UINT8	mode;		/* mode  CSM / 3SLOT    */
	UINT8	fn_h;		/* freq latch           */
	int		TA;			/* timer a              */
	int		TAC;		/* timer a maxval       */
	int		TAT;		/* timer a ticker       */
	UINT8	TB;			/* timer b              */
	UINT8	pad[3];
	int		TBC;		/* timer b maxval       */
	int		TBT;		/* timer b ticker       */
	/* local time tables */
	INT32	dt_tab[8][32];/* DeTune table       */
} FM_ST;

/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct
{
	UINT32  fc[3];			/* fnum3,blk3: calculated */
	UINT8	fn_h;			/* freq3 latch */
	UINT8	kcode[3];		/* key code */
	UINT32	block_fnum[3];	/* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct
{
	FM_ST	ST;				/* general state */
	FM_3SLOT SL3;			/* 3 slot mode state */
	UINT32  pan;			/* fm channels output mask (bit 1 = enable) */

	UINT32	eg_cnt;			/* #0xb38 global envelope generator counter */
	UINT32	eg_timer;		/* #0xb3c global envelope generator counter works at frequency = chipclock/64/3 */
	UINT32	eg_timer_add;	/* #0xb40 step of eg_timer */

	/* LFO */
	UINT32	lfo_cnt;
	UINT32	lfo_inc;

	UINT32	lfo_freq[8];	/* LFO FREQ table */
} FM_OPN;

/* here's the virtual YM2612 */
typedef struct
{
	UINT8		REGS[0x200];		/* registers (for save states)       */
	INT32		addr_A1;			/* address line A1      */

	FM_CH		CH[6];				/* channel state (0x168 bytes each)? */

	/* dac output (YM2612) */
	int			dacen;
	INT32		dacout;

	FM_OPN		OPN;				/* OPN state            */

	UINT32		slot_mask;			/* active slot mask (performance hack) */
} YM2612;
#endif

extern int   *ym2612_dacen;
extern INT32 *ym2612_dacout;
extern FM_ST *ym2612_st;


#define YM2612Read() ym2612_st->status

#define YM2612PicoTick(n) \
{ \
	/* timer A */ \
	if(ym2612_st->mode & 0x01 && (ym2612_st->TAT+=64*n) >= ym2612_st->TAC) { \
		ym2612_st->TAT -= ym2612_st->TAC; \
		if(ym2612_st->mode & 0x04) ym2612_st->status |= 1; \
	} \
 \
	/* timer B */ \
	if(ym2612_st->mode & 0x02 && (ym2612_st->TBT+=64*n) >= ym2612_st->TBC) { \
		ym2612_st->TBT -= ym2612_st->TBC; \
		if(ym2612_st->mode & 0x08) ym2612_st->status |= 2; \
	} \
}


void YM2612Init_(int baseclock, int rate);
void YM2612ResetChip_(void);
int  YM2612UpdateOne_(int *buffer, int length, int stereo, int is_buf_empty);

int  YM2612Write_(unsigned int a, unsigned int v);
unsigned char YM2612Read_(void);

int  YM2612PicoTick_(int n);
void YM2612PicoStateLoad_(void);

void *YM2612GetRegs(void);

#ifndef __GP2X__
#define YM2612Init          YM2612Init_
#define YM2612ResetChip     YM2612ResetChip_
#define YM2612UpdateOne     YM2612UpdateOne_
#define YM2612Write         YM2612Write_
#define YM2612PicoStateLoad YM2612PicoStateLoad_
#else
/* GP2X specific */
#include "../../platform/gp2x/940ctl.h"
extern int PicoOpt;
#define YM2612Init(baseclock,rate) { \
	YM2612Init_(baseclock, rate); \
}
#define YM2612ResetChip() { \
	YM2612ResetChip_(); \
}
#define YM2612UpdateOne(buffer,length,stereo,is_buf_empty) \
	YM2612UpdateOne_(buffer, length, stereo, is_buf_empty);
#define YM2612Write(a,v) \
	YM2612Write_(a, v)
#define YM2612PicoStateLoad() { \
	YM2612PicoStateLoad_(); \
}
#endif /* __GP2X__ */


#endif /* _H_FM_FM_ */
