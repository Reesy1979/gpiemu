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
#ifndef _GETSET_H_
#define _GETSET_H_

#include "ppu.h"
#include "dsp1.h"
#include "cpuexec.h"
#include "sa1.h"

INLINE uint8 FASTCALL S9xGetByte (uint32 Address, struct SCPUState * cpu)
{
#if defined(VAR_CYCLES) || defined(CPU_SHUTDOWN)
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#else
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#endif    
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef VAR_CYCLES
	cpu->Cycles += Memory.MemorySpeed [block];
#endif
#ifdef CPU_SHUTDOWN
	if (Memory.BlockIsRAM [block])
	    cpu->WaitAddress = cpu->PCAtOpcodeStart;
#endif
	return (*(GetAddress + (Address & 0xffff)));
    }

    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
#ifdef VAR_CYCLES
	if (!cpu->InDMA)
	    cpu->Cycles += ONE_CYCLE;
#endif	
	return (S9xGetPPU (Address & 0xffff));
    case CMemory::MAP_CPU:
#ifdef VAR_CYCLES
	cpu->Cycles += ONE_CYCLE;
#endif
	return (S9xGetCPU (Address & 0xffff));
    case CMemory::MAP_DSP:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif	
	return (S9xGetDSP (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	return (*(Memory.SRAM + ((Address & Memory.SRAMMask))));

    case CMemory::MAP_HIROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	return (*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 +
				  ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)));

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("R(B) %06x\n", Address);
#endif

    case CMemory::MAP_BWRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	return (*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)));

    case CMemory::MAP_C4:
	return (S9xGetC4 (Address & 0xffff));
    
    default:
    case CMemory::MAP_NONE:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
#ifdef DEBUGGER
	printf ("R(B) %06x\n", Address);
#endif
	return ((Address >> 8) & 0xff);
    }
}

INLINE uint16 S9xGetWord (uint32 Address, struct SCPUState * cpu)
{
    if (Address == 0x00001fff)
    {
	return (S9xGetByte (Address, cpu) | (S9xGetByte (Address + 1, cpu) << 8));
    }
#if defined(VAR_CYCLES) || defined(CPU_SHUTDOWN)
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#else
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#endif    
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef VAR_CYCLES
	cpu->Cycles += Memory.MemorySpeed [block] << 1;
#endif
#ifdef CPU_SHUTDOWN
	if (Memory.BlockIsRAM [block])
	    cpu->WaitAddress = cpu->PCAtOpcodeStart;
#endif
#ifdef FAST_LSB_WORD_ACCESS
	return (*(uint16 *) (GetAddress + (Address & 0xffff)));
#else
	return (*(GetAddress + (Address & 0xffff)) |
		(*(GetAddress + (Address & 0xffff) + 1) << 8));
#endif	
    }

    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
#ifdef VAR_CYCLES
	if (!cpu->InDMA)
	    cpu->Cycles += TWO_CYCLES;
#endif	
	return (S9xGetPPU (Address & 0xffff) |
		(S9xGetPPU ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_CPU:
#ifdef VAR_CYCLES   
	cpu->Cycles += TWO_CYCLES;
#endif
	return (S9xGetCPU (Address & 0xffff) |
		(S9xGetCPU ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_DSP:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif	
	return (S9xGetDSP (Address & 0xffff) |
		(S9xGetDSP ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	return (*(Memory.SRAM + (Address & Memory.SRAMMask)) |
		(*(Memory.SRAM + ((Address + 1) & Memory.SRAMMask)) << 8));

    case CMemory::MAP_HIROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	return (*(Memory.SRAM +
		  (((Address & 0x7fff) - 0x6000 +
		    ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) |
		(*(Memory.SRAM +
		   ((((Address + 1) & 0x7fff) - 0x6000 +
		     (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask)) << 8));

    case CMemory::MAP_BWRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	return (*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) |
		(*(Memory.BWRAM + (((Address + 1) & 0x7fff) - 0x6000)) << 8));

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("R(W) %06x\n", Address);
#endif

    case CMemory::MAP_C4:
	return (S9xGetC4 (Address & 0xffff) |
		(S9xGetC4 ((Address + 1) & 0xffff) << 8));
    
    default:
    case CMemory::MAP_NONE:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
#ifdef DEBUGGER
	printf ("R(W) %06x\n", Address);
#endif
	return (((Address >> 8) | (Address & 0xff00)) & 0xffff);
    }
}

INLINE void S9xSetByte (uint8 Byte, uint32 Address, struct SCPUState * cpu)
{
#if defined(CPU_SHUTDOWN)
    cpu->WaitAddress = NULL;
#endif
#if defined(VAR_CYCLES)
    int block;
    uint8 *SetAddress = Memory.WriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];
#else
    uint8 *SetAddress = Memory.WriteMap [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#endif

    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef VAR_CYCLES
	cpu->Cycles += Memory.MemorySpeed [block];
#endif
#ifdef CPU_SHUTDOWN
	SetAddress += Address & 0xffff;
	if (SetAddress == SA1.WaitByteAddress1 ||
	    SetAddress == SA1.WaitByteAddress2)
	{
	    SA1.Executing = SA1ICPU.S9xOpcodes != NULL;
	    SA1.WaitCounter = 0;
	}
	*SetAddress = Byte;
#else
	*(SetAddress + (Address & 0xffff)) = Byte;
#endif
	return;
    }

    switch ((int) SetAddress)
    {
    case CMemory::MAP_PPU:
#ifdef VAR_CYCLES
	if (!cpu->InDMA)
	    cpu->Cycles += ONE_CYCLE;
#endif	
	S9xSetPPU (Byte, Address & 0xffff);
	return;

    case CMemory::MAP_CPU:
#ifdef VAR_CYCLES   
	cpu->Cycles += ONE_CYCLE;
#endif
	S9xSetCPU (Byte, Address & 0xffff);
	return;

    case CMemory::MAP_DSP:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif	
	S9xSetDSP (Byte, Address & 0xffff);
	return;

    case CMemory::MAP_LOROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	if (Memory.SRAMMask)
	{
	    *(Memory.SRAM + (Address & Memory.SRAMMask)) = Byte;
	    cpu->SRAMModified = TRUE;
	}
	return;

    case CMemory::MAP_HIROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	if (Memory.SRAMMask)
	{
	    *(Memory.SRAM + (((Address & 0x7fff) - 0x6000 +
			      ((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) = Byte;
	    cpu->SRAMModified = TRUE;
	}
	return;

    case CMemory::MAP_BWRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = Byte;
	cpu->SRAMModified = TRUE;
	return;

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("W(B) %06x\n", Address);
#endif

    case CMemory::MAP_SA1RAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	*(Memory.SRAM + (Address & 0xffff)) = Byte;
	SA1.Executing = !SA1.Waiting;
	break;

    case CMemory::MAP_C4:
	S9xSetC4 (Byte, Address & 0xffff);
	return;
	
    default:
    case CMemory::MAP_NONE:
#ifdef VAR_CYCLES    
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif	
#ifdef DEBUGGER
	printf ("W(B) %06x\n", Address);
#endif
	return;
    }
}

INLINE void FASTCALL S9xSetWord (uint16 Word, uint32 Address, struct SCPUState * cpu)
{
#if defined(CPU_SHUTDOWN)
    cpu->WaitAddress = NULL;
#endif
#if defined (VAR_CYCLES)
    int block;
    uint8 *SetAddress = Memory.WriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];
#else
    uint8 *SetAddress = Memory.WriteMap [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#endif

    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef VAR_CYCLES
	cpu->Cycles += Memory.MemorySpeed [block] << 1;
#endif
#ifdef CPU_SHUTDOWN
	SetAddress += Address & 0xffff;
	if (SetAddress == SA1.WaitByteAddress1 ||
	    SetAddress == SA1.WaitByteAddress2)
	{
	    SA1.Executing = SA1ICPU.S9xOpcodes != NULL;
	    SA1.WaitCounter = 0;
	}
#ifdef FAST_LSB_WORD_ACCESS
	*(uint16 *) SetAddress = Word;
#else
	*SetAddress = (uint8) Word;
	*(SetAddress + 1) = Word >> 8;
#endif
#else
#ifdef FAST_LSB_WORD_ACCESS
	*(uint16 *) (SetAddress + (Address & 0xffff)) = Word;
#else
	*(SetAddress + (Address & 0xffff)) = (uint8) Word;
	*(SetAddress + ((Address + 1) & 0xffff)) = Word >> 8;
#endif
#endif
	return;
    }

    switch ((int) SetAddress)
    {
    case CMemory::MAP_PPU:
#ifdef VAR_CYCLES
	if (!cpu->InDMA)
	    cpu->Cycles += TWO_CYCLES;
#endif	
	S9xSetPPU ((uint8) Word, Address & 0xffff);
	S9xSetPPU (Word >> 8, (Address & 0xffff) + 1);
	return;

    case CMemory::MAP_CPU:
#ifdef VAR_CYCLES   
	cpu->Cycles += TWO_CYCLES;
#endif
	S9xSetCPU ((uint8) Word, (Address & 0xffff));
	S9xSetCPU (Word >> 8, (Address & 0xffff) + 1);
	return;

    case CMemory::MAP_DSP:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif	
	S9xSetDSP ((uint8) Word, (Address & 0xffff));
	S9xSetDSP (Word >> 8, (Address & 0xffff) + 1);
	return;

    case CMemory::MAP_LOROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	if (Memory.SRAMMask)
	{
	    *(Memory.SRAM + (Address & Memory.SRAMMask)) = (uint8) Word;
	    *(Memory.SRAM + ((Address + 1) & Memory.SRAMMask)) = Word >> 8;
	    cpu->SRAMModified = TRUE;
	}
	return;

    case CMemory::MAP_HIROM_SRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	if (Memory.SRAMMask)
	{
	    *(Memory.SRAM + 
	      (((Address & 0x7fff) - 0x6000 +
		((Address & 0xf0000) >> MEMMAP_SHIFT) & Memory.SRAMMask))) = (uint8) Word;
	    *(Memory.SRAM + 
	      ((((Address + 1) & 0x7fff) - 0x6000 +
		(((Address + 1) & 0xf0000) >> MEMMAP_SHIFT) & Memory.SRAMMask))) = (uint8) (Word >> 8);
	    cpu->SRAMModified = TRUE;
	}
	return;

    case CMemory::MAP_BWRAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
	*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = (uint8) Word;
	*(Memory.BWRAM + (((Address + 1) & 0x7fff) - 0x6000)) = (uint8) (Word >> 8);
	cpu->SRAMModified = TRUE;
	return;

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("W(W) %06x\n", Address);
#endif

    case CMemory::MAP_SA1RAM:
#ifdef VAR_CYCLES
	cpu->Cycles += SLOW_ONE_CYCLE;
#endif
	*(Memory.SRAM + (Address & 0xffff)) = (uint8) Word;
	*(Memory.SRAM + ((Address + 1) & 0xffff)) = (uint8) (Word >> 8);
	SA1.Executing = !SA1.Waiting;
	break;

    case CMemory::MAP_C4:
	S9xSetC4 (Word & 0xff, Address & 0xffff);
	S9xSetC4 ((uint8) (Word >> 8), (Address + 1) & 0xffff);
	return;
	
    default:
    case CMemory::MAP_NONE:
#ifdef VAR_CYCLES    
	cpu->Cycles += SLOW_ONE_CYCLE * 2;
#endif
#ifdef DEBUGGER
	printf ("W(W) %06x\n", Address);
#endif
	return;
    }
}

INLINE uint8 *GetBasePointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
	return (GetAddress);

    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
	return (Memory.FillRAM - 0x2000);
    case CMemory::MAP_CPU:
	return (Memory.FillRAM - 0x4000);
    case CMemory::MAP_DSP:
	return (Memory.FillRAM - 0x6000);
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
	return (Memory.SRAM);
    case CMemory::MAP_BWRAM:
	return (Memory.BWRAM - 0x6000);
    case CMemory::MAP_HIROM_SRAM:
	return (Memory.SRAM - 0x6000);
    case CMemory::MAP_C4:
	return (Memory.C4RAM - 0x6000);
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("GBP %06x\n", Address);
#endif

    default:
    case CMemory::MAP_NONE:
#ifdef DEBUGGER
	printf ("GBP %06x\n", Address);
#endif
	return (0);
    }
}

INLINE uint8 *S9xGetMemPointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
	return (GetAddress + (Address & 0xffff));

    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
	return (Memory.FillRAM - 0x2000 + (Address & 0xffff));
    case CMemory::MAP_CPU:
	return (Memory.FillRAM - 0x4000 + (Address & 0xffff));
    case CMemory::MAP_DSP:
	return (Memory.FillRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
	return (Memory.SRAM + (Address & 0xffff));
    case CMemory::MAP_BWRAM:
	return (Memory.BWRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_HIROM_SRAM:
	return (Memory.SRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_C4:
	return (Memory.C4RAM - 0x6000 + (Address & 0xffff));

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("GMP %06x\n", Address);
#endif
    default:
    case CMemory::MAP_NONE:
#ifdef DEBUGGER
	printf ("GMP %06x\n", Address);
#endif
	return (0);
    }
}

INLINE void FASTCALL S9xSetPCBase (uint32 Address, struct SCPUState * cpu)
{
#ifdef VAR_CYCLES
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#else
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
#endif    
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef VAR_CYCLES
	cpu->MemSpeed = Memory.MemorySpeed [block];
	cpu->MemSpeedx2 = cpu->MemSpeed << 1;
#endif
	cpu->PCBase = GetAddress;
	cpu->PC = GetAddress + (Address & 0xffff);
	return;
    }

    switch ((int) GetAddress)
    {
    case CMemory::MAP_PPU:
#ifdef VAR_CYCLES
	cpu->MemSpeed = ONE_CYCLE;
	cpu->MemSpeedx2 = TWO_CYCLES;
#endif	
	cpu->PCBase = Memory.FillRAM - 0x2000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;
	
    case CMemory::MAP_CPU:
#ifdef VAR_CYCLES   
	cpu->MemSpeed = ONE_CYCLE;
	cpu->MemSpeedx2 = TWO_CYCLES;
#endif
	cpu->PCBase = Memory.FillRAM - 0x4000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;
	
    case CMemory::MAP_DSP:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif	
	cpu->PCBase = Memory.FillRAM - 0x6000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;
	
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif
	cpu->PCBase = Memory.SRAM;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;

    case CMemory::MAP_BWRAM:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif
	cpu->PCBase = Memory.BWRAM - 0x6000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;
    case CMemory::MAP_HIROM_SRAM:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif
	cpu->PCBase = Memory.SRAM - 0x6000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;

    case CMemory::MAP_C4:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif
	cpu->PCBase = Memory.C4RAM - 0x6000;
	cpu->PC = cpu->PCBase + (Address & 0xffff);
	return;

    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
	printf ("SBP %06x\n", Address);
#endif
	
    default:
    case CMemory::MAP_NONE:
#ifdef VAR_CYCLES
	cpu->MemSpeed = SLOW_ONE_CYCLE;
	cpu->MemSpeedx2 = SLOW_ONE_CYCLE * 2;
#endif
#ifdef DEBUGGER
	printf ("SBP %06x\n", Address);
#endif
	cpu->PCBase = Memory.SRAM;
	cpu->PC = Memory.SRAM + (Address & 0xffff);
	return;
    }
}
#endif
