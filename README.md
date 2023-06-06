# gpiemu
Bare metal emulators and launcher for retroflag GPI v1

Bare metal environment provided by the circle-stdlib project.

https://github.com/smuehlst/circle-stdlib

## GPi Case Configuration

Your GPi Case must be in "Hat" input mode for correct operation.

If you're not able to navigate menus, set the input mode by holding **Up** on the D-Pad and **Select** for around five seconds, until the power LED flashes purple. Depending on your exact model of GPi Case, you may need to hold **Up** and **Start** instead.

## Build Instructions

1st follow setup instructions for circle-stdlib, see https://github.com/smuehlst/circle-stdlib

You should now have a toolchain and circle-stdlib installed.

Generate environment variables below, these are used by the makefiles in the various projects.
Alter the values to match your local installation of circle-stdlib and the toolchain

export CIRCLE_STDLIB=/home/dave/Documents/projects/circle-stdlib
export CIRCLE_TOOLCHAIN=/home/dave/Toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi

Need to configure Circle lib, we need to enable some additional settings.
Update the following file
$(CIRCLE_STDLIB)/libs/circle/include/circle/sysconfig.h

uncomment the following defines to enable PWM sound on the pi zero

USE_PWM_AUDIO_ON_ZERO

USE_GPIO18_FOR_LEFT_PWM_ON_ZERO

USE_GPIO19_FOR_RIGHT_PWM_ON_ZERO


and add a define for NO_USB_SOF_INTR - reduces USB overhead

#define NO_USB_SOF_INTR

Need to patch the libgloss io code to support file type properties in dirent.h.
Need to patch some of the USB code as well as circle struggles to init the USB fully.

See circle-patches folders
copy files to correct location
$(CIRCLE_STDLIB)/libs/circle-newlib/libgloss/circle/io.cpp
$(CIRCLE_STDLIB)/libs/circle-newlib/newlib/libc/sys/circle/sys/dirent.h
$(CIRCLE_STDLIB)/libs/circle/lib/usb/dwhcidevice.cpp
$(CIRCLE_STDLIB)/libs/circle/lib/usb/usbdevice.cpp

You should clean and rebuild circle-stdlib after this change
ie
make clean
make

Delete assert.h - causes conflicts, doesn't appear to be required..not in my code anyway.
$(CIRCLE_STDLIB)/install/arm-none-circle/include/assert.h

$(GPIEMU) = where ever you extracted the gpiemu repo to on your machine

Build Zlib
    cd $(GPIEMU)/zlib123
    make -f makefile.gpi clean
    make -f makefile.gpi
    make -f makefile.gpi install

Build libpng
    cd $(GPIEMU)/lpng1626
    make -f makefile.gpi clean
    make -f makefile.gpi
    make -f makefile.gpi install

Build GB emulator
    cd $(GPIEMU)/DingooGB
    make -f makefile.gpi.gb clean
    make -f makefile.gpi.gb

Build GBC emulator
    cd $(GPIEMU)/DingooGB
    make -f makefile.gpi.gbc clean
    make -f makefile.gpi.gbc

Build NES emulator
    cd $(GPIEMU)/DingooNES
    make -f makefile.gpi clean
    make -f makefile.gpi

