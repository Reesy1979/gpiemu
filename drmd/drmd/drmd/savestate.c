#include "sal.h"
#include "main.h"
#include "cyclone.h"
#include "drz80.h"
#include "drmd.h"
#include "menu.h"
#include "globals.h"
//#include "fm.h"
#include "sn76496.h"
#include "savestate.h"


void savestate_mem(char *saveaddress)
{
	switch(gCurrentEmuMode)
	{
		case EMU_MODE_MD:
			{
				struct MD_SAVESTATE_V6 *s=(struct MD_SAVESTATE_V6*)saveaddress;
				memset(s,0,gSaveStateSize);
				s->drmd_lines_per_frame=gDrmd.lines_per_frame;
				s->drmd_vdp_line=gDrmd.vdp_line;
				s->drmd_vdp_status=gDrmd.vdp_status;
				s->drmd_vdp_addr=gDrmd.vdp_addr;
				s->drmd_vdp_addr_latch=gDrmd.vdp_addr_latch;
				s->drmd_gamma=gDrmd.gamma;
				s->drmd_cpl_m68k=gDrmd.cpl_m68k;
				s->drmd_cpl_z80=gDrmd.cpl_z80;
				s->drmd_padselect=gDrmd.padselect;
				s->drmd_zbusreq=gDrmd.zbusreq;
				s->drmd_zbusack=gDrmd.zbusack;
				s->drmd_zreset=gDrmd.zreset;
				s->drmd_vdp_reg0=gDrmd.vdp_reg0;
				s->drmd_vdp_reg1=gDrmd.vdp_reg1;
				s->drmd_vdp_reg2=gDrmd.vdp_reg2;
				s->drmd_vdp_reg3=gDrmd.vdp_reg3;
				s->drmd_vdp_reg4=gDrmd.vdp_reg4;
				s->drmd_vdp_reg5=gDrmd.vdp_reg5;
				s->drmd_vdp_reg6=gDrmd.vdp_reg6;
				s->drmd_vdp_reg7=gDrmd.vdp_reg7;
				s->drmd_vdp_reg8=gDrmd.vdp_reg8;
				s->drmd_vdp_reg9=gDrmd.vdp_reg9;
				s->drmd_vdp_reg10=gDrmd.vdp_reg10;
				s->drmd_vdp_reg11=gDrmd.vdp_reg11;
				s->drmd_vdp_reg12=gDrmd.vdp_reg12;
				s->drmd_vdp_reg13=gDrmd.vdp_reg13;
				s->drmd_vdp_reg14=gDrmd.vdp_reg14;
				s->drmd_vdp_reg15=gDrmd.vdp_reg15;
				s->drmd_vdp_reg16=gDrmd.vdp_reg16;
				s->drmd_vdp_reg17=gDrmd.vdp_reg17;
				s->drmd_vdp_reg18=gDrmd.vdp_reg18;
				s->drmd_vdp_reg19=gDrmd.vdp_reg19;
				s->drmd_vdp_reg20=gDrmd.vdp_reg20;
				s->drmd_vdp_reg21=gDrmd.vdp_reg21;
				s->drmd_vdp_reg22=gDrmd.vdp_reg22;
				s->drmd_vdp_reg23=gDrmd.vdp_reg23;
				s->drmd_vdp_reg24=gDrmd.vdp_reg24;
				s->drmd_vdp_reg25=gDrmd.vdp_reg25;
				s->drmd_vdp_reg26=gDrmd.vdp_reg26;
				s->drmd_vdp_reg27=gDrmd.vdp_reg27;
				s->drmd_vdp_reg28=gDrmd.vdp_reg28;
				s->drmd_vdp_reg29=gDrmd.vdp_reg29;
				s->drmd_vdp_reg30=gDrmd.vdp_reg30;
				s->drmd_vdp_reg31=gDrmd.vdp_reg31;
				s->drmd_vdp_reg32=gDrmd.vdp_reg32;
				s->drmd_vdp_counter=gDrmd.vdp_counter;
				s->drmd_hint_pending=gDrmd.hint_pending;
				s->drmd_vint_pending=gDrmd.vint_pending;
				s->drmd_vdp_pending=gDrmd.vdp_pending;
				s->drmd_vdp_code=gDrmd.vdp_code;
				s->drmd_vdp_dma_fill=gDrmd.vdp_dma_fill;
				s->drmd_pad_1_status=gDrmd.pad_1_status;
				s->drmd_pad_1_com=gDrmd.pad_1_com;
				s->drmd_pad_2_status=gDrmd.pad_2_status;
				s->drmd_pad_2_com=gDrmd.pad_2_com;
				s->drmd_sram_start=gDrmd.sram_start;
				s->drmd_sram_end=gDrmd.sram_end;
				s->drmd_sram=gDrmd.sram;
				s->drmd_region=gDrmd.region;
				s->drmd_sram_flags=gDrmd.sram_flags;
				s->drmd_cpl_fm=gDrmd.cpl_fm;
				memcpy(s->drmd_genesis_rom_banks,gDrmd.genesis_rom_banks,0x10);
				s->drmd_pad=gDrmd.pad;
				s->drmd_pad_1_counter=gDrmd.pad_1_counter;
				s->drmd_pad_1_delay=gDrmd.pad_1_delay;
				s->drmd_pad_2_counter=gDrmd.pad_2_counter;
				s->drmd_pad_2_delay=gDrmd.pad_2_delay;
				s->drz80_Z80PC=gDrz80.Z80PC;
				s->drz80_Z80A=gDrz80.Z80A;
				s->drz80_Z80F=gDrz80.Z80F;
				s->drz80_Z80BC=gDrz80.Z80BC;
				s->drz80_Z80DE=gDrz80.Z80DE;
				s->drz80_Z80HL=gDrz80.Z80HL;
				s->drz80_Z80SP=gDrz80.Z80SP;
				s->drz80_Z80PC_BASE=gDrz80.Z80PC_BASE;
				s->drz80_Z80SP_BASE=gDrz80.Z80SP_BASE;
				s->drz80_Z80IX=gDrz80.Z80IX;
				s->drz80_Z80IY=gDrz80.Z80IY;
				s->drz80_Z80I=gDrz80.Z80I;
				s->drz80_Z80A2=gDrz80.Z80A2;
				s->drz80_Z80F2=gDrz80.Z80F2;
				s->drz80_Z80BC2=gDrz80.Z80BC2;
				s->drz80_Z80DE2=gDrz80.Z80DE2;
				s->drz80_Z80HL2=gDrz80.Z80HL2;
				s->drz80_Z80_IRQ=gDrz80.Z80_IRQ;
				s->drz80_Z80IF=gDrz80.Z80IF;
				s->drz80_Z80IM=gDrz80.Z80IM;
				s->drz80_Z80R=gDrz80.Z80R;
				s->drz80_z80irqvector=gDrz80.z80irqvector;
				s->drz80_Z80_NMI=gDrz80.Z80_NMI;
				memcpy(&s->cyclone_d,&gCyclone.d,0x8*4);
				memcpy(&s->cyclone_a,&gCyclone.a,0x8*4);
				s->cyclone_pc=gCyclone.pc;
				s->cyclone_srh=gCyclone.srh;
				s->cyclone_xc=gCyclone.xc;
				s->cyclone_flags=gCyclone.flags;
				s->cyclone_irq=gCyclone.irq;
				s->cyclone_osp=gCyclone.osp;
				s->cyclone_vector=gCyclone.vector;
				s->cyclone_stopped=gCyclone.stopped;
				s->cyclone_cycles=gCyclone.cycles;
				s->cyclone_membase=gCyclone.membase;
				memcpy(s->work_ram,gWorkRam,0x10000);
				memcpy(&s->vram,&gVram,0x10000);
				memcpy(s->zram,gZram,0x4000);
				memcpy(s->cram,gCram,0x80);
				memcpy(s->vsram,gVsram,0x80);
				//memcpy(&s->SL3,&SL3,sizeof(SL3));
				//memcpy(&s->ST,&ST,sizeof(ST));
				//memcpy(&s->OPN,&OPN,sizeof(OPN));
				//memcpy(&s->CH,&CH,sizeof(CH));
				//s->dacout=dacout;
				//s->dacen=dacen;
				//memcpy(s->OPN_pan,OPN_pan,6*2);
				memcpy(&s->PSG,&PSG,sizeof(PSG));
				//s->YMOPN_ST_dt_tab=(unsigned int)&YMOPN_ST_dt_tab[0];
				memcpy(s->sram,gSram,0x10000);
				break;
			}
	}
}

void loadstate_mem(char *loadaddress)
{
   int x=0,y=0;
   unsigned int old_DT_Table;
   switch(gCurrentEmuMode)
	{
		case EMU_MODE_MD:
			{
				struct MD_SAVESTATE_V6 *s=(struct MD_SAVESTATE_V6*)loadaddress;
				gDrmd.lines_per_frame=s->drmd_lines_per_frame;
				gDrmd.vdp_line=s->drmd_vdp_line;
				gDrmd.vdp_status=s->drmd_vdp_status;
				gDrmd.vdp_addr=s->drmd_vdp_addr;
				gDrmd.vdp_addr_latch=s->drmd_vdp_addr_latch;
				gDrmd.gamma=s->drmd_gamma;
				gDrmd.cpl_m68k=s->drmd_cpl_m68k;
				gDrmd.cpl_z80=s->drmd_cpl_z80;
				gDrmd.padselect=s->drmd_padselect;
				gDrmd.zbusreq=s->drmd_zbusreq;
				gDrmd.zbusack=s->drmd_zbusack;
				gDrmd.zreset=s->drmd_zreset;
				gDrmd.vdp_reg0=s->drmd_vdp_reg0;
				gDrmd.vdp_reg1=s->drmd_vdp_reg1;
				gDrmd.vdp_reg2=s->drmd_vdp_reg2;
				gDrmd.vdp_reg3=s->drmd_vdp_reg3;
				gDrmd.vdp_reg4=s->drmd_vdp_reg4;
				gDrmd.vdp_reg5=s->drmd_vdp_reg5;
				gDrmd.vdp_reg6=s->drmd_vdp_reg6;
				gDrmd.vdp_reg7=s->drmd_vdp_reg7;
				gDrmd.vdp_reg8=s->drmd_vdp_reg8;
				gDrmd.vdp_reg9=s->drmd_vdp_reg9;
				gDrmd.vdp_reg10=s->drmd_vdp_reg10;
				gDrmd.vdp_reg11=s->drmd_vdp_reg11;
				gDrmd.vdp_reg12=s->drmd_vdp_reg12;
				gDrmd.vdp_reg13=s->drmd_vdp_reg13;
				gDrmd.vdp_reg14=s->drmd_vdp_reg14;
				gDrmd.vdp_reg15=s->drmd_vdp_reg15;
				gDrmd.vdp_reg16=s->drmd_vdp_reg16;
				gDrmd.vdp_reg17=s->drmd_vdp_reg17;
				gDrmd.vdp_reg18=s->drmd_vdp_reg18;
				gDrmd.vdp_reg19=s->drmd_vdp_reg19;
				gDrmd.vdp_reg20=s->drmd_vdp_reg20;
				gDrmd.vdp_reg21=s->drmd_vdp_reg21;
				gDrmd.vdp_reg22=s->drmd_vdp_reg22;
				gDrmd.vdp_reg23=s->drmd_vdp_reg23;
				gDrmd.vdp_reg24=s->drmd_vdp_reg24;
				gDrmd.vdp_reg25=s->drmd_vdp_reg25;
				gDrmd.vdp_reg26=s->drmd_vdp_reg26;
				gDrmd.vdp_reg27=s->drmd_vdp_reg27;
				gDrmd.vdp_reg28=s->drmd_vdp_reg28;
				gDrmd.vdp_reg29=s->drmd_vdp_reg29;
				gDrmd.vdp_reg30=s->drmd_vdp_reg30;
				gDrmd.vdp_reg31=s->drmd_vdp_reg31;
				gDrmd.vdp_reg32=s->drmd_vdp_reg32;
				gDrmd.vdp_counter=s->drmd_vdp_counter;
				gDrmd.hint_pending=s->drmd_hint_pending;
				gDrmd.vint_pending=s->drmd_vint_pending;
				gDrmd.vdp_pending=s->drmd_vdp_pending;
				gDrmd.vdp_code=s->drmd_vdp_code;
				gDrmd.vdp_dma_fill=s->drmd_vdp_dma_fill;
				gDrmd.pad_1_status=s->drmd_pad_1_status;
				gDrmd.pad_1_com=s->drmd_pad_1_com;
				gDrmd.pad_2_status=s->drmd_pad_2_status;
				gDrmd.pad_2_com=s->drmd_pad_2_com;
				gDrmd.sram_start=s->drmd_sram_start;
				gDrmd.sram_end=s->drmd_sram_end;
				gDrmd.sram=s->drmd_sram;
				gDrmd.region=s->drmd_region;
				gDrmd.sram_flags=s->drmd_sram_flags;
				gDrmd.cpl_fm=s->drmd_cpl_fm;
				memcpy(gDrmd.genesis_rom_banks,s->drmd_genesis_rom_banks,0x10);
				gDrmd.pad=s->drmd_pad;
				gDrmd.pad_1_counter=s->drmd_pad_1_counter;
				gDrmd.pad_1_delay=s->drmd_pad_1_delay;
				gDrmd.pad_2_counter=s->drmd_pad_2_counter;
				gDrmd.pad_2_delay=s->drmd_pad_2_delay;
				gDrz80.Z80PC=s->drz80_Z80PC;
				gDrz80.Z80A=s->drz80_Z80A;
				gDrz80.Z80F=s->drz80_Z80F;
				gDrz80.Z80BC=s->drz80_Z80BC;
				gDrz80.Z80DE=s->drz80_Z80DE;
				gDrz80.Z80HL=s->drz80_Z80HL;
				gDrz80.Z80SP=s->drz80_Z80SP;
				gDrz80.Z80PC_BASE=s->drz80_Z80PC_BASE;
				gDrz80.Z80SP_BASE=s->drz80_Z80SP_BASE;
				gDrz80.Z80IX=s->drz80_Z80IX;
				gDrz80.Z80IY=s->drz80_Z80IY;
				gDrz80.Z80I=s->drz80_Z80I;
				gDrz80.Z80A2=s->drz80_Z80A2;
				gDrz80.Z80F2=s->drz80_Z80F2;
				gDrz80.Z80BC2=s->drz80_Z80BC2;
				gDrz80.Z80DE2=s->drz80_Z80DE2;
				gDrz80.Z80HL2=s->drz80_Z80HL2;
				gDrz80.Z80_IRQ=s->drz80_Z80_IRQ;
				gDrz80.Z80IF=s->drz80_Z80IF;
				gDrz80.Z80IM=s->drz80_Z80IM;
				gDrz80.Z80R=s->drz80_Z80R;
				gDrz80.z80irqvector=s->drz80_z80irqvector;
				gDrz80.Z80_NMI=s->drz80_Z80_NMI;
#ifdef EMU_C68K
				memcpy(&gCyclone.d,&s->cyclone_d,0x8*4);
				memcpy(&gCyclone.a,&s->cyclone_a,0x8*4);
				gCyclone.pc=s->cyclone_pc;
				gCyclone.srh=s->cyclone_srh;
				gCyclone.xc=s->cyclone_xc;
				gCyclone.flags=s->cyclone_flags;
				gCyclone.irq=s->cyclone_irq;
				gCyclone.osp=s->cyclone_osp;
				gCyclone.vector=s->cyclone_vector;
				gCyclone.stopped=s->cyclone_stopped;
				gCyclone.cycles=s->cyclone_cycles;
				gCyclone.membase=s->cyclone_membase;
#endif
				memcpy(&gWorkRam,s->work_ram,0x10000);
				memcpy(&gVram,&s->vram,0x10000);
				memcpy(&gZram,s->zram,0x4000);
				memcpy(gCram,s->cram,0x80);
				memcpy(gVsram,s->vsram,0x80);

				//memcpy(&SL3,&s->SL3,sizeof(SL3));
				//memcpy(&ST,&s->ST,sizeof(ST));
				//memcpy(&OPN,&s->OPN,sizeof(OPN));
				//memcpy(&CH,&s->CH,sizeof(CH));
				//dacout=s->dacout;
				//dacen=s->dacen;
				//memcpy(OPN_pan,s->OPN_pan,6*2);
				memcpy(&PSG,&s->PSG,sizeof(PSG));
				//old_DT_Table=s->YMOPN_ST_dt_tab;
				memcpy(&gSram,s->sram,0x10000);


				for(x=0;x<6;x++)
				{
				for(y=0;y<4;y++)
				{
				//CH[x].SLOT[y].DT=(int*)(((unsigned int)&YMOPN_ST_dt_tab[0])+((unsigned int)(CH[x].SLOT[y].DT)-old_DT_Table));
				}
				}

				// update direct memory pointers, this is because
				// for example if DrMD is compiled with slight changes
				// the location of RomData or main md memory is
				// moved but the Z80PC will still be pointing at a memory
				// address based on the old location of rom data

				MDInitEmulation();
#ifdef EMU_C68K
				Cyclone_Init();
#endif
				//DrMD_DrZ80_Init();
#ifdef EMU_C68K
				// update cyclone pointers
				gCyclone.pc=DrMDCheckPc(gCyclone.pc);  // rebase pc
#endif
				// update drz80 pointers
				gDrz80.Z80PC=gDrz80.Z80PC-gDrz80.Z80PC_BASE;
				gDrz80.Z80PC=DrMD_Z80_Rebase_PC(gDrz80.Z80PC);

				gDrz80.Z80SP=gDrz80.Z80SP-gDrz80.Z80SP_BASE;
				gDrz80.Z80SP=DrMD_Z80_Rebase_SP(gDrz80.Z80SP);

				//Now need to reload banked rom banks
				for(x=0;x<7;x++)
				{
				   genesis_rom_bank_set(x,gDrmd.genesis_rom_banks[x]);
				}

				// re-sync gp32 pal and md pal;
				//update_md_pal();
			}

			break;
	}
}
