#ifndef _MAIN_H_
#define _MAIN_H_

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

#define MD_CLOCK_NTSC 				53693175.0
#define MD_CLOCK_PAL  				53203424.0
#define MAX_ROM_SIZE 				0x300000 //ssf2(40mbits)
#define MAX_CPU_MODES				40
#define MD_BUTTON_UP     0
#define MD_BUTTON_DOWN   1 
#define MD_BUTTON_LEFT   2  
#define MD_BUTTON_RIGHT  3 
#define MD_BUTTON_B      4
#define MD_BUTTON_C      5 
#define MD_BUTTON_A      6 
#define MD_BUTTON_START  7
#define MD_BUTTON_Z  		8
#define MD_BUTTON_Y  		9
#define MD_BUTTON_X  		10
#define MD_BUTTON_MODE  	11 

#define SMS_CLOCK_NTSC		3579545
#define SMS_CLOCK_PAL       3579545
#define SMS_BUTTON_UP     0
#define SMS_BUTTON_DOWN   1 
#define SMS_BUTTON_LEFT   2 
#define SMS_BUTTON_RIGHT  3 
#define SMS_BUTTON_1      4 
#define SMS_BUTTON_2      5 
#define SMS_BUTTON_START  7 

#define GG_BUTTON_UP     0
#define GG_BUTTON_DOWN   1 
#define GG_BUTTON_LEFT   2  
#define GG_BUTTON_RIGHT  3
#define GG_BUTTON_1      4 
#define GG_BUTTON_2      5 
#define GG_BUTTON_START  7 

#define MD_SCREEN_WIDTH		320
#define MD_SCREEN_HEIGHT	240

#define SMS_SCREEN_WIDTH	240
#define SMS_SCREEN_HEIGHT	192

#define GG_SCREEN_WIDTH		160
#define GG_SCREEN_HEIGHT	144

void MDInitEmulation(void);

#endif /* _MAIN_H_ */

