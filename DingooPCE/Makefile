PLATFORM = platform.a320.native

# Define the applications properties here:

APP_FILE    = DingooPCE
APP_VERSION = 1.0
APP_NAME    = Dingoo PCE emulator
APP_TYPES   = PCE|SGX

# Define the compiler settings here:

CPP = mipsel-linux-g++
CC  = mipsel-linux-gcc
LD  = mipsel-linux-ld

# Toolchain includes
TOOLCHAIN_INC = -I $(MIPSTOOLS)/mipsel-linux/include \
                -I $(DINGOO_SDK)/include \
                -I $(DINGOO_SDK)/include/sys \
                -I $(DINGOO_SDK)/include/zlib \
                -I $(DINGOO_SDK)/include/stlport

# Platform sources
PLATFORM_SRC  = ../platform/$(PLATFORM) \
               	../platform/components \
            	../platform/components/zip \
            	../platform

# Platform includes
PLATFORM_INC  = -I ../platform/$(PLATFORM) \
            	-I ../platform/components \
            	-I ../platform/components/zip \
            	-I ../platform

# Emulator sources
EMULATOR_SRC  = ./mednafen \
            	./mednafen/pce_fast \
            	./mednafen/cdrom \
            	./mednafen/hw_misc/arcade_card \
            	./mednafen/hw_sound/pce_psg \
            	./mednafen/sound \
            	./mednafen/video \
            	./mednafen/compress \
            	./emulator

# Emulator includes
EMULATOR_INC  = -I ./mednafen \
            	-I ./mednafen/include \
            	-I ./mednafen/hw_misc \
            	-I ./mednafen/hw_sound \
            	-I ./resources \
            	-I ./emulator

# Compilator and linker flags
SOURCE    = $(PLATFORM_SRC) $(EMULATOR_SRC)
INCLUDE   = $(TOOLCHAIN_INC) $(PLATFORM_INC) $(EMULATOR_INC)

OPTIMIZE  = -O3 -march=mips32 -mtune=mips32r2
OPTIMIZE += -ffast-math
OPTIMIZE += -fsingle-precision-constant
OPTIMIZE += -fno-strength-reduce
OPTIMIZE += -mbranch-likely
OPTIMIZE += -funroll-loops
OPTIMIZE += -fno-builtin

#OPTIMIZE  = -O3 -mips32

DEFINES   = -DMPU_JZ4740 -D_DINGOO -DNDEBUG
DEFINES  += -DEMU_DISPLAY_NAME="\"$(APP_NAME)\"" -DEMU_VERSION="\"$(APP_VERSION)\"" -DEMU_FILE_NAME="\"$(APP_FILE)\"" -DEMU_FILE_TYPES="\"$(APP_TYPES)\""
DEFINES  += -DHAVE_CONFIG_H -DSysDDec=float
DEFINES  += -DDISABLE_REGION_SELECT -DDISABLE_CHEATING -DDISABLE_IPS_PATCHING

CC_OPTS   = -G0 -c -Wall -Wextra $(OPTIMIZE) -mno-abicalls -msoft-float $(INCLUDE) $(DEFINES)
CC_OPTS_A = $(CC_OPTS) -D_ASSEMBLER_
CPP_OPTS  = $(CC_OPTS) -fno-rtti -fno-exceptions -fno-threadsafe-statics

LIB_PATH  = $(DINGOO_SDK)/lib
LIBS      = -lpng -lz -lsml -lc -lm -ljz4740 -lgcc -lstdc++

LD_SCRIPT = $(DINGOO_SDK)/lib/dingoo.xn
LD_OPTS   = -nodefaultlibs --script $(LD_SCRIPT) -L$(LIB_PATH) $(LIBS) -o $(APP_FILE).elf

# Find all source files

SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
SRC_S   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.S))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ_S   = $(patsubst %.S, %.o, $(SRC_S))
OBJ     = $(OBJ_CPP) $(OBJ_C) $(OBJ_S)

# Compile rules.

.PHONY : all

all : $(APP_FILE).sim

$(APP_FILE).sim : $(APP_FILE).app
	cp $(APP_FILE).app $(APP_FILE).sim

$(APP_FILE).app : $(APP_FILE).elf
	$(DINGOO_SDK)/tools/elf2app/elf2app $(APP_FILE)

$(APP_FILE).elf : $(OBJ)
	$(LD) $(OBJ) $(LD_OPTS)

$(OBJ_CPP) : %.o : %.cpp
	$(CPP) $(CPP_OPTS) -o $@ $<

$(OBJ_C) : %.o : %.c
	$(CC) $(CC_OPTS) -o $@ $<

$(OBJ_S) : %.o : %.S
	$(CC) $(CC_OPTS_A) -o $@ $<

# Clean rules

.PHONY : clean

clean :
	rm -f $(OBJ) $(APP_FILE).elf $(APP_FILE).bin $(APP_FILE).app $(APP_FILE).sim
