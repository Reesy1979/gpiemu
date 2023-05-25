//
// hvsdisplay.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016-2022  R. Stange <rsta2@o2online.de>
//
// Information to implement HVS is from:
//	https://blog.benjdoherty.com/2019/05/21/Exploring-Hardware-Compositing-With-the-Raspberry-Pi/
//	https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vc4/vc4_plane.c
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <hvsdisplay.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <circle/memio.h>
#include <circle/new.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

CHVSDisplay::CHVSDisplay (CInterruptSystem *pInterrupt, int height, int width, int stride, int pixelsize)
:	m_pInterruptSystem (pInterrupt)
{
	assert (m_pInterruptSystem != 0);

	// Create 2 frame buffers - oversized, use u32 to force alignment for quicker transpose
	mFB[0].ptr=(void*)new u32[height*width*pixelsize];
	mFB[0].width=width;
	mFB[0].height=height;
	mFB[0].stride=stride;
	mFB[0].pixelsize=pixelsize;

	mFB[1].ptr=(void*)new u32[height*width*pixelsize];
	mFB[1].width=width;
	mFB[1].height=height;
	mFB[1].stride=stride;
	mFB[1].pixelsize=pixelsize;
	
	mFB[2].ptr=mFB[0].ptr; //reuse buffer
	mFB[2].width=width;
	mFB[2].height=height;
	mFB[2].stride=stride;
	mFB[2].pixelsize=pixelsize;
	
	mFB[3].ptr=mFB[1].ptr; //reuse buffer
	mFB[3].width=width;
	mFB[3].height=height;
	mFB[3].stride=stride;
	mFB[3].pixelsize=pixelsize;

#if defined(_NEED_TRANSPOSE)	
	mTempFB=(u16*)new u32[height*width*pixelsize];
	memset(mTempFB, 0, height*width*pixelsize);
#endif
	mCurrFB=0;
	memset(mFB[0].ptr, 0, height*width*pixelsize);
	memset(mFB[1].ptr, 0, height*width*pixelsize);
	
	
	m_bIRQConnected = FALSE;
	
}

CHVSDisplay::~CHVSDisplay (void)
{
	// disconnect IRQ
	assert (m_pInterruptSystem != 0);
	if (m_bIRQConnected)
	{
		//m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_PIXELVALVE1);
		m_pInterruptSystem->DisconnectIRQ (ARM_IRQ_PWA0);
		
	}

	m_pInterruptSystem = 0;
	
}


void CHVSDisplay::WipeDisplaylist(void) 
{
	for (int i=0; i<1024; i++) 
	{
		mpDlistMemory[i] = CONTROL_END;
	}
	//mDisplaySlot = 0;  firmware is messing around at start of memory, so avoid it
	mDisplaySlot = 800;
}

boolean CHVSDisplay::Initialize ()
{
	write32(SCALER_DISPCTRL,read32(SCALER_DISPCTRL)&~SCALER_DISPCTRL_ENABLE); // disable HVS
	write32(SCALER_DISPCTRL,SCALER_DISPCTRL_ENABLE | 0x9a0dddff); // re-enable HVS

	for (int i=0; i<3; i++) 
	{
		mpHvsChannels[i].dispctrl = SCALER_DISPCTRLX_RESET;
		mpHvsChannels[i].dispctrl = 0;
		mpHvsChannels[i].dispbkgnd = 0x1020202; // bit 24
	}

	mpHvsChannels[2].dispbase = BASE_BASE(0)      | BASE_TOP(0x7f0);
	mpHvsChannels[1].dispbase = BASE_BASE(0xf10)  | BASE_TOP(0x50f0);
	mpHvsChannels[0].dispbase = BASE_BASE(0x800) | BASE_TOP(0xf00);
	
	WipeDisplaylist();
	mScalingKernel=4080;
	UploadScalingKernel();
	
	write32(SCALER_DISPEOLN,0x40000000);

	mpHvsChannels[0].dispctrl = SCALER_DISPCTRLX_RESET;
	mpHvsChannels[0].dispctrl = SCALER_DISPCTRLX_ENABLE | SCALER_DISPCTRL_W(mFB[0].width) | SCALER_DISPCTRL_H(mFB[0].height);
	mpHvsChannels[0].dispbkgnd = SCALER_DISPBKGND_AUTOHS | 0x020202;
	mpHvsChannels[1].dispctrl = SCALER_DISPCTRLX_RESET;
	mpHvsChannels[1].dispctrl = SCALER_DISPCTRLX_ENABLE | SCALER_DISPCTRL_W(mFB[0].width) | SCALER_DISPCTRL_H(mFB[0].height);
	mpHvsChannels[1].dispbkgnd = SCALER_DISPBKGND_AUTOHS | 0x020202;
    
        // Write frame buffers to HVS as planes 0 and 1
	AddPlane(&mFB[0], 0, 0, false);
	AddPlane(&mFB[1], 0, 0, false);
	
	//Record end position of normal planes, this will be the start of scaled display lists which will be altered
	mScaledDispListStart = mDisplaySlot;
	mScaled =0;
	
	if (!m_bIRQConnected)
	{
		assert (m_pInterruptSystem != 0);
		
		pixel_valve *rawpv = (pixel_valve*)(ARM_IO_BASE + 0x206000);
		rawpv->int_enable = 0;
		rawpv->int_status = 0xff;
		
		//m_pInterruptSystem->ConnectIRQ (ARM_IRQ_PIXELVALVE1, InterruptStub, this); //Channel 1??
		m_pInterruptSystem->ConnectIRQ (ARM_IRQ_PWA0, InterruptStub, this);

		m_bIRQConnected = TRUE;

		rawpv->int_enable = PV_INTEN_VFP_START; // | 0x3f;
	}

	
	return TRUE;
}

int CHVSDisplay::GetPrevBufferIndex(void)
{
	int buf=mCurrFB;
	buf--;
	if(buf<0) buf = FRAME_BUFFER_COUNT-1;
	return buf;
}

u16 *CHVSDisplay::GetBuffer(void)
{
#if defined(_NEED_TRANSPOSE)
	return (u16*)mTempFB;
#else
	return (u16*)mFB[GetPrevBufferIndex()].ptr;
#endif
}

#if defined(_NEED_TRANSPOSE)
static void transpose_screen2(u16 *src, u16 *dst)
{
	//src = 320x240 x stride = 1  y stride = 320
	//dst = 240x320 x string = 240 y stride = 1
	
	u16 *d;
	u32 off= 0;

	for(int y=0;y<240;y++)
	{
		d=dst+(240-y);
		for(int x=0;x<320;x++)
		{
			d[off+=240]=*src++;
		}
		off=0;
	}
}

static void transpose_screen(u16 *src, u16 *dst, int height, int width)
{
	//src = 320x240 x stride = 1  y stride = 320
	//dst = 240x320 x string = 240 y stride = 1
	
	u16 *d, *s;
	u32 off= 0;

	for(int y=0;y<height;y++)
	{
		d=dst+(height-y-1);
		s=src+(y*320);
		for(int x=0;x<width;x++)
		{
			d[off+=240]=*s++;
		}
		off=0;
	}
}

//Hacky asm to transpose buffer as quickly as I could think of...
extern "C" void transpose_320x240_to_240x320(u16 *src, u16 *dest);

#endif

void CHVSDisplay::Flip (bool vsync)
{
	//Wait for next PV interupt to increase frame count
	int currFrame = mFrameCount;

#if defined(_NEED_TRANSPOSE)	
	if(mScaled)
	{
		transpose_screen(mTempFB, (u16*)mFB[GetPrevBufferIndex()].ptr, mScaledHeight, mScaledWidth);
	}
	else
	{
		//transpose_320x240_to_240x320 15 fps faster than transpose_screen
		transpose_screen(mTempFB, (u16*)mFB[GetPrevBufferIndex()].ptr, 240, 320);
		//transpose_320x240_to_240x320((u16*)mTempFB, (u16*)mFB[GetPrevBufferIndex()].ptr);
		//transpose_screen(mTempFB, (u16*)mFB[GetPrevBufferIndex()].ptr);
	}
#endif	
	//then flip buffers

	mFlipRequested=1;
	if(vsync)
	{
		
		while(mFlipRequested)
		{
			//sleep?
		}
	}

	if(mCurrFB+1>=FRAME_BUFFER_COUNT)
	{
		mCurrFB=0;
	}
	else
	{
		mCurrFB++;
	}

	
}

void CHVSDisplay::Scale(int srcW, int srcH)
{
	//Setup scaled display lists
	//If already scaled, switch to unscaled while we fuck with the display list memory
	mDisplaySlot=mScaledDispListStart;
	
	if((srcW==mFB[0].width && srcH==mFB[0].height)
	   ||
	   (srcW==0 && srcH==0))
	{
		//Disable scaling
		mScaled = 0;
	}
	else
	{
		//Enable scaling
		mScaledLayerCount=0;
		mScaledHeight=srcH;
		mScaledWidth=srcW;
		AddPlaneScaled(&mFB[2], 0, 0, srcW, srcH, false);
		AddPlaneScaled(&mFB[3], 0, 0, srcW, srcH, false);
		mScaled = 2;  // 2 for a reason, I'll use this as an offet on the mFB array
	}
}

void CHVSDisplay::WriteTPZ(u32 source, u32 dest) 
{
	u32 scale = (1<<16) * source / dest;
	u32 recip = ~0 / scale;
	mpDlistMemory[mDisplaySlot++] = scale << 8;
	mpDlistMemory[mDisplaySlot++] = recip & 0xffff;
}

void CHVSDisplay::WritePPF(u32 source, u32 dest)
{
	u32 scale = (1<<16) * source / dest;
	mpDlistMemory[mDisplaySlot++] = SCALER_PPF_AGC | (scale << 8) | (0 << 0);
}

void CHVSDisplay::UploadScalingKernel(void) 
{
	int kernel_start = mScalingKernel;
	#define PACK(a,b,c) ( (((a) & 0x1ff) << 0) | (((b) & 0x1ff) << 9) | (((c) & 0x1ff) << 18) )
	// the Mitchell/Netravali filter copied from the linux source
	const uint32_t half_kernel[] = { PACK(0, -2, -6), PACK(-8, -10, -8), PACK(-3, 2, 18), PACK(50, 82, 119), PACK(155, 187, 213), PACK(227, 227, 0) };
	for (int i=0; i<11; i++) 
	{
		if (i < 6) 
		{
			mpDlistMemory[kernel_start + i] = half_kernel[i];
		} 
		else 
		{
			mpDlistMemory[kernel_start + i] = half_kernel[11 - i - 1];
		}
	}
}

void CHVSDisplay::AddPlaneScaled(hvs_surface *fb, int x, int y, int scale_height, int scale_width, bool hflip) 
{
	assert(fb);
	//if ((x < 0) || (y < 0)) printf("rendering FB of size %dx%d at %dx%d, scaled down to %dx%d\n", fb->width, fb->height, x, y, width, height);
	enum scaling_mode 
	{
		scaling_none,
		PPF, // upscaling?
		TPZ // downscaling?
	};
	enum scaling_mode xmode, ymode;
	
	if (scale_width > fb->width) xmode = TPZ;
	else if (scale_width < fb->width) xmode = PPF;
	else xmode = TPZ;

	if (scale_height > fb->height) ymode = TPZ;
	else if (scale_height < fb->height) ymode = PPF;
	else ymode = TPZ;

	int scl0;
	switch ((xmode << 2) | ymode) 
	{
	case (PPF << 2) | PPF:
		scl0 = SCALER_CTL0_SCL_H_PPF_V_PPF;     // 0
		break;
	case (TPZ << 2) | PPF:
		scl0 = SCALER_CTL0_SCL_H_TPZ_V_PPF;     // 1
		break;
	case (PPF << 2) | TPZ:
		scl0 = SCALER_CTL0_SCL_H_PPF_V_TPZ;     // 2
		break;
	case (TPZ << 2) | TPZ:
		scl0 = SCALER_CTL0_SCL_H_TPZ_V_TPZ;     // 3
		break;
	case (PPF << 2) | scaling_none:
		scl0 = SCALER_CTL0_SCL_H_PPF_V_NONE;    // 4
		break;
	case (scaling_none << 2) | PPF:
		scl0 = SCALER_CTL0_SCL_H_NONE_V_PPF;    // 5
		break;
	case (scaling_none << 2) | TPZ:
		scl0 = SCALER_CTL0_SCL_H_NONE_V_TPZ;    // 6
		break;
	case (TPZ << 2) | scaling_none:
		// randomly doesnt work right
		scl0 = SCALER_CTL0_SCL_H_TPZ_V_NONE;    // 7
			break;
	}
	
	//Record start position in display list memory
  	fb->hvsstart=mDisplaySlot;
  	
	int start = mDisplaySlot;
	mpDlistMemory[mDisplaySlot++] = 0 // CONTROL_VALID
	//    | CONTROL_WORDS(14)
	| CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_ARGB)
	//    | CONTROL0_VFLIP // makes the HVS addr count down instead, pointer word must be last line of image
	| (hflip ? CONTROL0_HFLIP : 0)
	| CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565)
	| (scl0 << 5)
	| (scl0 << 8); // SCL1
	mpDlistMemory[mDisplaySlot++] = POS0_X(x) | POS0_Y(y) | POS0_ALPHA(0xff);  // position word 0
	mpDlistMemory[mDisplaySlot++] = fb->width | (fb->height << 16);                    // position word 1
	mpDlistMemory[mDisplaySlot++] = POS2_H(scale_height) | POS2_W(scale_width) | 1<<30;    // position word 2
	mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF;                                // position word 3, dummy for HVS state
	mpDlistMemory[mDisplaySlot++] = BUS_ADDRESS((u32)fb->ptr);
	mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF;                                // pointer context word 0 dummy for HVS state
	mpDlistMemory[mDisplaySlot++] = fb->stride * fb->pixelsize;                // pitch word 0
	mpDlistMemory[mDisplaySlot++] = mScaledLayerCount * 2400;         // LBM base addr
	mScaledLayerCount++;
	if (xmode == PPF) 
	{
		WritePPF(scale_width, fb->width);
	}

	if (ymode == PPF) 
	{
		WritePPF(scale_height, fb->height);
		mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF; // context for scaling
	}
	
	if (xmode == TPZ) 
	{
		WriteTPZ(scale_width, fb->width);
	}

	if (ymode == TPZ) 
	{
		WriteTPZ(scale_height, fb->height);
		mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF; // context for scaling
	}

	if (ymode == PPF || xmode == PPF) 
	{
	// TODO, if PPF is in use, write 4 pointers to the scaling kernels
		uint32_t kernel = mScalingKernel;
		mpDlistMemory[mDisplaySlot++] = kernel;
		mpDlistMemory[mDisplaySlot++] = kernel;
		mpDlistMemory[mDisplaySlot++] = kernel;
		mpDlistMemory[mDisplaySlot++] = kernel;
	}
  
	mpDlistMemory[start] |= CONTROL_VALID | CONTROL_WORDS(mDisplaySlot - start);
	mpDlistMemory[mDisplaySlot++] = CONTROL_END;
}

void CHVSDisplay::AddPlane(hvs_surface *fb, int x, int y, bool hflip) 
{
  assert(fb);
  
  //Record start position in display list memory
  fb->hvsstart=mDisplaySlot;
  
  //printf("rendering FB of size %dx%d at %dx%d\n", fb->width, fb->height, x, y);
  mpDlistMemory[mDisplaySlot++] = CONTROL_VALID
    | CONTROL_WORDS(7)
    | CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_ARGB)
    //| CONTROL0_VFLIP // makes the HVS addr count down instead, pointer word must be last line of image
    | (hflip ? CONTROL0_HFLIP : 0)
    | CONTROL_UNITY
    | CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
  mpDlistMemory[mDisplaySlot++] = POS0_X(x) | POS0_Y(y) | POS0_ALPHA(0xff);
  mpDlistMemory[mDisplaySlot++] = POS2_H(fb->height) | POS2_W(fb->width);
  mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF; // dummy for HVS state
  mpDlistMemory[mDisplaySlot++] = BUS_ADDRESS((u32)fb->ptr);
  mpDlistMemory[mDisplaySlot++] = 0xDEADBEEF; // dummy for HVS state
  mpDlistMemory[mDisplaySlot++] = fb->stride * fb->pixelsize;
  mpDlistMemory[mDisplaySlot++] = CONTROL_END;
}

void CHVSDisplay::InterruptHandler (void)
{
	pixel_valve *rawpv = (pixel_valve*)(ARM_IO_BASE + 0x206000);
	uint32_t stat = rawpv->int_status;
	uint32_t ack = 0;
	
	if (stat & PV_INTEN_VFP_START) 
	{
		ack |= PV_INTEN_VFP_START;
		if(mFlipRequested)
		{
			write32(SCALER_DISPLIST0,mFB[mCurrFB+mScaled].hvsstart);
			mFlipRequested=0;
		}
		mFrameCount++;
		
	}
  
  	rawpv->int_status = ack;

  
}

void CHVSDisplay::InterruptStub (void *pParam)
{
	CHVSDisplay *pThis = (CHVSDisplay *) pParam;
	assert (pThis != 0);

	pThis->InterruptHandler ();
}

