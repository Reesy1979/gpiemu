//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"

#include <stdio.h>
#include <circle/startup.h>
#include <circle/gpiopin.h>
#include <circle/chainboot.h>


static u32 buttons;

CKernel::CKernel (const char *kernel)
:	CStdlibAppStdio ("02-stdio-hello"),
	FromKernel (kernel)
{
	//mActLED.Blink (5);	// show we are alive
	//m_2DGraphics = new C2DGraphics(mOptions->GetWidth (), mOptions->GetHeight ());
	//mpFrameBuffer = new CBcmFrameBuffer (mOptions->GetWidth (), mOptions->GetHeight (), DEPTH,
	//			  mOptions->GetWidth (), mOptions->GetHeight (),
	//			  0, FALSE);
	mpDisplay = new CHVSDisplay(mInterrupt, 320, 240, 240, 2);
}

CKernel::~CKernel (void)
{
	//delete mpFrameBuffer;
	//mpFrameBuffer = 0;
	
	delete mpDisplay;
	mpDisplay = 0;
}


const char *CKernel::GetKernelName(void)
{
        return FromKernel;
}
        
boolean CKernel::Initialize () 
{
	//Seems to get the DPI display working.
	CGPIOPin gpio00 (0, GPIOModeAlternateFunction2);
	CGPIOPin gpio01 (1, GPIOModeAlternateFunction2);
	CGPIOPin gpio02 (2, GPIOModeAlternateFunction2);
	CGPIOPin gpio03 (3, GPIOModeAlternateFunction2);
	CGPIOPin gpio04 (4, GPIOModeAlternateFunction2);
	CGPIOPin gpio05 (5, GPIOModeAlternateFunction2);
	CGPIOPin gpio06 (6, GPIOModeAlternateFunction2);
	CGPIOPin gpio07 (7, GPIOModeAlternateFunction2);
	CGPIOPin gpio08 (8, GPIOModeAlternateFunction2);
	CGPIOPin gpio09 (9, GPIOModeAlternateFunction2);
	CGPIOPin gpio10 (10, GPIOModeAlternateFunction2);
	CGPIOPin gpio11 (11, GPIOModeAlternateFunction2);
	CGPIOPin gpio12 (12, GPIOModeAlternateFunction2);
	CGPIOPin gpio13 (13, GPIOModeAlternateFunction2);
	CGPIOPin gpio14 (14, GPIOModeAlternateFunction2);
	CGPIOPin gpio15 (15, GPIOModeAlternateFunction2);
	CGPIOPin gpio16 (16, GPIOModeAlternateFunction2);
	CGPIOPin gpio17 (17, GPIOModeAlternateFunction2);
	CGPIOPin gpio20 (20, GPIOModeAlternateFunction2);
	CGPIOPin gpio21 (21, GPIOModeAlternateFunction2);
	CGPIOPin gpio22 (22, GPIOModeAlternateFunction2);
	CGPIOPin gpio23 (23, GPIOModeAlternateFunction2);
	CGPIOPin gpio24 (24, GPIOModeAlternateFunction2);
	CGPIOPin gpio25 (25, GPIOModeAlternateFunction2);
	CGPIOPin gpio26 (26, GPIOModeAlternateFunction2);
	CGPIOPin gpio27 (27, GPIOModeAlternateFunction2);

	gpio00.SetPullMode(GPIOPullModeOff);
	gpio01.SetPullMode(GPIOPullModeOff);
	gpio02.SetPullMode(GPIOPullModeOff);
	gpio03.SetPullMode(GPIOPullModeOff);
	gpio04.SetPullMode(GPIOPullModeOff);
	gpio05.SetPullMode(GPIOPullModeOff);
	gpio06.SetPullMode(GPIOPullModeOff);
	gpio07.SetPullMode(GPIOPullModeOff);
	gpio08.SetPullMode(GPIOPullModeOff);
	gpio09.SetPullMode(GPIOPullModeOff);
	gpio10.SetPullMode(GPIOPullModeOff);
	gpio11.SetPullMode(GPIOPullModeOff);
	gpio12.SetPullMode(GPIOPullModeOff);
	gpio13.SetPullMode(GPIOPullModeOff);
	gpio14.SetPullMode(GPIOPullModeOff);
	gpio15.SetPullMode(GPIOPullModeOff);
	gpio16.SetPullMode(GPIOPullModeOff);
	gpio17.SetPullMode(GPIOPullModeOff);
	gpio20.SetPullMode(GPIOPullModeOff);
	gpio21.SetPullMode(GPIOPullModeOff);
	gpio22.SetPullMode(GPIOPullModeOff);
	gpio23.SetPullMode(GPIOPullModeOff);
	gpio24.SetPullMode(GPIOPullModeOff);
	gpio25.SetPullMode(GPIOPullModeOff);
	gpio26.SetPullMode(GPIOPullModeOff);
	gpio27.SetPullMode(GPIOPullModeOff);

#ifdef USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
	CGPIOPin audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction5);
#else
	CGPIOPin audio1 (GPIOPinAudioLeft, GPIOModeAlternateFunction0);
#endif // USE_GPIO18_FOR_LEFT_PWM_ON_ZERO
#ifdef USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
	CGPIOPin audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction5);
#else
	CGPIOPin audio2 (GPIOPinAudioRight, GPIOModeAlternateFunction0);
#endif // USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO
	audio1.SetPullMode(GPIOPullModeOff);
	audio2.SetPullMode(GPIOPullModeOff);
	
	if(!CStdlibAppStdio::Initialize ())
	{
		return false;
	}
	
	//if(!m_2DGraphics->Initialize ())
	//{
	//	return false;
	//}
	
	//if(!mpFrameBuffer->Initialize ())
	//{
	//	return false;
	//}
    
        if(!mpDisplay->Initialize ())
	{
		return false;
	}
    	
    	return true;
}

void CKernel::InputInitialise()
{
	if (m_pGamePad[0] != 0)
	{
		//Already done
		return;
	}

	while(m_pGamePad[0] == 0)
	{
		m_pGamePad[0] = (CUSBGamePadDevice *)
			mDeviceNameService->GetDevice ("upad", 1, FALSE);
			
		if(m_pGamePad[0] == 0)
		{
			mLogger->Write (GetKernelName (), LogError,
		                                 "Failed to get gamepad, resetting");
			delete mUSBHCI;
			mUSBHCI = 0;
			mUSBHCI = new CUSBHCIDevice(mInterrupt, mTimer, FALSE);
			if (!mUSBHCI->Initialize (TRUE))
		        {
		        	mLogger->Write (GetKernelName (), LogError,
		                                 "USB failed to init");
		                while(1)
		                {
		                }
		        }
		}
	}
	
	const TGamePadState *pState = m_pGamePad[0]->GetInitialState ();
	assert (pState != 0);

	mLogger->Write (GetKernelName (), LogNotice, "Gamepad %u: %d Button(s) %d Hat(s)",
			1, pState->nbuttons, pState->nhats);

	for (int i = 0; i < pState->naxes; i++)
	{
		mLogger->Write (GetKernelName (), LogNotice,
				"Gamepad %u: Axis %d: Minimum %d Maximum %d",
				1, i+1, pState->axes[i].minimum,
				pState->axes[i].maximum);
	}

	m_pGamePad[0]->RegisterRemovedHandler (GamePadRemovedHandler, this);
	m_pGamePad[0]->RegisterStatusHandler (GamePadStatusHandler);

	mLogger->Write (GetKernelName (), LogNotice, "Use your gamepad controls!");
}

void CKernel::Log(char *msg)
{
	mLogger->Write (GetKernelName (), LogNotice, msg);
}

void CKernel::GamePadStatusHandler (unsigned nDeviceIndex, const TGamePadState *pState)
{
	//CString Msg;
	//Msg.Format ("Gamepad %u: Buttons 0x%X", nDeviceIndex+1, pState->buttons);
	buttons=pState->buttons;

	//CLogger::Get ()->Write ("DAVE", LogNotice, Msg);
}

u32 CKernel::InputGet(void)
{
	return buttons;
}

void CKernel::GamePadRemovedHandler (CDevice *pDevice, void *pContext)
{
	CKernel *pThis = (CKernel *) pContext;
	assert (pThis != 0);

	for (unsigned i = 0; i < MAX_GAMEPADS; i++)
	{
		if (pThis->m_pGamePad[i] == (CUSBGamePadDevice *) pDevice)
		{
			//CLogger::Get ()->Write (pThis->GetKernelName (), LogDebug, "Gamepad %u removed", i+1);
			pThis->m_pGamePad[i] = 0;

			break;
		}
	}
}

void CKernel::VideoScale(int srcW, int srcH)
{
	mpDisplay->Scale(srcW, srcH);
}

u32 CKernel::VideoGetBuffer()
{
	//return (u32)m_2DGraphics->GetBuffer();
	//return (u32)mpFrameBuffer->GetBuffer();
	return (u32)mpDisplay->GetBuffer();
}

void CKernel::VideoFlip(bool vsync)
{
	//mLogger->Write (GetKernelName (), LogNotice, "flip");
	//m_2DGraphics->UpdateDisplay();
	//mpFrameBuffer->WaitForVerticalSync();
	mpDisplay->Flip(vsync);
}

u32 CKernel::TimerGetTicks()
{
	//return mTimer.GetTicks();
	return mTimer->GetClockTicks();
}

void CKernel::AudioInit(s32 rate, s32 Hz, int (*userCallback)(short *buffer, int samplecount))
{
	mpSound = new CPWMRingBuffer (mInterrupt, rate, 4096, userCallback);
	
	if (!mpSound->Start ())
	{
		mLogger->Write (GetKernelName (), LogPanic, "Cannot start sound device");
	}
	
}

void CKernel::AudioClose()
{
	mpSound->Cancel();
	while(mpSound->IsActive())
	{
	}
	delete mpSound;
}

u32 *CKernel::AudioBufferGet(u32 bufferNo)
{
	return mpSound->AudioBufferGet(bufferNo);
}

u32 CKernel::AudioBufferGetCurrIdx()
{
	return mpSound->AudioBufferGetCurrIdx();
}

CStdlibAppStdio::TShutdownMode CKernel::Run (void)
{
	return ShutdownHalt;
}

//const char[] kernelName = {"DAVE"};
CKernel Kernel("DAVE");
//CKernel *Kernel;

//extern "C"
//{

int KernelInit()
{ 
	if (!Kernel.Initialize ())
	{
		return 1;
	}
	
	Kernel.InputInitialise();

	return 0; 
}

int KernelReset()
{ 
	Kernel.Cleanup();

	return 0; 
}

u32 KernelVideoGetBuffer()
{
	return Kernel.VideoGetBuffer();
}

void KernelVideoScale(int srcW, int srcH)
{
	Kernel.VideoScale(srcW, srcH);
}

void KernelVideoFlip(bool vsync)
{
	Kernel.VideoFlip(vsync);
}

u32 KernelInputGet()
{
#if 0
        Y = 0x80
        X = 0x400
        B = 0x100
        A = 0x200
        UP = 0x8000
        DOWN = 0X20000
        LEFT = 0X40000
        RIGHT = 0X10000
	SELECT = 0X800
	START = 0X4000
	L = 0X20
	R = 0X40
#endif
	return Kernel.InputGet();
}

u32 KernelTimerGetTicks()
{
	return Kernel.TimerGetTicks();
}

void KernelAudioInit(s32 rate, s32 Hz, int (*userCallback)(short *buffer, int samplecount))
{
	Kernel.AudioInit(rate, Hz, userCallback);
}

void KernelAudioClose()
{
	Kernel.AudioClose();
}

u16 *KernelAudioBufferGet(u32 bufferNo)
{
	return (u16*)Kernel.AudioBufferGet(bufferNo);
}

u32 KernelAudioBufferGetCurrIdx(void)
{
	return Kernel.AudioBufferGetCurrIdx();
}

void KernelChainBoot(const void *pKernelImage, size_t nKernelSize)
{
	EnableChainBoot (pKernelImage, nKernelSize);
}

#if 1
void KernelLog(char *msg)
{

	FILE *f = fopen("log.txt","ab");
	if(f)
	{
		fwrite(msg,1, strlen(msg),f);
		fclose(f);
	}

}

extern "C" void KernelLogC(char *msg)
{

	FILE *f = fopen("log.txt","ab");
	if(f)
	{
		fwrite(msg,1, strlen(msg),f);
		fclose(f);
	}

}
#endif

//}