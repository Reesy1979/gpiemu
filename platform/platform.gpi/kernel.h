//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _kernel_h
#define _kernel_h

#include <circle_stdlib_app.h>

#include <string.h>
#include <circle/usb/usbgamepad.h>
#include "hvsdisplay.h"
#include "pwmringbuffer.h"

#define MAX_GAMEPADS	2

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel : public CStdlibAppStdio
{
public:
	CKernel (const char *kernel);

	const char *GetKernelName(void);
        
	~CKernel (void);

	boolean Initialize (void);
	boolean Reset(void);
	
	TShutdownMode Run (void);

        u32 VideoGetBuffer(void);
        void VideoScale(int srcW, int srcH);
	void VideoFlip(bool vsync);
	void InputInitialise(void);
	u32 InputGet(void);
        u32 TimerGetTicks(void);
	void AudioInit(s32 rate, s32 Hz, int (*userCallback)(short *buffer, int samplecount));
	void AudioClose(void);
	u32 *AudioBufferGet(u32 bufferNo);
	u32 AudioBufferGetCurrIdx(void);
	void WaveoutInit(s32 rate);
	void WaveoutWrite(short *buffer, int samplecount);
	bool WaveoutIsActive(void);
	void Log(char *msg);
private:
	static void GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState);
	static void GamePadRemovedHandler (CDevice *pDevice, void *pContext);

	CUSBGamePadDevice * volatile m_pGamePad[MAX_GAMEPADS];
	
	char const *FromKernel;
        CHVSDisplay    *mpDisplay;
	CPWMRingBuffer     *mpSound;

};




#endif
