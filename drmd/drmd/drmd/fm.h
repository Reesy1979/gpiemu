#ifdef __cplusplus
extern "C" {
#endif


#ifndef _H_FM_FM_
#define _H_FM_FM_
#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)

#define MAX_ATT_INDEX	(ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX	(0)			/* 0 */

#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
#define LFO_SH			24  /*  8.24 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */


#define MAXOUT		(+32767)
#define MINOUT		(-32768)


/*	TL_TAB_LEN is calculated as:
*	13 - sinus amplitude bits     (Y axis)
*	2  - sinus sign bit           (Y axis)
*	TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
#define ENV_QUIET		(TL_TAB_LEN>>3)  // 0x340

#define RATE_STEPS (8)

extern const unsigned int sl_table[];
extern const unsigned char dt_tab[];
extern const unsigned char opn_fktable[16];
extern const unsigned char eg_rate_select[];
extern const unsigned char eg_rate_shift[];
extern const unsigned char eg_inc[19*RATE_STEPS];
extern const unsigned char lfo_pm_output[7*8][8];
extern const unsigned int lfo_samples_per_step[8];
extern const unsigned char lfo_ams_depth_shift[4];
extern const signed char lfo_pm_table[128*8*32];
extern signed int YMOPN_ST_dt_tab[8][32];
extern unsigned int OPN_fn_table[4096];	/* fnumber->increment counter */
extern unsigned int OPN_lfo_freq[8];	/* LFO FREQ table */
extern const signed short tl_tab[];
extern const unsigned short sin_tab[];

extern void update_tables();
extern void fm_update_timers(void);
extern void fm_vdp_line_update(void);
extern void fm_update_dac(void);
extern int YM2612Init();
void YM2612ResetChip();
void YM2612UpdateOne(short *buffer,unsigned int length);
void fm_write(int a, unsigned char v);
int fm_read(int address);
void RefreshFm (void);
void update_timers(void);

#define INLINE static inline
#define FMSAMPLE signed short

/* register number to channel number , slot offset */
#define OPN_SLOT(N) ((N>>2)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* struct describing a single operator (SLOT) */
typedef struct
{
	unsigned int eg_sh_active_mask;
	unsigned int	sl;			/* sustain level:sl_table[SL] */
	unsigned int eg_sh_d1r_mask;
	unsigned int eg_sh_d2r_mask;
	unsigned int eg_sh_rr_mask;
	unsigned int eg_sh_ar_mask;
	unsigned int	tl;			/* total level: TL << 3	*/
	unsigned int	vol_out;	/* current output from EG circuit (without AM from LFO) */
	/* LFO */
	unsigned int	AMmask;		/* AM enable flag */

	/* Phase Generator */
	unsigned int	phase;		/* phase counter */
	unsigned int	Incr;		/* phase step */
	unsigned int	mul;		/* multiple        :ML_TABLE[ML] */
	unsigned int	key;		/* 0=last key was KEY OFF, 1=KEY ON	*/

	unsigned int	ar;			/* attack rate  */
	unsigned int	d1r;		/* decay rate   */
	unsigned int	d2r;		/* sustain rate */
	unsigned int	rr;			/* release rate */

	int	volume;		/* envelope counter	*/
	int	*DT;		/* detune          :dt_tab[DT] */
	// 19 * 4
	// 13
	// 0x59 = align 0x5C
	unsigned char	state;		/* phase type */
	unsigned char	eg_sel_ar;	/*  (attack state) */
	unsigned char	eg_sh_ar;	/*  (attack state) */
	unsigned char	eg_sel_d1r;	/*  (decay state) */

	unsigned char	eg_sh_d1r;	/*  (decay state) */
	unsigned char	eg_sel_d2r;	/*  (sustain state) */
	unsigned char	eg_sh_d2r;	/*  (sustain state) */
	unsigned char	eg_sel_rr;	/*  (release state) */

	unsigned char	eg_sh_rr;	/*  (release state) */
	unsigned char	ssg;		/* SSG-EG waveform */
	unsigned char	ssgn;		/* SSG-EG negated output */
	unsigned char	KSR;		/* key scale rate  :3-KSR */

	unsigned char	ksr;		/* key scale rate  :kcode>>(3-KSR) */
	unsigned char   pad1;
	unsigned char   pad2;
	unsigned char   pad3;

} FM_SLOT;

typedef struct
{
  unsigned int	fc;			/* fnum,blk:adjusted to sample rate	*/
	unsigned int	block_fnum;	/* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
	int	pms;		/* channel PMS */
	int	op1_out[2];	/* op1 output for feedback */
	int	mem_value;	/* delayed sample (MEM) value */
	unsigned char	ALGO;		/* algorithm */
	unsigned char	FB;			/* feedback shift */
	unsigned char	ams;		/* channel AMS */
	unsigned char	kcode;		/* key code: */

	FM_SLOT	SLOT[4];	/* four SLOTs (operators) */
} FM_CH;


typedef struct
{
  unsigned int	mode;		// mode  CSM / 3SLOT
	int		TA;			// timer a
	int		TA_Count;		// timer a counter
	int 		TA_Base;
	int		TB_Count;		//timer b counter
	int 		TB_Base;
	unsigned char   TB;			// timer b
	unsigned char	address;	// address register
	unsigned char	irq;		// interrupt level
	unsigned char	irqmask;	// irq mask
	unsigned char	status;		// status flag
	unsigned char	prescaler_sel;// prescaler selector
	unsigned char	fn_h;		// freq latch
	unsigned char   pad1;

} FM_ST;

/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct
{
	unsigned int  fc[3];	/* fnum3,blk3: calculated */
	unsigned int	block_fnum[3];	/* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
	unsigned char	fn_h;			/* freq3 latch */
	unsigned char	kcode[3];		/* key code */

} FM_3SLOT;

/* OPN/A/B common state */
typedef struct
{
	/* LFO */
	unsigned int	lfo_inc;
	unsigned int	lfo_cnt;

	unsigned int	eg_cnt;			/* global envelope generator counter */
	unsigned int	eg_timer;		/* global envelope generator counter works at frequency = chipclock/64/3 */
	unsigned int	eg_timer_add;	/* step of eg_timer */
	unsigned int	eg_timer_overflow;/* envelope generator timer overlfows every 3 samples (on real chip) */
} FM_OPN;

extern char OPN_pan[];	/* fm channels output masks */

extern FM_3SLOT SL3;			/* 3 slot mode state */
extern FM_ST	ST;				/* general state */
extern FM_OPN OPN;				/* OPN state			*/
extern FM_CH CH[6];				/* channel state		*/
extern int dacout;
extern int dacen;
extern unsigned short dac_buffer[];
extern int dac_sample;
extern unsigned int timer_base;

typedef int (*algo)(FM_CH *CH);

extern const algo algofuncs[8];


#endif /* _H_FM_FM_ */

#ifdef __cplusplus
} // End of extern "C"
#endif
