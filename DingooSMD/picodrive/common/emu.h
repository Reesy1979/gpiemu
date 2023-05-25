// (c) Copyright 2006-2007 notaz, All rights reserved.
// Free for non-commercial use.

// For commercial use, separate licencing terms must be obtained.

extern char romFileName[256];
extern char ipsFileName[256];
extern char menuErrorMsg[64];
extern char noticeMsg   [64];

extern unsigned char *rom_data;
extern unsigned char *movie_data;

int   emu_ReloadRom(void);
char *emu_GetSaveFName(int load, int is_sram, int slot);
void  emu_updateMovie(void);
int   emu_cdCheck(int *pregion);
int   emu_findBios(int region, char **bios_file);
int   emu_SaveLoadSRAM(int load);
int   emu_SaveLoadState(const char *saveFname, int load);
