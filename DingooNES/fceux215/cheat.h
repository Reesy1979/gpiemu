void FCEU_CheatResetRAM(void);
void FCEU_CheatAddRAM(int s, uint32 A, uint8 *p);

void FCEU_LoadGameCheats(FILE *override);
void FCEU_FlushGameCheats(FILE *override, int nosave);
void FCEU_ApplyPeriodicCheats(void);
void FCEU_PowerCheats(void);

int FCEU_CheatGetByte(uint32 A);
void FCEU_CheatSetByte(uint32 A, uint8 V);

#if defined(_DINGOO) || defined(_GPI) || defined(_LINUX)
extern int cheatCodeMap[];
extern int cheatCodeCount;
int FCEUI_ToggleCheat(uint32 which, int enable);
#endif
