
#include <stdio.h>
#include "sal.h"
#include "cyclone.h"
#include "drz80.h"
#include "drmd.h"
#include "main.h"
#include "fm.h"
#include "sn76496.h"
#include "globals.h"
#include "savestate.h"

static u8 mPadConfig[32];
static s32 mShowFps=0;
static s32 mFrameSkip=0;
static double mSoundRate=0;
static s32 mCpuSpeed=0;
static s32 mScalingMode=0;
static s32 mFpsDisplay[256];
static s32 mSaveTimer=0;
static s8 mSaveMsg[256];
static s32 mRomLoaded=0;
static s32 mFrames=0; // Frames per Time
static s32 mFrameLimit=60;
static s32 mSMSPauseButton;

static u32 mQuickSavePresent;
static s8 *mVolume=NULL;
static s32 mVolTimer=0;
static s8 mVolMsg[256];
static u32 *mPad=NULL;

void MDInitPaletteLookup(void)
{
   u16 mdpal=0;
   u8 R,G,B;
   for(mdpal=0;mdpal<0x1000;mdpal++)
   {
      // md  0000 bbb0 ggg0 rrr0
      // gp  rrrr rggg ggbb bbbi
      //R=(mdpal>>1)&7; // 0000 0RRR - 3 bits Red
      //R=(R|R<<3)>>1;  // 000R RRRR - 5 bits Red
      //G=(mdpal>>5)&7;
      //G=(G|G<<3)>>1;
      //B=(mdpal>>9)&7;
      //B=(B|B<<3)>>1;

      // md  0000 bbb0 ggg0 rrr0
      // gp  rrrr rrrr gggg gggg bbbb bbbb
      R=(mdpal>>1)&7; // 0000 0RRR - 3 bits Red
      R=(R|R<<3|R<<6)>>1;  // 000R RRRR - 5 bits Red
      G=(mdpal>>5)&7;
      G=(G|G<<3|G<<6)>>1;
      B=(mdpal>>9)&7;
      B=(B|B<<3|B<<6)>>1;

      gPalLookup[mdpal]=SAL_RGB_PAL(R,G,B);
   }
}

void MDInitSoundTiming(void)
{
     s32 i;
     double sampleTime;
     double cpuSpeed;

     if(mFrameLimit==60) cpuSpeed=MD_CLOCK_NTSC;
     else cpuSpeed=MD_CLOCK_PAL;

     //gDrmd.cpl_fm = (double)(((cpuSpeed / 7.0) / (double)mSoundRate) / 144.0);
     gDrmd.cpl_fm = (double)(((cpuSpeed / 7.0) / 44100.0) / 144.0);

	 // hack for speed
	 //OPN.eg_timer_add = 211570;
	 // correct values
     OPN.eg_timer_add = ((double)(1<<EG_SH))  *  gDrmd.cpl_fm;  // 16K 211570.22106782106782106782106782
							// 11K 316635.70499946010150091782744844
						        // 8K  423140.44213564213564213564213564
     OPN.eg_timer_overflow = (( 3 ) * (1<<EG_SH))<<1;  // always 196608
     timer_base = ((((cpuSpeed / 7.0)/(double)mFrameLimit)/((double)gDrmd.lines_per_frame-1.0))/144.0)* 4096.0;
     //timer_base = gDrmd.cpl_fm * 4096.0;  // 16k 13223.138816738816738816738816739
                                         // 11k  19789.731562466256343807364215527
					 // 8k 26446.277633477633477633477633478

     update_tables();

     PSG_sn_UPDATESTEP = (u32)((((double)(((u32)mSoundRate<<STEP_BITS)))*16.0)/(double)SMS_CLOCK_NTSC);

     sampleTime = ((double)mSoundRate/(double)mFrameLimit)/((double)gDrmd.lines_per_frame-1.0);
     for(i=0;i<=gDrmd.lines_per_frame-1;i++)
     {
		gSampleCountLookup[i] = sampleTime * i;
     }
}

void PaletteSet(u32 index, u32 color)
{
	//if(mRenderMode&MENU_RENDER_MODE_16BIT) return;
	color=gPalLookup[color];

	sal_VideoPaletteSet(index,color);
}

static
s32 RoundDouble(double val)
{
	if ((val - (double) (s32) val) > 0.5) return (s32) (val + 1);
	else return (s32) val;
}

static
void ByteSwap(u8 *data, s32 len)
{
  s32 i=0;

  if (len<2) return; // Too short

  do
  {
    u16 *pd=(u16 *)(data+i);
    s32 value=*pd; // Get 2 bytes

    value=(value<<8)|(value>>8); // Byteswap it
    *pd=(u16)value; // Put 2b ytes
    i+=2;
  }
  while (i+2<=len);
}


// Interleve a 16k block and byteswap
static
void InterleveBlock(u8 *dest,u8 *src)
{
  s32 i=0;
  for (i=0;i<0x2000;i++) dest[(i<<1)  ]=src[       i]; // Odd
  for (i=0;i<0x2000;i++) dest[(i<<1)+1]=src[0x2000+i]; // Even
}

// Decode a SMD file
static
s32 DecodeSmd(u8 *data, s32 len)
{
  u8 *temp=NULL;
  s32 i=0;

  temp=(u8 *)malloc(0x4000);
  if (temp==NULL) return SAL_ERROR;
  memset(temp,0,0x4000);

  // Interleve each 16k block and shift down by 0x200:
  for (i=0; i+0x4200<=len; i+=0x4000)
  {
    InterleveBlock(temp,data+0x200+i); // Interleve 16k to temporary buffer
    memcpy(data+i,temp,0x4000); // Copy back in
  }

  free(temp);
  return SAL_OK;
}

s32 CheckSram()
{
	s32 i=0,c=0;
	for (i=0;i<0x10000;i++)
	{
		if(gSram[i]!=0)c++;
	}
	return c;
}

static
void DetectCountryGenesis(u32 *gameMode, u32 *cpuMode)
{
	s32 cTab[3] = {4, 1, 8};
	s32 gmTab[3] = {1, 0, 1};
	s32 cmTab[3] = {0, 0, 1};
	s32 i, coun = 0;
	s8 c;

	if (!strncasecmp((char *) &gRomData[0x1F0], "eur", 3)) coun |= 8;
	else if (!strncasecmp((char *) &gRomData[0x1F0], "usa", 3)) coun |= 4;
	else if (!strncasecmp((char *) &gRomData[0x1F0], "jap", 3)) coun |= 1;
	else for(i = 0; i < 4; i++)
	{
		c = toupper(gRomData[0x1F0 + i]);

		if (c == 'U') coun |= 4;
		else if (c == 'J') coun |= 1;
		else if (c == 'E') coun |= 8;
		else if (c < 16) coun |= c;
		else if ((c >= '0') && (c <= '9')) coun |= c - '0';
		else if ((c >= 'A') && (c <= 'F')) coun |= c - 'A' + 10;
	}

	if (coun & cTab[0])
	{
		*gameMode = gmTab[0];
		*cpuMode = cmTab[0];
	}
	else if (coun & cTab[1])
	{
		*gameMode = gmTab[1];
		*cpuMode = cmTab[1];
	}
	else if (coun & cTab[2])
	{
		*gameMode = gmTab[2];
		*cpuMode = cmTab[2];
	}
	else if (coun & 2)
	{
		*gameMode = 0;
		*cpuMode = 1;
	}
	else
	{
		*gameMode = 1;
		*cpuMode = 0;
	}
}

static
u32 DrMDCheckPc(u32 pc)
{
  pc-=gCyclone.membase; // Get real pc
  pc&=0xffffff;

  if (pc<gDrmd.romsize)
  {
    gCyclone.membase=(s32)gDrmd.cart_rom; // Program Counter in Rom
  }
  else if ((pc&0xe00000)==0xe00000)
  {
    gCyclone.membase=(s32)gDrmd.work_ram-(pc&0xff0000); // Program Counter in Ram
  }
  else
  {
    // Error - Program Counter is invalid
    gCyclone.membase=(s32)gDrmd.cart_rom;
  }
  return gCyclone.membase+pc;
}

static
void CycloneReset()
{
  gCyclone.srh = 0x27;
  gCyclone.a[7]=m68k_read_memory_32(0);
  gCyclone.membase = 0;
  gCyclone.pc=DrMDCheckPc(m68k_read_memory_32(4));
}

static
void DrMDDrZ80Reset()
{
  gDrz80.Z80A = 0;
  gDrz80.Z80F = 1<<2;  // set ZFlag
  gDrz80.Z80BC = 0;
  gDrz80.Z80DE = 0;
  gDrz80.Z80HL = 0;
  gDrz80.Z80A2 = 0;
  gDrz80.Z80F2 = 1<<2;  // set ZFlag
  gDrz80.Z80BC2 = 0;
  gDrz80.Z80DE2 = 0;
  gDrz80.Z80HL2 = 0;
  gDrz80.Z80IX = 0xFFFF0000;
  gDrz80.Z80IY = 0xFFFF0000;
  gDrz80.Z80I = 0;
  gDrz80.Z80IM = 0;
  gDrz80.Z80_IRQ = 0;
  gDrz80.Z80IF = 0;
  gDrz80.Z80PC=DrMD_Z80_Rebase_PC(0);
  gDrz80.Z80SP=DrMD_Z80_Rebase_SP(0);
}

static
void MDResetEmulation(u32 romSize)
{
	u32 gameMode=0;
	u32 cpuMode=0;
	// reset DrMD
	gDrmd.m68k_aim = 0;
	gDrmd.m68k_total = 0;
	gDrmd.zbank = 0;
	gDrmd.romsize = romSize;
	// hword variables
	gDrmd.vdp_line = 0;
	//gDrmd.vdp_status = 0;
	gDrmd.vdp_addr = 0;
	gDrmd.vdp_addr_latch = 0;
	// byte variables
	gDrmd.pad = 0;
	gDrmd.padselect = 0x40;
	gDrmd.zbusreq = 0;
	gDrmd.zbusack = 1;
	gDrmd.zreset = 0;
	gDrmd.vdp_reg0 = 0;
	gDrmd.vdp_reg1 = 0;
	gDrmd.vdp_reg2 = 0;
	gDrmd.vdp_reg3 = 0;
	gDrmd.vdp_reg4 = 0;
	gDrmd.vdp_reg5 = 0;
	gDrmd.vdp_reg6 = 0;
	gDrmd.vdp_reg7 = 0;
	gDrmd.vdp_reg8 = 0;
	gDrmd.vdp_reg9 = 0;
	gDrmd.vdp_reg10 = 0;
	gDrmd.vdp_reg11 = 0;
	gDrmd.vdp_reg12 = 0;
	gDrmd.vdp_reg13 = 0;
	gDrmd.vdp_reg14 = 0;
	gDrmd.vdp_reg15 = 0;
	gDrmd.vdp_reg16 = 0;
	gDrmd.vdp_reg17 = 0;
	gDrmd.vdp_reg18 = 0;
	gDrmd.vdp_reg19 = 0;
	gDrmd.vdp_reg20 = 0;
	gDrmd.vdp_reg21 = 0;
	gDrmd.vdp_reg22 = 0;
	gDrmd.vdp_reg23 = 0;
	gDrmd.vdp_reg24 = 0;
	gDrmd.vdp_reg25 = 0;
	gDrmd.vdp_reg26 = 0;
	gDrmd.vdp_reg27 = 0;
	gDrmd.vdp_reg28 = 0;
	gDrmd.vdp_reg29 = 0;
	gDrmd.vdp_reg30 = 0;
	gDrmd.vdp_reg31 = 0;
	gDrmd.vdp_reg32 = 0;
	gDrmd.vdp_counter = 255;
	gDrmd.hint_pending = 0;
	gDrmd.vint_pending = 0;
	gDrmd.vdp_pending = 0;
	gDrmd.vdp_code = 0;
	gDrmd.vdp_dma_fill = 0;
	gDrmd.pad_1_status = 0xFF;
	gDrmd.pad_1_com = 0x00;
	gDrmd.pad_2_status = 0xFF;
	gDrmd.pad_2_com = 0x00;
	gDrmd.sram_start = 0;
	gDrmd.sram_end = 0;
	gDrmd.sram_flags = 0;

	switch(gMDMenuOptions.force_region)
	{
		default:
		case 0:  // auto
			DetectCountryGenesis(&gameMode,&cpuMode);
			break;
		case 1: // Usa 60 fps
			gameMode = 1;
			cpuMode = 0;
			break;
		case 2: // Europe 50 fps
			gameMode = 1;
			cpuMode = 1;
			break;
		case 3: // Japan 60 fps
			gameMode = 0;
			cpuMode = 0;
			break;
		case 4: // Japan 50 fps
			gameMode = 0;
			cpuMode = 1;
			break;
	}

  if(cpuMode) // pal
  {
     gDrmd.vdp_status=1;
     gDrmd.lines_per_frame = 312;
     gDrmd.cpl_z80 = RoundDouble((((double) MD_CLOCK_PAL / 15.0) / 50.0) / 312.0);
     gDrmd.cpl_m68k = RoundDouble((((double) MD_CLOCK_PAL / 7.0) / 50.0) / 312.0);
     mFrameLimit=50;
  }
  else // ntsc
  {
     gDrmd.vdp_status=0;
     gDrmd.lines_per_frame = 262;
     gDrmd.cpl_z80 = RoundDouble((((double) MD_CLOCK_NTSC / 15.0) / 60.0) / 262.0);
     gDrmd.cpl_m68k = RoundDouble((((double) MD_CLOCK_NTSC / 7.0) / 60.0) / 262.0);
     mFrameLimit=60;
  }
	/*
	(sound rate/frame_rate) =  buffer size
	60 fps
	11Khz = 92
	16Khz = 138
	22Khz = 184

	50 fps
	11Khz = 111
	16Khz = 165
	22Khz = 221

	*/

  gDrmd.region=((((gameMode)<<1)|(cpuMode))<<6)|0x20;

  //for(x=0;x<0x10;x++)
  //{
  // gDrmd.genesis_rom_banks,0,0x10); //default memory rom banks
  //}
  genesis_rom_banks_reset();

  // setup sram values
  if ((gRomData[424 + 8] == 'R') && (gRomData[424 + 9] == 'A') && (gRomData[424 + 10] & 0x40))
	{
		gDrmd.sram_start = gRomData[436] << 24;
		gDrmd.sram_start |= gRomData[437] << 16;
		gDrmd.sram_start |= gRomData[438] << 8;
		gDrmd.sram_start |= gRomData[439];
		gDrmd.sram_start &= 0x0F80000;		// multiple de 0x080000

		gDrmd.sram_end = gRomData[440] << 24;
		gDrmd.sram_end |= gRomData[441] << 16;
		gDrmd.sram_end |= gRomData[442] << 8;
		gDrmd.sram_end |= gRomData[443];
	}
	else
	{
		gDrmd.sram_start = 0x200000;
		gDrmd.sram_end = 0x200000 + (64 * 1024) - 1;
	}

	if ((gDrmd.sram_start > gDrmd.sram_end) || ((gDrmd.sram_end - gDrmd.sram_start) >= (64 * 1024)))
		gDrmd.sram_end = gDrmd.sram_start + (64 * 1024) - 1;

	if (gDrmd.romsize <= (2 * 1024 * 1024))
	{
		gDrmd.sram_flags = (1<<0)|(1<<1);
	}

	gDrmd.sram_start &= 0xFFFFFFFE;
	gDrmd.sram_end |= 0x00000001;

	// reset Cyclone
	gCyclone.checkpc=DrMDCheckPc;
	gCyclone.fetch8 =gCyclone.read8 =m68k_read_memory_8;
	gCyclone.fetch16=gCyclone.read16=m68k_read_memory_16;
	gCyclone.fetch32=gCyclone.read32=m68k_read_memory_32;
	gCyclone.write8 =m68k_write_memory_8;
	gCyclone.write16=m68k_write_memory_16;
	gCyclone.write32=m68k_write_memory_32;
	gCyclone.IrqCallback = m68k_irq_callback;

	CycloneReset();

	// reset DrZ80
	DrMDDrZ80Reset();

	// clear memory
	memset(&gWorkRam,0,0x10000);
	memset(&gSram,0,0x10000);
	memset(&gVram,0,0x10000);
	memset(&gCram,0,0x80);
	memset(&gZram,0,0x4000);
	memset(&gVsram,0,0x40);

	MDInitSoundTiming();
	SN76496_sh_start();
	//YM2612Init();
}

static
s32 MDRomLoad()
{
	s8 filename[SAL_MAX_PATH];
	s32 fileSize=0;

	// clear ROM area
	memset(gRomData,0,MAX_ROM_SIZE);

	// get full filename
	strcpy(filename,gCurrentRomFilename);

	if(sal_ZipCheck(filename))
	{
		MenuMessageBox("Unzipping Rom......",filename,"",MENU_MESSAGE_BOX_MODE_MSG);

		if(sal_ZipLoad(filename,(s8*)gRomData,MAX_ROM_SIZE,&fileSize)!=SAL_OK)
		{
			MenuMessageBox("Failed to unzip rom",sal_LastErrorGet(),"",MENU_MESSAGE_BOX_MODE_PAUSE);
			return SAL_ERROR;
		}
	}
	else
	{
		MenuMessageBox("Loading Rom......",filename,"",MENU_MESSAGE_BOX_MODE_MSG);

		if(sal_FileLoad(filename, gRomData, MAX_ROM_SIZE, (u32*)&fileSize)!=SAL_OK)
		{
			MenuMessageBox("Failed to load rom",sal_LastErrorGet(),"",MENU_MESSAGE_BOX_MODE_PAUSE);
			return SAL_ERROR;
		}
	}

	if ((fileSize&0x3fff)==0x200)
	{
		// Decode and byteswap SMD
		DecodeSmd(gRomData,fileSize);
		fileSize-=0x200;
	}
	else
	{
		// Just byteswap Rom
		ByteSwap(gRomData,fileSize);
	}

	MDInitEmulation();

	MDResetEmulation(fileSize);

	//auto load default config for this rom if one exists
	if (LoadMenuOptions(gMDSystemDir, filename, MENU_OPTIONS_EXT, (s8*)&gMDMenuOptions, sizeof(gMDMenuOptions),0)!=SAL_OK)
	{
		if (LoadMenuOptions(gMDSystemDir, MENU_OPTIONS_FILENAME, MENU_OPTIONS_EXT, (s8*)&gMDMenuOptions, sizeof(gMDMenuOptions),0)!=SAL_OK)
		{
			MDDefaultMenuOptions();
		}
	}

	// LOAD SRAM
	if (gMDMenuOptions.auto_sram)
	{
		//LoadSram(gMDSystemDir,filename,SRAM_FILE_EXT,(s8*)&gSram);
	}

	return SAL_OK;
}

static
void DrMDDrZ80SetIrq(u32 irq)
{
    gDrz80.z80irqvector = 0xFF;
	gDrz80.Z80_IRQ = irq;
}

static
void DrMDDrZ80IrqCallback(void)
{
    gDrz80.Z80_IRQ=0;  // lower irq when in accepted
}

static
s32 SekInterrupt(s32 irq)
{
  gCyclone.irq=(u8)irq;
  return 0;
}

static
void *CenterFb()
{
	return sal_VideoGetBuffer();
}

void MDInitEmulation(void)
{
	gDrmd.m68k_context = (u32)&gCyclone; // pointer to M68K emulator
	gDrmd.m68k_run = CycloneRun;     //pointer to Mk68k run functoin
	gDrmd.m68k_set_irq = SekInterrupt;

	gDrmd.z80_context = (u32)&gDrz80;
	gDrmd.z80_run = DrZ80Run;
	gDrmd.z80_set_irq = DrMDDrZ80SetIrq;
	gDrmd.z80_reset = DrMDDrZ80Reset;
	gDrmd.fm_write = fm_write;
	gDrmd.fm_read = fm_read;

	gDrmd.psg_write = psg_write;
	gDrmd.cart_rom = (u32)gRomData;
	gDrmd.work_ram = (u32)gWorkRam;
	gDrmd.zram = (u32)gZram;
	gDrmd.vram = (u32)gVram;
	gDrmd.cram = (u32)gCram;
	gDrmd.palette_set = PaletteSet;
	gDrmd.vsram = (u32)gVsram;
	gDrmd.sram = (u32)gSram;

	gDrz80.z80_write8=DrMD_Z80_write_8;
	gDrz80.z80_write16=DrMD_Z80_write_16;
	gDrz80.z80_in=DrMD_Z80_In;
	gDrz80.z80_out=DrMD_Z80_Out;
	gDrz80.z80_read8=DrMD_Z80_read_8;
	gDrz80.z80_read16=DrMD_Z80_read_16;
	gDrz80.z80_rebasePC=DrMD_Z80_Rebase_PC;
	gDrz80.z80_rebaseSP=DrMD_Z80_Rebase_SP;
	gDrz80.z80_irq_callback=DrMDDrZ80IrqCallback;

}

void InvalidOpCallback(u32 opcode)
{
   /*char text[256];
   gp_initFramebuffer(framebuffer16[0],16,60,menu_options.lcdver);
   gp_clearFramebuffer16(framebuffer16[0],(unsigned short)RGB(0,0,0));
   sprintf(text,"Invalid opcode : %x",opcode);
   gp_drawString(8,8,sal_strlen(text),text,(unsigned short)RGB(31,31,31),framebuffer16[0]);
   while(1==1)
   {
   }*/
}

void illegal_memory_io(u32 address, s8 *error_text)
{
   /*char text[256];
   gp_initFramebuffer(framebuffer16[0],16,60,menu_options.lcdver);
   gp_clearFramebuffer16(framebuffer16[0],(unsigned short)RGB(0,0,0));
   sprintf(text,"Invalid address io : %x",address);
   gp_drawString(8,8,sal_strlen(text),text,(unsigned short)RGB(31,31,31),framebuffer16[0]);
   gp_drawString(8,16,sal_strlen(error_text),error_text,(unsigned short)RGB(31,31,31),framebuffer16[0]);
   while(1==1)
   {
   }
   */
}

void CycloneClearIRQ(s32 irq)
{
	gCyclone.irq=irq;
}

static
s32 DoFrame(u32 skip)
{
	s32 quit=0;
	s32 i=0;
	s32 p=0;

	u32 keys=sal_InputPoll();

	if((keys&SAL_INPUT_MENU_ENTER)==SAL_INPUT_MENU_ENTER)
	{
		quit=1;
	}
	else if((keys&SAL_INPUT_MENU_QUICKLOAD) == SAL_INPUT_MENU_QUICKLOAD)
	{
		// quick load
		if(mQuickSavePresent)
		{
			loadstate_mem(gQuickState);
			sal_InputIgnore();
		}
	}
	else if((keys&SAL_INPUT_MENU_QUICKSAVE) == SAL_INPUT_MENU_QUICKSAVE)
	{
		// quick save
		savestate_mem(gQuickState);
		mQuickSavePresent=1;
		sal_InputIgnore();
	}
#ifdef SAL_INPUT_NEED_VOLUME
	else if (keys&SAL_INPUT_VOL_UP)
	{
		mVolume[0]+=1;
		if (mVolume[0] > 100) mVolume[0] = 100;
		sal_AudioSetVolume(mVolume[0],mVolume[0]);

		mVolTimer=60;
		sprintf(mVolMsg,"Vol:%d",mVolume[0]);
	}
	else if (keys&SAL_INPUT_VOL_DOWN)
	{
		mVolume[0]-=1;
		if (mVolume[0] > 100) mVolume[0] = 0;
		sal_AudioSetVolume(mVolume[0],mVolume[0]);

		mVolTimer=60;
		sprintf(mVolMsg,"Vol:%d",mVolume[0]);
	}
#endif
	else
	{
		for(i=0;i<32;i++)
		{
			if ((keys&(1<<i))&&(mPadConfig[i]!=0xFF)) p|=(1<<mPadConfig[i]);
		}

		if(!quit) *mPad=p;
	}

	DrMDRun(skip);

	if(skip)
	{
		mFrames++;

		if(mVolTimer)
		{
			mVolTimer--;
			sal_VideoDrawRect(0, 0, 8<<3, 8, 0x0);
			if(mVolTimer>5)
			{
				sal_VideoPrint(0,0,mVolMsg,0xFFFF);
			}
		}
		else if(mSaveTimer)
		{
			mSaveTimer--;
			sal_VideoDrawRect(0, 0, 6<<3, 8, 0x0);
			if(mSaveTimer>5)
			{
				sal_VideoPrint(0,0,mSaveMsg,0xFFFF);
			}
		}
		else if(mShowFps)
		{
			sal_VideoDrawRect(0, 0, strlen(mFpsDisplay)<<3, 8, 0x0);
			sal_VideoPrint(0,0,mFpsDisplay,0xFFFF);
		}
		sal_VideoFlip(0);
		gDrmd.frame_buffer=(u32)sal_VideoGetBuffer();
	}

	return quit;
}

static
s32 RunNoAudio()
{
  s32 quit=0,ticks=0,done=0,i=0;
  s32 tick=0,fps=0;
  u32 timer=0;


  	mFrames=0;
  	  while (quit==0)
	  {
	    	timer=sal_TimerRead();  // get milliseconds elapsed
	    	timer=timer/(1000000/mFrameLimit);

		if(timer-tick>mFrameLimit) // a second has past - calc FPS
	    	{
	       		fps=mFrames;
	       		mFrames=0;
	       		tick=timer;
	       		sprintf(mFpsDisplay,"Fps: %d",fps);

	    	}

	    	ticks=timer-done;
	    	if(ticks<1) continue;

		if(mFrameSkip==0)
	    	{
	       		if(ticks>10) ticks=10;
	       		for (i=0; i<ticks-1; i++)
	       		{
				quit|=DoFrame(0);
	       		}
	       		if(ticks>=1)
	       		{
				quit|=DoFrame(1);
	       		}
	    	}
	    	else
	    	{
	       		if(ticks>mFrameSkip) ticks=mFrameSkip;
	       		for (i=0; i<ticks-1; i++)
	       		{
				quit|=DoFrame(0);
	       		}
 	       		if(ticks>=1)
	       		{
				quit|=DoFrame(1);
	       		}
	    	}
		done=timer;
	  }
  return 0;
}

static
s32 RunAudio()
{
	s32 quit=0,done=0,aim=0,i=0;
	s32 fps=0,skip=0;
	u32 tick=0,timer=0;

	mFrames=0;
	skip=mFrameSkip-1;

	if(sal_AudioInit(mSoundRate, 16, 1, mFrameLimit)==SAL_ERROR)
	{
		MenuMessageBox("Audio init failed",sal_LastErrorGet(),"",MENU_MESSAGE_BOX_MODE_PAUSE);
		return 1;
	}

	gSoundSampleCount=sal_AudioGetSampleCount()>>1;

	sal_AudioSetVolume(mVolume[0],mVolume[0]);

	// Get initial position in the audio loop:
	done=sal_AudioGetPrevBufferIndex(sal_AudioGetCurrentBufferIndex());

	while (quit==0)
	{
    		for (i=0;i<10;i++)
		{
			timer=sal_TimerRead();
			timer=timer/(1000000/mFrameLimit);
			if(timer-tick>mFrameLimit) // a second has past - calc FPS
			{
	       			fps=mFrames;
				mFrames=0;
				tick=timer;
				sprintf(mFpsDisplay,"Fps: %d",fps);
			}

			aim=sal_AudioGetPrevBufferIndex(sal_AudioGetCurrentBufferIndex());
			if (done!=aim)
			{
				//We need to render more audio:
				gSoundBuffer=sal_AudioGetBuffer(done);
        gLastSample=0;
				gDacInPos=0;
				gDacOutPos=0;
				done=sal_AudioGetNextBufferIndex(done);

				if(mFrameSkip==0)
				{
					if (done==aim) 	quit|=DoFrame(1); // Render last frame
					else           quit|=DoFrame(0);
				}
				else
				{
					if (skip)
					{
						quit|=DoFrame(0); // Render last frame
						skip--;
					}
					else
					{
						quit|=DoFrame(1);
						skip=mFrameSkip-1;
					}
				}
				RefreshFm();
				YM2612UpdateOne(gSoundBuffer,gSoundSampleCount);
			}
      			if (done==aim) break; // Up to date now
    		}

    		done=aim; // Make sure up to date
  	}

  sal_AudioClose();
  return 0;
}

#if 0
char **g_argv;
int mainEntry(int argc, char *argv[])
{
	int i=0,action=0;
	g_argv = argv;
	char text[40];
	char text2[40];

	sal_VideoInit(16,0,60);
	sal_VideoClearAll(0);
#if 0
	_sbrk(1);

	u32 test=nHeapEnd;
	void *eatme;
	while(1)
	{
		sprintf(text,"heap:%x eatme %x",test, eatme);
		sal_VideoDebug(text,0);
		eatme=malloc(0x1000);
		if(eatme==0)
		{
			sal_VideoDebug("malloc failed",1);
		}
		test=nHeapEnd;
	}
#endif
#if 0
	int fps=0;
	//char text[256];
	while(1)
	{
		sal_VideoClear(0x0);
		fps++;

		sprintf(text,"fps:%x",fps);
		sal_VideoPrint(0,0,text,0xFFFF);

		sal_VideoFlip(1);
	}
#endif
	sal_CpuSpeedSet(GetMenuCpuSpeed());





	GetSystemDir(gSystemRootDir);

	MainMenuInit();

	gRomData=(unsigned char *)malloc(MAX_ROM_SIZE);

	if(gRomData==0)
	{
		MenuMessageBox("Failed to allocate rom area","","",MENU_MESSAGE_BOX_MODE_PAUSE);
	}

	strcpy(gSystemDir,gSystemRootDir);

	strcpy(gMDSystemDir,gSystemRootDir);

	//Find a nice even number that is bigger than the size
	//of a save state.  Use this number to malloc space to
	//hold savestates as using sizeof struct seems to break it
	//probably some sort of alignment problem.

	i=0;
	while(i<=(sizeof(struct MD_SAVESTATE_V6)))
	{
		i+=0x1000;
	}

	gQuickState=(char *)malloc(i); // add some more
	gTempState=(char *)malloc(i);
	gCurrentState=(char *)malloc(i);

	gSaveStateSize = i;

	// setup DrMD dir in GPMM
	MenuMessageBox("Checking directories","","",MENU_MESSAGE_BOX_MODE_MSG);

	//Make sure required directories exist
	if(sal_DirectoryCreate(gMDSystemDir)!=SAL_OK)
	{
		MenuMessageBox(sal_LastErrorGet(),"","",MENU_MESSAGE_BOX_MODE_PAUSE);
	}

	// Load options
	if (LoadMenuOptions(gMDSystemDir,MENU_OPTIONS_FILENAME,MENU_OPTIONS_EXT,(char*)&gMDMenuOptions, sizeof(gMDMenuOptions),0)!=SAL_OK)
	{
		MDDefaultMenuOptions();
	}

	// Load default rom directories
	if (LoadMenuOptions(gMDSystemDir,DEFAULT_ROM_DIR_FILENAME,DEFAULT_ROM_DIR_EXT,(char*)gMDRomDir, SAL_MAX_PATH,0)!=SAL_OK)
	{
		strcpy(gMDRomDir,gSystemRootDir);
	}

	sal_VideoPaletteSet(0x51,SAL_RGB_PAL(255,255,255));
	sal_VideoPaletteSet(0xFF,SAL_RGB_PAL(255,255,255));

	memset(&gCyclone,0,sizeof(gCyclone));
	memset(&gDrz80,0,sizeof(gDrz80));
	memset(&gDrmd,0,sizeof(gDrmd));

	//main loop
	for (;;)
	{
		action=MainMenu(action);

		if (action==EVENT_EXIT_APP) break;

		if (action==EVENT_LOAD_MD_ROM)
		{
			gCurrentEmuMode = EMU_MODE_MD;
			mRomLoaded=MDRomLoad();
			if(mRomLoaded)
			{
				action=EVENT_RUN_MD_ROM;   // rom loaded okay so continue with emulation
				strcpy(gSystemDir,gMDSystemDir);
				mQuickSavePresent=0;
			}
			else
			{
				action=0;   // rom failed to load so return to menu
			}
		}

		if ((action==EVENT_RESET_MD_ROM)&&mRomLoaded)
		{
			MDResetEmulation(gDrmd.romsize);
			action=EVENT_RUN_MD_ROM;
		}

		if ((action==EVENT_RUN_MD_ROM)&&mRomLoaded)
		{
			memcpy(mPadConfig,gMDMenuOptions.pad_config,sizeof(gMDMenuOptions.pad_config));
			mShowFps=gMDMenuOptions.show_fps;
			mFrameSkip=gMDMenuOptions.frameskip;
			mSoundRate=gMDMenuOptions.sound_rate;
			mCpuSpeed=gMDMenuOptions.cpu_speed;
			mScalingMode=gMDMenuOptions.scaling_mode;
			gSoundOn=gMDMenuOptions.sound_on;
			sal_CpuSpeedSet(mCpuSpeed);
			sprintf(mFpsDisplay,"");

			sal_VideoInit(16,0,60);
			sal_VideoClearAll(0);
			gDrmd.render_line = md_render_16;

			gDrmd.pad_1_type = gMDMenuOptions.pad_type;

			mVolume=(char*)&gMDMenuOptions.volume;

			gDrmd.frame_buffer=(unsigned int)CenterFb();
			mPad = (unsigned int*)&gDrmd.pad;

			MDInitPaletteLookup();
			MDInitSoundTiming(); // update fm tables incase of sound rate change

            		if(gSoundOn) RunAudio();
			else RunNoAudio();
		}
	}


	free(gCurrentState);
	free(gTempState);
	free(gQuickState);
	free(gRomData);
	sal_Reset();

	return(0);
}
#endif