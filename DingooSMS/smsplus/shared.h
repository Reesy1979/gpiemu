#ifndef _SHARED_H_
#define _SHARED_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <math.h>
#include <limits.h>
#include <zlib.h>

#include "z80.h"
#include "sms.h"
#include "pio.h"
#include "memz80.h"
#include "vdp.h"
#include "render.h"
#include "tms.h"
#include "sn76489.h"
#include "emu2413.h"
#include "ym2413.h"
#include "fmintf.h"
#include "sound.h"
#include "system.h"
//@@#include "error.h"
#include "loadrom.h"
#include "state.h"

#ifdef __cplusplus
} // End of extern "C"
#endif

#endif /* _SHARED_H_ */
