#ifndef FS_H
#define FS_H

#include "stdint.h"

/* ============================================
   SIMPLE IN-MEMORY FILESYSTEM
   
   Since we have no disk driver yet, files live
   in RAM. Everything is lost on reboot — but it
   works great for a first filesystem!
   ============================================ */

#define FS_MAX_FILES     32
#define FS_MAX_DIRS      16
#define FS_NAME_LEN      32
#define FS_MAX_FILE_SIZE 512

typedef struct {
    char     name[FS_NAME_LEN];
    char     data[FS_MAX_FILE_SIZE];
    uint32_t size;
    int      used;
    int      dir_index;   /* which directory this file lives in */
} fs_file_t;

typedef struct {
    char name[FS_NAME_LEN];
    int  used;
    int  parent;          /* index of parent dir, -1 for root */
} fs_dir_t;

void fs_init(void);

/* Directory operations */
int  fs_mkdir(const char* name);
int  fs_ls(void);
int  fs_cd(const char* name);
void fs_pwd(void);

/* File operations */
int  fs_create(const char* name, const char* data);
int  fs_cat(const char* name);
int  fs_load(const char* name, char* out, int max);

/* Current directory index */
extern int fs_current_dir;

#endif
