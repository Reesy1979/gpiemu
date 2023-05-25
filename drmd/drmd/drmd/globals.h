#ifndef _GLOBALS_H_
#define _GLOBALS_H_

extern struct Cyclone gCyclone;
extern struct DrZ80 gDrz80;
extern struct DrMD gDrmd;
extern struct PSG_CONTEXT PSG;
extern unsigned int PSG_sn_UPDATESTEP;
extern s32 gSaveStateSize;
extern s8 *gRomDir;
extern s8 *gSystemDir;  // system path currently in use
extern s8 gSystemRootDir[];
extern s8 gMDSystemDir[];
extern s8 gMDRomDir[];
extern s8 gLastSaveName[];
extern s8 gLastRomName[];
extern u8 gWorkRam[]; // scratch ram
extern u8 gZram[];
extern u8 gSram[]; // sram
extern u32 gTileCache[];
extern u16 gVram[];
extern u16 gVsram[];
extern u16 gCram[];
extern u8 gSram1[]; // sram 1
extern u8 gSram2[]; // sram 2
extern u16 gLocalPal[];
extern s32 gCurrentEmuMode;
extern u16 gSampleCountLookup[];
extern s32 gDacInPos;
extern s32 gDacOutPos;
extern s32 gLastSample;
extern u8 *gRomData;
extern u32 gPalLookup[];
extern struct MENU_OPTIONS gMDMenuOptions;
extern s8 gCurrentRomFilename[];
extern s8 *gCurrentState;
extern s8 *gTempState;
extern s8 *gQuickState;
extern s16 *gSoundBuffer;
extern u32 gSoundSampleCount;
extern u32 gSoundOn;
#endif
