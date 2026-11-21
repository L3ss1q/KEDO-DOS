#ifndef KEDO_H
#define KEDO_H

#include <stddef.h>
#include "kedoError.h"

#define KEDO_VERSION "2.12132034+2.12132034i"
#define KEDO_LINE_MAX 512
#define KEDO_PATH_MAX 1024

typedef struct {
    char root[KEDO_PATH_MAX];
    char cwd[KEDO_PATH_MAX]; /* relative to root; empty string means root */
} KedoFS;

int kedo_fs_init(KedoFS *fs, const char *root);
KedoError kedo_fs_list(KedoFS *fs, const char *path);
KedoError kedo_fs_cd(KedoFS *fs, const char *path);
KedoError kedo_fs_mkdir(KedoFS *fs, const char *path);
KedoError kedo_fs_touch(KedoFS *fs, const char *path);
KedoError kedo_fs_write(KedoFS *fs, const char *path, const char *data, int append);
KedoError kedo_fs_cat(KedoFS *fs, const char *path);
KedoError kedo_fs_remove(KedoFS *fs, const char *path, int directory);
KedoError kedo_fs_tree(KedoFS *fs);
KedoError kedo_fs_pwd(KedoFS *fs);
void kedo_fs_close(KedoFS *fs);

void kedo_shell_run(KedoFS *fs);
void kedo_print_error(KedoError error, const char *context);
void kedo_panic(KedoError code, const char *message, const char *file, int line);

#endif
