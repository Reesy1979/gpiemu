#ifndef SAVE_H
#define SAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

void savestate(FILE *f);
void loadstate(FILE *f);
#ifdef __cplusplus
} // End of extern "C"
#endif

void system_save_state(void);
void system_load_state(void);

#endif

