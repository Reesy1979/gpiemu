#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include <wrap_fatfs.h>

typedef struct _CIRCLE_DIR DIR;

/* Directory entry as returned by readdir */
struct dirent {
        ino_t  d_ino;
	char   d_type;
        char   d_name[FF_LFN_BUF + 1];
};

DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int readdir_r(DIR *__restrict, struct dirent *__restrict,
              struct dirent **__restrict);
void rewinddir(DIR *);
int closedir(DIR *);

#ifdef __cplusplus
}
#endif

#endif
