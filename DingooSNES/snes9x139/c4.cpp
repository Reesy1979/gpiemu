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
#include <math.h>
#include <stdlib.h>

extern "C" {

short C4WFXVal;
short C4WFYVal;
short C4WFZVal;
short C4WFX2Val;
short C4WFY2Val;
short C4WFDist;
short C4WFScale;

static SysDDec tanval;
static SysDDec c4x, c4y, c4z;
static SysDDec c4x2, c4y2, c4z2;

void C4TransfWireFrame ()
{
    c4x = (SysDDec) C4WFXVal;
    c4y = (SysDDec) C4WFYVal;
    c4z = (SysDDec) C4WFZVal - 0x95;
    
    // Rotate X
    tanval = -(SysDDec) C4WFX2Val * 3.14159265 * 2 / 128;
    c4y2 = c4y * cos (tanval) - c4z * sin (tanval);
    c4z2 = c4y * sin (tanval) + c4z * cos (tanval);
    
    // Rotate Y
    tanval = -(SysDDec)C4WFY2Val*3.14159265*2/128;
    c4x2 = c4x * cos (tanval) + c4z2 * sin (tanval);
    c4z = c4x * - sin (tanval) + c4z2 * cos (tanval);
    
    // Rotate Z
    tanval = -(SysDDec) C4WFDist * 3.14159265*2 / 128;
    c4x = c4x2 * cos (tanval) - c4y2 * sin (tanval);
    c4y = c4x2 * sin (tanval) + c4y2 * cos (tanval);
    
    // Scale
    C4WFXVal = (short) (c4x*(SysDDec)C4WFScale/(0x90*(c4z+0x95))*0x95);
    C4WFYVal = (short) (c4y*(SysDDec)C4WFScale/(0x90*(c4z+0x95))*0x95);
}

void C4TransfWireFrame2 ()
{
    c4x = (SysDDec)C4WFXVal;
    c4y = (SysDDec)C4WFYVal;
    c4z = (SysDDec)C4WFZVal;
    
    // Rotate X
    tanval = -(SysDDec) C4WFX2Val * 3.14159265 * 2 / 128;
    c4y2 = c4y * cos (tanval) - c4z * sin (tanval);
    c4z2 = c4y * sin (tanval) + c4z * cos (tanval);
    
    // Rotate Y
    tanval = -(SysDDec) C4WFY2Val * 3.14159265 * 2 / 128;
    c4x2 = c4x * cos (tanval) + c4z2 * sin (tanval);
    c4z = c4x * -sin (tanval) + c4z2 * cos (tanval);
    
    // Rotate Z
    tanval = -(SysDDec)C4WFDist * 3.14159265 * 2 / 128;
    c4x = c4x2 * cos (tanval) - c4y2 * sin (tanval);
    c4y = c4x2 * sin (tanval) + c4y2 * cos (tanval);
    
    // Scale
    C4WFXVal =(short)(c4x * (SysDDec)C4WFScale / 0x100);
    C4WFYVal =(short)(c4y * (SysDDec)C4WFScale / 0x100);
}

void C4CalcWireFrame ()
{
    C4WFXVal = C4WFX2Val - C4WFXVal;
    C4WFYVal = C4WFY2Val - C4WFYVal;
    if (abs (C4WFXVal) > abs (C4WFYVal))
    {
        C4WFDist = abs (C4WFXVal) + 1;
        C4WFYVal = (short) (256 * (SysDDec) C4WFYVal / abs (C4WFXVal));
        if (C4WFXVal < 0)
            C4WFXVal = -256;
        else 
            C4WFXVal = 256;
    }
    else
    {
        if (C4WFYVal != 0) 
        {
            C4WFDist = abs(C4WFYVal)+1;
            C4WFXVal = (short) (256 * (SysDDec)C4WFXVal / abs (C4WFYVal));
            if (C4WFYVal < 0)
                C4WFYVal = -256;
            else 
                C4WFYVal = 256;
        }
        else 
            C4WFDist = 0;
    }
}

short C41FXVal;
short C41FYVal;
short C41FAngleRes;
short C41FDist;
short C41FDistVal;

void C4Op1F ()
{
    if (C41FXVal == 0) 
    {
        if (C41FYVal > 0) 
            C41FAngleRes = 0x80;
        else 
            C41FAngleRes = 0x180;
    }
    else 
    {
        tanval = (SysDDec) C41FYVal / C41FXVal;
        C41FAngleRes = (short) (atan (tanval) / (3.141592675 * 2) * 512);
        C41FAngleRes = C41FAngleRes;
        if (C41FXVal< 0) 
            C41FAngleRes += 0x100;
        C41FAngleRes &= 0x1FF;
    }
}

void C4Op15()
{
    tanval = sqrt ((SysDDec) C41FYVal * C41FYVal + (SysDDec) C41FXVal * C41FXVal);
    C41FDist = (short) tanval;
}

void C4Op0D()
{
    tanval = sqrt ((SysDDec) C41FYVal * C41FYVal + (SysDDec) C41FXVal * C41FXVal);
    tanval = C41FDistVal / tanval;
    C41FYVal = (short) (C41FYVal * tanval * 0.99);
    C41FXVal = (short) (C41FXVal * tanval * 0.98);
}
}
