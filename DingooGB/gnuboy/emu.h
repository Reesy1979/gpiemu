#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

void emu_run();
void emu_reset();
void emu_pause(int paused);
int emu_paused(void);

#ifdef __cplusplus
}
#endif

#endif


