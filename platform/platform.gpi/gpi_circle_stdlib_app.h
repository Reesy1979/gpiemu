/**
 * Convenience classes that package different levels
 * of functionality of Circle applications.
 *
 * Derive the kernel class of the application from one of
 * the CStdlibApp* classes and implement at least the
 * Run () method. Extend the Initalize () and Cleanup ()
 * methods if necessary.
 */
#ifndef _gpi_circle_stdlib_app_h
#define _gpi_circle_stdlib_app_h

#include <circle/actled.h>
#include <circle/string.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/nulldevice.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <SDCard/emmc.h>
#include <circle/input/console.h>
#include <circle/sched/scheduler.h>
#include <circle/net/netsubsystem.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>

#include <circle_glue.h>
#include <string.h>

/**
 * Basic Circle Stdlib application that supports GPIO access.
 */
class CStdlibApp
{
public:
        enum TShutdownMode
        {
                ShutdownNone,
                ShutdownHalt,
                ShutdownReboot
        };

        CStdlibApp (const char *kernel) :
                FromKernel (kernel)
        {
        	mDeviceNameService = new CDeviceNameService();
        	mInterrupt = new CInterruptSystem();
        	mOptions = new CKernelOptions();
        	mNullDevice = new CNullDevice();
                mExceptionHandler = new CExceptionHandler();
        }

        virtual ~CStdlibApp (void)
        {
        }

        virtual bool Initialize (void)
        {
                return mInterrupt->Initialize ();
        }

        virtual void Cleanup (void)
        {
        	delete mExceptionHandler;
        	mExceptionHandler = 0;
        	
        	delete mNullDevice;
        	mNullDevice = 0;
        	
        	delete mOptions;
        	mOptions = 0;
        	
        	delete mInterrupt;
        	mInterrupt = 0;
        	
        	delete mDeviceNameService;
        	mDeviceNameService = 0;
        }

        virtual TShutdownMode Run (void) = 0;

        const char *GetKernelName(void) const
        {
                return FromKernel;
        }

protected:
        //CActLED            mActLED;
        CKernelOptions     *mOptions;
        CDeviceNameService *mDeviceNameService;
        CNullDevice        *mNullDevice;
        CExceptionHandler  *mExceptionHandler;
        CInterruptSystem   *mInterrupt;

private:
        char const *FromKernel;
};

/**
 * Stdlib application that adds screen support
 * to the basic CStdlibApp features.
 */
class CStdlibAppScreen : public CStdlibApp
{
public:
        CStdlibAppScreen(const char *kernel)
                : CStdlibApp (kernel)
        {
		   mScreen = new CScreenDevice(mOptions->GetWidth (), mOptions->GetHeight ());
		   mSerial = new CSerialDevice();
		   mTimer = new CTimer(mInterrupt);
		   mLogger = new CLogger(mOptions->GetLogLevel (), mTimer);
        }

        virtual bool Initialize (void)
        {
                if (!CStdlibApp::Initialize ())
                {
                        return false;
                }

                if (!mScreen->Initialize ())
                {
                        return false;
                }

                if (!mSerial->Initialize (115200))
                {
                        return false;
                }

                CDevice *pTarget = 
                        mDeviceNameService->GetDevice (mOptions->GetLogDevice (), false);
                if (pTarget == 0)
                {
                        pTarget = mScreen;
                }

                if (!mLogger->Initialize (pTarget))
                {
                        return false;
                }

                return mTimer->Initialize ();
        }

	virtual void Cleanup (void)
        {
        	delete mLogger;
        	mLogger = 0;
        	
        	delete mTimer;
        	mTimer = 0;
        	
        	delete mSerial;
        	mSerial = 0;
        	
        	delete mScreen;
        	mScreen = 0;
        	
        	CStdlibApp::Cleanup ();
        }
        
protected:
        CScreenDevice   *mScreen;
        CSerialDevice   *mSerial;
        CTimer          *mTimer;
        CLogger         *mLogger;
};

/**
 * Stdlib application that adds stdio support
 * to the CStdlibAppScreen functionality.
 */
class CStdlibAppStdio: public CStdlibAppScreen
{
private:
        char const *mpPartitionName;

public:
        // TODO transform to constexpr
        // constexpr char static DefaultPartition[] = "emmc1-1";
#define CSTDLIBAPP_LEGACY_DEFAULT_PARTITION "emmc1-1"
#define CSTDLIBAPP_DEFAULT_PARTITION "SD:"

        CStdlibAppStdio (const char *kernel,
                         const char *pPartitionName = CSTDLIBAPP_DEFAULT_PARTITION)
                : CStdlibAppScreen (kernel),
                  mpPartitionName (pPartitionName)
        {
        	mUSBHCI = new CUSBHCIDevice(mInterrupt, mTimer, FALSE);
		mEMMC = new CEMMCDevice(mInterrupt, mTimer, 0);
		mFileSystem = new FATFS();
		mConsole = new CConsole(0, TRUE);
        }

        virtual bool Initialize (void)
        {
                if (!CStdlibAppScreen::Initialize ())
                {
                        return false;
                }

		mLogger->Write (GetKernelName (), LogNotice, "Try EMMC");
		
                if (!mEMMC->Initialize ())
                {
                        return false;
                }

                char const *partitionName = mpPartitionName;

                // Recognize the old default partion name
                if (strcmp(partitionName, CSTDLIBAPP_LEGACY_DEFAULT_PARTITION) == 0)
                {
                        partitionName = CSTDLIBAPP_DEFAULT_PARTITION;
                }

		mLogger->Write (GetKernelName (), LogNotice, "Try mount");
		
                if (f_mount (mFileSystem, partitionName, 1) != FR_OK)
                {
                        mLogger->Write (GetKernelName (), LogError,
                                         "Cannot mount partition: %s", partitionName);

                        return false;
                }

		mLogger->Write (GetKernelName (), LogNotice, "Try Console");

                if (!mConsole->Initialize ())
                {
                        return false;
                }

		mLogger->Write (GetKernelName (), LogNotice, "Try CGlueStdioInit");
                // Initialize newlib stdio with a reference to Circle's console
                CGlueStdioInit ((CConsole&)mConsole);
                
                
		
		mLogger->Write (GetKernelName (), LogNotice, "Init USB 1");
                if (!mUSBHCI->Initialize ())
                {
                	mLogger->Write (GetKernelName (), LogError,
                                         "USB failed to init");
                        return false;
                }
	
                mLogger->Write (GetKernelName (), LogNotice, "Compile time: " __DATE__ " " __TIME__);
		
                return true;
        }

        virtual void Cleanup (void)
        {
                f_mount(0, "", 0);

		delete mUSBHCI;
		mUSBHCI = 0;
		delete mEMMC;
		mEMMC = 0;
		delete mFileSystem;
		mFileSystem = 0;
		delete mConsole;
		
		mConsole = 0;

                CStdlibAppScreen::Cleanup ();
        }

protected:
        CUSBHCIDevice   *mUSBHCI;
        CEMMCDevice     *mEMMC;
        FATFS           *mFileSystem;
        CConsole        *mConsole;
};

/**
 * Stdlib application that adds network functionality
 * to the CStdlibAppStdio features.
 */
class CStdlibAppNetwork: public CStdlibAppStdio
{
public:
        #define CSTDLIBAPP_WLAN_FIRMWARE_PATH   CSTDLIBAPP_DEFAULT_PARTITION "/firmware/"
        #define CSTDLIBAPP_WLAN_CONFIG_FILE     CSTDLIBAPP_DEFAULT_PARTITION "/wpa_supplicant.conf"

        CStdlibAppNetwork (const char *kernel,
                   const char *pPartitionName = CSTDLIBAPP_DEFAULT_PARTITION,
                   const u8 *pIPAddress      = 0,       // use DHCP if pIPAddress == 0
                   const u8 *pNetMask        = 0,
                   const u8 *pDefaultGateway = 0,
                   const u8 *pDNSServer      = 0,
                   TNetDeviceType DeviceType = NetDeviceTypeEthernet)
          : CStdlibAppStdio(kernel, pPartitionName),
            mDeviceType (DeviceType),
            mWLAN (CSTDLIBAPP_WLAN_FIRMWARE_PATH),
            mNet(pIPAddress, pNetMask, pDefaultGateway, pDNSServer, DEFAULT_HOSTNAME, DeviceType),
            mWPASupplicant (CSTDLIBAPP_WLAN_CONFIG_FILE)
        {
        }

        virtual bool Initialize (bool const bWaitForActivate = true)
        {
                if (!CStdlibAppStdio::Initialize ())
                {
                        return false;
                }

                if (mDeviceType == NetDeviceTypeWLAN)
                {
                        if (!mWLAN.Initialize ())
                        {
                                return false;
                        }
                }

                if (!mNet.Initialize (false))
                {
                        return false;
                }

                if (mDeviceType == NetDeviceTypeWLAN)
                {
                        if (!mWPASupplicant.Initialize ())
                        {
                                return false;
                        }
                }

                while (bWaitForActivate && !mNet.IsRunning ())
                {
                        mScheduler.Yield ();
                }

                return true;
        }

protected:
        CScheduler      mScheduler;
        TNetDeviceType  mDeviceType;
        CBcm4343Device  mWLAN;
        CNetSubSystem   mNet;
        CWPASupplicant  mWPASupplicant;
};
#endif
