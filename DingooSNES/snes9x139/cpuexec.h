/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
#ifndef _CPUEXEC_H_
#define _CPUEXEC_H_
#include "ppu.h"
#include "memmap.h"
#include "65c816.h"

#define DO_HBLANK_CHECK() \
    if (CPU.Cycles >= CPU.NextEvent) \
	S9xDoHBlankProcessing ();

struct SOpcodes {
#ifdef __WIN32__
	void (__cdecl *S9xOpcode)(struct SRegisters *, struct SICPU *, struct SCPUState *);
#else
	void (*S9xOpcode)(struct SRegisters *, struct SICPU *, struct SCPUState *);
#endif
};



START_EXTERN_C
#ifdef THREADCPU
void S9xMainLoop (struct SRegisters *, struct SICPU *, struct SCPUState *);
#else
void S9xMainLoop (void);
#endif
void S9xReset (void);
void S9xDoHBlankProcessing ();
void S9xClearIRQ (uint32);
void S9xSetIRQ (uint32);

extern struct SOpcodes S9xOpcodesM1X1 [256];
extern struct SOpcodes S9xOpcodesM1X0 [256];
extern struct SOpcodes S9xOpcodesM0X1 [256];
extern struct SOpcodes S9xOpcodesM0X0 [256];

#ifndef VAR_CYCLES
extern uint8 S9xE1M1X1 [256];
extern uint8 S9xE0M1X0 [256];
extern uint8 S9xE0M1X1 [256];
extern uint8 S9xE0M0X0 [256];
extern uint8 S9xE0M0X1 [256];
#endif

extern struct SICPU ICPU;
END_EXTERN_C

#define S9xUnpackStatus_OP() \
{ \
    icpu->_Zero = (reg->PL & Zero) == 0; \
    icpu->_Negative = (reg->PL & Negative); \
    icpu->_Carry = (reg->PL & Carry); \
    icpu->_Overflow = (reg->PL & Overflow) >> 6; \
}

#define S9xPackStatus_OP() \
{ \
    reg->PL &= ~(Zero | Negative | Carry | Overflow); \
    reg->PL |= (icpu->_Carry & 0xff) | (((icpu->_Zero & 0xff) == 0) << 1) | \
		    (icpu->_Negative & 0x80) | ((icpu->_Overflow & 0xff) << 6); \
}

STATIC INLINE VOID S9xUnpackStatus() 
{ 
    ICPU._Zero = (Registers.PL & Zero) == 0; 
    ICPU._Negative = (Registers.PL & Negative); 
    ICPU._Carry = (Registers.PL & Carry); \
    ICPU._Overflow = (Registers.PL & Overflow) >> 6; 
}

STATIC INLINE VOID S9xPackStatus() 
{ 
    Registers.PL &= ~(Zero | Negative | Carry | Overflow); 
    Registers.PL |= (ICPU._Carry & 0xff) | (((ICPU._Zero & 0xff) == 0) << 1) | 
		    (ICPU._Negative & 0x80) | ((ICPU._Overflow & 0xff) << 6); 
}

STATIC INLINE void CLEAR_IRQ_SOURCE (uint32 M)
{
    CPU.IRQActive &= ~M;
    if (!CPU.IRQActive)
	CPU.Flags &= ~IRQ_PENDING_FLAG;
}
	
STATIC INLINE void S9xFixCycles (struct SRegisters * reg, struct SICPU * icpu)
{
    if (CHECKEMULATION())
    {
#ifndef VAR_CYCLES
	icpu->Speed = S9xE1M1X1;
#endif
	icpu->S9xOpcodes = S9xOpcodesM1X1;
    }
    else
    if (CHECKMEMORY())
    {
	if (CHECKINDEX())
	{
#ifndef VAR_CYCLES
	    icpu->Speed = S9xE0M1X1;
#endif
	    icpu->S9xOpcodes = S9xOpcodesM1X1;
	}
	else
	{
#ifndef VAR_CYCLES
	    icpu->Speed = S9xE0M1X0;
#endif
	    icpu->S9xOpcodes = S9xOpcodesM1X0;
	}
    }
    else
    {
	if (CHECKINDEX())
	{
#ifndef VAR_CYCLES
	    icpu->Speed = S9xE0M0X1;
#endif
	    icpu->S9xOpcodes = S9xOpcodesM0X1;
	}
	else
	{
#ifndef VAR_CYCLES
	    icpu->Speed = S9xE0M0X0;
#endif
	    icpu->S9xOpcodes = S9xOpcodesM0X0;
	}
    }
}

STATIC INLINE void S9xReschedule ()
{
    uint8 which;
    long max;
    
    if (CPU.WhichEvent == HBLANK_START_EVENT ||
	CPU.WhichEvent == HTIMER_AFTER_EVENT)
    {
	which = HBLANK_END_EVENT;
	max = Settings.H_Max;
    }
    else
    {
	which = HBLANK_START_EVENT;
	max = Settings.HBlankStart;
    }

    if (PPU.HTimerEnabled &&
        (long) PPU.HTimerPosition < max &&
	(long) PPU.HTimerPosition > CPU.NextEvent &&
	(!PPU.VTimerEnabled ||
	 (PPU.VTimerEnabled && CPU.V_Counter == PPU.IRQVBeamPos)))
    {
	which = (long) PPU.HTimerPosition < Settings.HBlankStart ?
			HTIMER_BEFORE_EVENT : HTIMER_AFTER_EVENT;
	max = PPU.HTimerPosition;
    }
    CPU.NextEvent = max;
    CPU.WhichEvent = which;
}

#endif
