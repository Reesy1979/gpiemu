# gpiemu
Bare metal emulators and launcher for retroflag GPI v1

Bare metal environment provided by the circle-stdlib project.

https://github.com/smuehlst/circle-stdlib

## GPi Case Configuration

Your GPi Case must be in "Hat" input mode for correct operation.

If you're not able to navigate menus, set the input mode by holding **Up** on the D-Pad and **Select** for around five seconds, until the power LED flashes purple. Depending on your exact model of GPi Case, you may need to hold **Up** and **Start** instead.

## Build Instructions

1st follow setup instructions for circle-stdlib, see https://github.com/smuehlst/circle-stdlib<br>
<br>
You should now have a toolchain and circle-stdlib installed.<br>
<br>
Generate environment variables below, these are used by the makefiles in the various projects.
Alter the values to match your local installation of circle-stdlib and the toolchain
<br>
export CIRCLE_STDLIB=/home/dave/Documents/projects/circle-stdlib<br>
export CIRCLE_TOOLCHAIN=/home/dave/Toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi<br>
<br>
Need to configure Circle lib, we need to enable some additional settings.<br>
Update the following file<br>
$(CIRCLE_STDLIB)/libs/circle/include/circle/sysconfig.h<br>
<br>
uncomment the following defines to enable PWM sound on the pi zero<br>
<br>
USE_PWM_AUDIO_ON_ZERO<br>
USE_GPIO18_FOR_LEFT_PWM_ON_ZERO<br>
USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO<br>
<br>
and add a define for NO_USB_SOF_INTR - reduces USB overhead<br>
#define NO_USB_SOF_INTR<br>
<br>
Need to patch the libgloss io code to support file type properties in dirent.h.<br>
Need to patch some of the USB code as well as circle struggles to init the USB fully.<br>
<br>
See circle-patches folders<br>
copy files to correct location<br>
$(CIRCLE_STDLIB)/libs/circle-newlib/libgloss/circle/io.cpp<br>
$(CIRCLE_STDLIB)/libs/circle-newlib/newlib/libc/sys/circle/sys/dirent.h<br>
$(CIRCLE_STDLIB)/libs/circle/lib/usb/dwhcidevice.cpp<br>
$(CIRCLE_STDLIB)/libs/circle/lib/usb/usbdevice.cpp<br>
<br>
You should clean and rebuild circle-stdlib after this change<br>
ie<br>
make clean<br>
make<br>
<br>
Delete assert.h - causes conflicts, doesn't appear to be required..not in my code anyway.<br>
$(CIRCLE_STDLIB)/install/arm-none-circle/include/assert.h<br>
<br>
$(GPIEMU) = where ever you extracted the gpiemu repo to on your machine<br>
<br>
Build Zlib<br>
    cd $(GPIEMU)/zlib123<br>
    make -f makefile.gpi clean<br>
    make -f makefile.gpi<br>
    make -f makefile.gpi install<br>
<br>
Build libpng<br>
    cd $(GPIEMU)/lpng1626<br>
    make -f makefile.gpi clean<br>
    make -f makefile.gpi<br>
    make -f makefile.gpi install<br>
<br>
Build GB emulator<br>
    cd $(GPIEMU)/DingooGB<br>
    make -f makefile.gpi.gb clean<br>
    make -f makefile.gpi.gb<br>
<br>
Build GBC emulator<br>
    cd $(GPIEMU)/DingooGB<br>
    make -f makefile.gpi.gbc clean<br>
    make -f makefile.gpi.gbc<br>
<br>
Build NES emulator<br>
    cd $(GPIEMU)/DingooNES<br>
    make -f makefile.gpi clean<br>
    make -f makefile.gpi<br>

