#ifdef __cplusplus
extern "C" {
#endif


#ifndef SN76496_H
#define SN76496_H

#define MAX_OUTPUT  0x7fff
#define STEP_BITS	16
#define STEP        1<<STEP_BITS
#define FB_WNOISE   0x12000
#define FB_PNOISE   0x08000
#define NG_PRESET   0x0F35

struct PSG_CONTEXT
{
       int sn_Volume[4];
       int sn_Count[4];
       int sn_Output[4];
       int sn_Period[4];
       int sn_Register[8];
       int sn_VolTable[16];
       int sn_LastRegister;
       int sn_NoiseFB;
       unsigned int sn_RNG;
};

/* Function prototypes */
int SN76496_sh_start( void );
void psg_write(int data);

#endif

#ifdef __cplusplus
} // End of extern "C"
#endif
