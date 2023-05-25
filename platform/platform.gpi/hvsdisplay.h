//
// hvsdisplay.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2022  R. Stange <rsta2@o2online.de>
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
#ifndef _circle_hvsdisplay_h
#define _circle_hvsdisplay_h

#include <circle/interrupt.h>
#include <circle/types.h>
#include <circle/bcm2835.h>
#include <circle/bcm2835int.h>
#include <stdint.h>


#define SCALER_BASE (ARM_IO_BASE + 0x400000)

#define SCALER_DISPCTRL     (SCALER_BASE + 0x00)
#define SCALER_DISPSTAT     (SCALER_BASE + 0x04)
#define SCALER_DISPCTRL_ENABLE  (1<<31)
#define SCALER_DISPEOLN     (SCALER_BASE + 0x18)
#define SCALER_DISPLIST0    (SCALER_BASE + 0x20)
#define SCALER_DISPLIST1    (SCALER_BASE + 0x24)
#define SCALER_DISPLIST2    (SCALER_BASE + 0x28)

struct hvs_channel {
  volatile u32 dispctrl;
  volatile u32 dispbkgnd;
  volatile u32 dispstat;
  // 31:30  mode
  // 29     full
  // 28     empty
  // 17:12  frame count
  // 11:0   line
  volatile u32 dispbase;
};



struct hvs_channel_config {
  u32 width;
  u32 height;
  bool interlaced;
};



#define SCALER_STAT_LINE(n) ((n) & 0xfff)

#define SCALER_DISPCTRL0    (SCALER_BASE + 0x40)
#define SCALER_DISPCTRLX_ENABLE (1<<31)
#define SCALER_DISPCTRLX_RESET  (1<<30)
#define SCALER_DISPCTRL_W(n)    ((n & 0xfff) << 12)
#define SCALER_DISPCTRL_H(n)    (n & 0xfff)
#define SCALER_DISPBKGND_AUTOHS    (1<<31)
#define SCALER_DISPBKGND_INTERLACE (1<<30)
#define SCALER_DISPBKGND_GAMMA     (1<<29)
#define SCALER_DISPBKGND_FILL      (1<<24)

#define BASE_BASE(n) (n & 0xffff)
#define BASE_TOP(n) ((n & 0xffff) << 16)


#define SCALER_LIST_MEMORY  (ARM_IO_BASE + 0x402000)


#define CONTROL_FORMAT(n)       (n & 0xf)
#define CONTROL_END             (1<<31)
#define CONTROL_VALID           (1<<30)
#define CONTROL_WORDS(n)        (((n) & 0x3f) << 24)
#define CONTROL0_FIXED_ALPHA    (1<<19)
#define CONTROL0_HFLIP          (1<<16)
#define CONTROL0_VFLIP          (1<<15)
#define CONTROL_PIXEL_ORDER(n)  ((n & 3) << 13)
#define CONTROL_SCL1(scl)       (scl << 8)
#define CONTROL_SCL0(scl)       (scl << 5)
#define CONTROL_UNITY           (1<<4)

#define FRAME_BUFFER_COUNT      2

enum hvs_pixel_format {
	/* 8bpp */
	HVS_PIXEL_FORMAT_RGB332 = 0,
	/* 16bpp */
	HVS_PIXEL_FORMAT_RGBA4444 = 1,
	HVS_PIXEL_FORMAT_RGB555 = 2,
	HVS_PIXEL_FORMAT_RGBA5551 = 3,
	HVS_PIXEL_FORMAT_RGB565 = 4,
	/* 24bpp */
	HVS_PIXEL_FORMAT_RGB888 = 5,
	HVS_PIXEL_FORMAT_RGBA6666 = 6,
	/* 32bpp */
	HVS_PIXEL_FORMAT_RGBA8888 = 7,

	HVS_PIXEL_FORMAT_YCBCR_YUV420_3PLANE = 8,
	HVS_PIXEL_FORMAT_YCBCR_YUV420_2PLANE = 9,
	HVS_PIXEL_FORMAT_YCBCR_YUV422_3PLANE = 10,
	HVS_PIXEL_FORMAT_YCBCR_YUV422_2PLANE = 11,
	HVS_PIXEL_FORMAT_H264 = 12,
	HVS_PIXEL_FORMAT_PALETTE = 13,
	HVS_PIXEL_FORMAT_YUV444_RGB = 14,
	HVS_PIXEL_FORMAT_AYUV444_RGB = 15,
	HVS_PIXEL_FORMAT_RGBA1010102 = 16,
	HVS_PIXEL_FORMAT_YCBCR_10BIT = 17,
};

#define HVS_PIXEL_ORDER_RGBA			0
#define HVS_PIXEL_ORDER_BGRA			1
#define HVS_PIXEL_ORDER_ARGB			2
#define HVS_PIXEL_ORDER_ABGR			3

#define HVS_PIXEL_ORDER_XBRG			0
#define HVS_PIXEL_ORDER_XRBG			1
#define HVS_PIXEL_ORDER_XRGB			2
#define HVS_PIXEL_ORDER_XBGR			3

#define SCALER_CTL0_SCL_H_PPF_V_PPF		0
#define SCALER_CTL0_SCL_H_TPZ_V_PPF		1
#define SCALER_CTL0_SCL_H_PPF_V_TPZ		2
#define SCALER_CTL0_SCL_H_TPZ_V_TPZ		3
#define SCALER_CTL0_SCL_H_PPF_V_NONE		4
#define SCALER_CTL0_SCL_H_NONE_V_PPF		5
#define SCALER_CTL0_SCL_H_NONE_V_TPZ		6
#define SCALER_CTL0_SCL_H_TPZ_V_NONE		7

#define POS0_X(n) (n & 0xfff)
#define POS0_Y(n) ((n & 0xfff) << 12)
#define POS0_ALPHA(n) ((n & 0xff) << 24)

#define POS2_W(n) (n & 0xffff)
#define POS2_H(n) ((n & 0xffff) << 16)

#define SCALER_PPF_AGC (1<<30)

struct pv_timings {
  uint16_t vfp, vsync, vbp, vactive;
  uint16_t hfp, hsync, hbp, hactive;
  uint16_t vfp_even, vsync_even, vbp_even, vactive_even;
  bool interlaced;
  int clock_mux;
};

struct pixel_valve {
  volatile uint32_t c;
  volatile uint32_t vc;
  volatile uint32_t vsyncd_even;
  volatile uint32_t horza;
  volatile uint32_t horzb;
  volatile uint32_t verta;
  volatile uint32_t vertb;
  volatile uint32_t verta_even;
  volatile uint32_t vertb_even;
  volatile uint32_t int_enable;
  volatile uint32_t int_status;
  volatile uint32_t h_active;
};



#define PV_CONTROL_FIFO_CLR (1<<1)
#define PV_CONTROL_EN       (1<<0)

#define PV_INTEN_HSYNC_START (1<<0)
#define PV_INTEN_HBP_START   (1<<1)
#define PV_INTEN_HACT_START  (1<<2)
#define PV_INTEN_HFP_START   (1<<3)
#define PV_INTEN_VSYNC_START (1<<4)
#define PV_INTEN_VBP_START   (1<<5)
#define PV_INTEN_VACT_START  (1<<6)
#define PV_INTEN_VFP_START   (1<<7)
#define PV_INTEN_VFP_END     (1<<8)
#define PV_INTEN_IDLE        (1<<9)

enum scaling_mode {
  none,
  PPF, // upscaling?
  TPZ // downscaling?
};

typedef struct hvs_surface 
{
    void *ptr;
    u32 width;
    u32 height;
    u32 stride;
    u32 pixelsize;
    u32 len;
    u32 alpha;
    u32 hvsstart;
} hvs_surface;

class CHVSDisplay
{
public:
	CHVSDisplay (CInterruptSystem *pInterrupt, int height, int width, int stride, int pixelsize);

	virtual ~CHVSDisplay (void);

	boolean Initialize (void);
	u16 *GetBuffer(void);
	void Flip (bool vsync);
	void Scale(int srcW, int srcH);

protected:


private:
	void InterruptHandler (void);
	static void InterruptStub (void *pParam);

private:
	int GetPrevBufferIndex(void);
	void AddPlane(hvs_surface *fb, int x, int y, bool hflip);
	void AddPlaneScaled(hvs_surface *fb, int x, int y, int scale_height, int scale_width, bool hflip);
	void WriteTPZ(u32 source, u32 dest);
	void WritePPF(u32 source, u32 dest);
	void WipeDisplaylist(void);
	void UploadScalingKernel(void);
	CInterruptSystem *m_pInterruptSystem;

	boolean m_bIRQConnected;

	volatile u32* mpDlistMemory = (volatile u32*)SCALER_LIST_MEMORY;
	volatile hvs_channel *mpHvsChannels = (volatile hvs_channel*)SCALER_DISPCTRL0;
	
	pixel_valve *getPvAddr(int pvnr);
	
	int mScalingKernel=4080;
	int mDisplaySlot=0;
	int mScaledDispListStart=0;
	int mScaledLayerCount=0;
	int mCurrFB=0;
	int mScaled=0;
	int mScaledHeight=0;
	int mScaledWidth=0;
	volatile int mFlipRequested = 0;
	volatile int mFrameCount = 0;
	hvs_surface mFB[4];
#if defined(_NEED_TRANSPOSE)
	u16 *mTempFB;
#endif
};

#endif
