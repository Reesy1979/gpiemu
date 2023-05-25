
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <zlib.h>
#include "pico/PicoInt.h"
#include "pico/Patch.h"

char romFileName[256];
char ipsFileName[256];
char menuErrorMsg[64];
char noticeMsg   [64];

unsigned char* rom_data   = NULL;
unsigned char* movie_data = NULL;
static int	   movie_size = 0;

// provided by platform code:
extern void emu_noticeMsgUpdated(void);
extern void emu_getMainDir(char *dst, int len);
extern void menu_romload_prepare(const char *rom_name);
extern void menu_romload_end(void);

// utilities
static void strlwr_(char* string)
{
	while ( (*string++ = (char)tolower(*string)) );
}

static int try_rfn_cut(void)
{
	FILE *tmp;
	char *p;

	p = romFileName + strlen(romFileName) - 1;
	for (; p > romFileName; p--)
		if (*p == '.') break;
	*p = 0;

	if((tmp = fopen(romFileName, "rb"))) {
		fclose(tmp);
		return 1;
	}
	return 0;
}

static void get_ext(char *file, char *ext)
{
	char *p;

	p = file + strlen(file) - 4;
	if (p < file) p = file;
	strncpy(ext, p, 4);
	ext[4] = 0;
	strlwr_(ext);
}

char *biosfiles_us[] = { "us_scd2_9306", "SegaCDBIOS9303", "us_scd1_9210" };
char *biosfiles_eu[] = { "eu_mcd2_9306", "eu_mcd2_9303",   "eu_mcd1_9210" };
char *biosfiles_jp[] = { "jp_mcd1_9112", "jp_mcd1_9111" };

int emu_findBios(int region, char **bios_file)
{
	static char bios_path[1024];
	int i, count;
	char **files;
	FILE *f = NULL;

	if (region == 4) { // US
		files = biosfiles_us;
		count = sizeof(biosfiles_us) / sizeof(char *);
	} else if (region == 8) { // EU
		files = biosfiles_eu;
		count = sizeof(biosfiles_eu) / sizeof(char *);
	} else if (region == 1 || region == 2) {
		files = biosfiles_jp;
		count = sizeof(biosfiles_jp) / sizeof(char *);
	} else {
		return 0;
	}

	for (i = 0; i < count; i++)
	{
		emu_getMainDir(bios_path, sizeof(bios_path));
		strcat(bios_path, files[i]);
		strcat(bios_path, ".bin");
		f = fopen(bios_path, "rb");
		if (f) break;

		bios_path[strlen(bios_path) - 4] = 0;
		strcat(bios_path, ".zip");
		f = fopen(bios_path, "rb");
		if (f) break;
	}

	if (f) {
		lprintf("using bios: %s\n", bios_path);
		fclose(f);
		if (bios_file) *bios_file = bios_path;
		return 1;
	} else {
		sprintf(menuErrorMsg, "no %s BIOS files found, read docs",
			region != 4 ? (region == 8 ? "EU" : "JAP") : "USA");
		lprintf("%s\n", menuErrorMsg);
		return 0;
	}
}

/* check if the name begins with BIOS name */
static int emu_isBios(const char *name)
{
	int i;
	for (i = 0; i < sizeof(biosfiles_us)/sizeof(biosfiles_us[0]); i++)
		if (strstr(name, biosfiles_us[i]) != NULL) return 1;
	for (i = 0; i < sizeof(biosfiles_eu)/sizeof(biosfiles_eu[0]); i++)
		if (strstr(name, biosfiles_eu[i]) != NULL) return 1;
	for (i = 0; i < sizeof(biosfiles_jp)/sizeof(biosfiles_jp[0]); i++)
		if (strstr(name, biosfiles_jp[i]) != NULL) return 1;
	return 0;
}

/* checks if romFileName points to valid MegaCD image
 * if so, checks for suitable BIOS */
int emu_cdCheck(int *pregion)
{
	unsigned char buf[32];
	pm_file *cd_f;
	int type = 0, region = 4; // 1: Japan, 4: US, 8: Europe

	cd_f = pm_open(romFileName);
	if (!cd_f) return 0; // let the upper level handle this

	if (pm_read(buf, 32, cd_f) != 32) {
		pm_close(cd_f);
		return 0;
	}

	if (!strncasecmp("SEGADISCSYSTEM", (char *)buf+0x00, 14)) type = 1;       // Sega CD (ISO)
	if (!strncasecmp("SEGADISCSYSTEM", (char *)buf+0x10, 14)) type = 2;       // Sega CD (BIN)
	if (type == 0) {
		pm_close(cd_f);
		return 0;
	}

	/* it seems we have a CD image here. Try to detect region now.. */
	pm_seek(cd_f, (type == 1) ? 0x100+0x10B : 0x110+0x10B, SEEK_SET);
	pm_read(buf, 1, cd_f);
	pm_close(cd_f);

	if (buf[0] == 0x64) region = 8; // EU
	if (buf[0] == 0xa1) region = 1; // JAP

	lprintf("detected %s Sega/Mega CD image with %s region\n",
		type == 2 ? "BIN" : "ISO", region != 4 ? (region == 8 ? "EU" : "JAP") : "USA");

	if (pregion != NULL) *pregion = region;

	return type;
}

static void romfname_ext(char *dst, const char *prefix, const char *ext)
{
	char *p;
	int prefix_len = 0;

	// make save filename
	for (p = romFileName+strlen(romFileName)-1; p >= romFileName && *p != '/'; p--); p++;
	*dst = 0;
	if (prefix) {
		strcpy(dst, prefix);
		prefix_len = strlen(prefix);
	}
	strncpy(dst + prefix_len, p, 511-prefix_len);
	dst[511-8] = 0;
	if (dst[strlen(dst)-4] == '.') dst[strlen(dst)-4] = 0;
	if (ext) strcat(dst, ext);
}

int emu_ReloadRom(void)
{
	unsigned int rom_size = 0;
	char *used_rom_name = romFileName;
	char ext[5];
	pm_file *rom;
	int ret, cd_state, cd_region, cfg_loaded = 0;

	lprintf("emu_ReloadRom(%s)\n", romFileName);

	get_ext(romFileName, ext);

	// detect wrong extensions
	if(!strcmp(ext, ".srm") || !strcmp(ext, "s.gz") || !strcmp(ext, ".mds")) { // s.gz ~ .mds.gz
		sprintf(menuErrorMsg, "Not a ROM selected.");
		return 0;
	}

	PicoPatchUnload();

	// check for movie file
	if(movie_data) {
		free(movie_data);
		movie_data = 0;
	}
#if 0
	if(!strcmp(ext, ".gmv")) {
		// check for both gmv and rom
		int dummy;
		FILE *movie_file = fopen(romFileName, "rb");
		if(!movie_file) {
			sprintf(menuErrorMsg, "Failed to open movie.");
			return 0;
		}
		fseek(movie_file, 0, SEEK_END);
		movie_size = ftell(movie_file);
		fseek(movie_file, 0, SEEK_SET);
		if(movie_size < 64+3) {
			sprintf(menuErrorMsg, "Invalid GMV file.");
			fclose(movie_file);
			return 0;
		}
		movie_data = malloc(movie_size);
		if(movie_data == NULL) {
			sprintf(menuErrorMsg, "low memory.");
			fclose(movie_file);
			return 0;
		}
		fread(movie_data, 1, movie_size, movie_file);
		fclose(movie_file);
		if (strncmp((char *)movie_data, "Gens Movie TEST", 15) != 0) {
			sprintf(menuErrorMsg, "Invalid GMV file.");
			return 0;
		}
		dummy = try_rfn_cut() || try_rfn_cut();
		if (!dummy) {
			sprintf(menuErrorMsg, "Could't find a ROM for movie.");
			return 0;
		}
		get_ext(romFileName, ext);
	} else 

	if (!strcmp(ext, ".pat")) {
		int dummy;
		PicoPatchLoad(romFileName);
		dummy = try_rfn_cut() || try_rfn_cut();
		if (!dummy) {
			sprintf(menuErrorMsg, "Could't find a ROM to patch.");
			return 0;
		}
		get_ext(romFileName, ext);
	}
#else
	// load GG patches list
	char ggFname[512];
	romfname_ext(ggFname, NULL, ".cht");
	PicoPatchLoad(ggFname);
#endif

	if ((PicoMCD & 1) && Pico_mcd != NULL) Stop_CD();

	// check for MegaCD image
	cd_state = emu_cdCheck(&cd_region);
	if (cd_state > 0)
	{
		// valid CD image, check for BIOS..
#if 0
		// we need to have config loaded at this point
		ret = emu_ReadConfig(READ_CONFIG_FOR_GAME, READ_CONFIG_SKIP_DEF);
		if (!ret) emu_ReadConfig(READ_CONFIG_GLOBAL, READ_CONFIG_SKIP_DEF);
#endif
		cfg_loaded = 1;

		if (PicoRegionOverride) {
			cd_region = PicoRegionOverride;
			lprintf("overrided region to %s\n", cd_region != 4 ? (cd_region == 8 ? "EU" : "JAP") : "USA");
		}
		if (!emu_findBios(cd_region, &used_rom_name)) {
			// bios_help() ?
			return 0;
		}

		PicoMCD |= 1;
		get_ext(used_rom_name, ext);
	} else
	{
		if (PicoMCD & 1) Stop_CD();
		PicoMCD &= ~1;
	}

	ips_define(ipsFileName);
	rom = pm_open(used_rom_name);
	if(!rom) {
		sprintf(menuErrorMsg, "Failed to open rom.");
		return 0;
	}

	menu_romload_prepare(used_rom_name); // also CD load

	if(rom_data) {
		free(rom_data);
		rom_data = 0;
		rom_size = 0;
	}

	if( (ret = PicoCartLoad(rom, &rom_data, &rom_size)) ) {
		sprintf(menuErrorMsg, "PicoCartLoad() failed.");
		lprintf("%s\n", menuErrorMsg);
		pm_close(rom);
		menu_romload_end();
		return 0;
	}
	pm_close(rom);

	// detect wrong files (Pico crashes on very small files), also see if ROM EP is good
	if(rom_size <= 0x200 || strncmp((char *)rom_data, "Pico", 4) == 0 ||
	  ((*(unsigned char *)(rom_data+4)<<16)|(*(unsigned short *)(rom_data+6))) >= (int)rom_size) {
		if (rom_data) free(rom_data);
		rom_data = 0;
		sprintf(menuErrorMsg, "Not a ROM selected.");
		menu_romload_end();
		return 0;
	}
#if 0
	// load config for this ROM (do this before insert to get correct region)
	if (!cfg_loaded) {
		ret = emu_ReadConfig(READ_CONFIG_FOR_GAME, READ_CONFIG_SKIP_DEF);
		if (!ret) emu_ReadConfig(READ_CONFIG_GLOBAL, READ_CONFIG_SKIP_DEF);
	}
#endif
	lprintf("PicoCartInsert(%p, %d);\n", rom_data, rom_size);
	if(PicoCartInsert(rom_data, rom_size)) {
		sprintf(menuErrorMsg, "Failed to load ROM.");
		menu_romload_end();
		return 0;
	}

	Pico.m.frame_count = 0;

	// insert CD if it was detected
	if (cd_state > 0) {
		ret = Insert_CD(romFileName, cd_state == 2);
		if (ret != 0) {
			sprintf(menuErrorMsg, "Insert_CD() failed, invalid CD image?");
			lprintf("%s\n", menuErrorMsg);
			menu_romload_end();
			return 0;
		}
	}

	menu_romload_end();
#if 0
	if (!emu_isBios(romFileName))
	{
		// emu_ReadConfig() might have messed currentConfig.lastRomFile
		strncpy(currentConfig.lastRomFile, romFileName, sizeof(currentConfig.lastRomFile)-1);
		currentConfig.lastRomFile[sizeof(currentConfig.lastRomFile)-1] = 0;
	}
#endif
	if (PicoPatches) {
		PicoPatchPrepare();
		PicoPatchApply();
	}

	// additional movie stuff
	if (movie_data) {
		if(movie_data[0x14] == '6')
		     PicoOpt |=  PicoOpt_6button_gamepad; // 6 button pad
		else PicoOpt &= ~PicoOpt_6button_gamepad;
		PicoOpt |= (PicoOpt_accurate_timing | PicoOpt_disable_vdp_fifo); // accurate timing, no VDP fifo timing
		if(movie_data[0xF] >= 'A') {
			if(movie_data[0x16] & 0x80) {
				PicoRegionOverride = 8;
			} else {
				PicoRegionOverride = 4;
			}
			PicoReset(0);
			// TODO: bits 6 & 5
		}
		movie_data[0x18+30] = 0;
		sprintf(noticeMsg, "MOVIE: %s", (char *) &movie_data[0x18]);
	}
	else
	{
		PicoOpt &= ~PicoOpt_disable_vdp_fifo;
		if(Pico.m.pal) {
			strcpy(noticeMsg, "PAL SYSTEM / 50 FPS");
		} else {
			strcpy(noticeMsg, "NTSC SYSTEM / 60 FPS");
		}
	}
	emu_noticeMsgUpdated();

	// load SRAM for this ROM
	emu_SaveLoadSRAM(1);

	return 1;
}

void emu_updateMovie(void)
{
	int offs = Pico.m.frame_count*3 + 0x40;
	if (offs+3 > movie_size) {
		free(movie_data);
		movie_data = 0;
		strcpy(noticeMsg, "END OF MOVIE.");
		lprintf("END OF MOVIE.\n");
		emu_noticeMsgUpdated();
	} else {
		// MXYZ SACB RLDU
		PicoPad[0] = ~movie_data[offs]   & 0x8f; // ! SCBA RLDU
		if(!(movie_data[offs]   & 0x10)) PicoPad[0] |= 0x40; // A
		if(!(movie_data[offs]   & 0x20)) PicoPad[0] |= 0x10; // B
		if(!(movie_data[offs]   & 0x40)) PicoPad[0] |= 0x20; // A
		PicoPad[1] = ~movie_data[offs+1] & 0x8f; // ! SCBA RLDU
		if(!(movie_data[offs+1] & 0x10)) PicoPad[1] |= 0x40; // A
		if(!(movie_data[offs+1] & 0x20)) PicoPad[1] |= 0x10; // B
		if(!(movie_data[offs+1] & 0x40)) PicoPad[1] |= 0x20; // A
		PicoPad[0] |= (~movie_data[offs+2] & 0x0A) << 8; // ! MZYX
		if(!(movie_data[offs+2] & 0x01)) PicoPad[0] |= 0x0400; // X
		if(!(movie_data[offs+2] & 0x04)) PicoPad[0] |= 0x0100; // Z
		PicoPad[1] |= (~movie_data[offs+2] & 0xA0) << 4; // ! MZYX
		if(!(movie_data[offs+2] & 0x10)) PicoPad[1] |= 0x0400; // X
		if(!(movie_data[offs+2] & 0x40)) PicoPad[1] |= 0x0100; // Z
	}
}

static int try_ropen_file(const char *fname)
{
	FILE *f;

	f = fopen(fname, "rb");
	if (f) {
		fclose(f);
		return 1;
	}
	return 0;
}

int emu_SaveLoadSRAM(int load)
{
	int ret = 0;
	static char saveFname[512];

	romfname_ext(saveFname, NULL, (PicoMCD & 1) ? ".brm" : ".srm");
	if (load && !try_ropen_file(saveFname)) return 0;

	FILE *sramFile;
	int sram_size;
	unsigned char *sram_data;
	int truncate = 1;
	if (PicoMCD & 1) {
		if (PicoOpt & PicoOpt_enable_cd_ramcart) { // MCD RAM cart?
			sram_size = 0x12000;
			sram_data = SRam.data;
			if (sram_data)
				memcpy32((int *)sram_data, (int *)Pico_mcd->bram, 0x2000/4);
		} else {
			sram_size = 0x2000;
			sram_data = Pico_mcd->bram;
			truncate  = 0; // the .brm may contain RAM cart data after normal brm
		}
	} else {
		sram_size = SRam.end - SRam.start + 1;
		if(Pico.m.sram_reg & 4) sram_size=0x2000;
		sram_data = SRam.data;
	}
	if (!sram_data) return 0; // SRam forcefully disabled for this game

	if (load) {
		sramFile = fopen(saveFname, "rb");
		if(!sramFile) return -1;
		fread(sram_data, 1, sram_size, sramFile);
		fclose(sramFile);
		if ((PicoMCD & 1) && (PicoOpt & PicoOpt_enable_cd_ramcart))
			memcpy32((int *)Pico_mcd->bram, (int *)sram_data, 0x2000/4);
	} else {
		// sram save needs some special processing
		// see if we have anything to save
		for (; sram_size > 0; sram_size--)
			if (sram_data[sram_size - 1]) break;

		if (sram_size) {
			sramFile = fopen(saveFname, truncate ? "wb" : "r+b");
			if (!sramFile) sramFile = fopen(saveFname, "wb"); // retry
			if (!sramFile) return -1;
			ret = fwrite(sram_data, 1, sram_size, sramFile);
			ret = (ret != sram_size) ? -1 : 0;
			fclose(sramFile);
		}
	}

	return ret;	
}

static int emu_fread  (void* ptr, size_t size, size_t count, FILE* stream) { return state_fread(ptr, count); }
static int emu_fwrite (const void* ptr, size_t size, size_t count, FILE* stream) { return state_fwrite(ptr, count); }
static int emu_feof   (FILE* stream) { return 0; }
static int emu_fseek  (FILE* stream, long int offset, int origin) { return state_fseek(offset, origin); }
static int emu_fclose (FILE* stream) { return state_fclose(); }

int emu_SaveLoadState(const char *saveFname, int load)
{
	int ret = 0;
	void *PmovFile = NULL;
	
	if(PmovFile = state_fopen(load ? "rb" : "wb")) 
	{
		areaRead  = (arearw    *) emu_fread;
		areaWrite = (arearw    *) emu_fwrite;
		areaEof   = (areaeof   *) emu_feof;
		areaSeek  = (areaseek  *) emu_fseek;
		areaClose = (areaclose *) emu_fclose;

		int ret = PmovState(load ? 6 : 5, PmovFile);
		areaClose(PmovFile);
		if(load) Pico.m.dirtyPal = 1;
		return ret;
	}
	return -1;
}
