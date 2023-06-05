###############################################################################
# Makefile for a GP32 application.
###############################################################################

# Change the line below to the name of your program
PROGRAM =    libz

# If you have any directories with include files, add them here with -I on
# the front of each
INCLUDES = -I . 

# list all .c files here
CFILES	=	adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c trees.c \
       zutil.c inflate.c infback.c inftrees.c inffast.c  

SFILES = 	

##############################################################################
# You shouldn't need to change anything below here, really
##############################################################################

TOOLCHAIN = /opt/open2x/gcc-4.1.1-glibc-2.3.6


ALL_INCLUDES = -I$(TOOLCHAIN)/arm-open2x-linux/include $(INCLUDES)
ALL_LIBS = -L$(TOOLCHAIN)/arm-open2x-linux/lib $(LIBS)

CFLAGS     =  -g

LDFLAGS = 

OFILES  =       $(SFILES:.s=.o) $(CFILES:.c=.o)

AR = $(TOOLCHAIN)/bin/arm-open2x-linux-ar rcs
CC = $(TOOLCHAIN)/bin/arm-open2x-linux-gcc
STRIP = $(TOOLCHAIN)/bin/arm-open2x-linux-strip

all:		$(PROGRAM).a

clean:
	-rm -f $(OFILES) $(MAPFILE) $(PROGRAM)

.PHONY: all clean

%.o:	%.c
	$(CC) $(ALL_INCLUDES) $(CFLAGS) -c $< -o $@

%.o:	%.s
	$(CC) $(ALL_INCLUDES) $(CFLAGS) -c $< -o $@

$(PROGRAM).a: $(OFILES)
	$(AR) $@ $(OFILES)

