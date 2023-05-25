
#include "sal.h"
#include "cyclone.h"
#include "drz80.h"
#include "drmd.h"
#include "sn76496.h"
#include "menu.h"

s32 gSaveStateSize=0;
s8 *gRomDir;

s8 gSystemRootDir[SAL_MAX_PATH];

s8 gMDSystemDir[SAL_MAX_PATH];
s8 gSystemDir[SAL_MAX_PATH];


s8 gMDRomDir[SAL_MAX_PATH];
s8 gSMSRomDir[SAL_MAX_PATH];
s8 gGGRomDir[SAL_MAX_PATH];

s8 gLastSaveName[SAL_MAX_PATH];
s8 gLastRomName[SAL_MAX_PATH];
u8 gZram[0x4000];
u16 gLocalPal[0x40];

u8 gWorkRam[0x10000]; // scratch ram
u8 gSram[0x10000]; // sram
u32 gTileCache[0x4000>>2];
u16 gVram[0x8000];
u16 gVsram[0x40];
u16 gCram[0x40];
s32 gCurrentEmuMode=EMU_MODE_NONE;
s32 gDacInPos;
s32 gDacOutPos;
s32 gLastSample;
u8 *gRomData=NULL;
u32 gPalLookup[0x1000];
struct MENU_OPTIONS gMDMenuOptions;
s8 gCurrentRomFilename[SAL_MAX_PATH]="";
s8 *gCurrentState;
s8 *gTempState;
s8 *gQuickState;
u16 gSampleCountLookup[400];
s16 *gSoundBuffer;
u32 gSoundSampleCount;
u32 gSoundOn=0;

struct Cyclone gCyclone;
struct DrZ80 gDrz80;
struct DrMD gDrmd;
struct PSG_CONTEXT PSG;
unsigned int PSG_sn_UPDATESTEP;
