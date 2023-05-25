#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

#include "defs.h"
#include "loader.h"
#include "regs.h"
#include "mem.h"
#include "gbhw.h"
#include "rtc.h"
#include "rc.h"
#include "lcd.h"

#if !defined(_GPI) && !defined(_LINUX)
#include "inflate.h"
#endif
#include "miniz.h"
#define XZ_USE_CRC64
#include "xz/xz.h"
#include "save.h"
#include "sound.h"
#include "sys.h"

static int mbc_table[256] =
{
	0, 1, 1, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3,
	3, 3, 3, 3, 0, 0, 0, 0, 0, 5, 5, 5, MBC_RUMBLE, MBC_RUMBLE, MBC_RUMBLE, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, MBC_HUC3, MBC_HUC1
};

static int rtc_table[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

static int batt_table[256] =
{
	0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	0
};

static int romsize_table[256] =
{
	2, 4, 8, 16, 32, 64, 128, 256, 512,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 128, 128, 128
	/* 0, 0, 72, 80, 96  -- actual values but bad to use these! */
};

static int ramsize_table[256] =
{
	1, 1, 1, 4, 16,
	4 /* FIXME - what value should this be?! */
};


static char *bootroms[2];
static char *romfile;
static char *sramfile;
static char *rtcfile;
static char *saveprefix;

static char *savename;
static char *savedir;

static int saveslot;

static int forcebatt, nobatt;
static int forcedmg, gbamode;

static int memfill = -1, memrand = -1;


static void initmem(void *mem, int size)
{
	char *p = mem;
	if (memrand >= 0)
	{
		srand(memrand ? memrand : time(0));
		while(size--) *(p++) = rand();
	}
	else if (memfill >= 0)
		memset(p, memfill, size);
}

int core_load_rom(char *filename, void **data, int *len);

int bootrom_load() {
	byte *data;
	int len;
	
	REG(RI_BOOT) = 0xff;
	if (!bootroms[gbhw.cgb] || !bootroms[gbhw.cgb][0]) return 0;
	if(!core_load_rom(bootroms[gbhw.cgb], &data, &len)) return -1;
	bootrom.bank = realloc(data, 16384);
	memset(bootrom.bank[0]+len, 0xff, 16384-len);
	memcpy(bootrom.bank[0]+0x100, rom.bank[0]+0x100, 0x100);
	REG(RI_BOOT) = 0xfe;
	return 0;
}

extern void Debug(char *msg, ...);

int rom_load()
{
	byte c, *data, *header;
	int len = 0, rlen;
	
	if(!core_load_rom(romfile, &data, &len)) return -1;
	
	header = data;
	Debug("rom_load 1\n");
	memcpy(rom.name, header+0x0134, 16);
	if (rom.name[14] & 0x80) rom.name[14] = 0;
	if (rom.name[15] & 0x80) rom.name[15] = 0;
	rom.name[16] = 0;

	c = header[0x0147];
	mbc.type = mbc_table[c];
	mbc.batt = (batt_table[c] && !nobatt) || forcebatt;
	rtc.batt = rtc_table[c];
	mbc.romsize = romsize_table[header[0x0148]];
	mbc.ramsize = ramsize_table[header[0x0149]];

	Debug("rom_load 2\n");
	if (!mbc.romsize) {
		return -1;
	}
	if (!mbc.ramsize) {
		return -1;
	}

	rlen = 16384 * mbc.romsize;

	c = header[0x0143];

	/* from this point on, we may no longer access data and header */
	rom.bank = realloc(data, rlen);
	if (rlen > len) memset(rom.bank[0]+len, 0xff, rlen - len);

	ram.sbank = malloc(8192 * mbc.ramsize);

	initmem(ram.sbank, 8192 * mbc.ramsize);
	initmem(ram.ibank, 4096 * 8);

	mbc.rombank = 1;
	mbc.rambank = 0;

	gbhw.cgb = ((c == 0x80) || (c == 0xc0)) && !forcedmg;
	gbhw.gba = (gbhw.cgb && gbamode);
    Debug("rom_load end\n");
	return 0;
}

int rom_load_simple(char *fn) {
	romfile = fn;
	return rom_load();
}

int sram_load()
{
	FILE *f;

	if (!mbc.batt || !sramfile || !*sramfile) return -1;

	/* Consider sram loaded at this point, even if file doesn't exist */
	ram.loaded = 1;

	f = fopen(sramfile, "rb");
	if (!f) return -1;
	fread(ram.sbank, 8192, mbc.ramsize, f);
	fclose(f);
	
	return 0;
}


int sram_save()
{
	FILE *f;

	/* If we crash before we ever loaded sram, DO NOT SAVE! */
	if (!mbc.batt || !sramfile || !ram.loaded || !mbc.ramsize)
		return -1;
	
	f = fopen(sramfile, "wb");
	if (!f) return -1;
	fwrite(ram.sbank, 8192, mbc.ramsize, f);
	fclose(f);
	
	return 0;
}


void state_save(int n)
{
	FILE *f;
	char *name;

	if (n < 0) n = saveslot;
	if (n < 0) n = 0;
	name = malloc(strlen(saveprefix) + 5);
	sprintf(name, "%s.%03d", saveprefix, n);

	if ((f = fopen(name, "wb")))
	{
		savestate(f);
		fclose(f);
	}
	free(name);
}


void state_load(int n)
{
	FILE *f;
	char *name;

	if (n < 0) n = saveslot;
	if (n < 0) n = 0;
	name = malloc(strlen(saveprefix) + 5);
	sprintf(name, "%s.%03d", saveprefix, n);

	if ((f = fopen(name, "rb")))
	{
		loadstate(f);
		fclose(f);
		vram_dirty();
		pal_dirty();
		sound_dirty();
		mem_updatemap();
	}
	free(name);
}

void rtc_save()
{
	FILE *f;
	if (!rtc.batt) return;
	if (!(f = fopen(rtcfile, "wb"))) return;
	rtc_save_internal(f);
	fclose(f);
}

void rtc_load()
{
	FILE *f;
	if (!rtc.batt) return;
	if (!(f = fopen(rtcfile, "r"))) return;
	rtc_load_internal(f);
	fclose(f);
}

#define FREENULL(X) do { free(X); X = 0; } while(0)
void loader_unload()
{
	sram_save();
	if (romfile) FREENULL(romfile);
	if (sramfile) FREENULL(sramfile);
	if (saveprefix) FREENULL(saveprefix);
	if (rom.bank) FREENULL(rom.bank);
	if (ram.sbank) FREENULL(ram.sbank);
	if (bootrom.bank) FREENULL(bootrom.bank);
	mbc.type = mbc.romsize = mbc.ramsize = mbc.batt = 0;
}

static char *base(char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) return p+1;
	return s;
}

static char *ldup(char *s)
{
	int i;
	char *n, *p;
	p = n = malloc(strlen(s));
	for (i = 0; s[i]; i++) if (isalnum(s[i])) *(p++) = tolower(s[i]);
	*p = 0;
	return n;
}

static void cleanup()
{
	sram_save();
	rtc_save();
	/* IDEA - if error, write emergency savestate..? */
}

int loader_init(char *s)
{
	char *name, *p;

	romfile = s;

	if(rom_load())
	{
		return -1;
	}

	bootrom_load();

	vid_settitle(rom.name);

	if (savename && *savename)
	{
		if (savename[0] == '-' && savename[1] == 0)
			name = ldup(rom.name);
		else name = strdup(savename);
	}
	else if (romfile && *base(romfile) && strcmp(romfile, "-"))
	{
		name = strdup(base(romfile));
		p = strchr(name, '.');
		if (p) *p = 0;
	}
	else name = ldup(rom.name);

	Debug("loader_init 4 - savedir:%s   name:%s\n", savedir, name);
	saveprefix = malloc(strlen(savedir) + strlen(name) + 2);
	sprintf(saveprefix, "%s/%s", savedir, name);
	//saveprefix = malloc(strlen(name) + 2);
	//sprintf(saveprefix, "./%s", savedir, name);

	Debug("loader_init 5\n");
	sramfile = malloc(strlen(saveprefix) + 5);
	strcpy(sramfile, saveprefix);
	strcat(sramfile, ".sav");

	Debug("loader_init 6\n");
	rtcfile = malloc(strlen(saveprefix) + 5);
	strcpy(rtcfile, saveprefix);
	strcat(rtcfile, ".rtc");
    Debug("loader_init 7\n");
	sram_load();
	rtc_load();;
	atexit(cleanup);
	return 0;
}

rcvar_t loader_exports[] =
{
	RCV_STRING("bootrom_dmg", &bootroms[0], "bootrom for DMG games"),
	RCV_STRING("bootrom_cgb", &bootroms[1], "bootrom for CGB games"),
	RCV_STRING("savedir", &savedir, "save directory"),
	RCV_STRING("savename", &savename, "base filename for saves"),
	RCV_INT("saveslot", &saveslot, "which savestate slot to use"),
	RCV_BOOL("forcebatt", &forcebatt, "save SRAM even on carts w/o battery"),
	RCV_BOOL("nobatt", &nobatt, "never save SRAM"),
	RCV_BOOL("forcedmg", &forcedmg, "force DMG mode for CGB carts"),
	RCV_BOOL("gbamode", &gbamode, "simulate cart being used on a GBA"),
	RCV_INT("memfill", &memfill, ""),
	RCV_INT("memrand", &memrand, ""),
	RCV_END
};









