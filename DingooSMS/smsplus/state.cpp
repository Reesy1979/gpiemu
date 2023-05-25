/******************************************************************************
 *  Sega Master System / GameGear Emulator
 *  Copyright (C) 1998-2007  Charles MacDonald
 *
 *  additionnal code by Eke-Eke (SMS Plus GX)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Nintendo Gamecube State Management
 *
 ******************************************************************************/

#include "shared.h"
#include "state.io.h"

void system_save_state()
{
    /* Save VDP context */
    state_fwrite(&vdp, sizeof(vdp_t));

    /* Save SMS context */
    state_fwrite(&sms, sizeof(sms_t));

	/* Save cart info */
    state_fwrite(cart.fcr, 4);
	
    /* Save SRAM */
    state_fwrite(cart.sram, 0x8000);

    /* Save Z80 context */
    state_fwrite(&Z80, sizeof(Z80_Regs));

    /* Save YM2413 context */
    state_fwrite(FM_GetContextPtr(), FM_GetContextSize());

    /* Save SN76489 context */
    state_fwrite(SN76489_GetContextPtr(0), SN76489_GetContextSize());
}

void system_load_state()
{
    uint8 *buf;

    /* Initialize everything */
    system_reset();

    /* Load VDP context */
    state_fread(&vdp, sizeof(vdp_t));

    /* Load SMS context */
    state_fread(&sms, sizeof(sms_t));
	
	/* Restore video & audio settings (needed if timing changed) */
	vdp_init();
//	sound_init();

	/* Set cart info */
    state_fread(cart.fcr, 4);
	
    /* Load SRAM content */
    state_fread(cart.sram, 0x8000);

    /* Load Z80 context */
    state_fread(&Z80, sizeof(Z80_Regs));
    Z80.irq_callback = sms_irq_callback;

    /* Load YM2413 context */
    buf = (uint8*)malloc(FM_GetContextSize());
    state_fread(buf, FM_GetContextSize());
    FM_SetContext(buf);
    free(buf);

    /* Load SN76489 context */
    buf = (uint8*)malloc(SN76489_GetContextSize());
    state_fread(buf, SN76489_GetContextSize());
    SN76489_SetContext(0, buf);
    free(buf);

    if ((sms.console != CONSOLE_COLECO) && (sms.console != CONSOLE_SG1000))
    {
        // Cartridge by default
        slot.rom = cart.rom;
        slot.pages = cart.pages;
        slot.mapper = cart.mapper;
        slot.fcr = &cart.fcr[0];

        // Restore mapping
        mapper_reset();
        cpu_readmap[0]  = &slot.rom[0];
        if (slot.mapper != MAPPER_KOREA_MSX)
        {
            mapper_16k_w(0, slot.fcr[0]);
            mapper_16k_w(1, slot.fcr[1]);
            mapper_16k_w(2, slot.fcr[2]);
            mapper_16k_w(3, slot.fcr[3]);
        }
        else
        {
            mapper_8k_w(0, slot.fcr[0]);
            mapper_8k_w(1, slot.fcr[1]);
            mapper_8k_w(2, slot.fcr[2]);
            mapper_8k_w(3, slot.fcr[3]);
        }
    }

    /* Force full pattern cache update */
    bg_list_index = 0x200;
    for(int i = 0; i < 0x200; i++)
    {
        bg_name_list[i] = i;
        bg_name_dirty[i] = -1;
    }

    /* Restore palette */
    for(int i = 0; i < REAL_PALETTE_SIZE; i++)
        palette_sync(i);
}

