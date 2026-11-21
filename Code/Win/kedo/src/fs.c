#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "kedo.h"

#ifdef _WIN32
#include <direct.h>
#define KEDO_MKDIR(p) _mkdir(p)
#define KEDO_RMDIR(p) _rmdir(p)
#else
#include <unistd.h>
#define KEDO_MKDIR(p) mkdir((p), 0777)
#define KEDO_RMDIR(p) rmdir(p)
#endif

static KedoError make_absolute(KedoFS *fs, const char *input, char *out, size_t cap) {
    char combined[KEDO_PATH_MAX];
    char normalized[KEDO_PATH_MAX];
    size_t used = 0;
    const char *p;
    if (!input || !*input || input[0] == '/' || input[0] == '\\' || strchr(input, ':'))
        return KEDO_ERR_INVALID_PATH;

    if (fs->cwd[0]) {
        if (snprintf(combined, sizeof combined, "%s/%s", fs->cwd, input) >= (int)sizeof combined)
            return KEDO_ERR_BUFFER_TOO_SMALL;
    } else {
        if (snprintf(combined, sizeof combined, "%s", input) >= (int)sizeof combined)
            return KEDO_ERR_BUFFER_TOO_SMALL;
    }

    p = combined;
    while (*p) {
        char part[256];
        size_t n = 0;
        while (*p == '/' || *p == '\\') ++p;
        if (!*p) break;
        while (*p && *p != '/' && *p != '\\') {
            if (n + 1 >= sizeof part) return KEDO_ERR_BUFFER_TOO_SMALL;
            part[n++] = *p++;
        }
        part[n] = '\0';
        if (strcmp(part, ".") == 0 || n == 0) continue;
        if (strcmp(part, "..") == 0) {
            if (used == 0) return KEDO_ERR_INVALID_PATH;
            while (used && normalized[used - 1] != '/') --used;
            if (used) --used;
            continue;
        }
        if (used && used + 1 >= sizeof normalized) return KEDO_ERR_BUFFER_TOO_SMALL;
        if (used) normalized[used++] = '/';
        if (used + n >= sizeof normalized) return KEDO_ERR_BUFFER_TOO_SMALL;
        memcpy(normalized + used, part, n);
        used += n;
    }
    normalized[used] = '\0';
    if (!used) return KEDO_ERR_INVALID_PATH;
    if (snprintf(out, cap, "%s/%s", fs->root, normalized) >= (int)cap)
        return KEDO_ERR_BUFFER_TOO_SMALL;
    return KEDO_OK;
}

static KedoError resolve_existing(KedoFS *fs, const char *path, char *out, size_t cap, struct stat *st) {
    KedoError e = make_absolute(fs, path, out, cap);
    if (e != KEDO_OK) return e;
    if (stat(out, st) != 0) {
        if (errno == ENOENT) return KEDO_ERR_NOT_FOUND;
        if (errno == EACCES) return KEDO_ERR_PERMISSION;
        return KEDO_ERR_IO;
    }
    return KEDO_OK;
}

int kedo_fs_init(KedoFS *fs, const char *root) {
    if (!fs || !root) return 0;
    if (snprintf(fs->root, sizeof fs->root, "%s", root) >= (int)sizeof fs->root) return 0;
    fs->cwd[0] = '\0';
    if (KEDO_MKDIR(fs->root) != 0 && errno != EEXIST) return 0;
    return 1;
}

KedoError kedo_fs_list(KedoFS *fs, const char *path) {
    char abs[KEDO_PATH_MAX];
    struct stat st;
    DIR *dir;
    struct dirent *entry;
    KedoError e;
    if (!path || !*path) {
        if (fs->cwd[0]) {
            if (snprintf(abs, sizeof abs, "%s/%s", fs->root, fs->cwd) >= (int)sizeof abs) return KEDO_ERR_BUFFER_TOO_SMALL;
        } else snprintf(abs, sizeof abs, "%s", fs->root);
    } else {
        e = resolve_existing(fs, path, abs, sizeof abs, &st);
        if (e != KEDO_OK) return e;
        if (!S_ISDIR(st.st_mode)) return KEDO_ERR_NOT_DIRECTORY;
    }
    dir = opendir(abs);
    if (!dir) return errno == EACCES ? KEDO_ERR_PERMISSION : KEDO_ERR_IO;
    puts("  NAME                          TYPE");
    puts("  ----------------------------  ----");
    while ((entry = readdir(dir)) != NULL) {
        struct stat item;
        char full[KEDO_PATH_MAX];
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        if (snprintf(full, sizeof full, "%s/%s", abs, entry->d_name) >= (int)sizeof full) continue;
        if (stat(full, &item) == 0)
            printf("  %-28s  %s\n", entry->d_name, S_ISDIR(item.st_mode) ? "<DIR>" : "FILE");
    }
    closedir(dir);
    return KEDO_OK;
}

KedoError kedo_fs_cd(KedoFS *fs, const char *path) {
    char abs[KEDO_PATH_MAX], candidate[KEDO_PATH_MAX];
    struct stat st;
    KedoError e;
    if (!path || !*path) { fs->cwd[0] = '\0'; return KEDO_OK; }
    if (!strcmp(path, "/") || !strcmp(path, "\\")) { fs->cwd[0] = '\0'; return KEDO_OK; }
    if (!strcmp(path, "..")) {
        size_t n = strlen(fs->cwd);
        if (!n) return KEDO_OK;
        while (n && fs->cwd[n - 1] != '/') --n;
        if (n) fs->cwd[n - 1] = '\0'; else fs->cwd[0] = '\0';
        return KEDO_OK;
    }
    e = make_absolute(fs, path, abs, sizeof abs);
    if (e != KEDO_OK) return e;
    if (stat(abs, &st) != 0) return errno == ENOENT ? KEDO_ERR_NOT_FOUND : KEDO_ERR_IO;
    if (!S_ISDIR(st.st_mode)) return KEDO_ERR_NOT_DIRECTORY;

    /* Reconstruct the normalized relative path from the absolute root suffix. */
    if (strncmp(abs, fs->root, strlen(fs->root)) != 0) return KEDO_ERR_INVALID_PATH;
    if (snprintf(candidate, sizeof candidate, "%s", abs + strlen(fs->root) + 1) >= (int)sizeof candidate)
        return KEDO_ERR_BUFFER_TOO_SMALL;
    if (strlen(candidate) >= sizeof fs->cwd) return KEDO_ERR_BUFFER_TOO_SMALL;
    strcpy(fs->cwd, candidate);
    return KEDO_OK;
}

KedoError kedo_fs_mkdir(KedoFS *fs, const char *path) {
    char abs[KEDO_PATH_MAX];
    if (!path || !*path) return KEDO_ERR_INVALID_PATH;
    KedoError e = make_absolute(fs, path, abs, sizeof abs);
    if (e != KEDO_OK) return e;
    if (KEDO_MKDIR(abs) == 0) return KEDO_OK;
    if (errno == EEXIST) return KEDO_ERR_ALREADY_EXISTS;
    if (errno == EACCES) return KEDO_ERR_PERMISSION;
    return KEDO_ERR_IO;
}

KedoError kedo_fs_touch(KedoFS *fs, const char *path) {
    char abs[KEDO_PATH_MAX];
    struct stat st;
    FILE *f;
    KedoError e = make_absolute(fs, path, abs, sizeof abs);
    if (e != KEDO_OK) return e;
    if (stat(abs, &st) == 0) return S_ISDIR(st.st_mode) ? KEDO_ERR_IS_DIRECTORY : KEDO_OK;
    f = fopen(abs, "ab");
    if (!f) return errno == EACCES ? KEDO_ERR_PERMISSION : KEDO_ERR_IO;
    fclose(f);
    return KEDO_OK;
}

KedoError kedo_fs_write(KedoFS *fs, const char *path, const char *data, int append) {
    char abs[KEDO_PATH_MAX];
    struct stat st;
    FILE *f;
    KedoError e = make_absolute(fs, path, abs, sizeof abs);
    if (e != KEDO_OK) return e;
    if (stat(abs, &st) == 0 && S_ISDIR(st.st_mode)) return KEDO_ERR_IS_DIRECTORY;
    f = fopen(abs, append ? "ab" : "wb");
    if (!f) return errno == EACCES ? KEDO_ERR_PERMISSION : KEDO_ERR_IO;
    if (data && fputs(data, f) == EOF) { fclose(f); return KEDO_ERR_IO; }
    if (fclose(f) != 0) return KEDO_ERR_IO;
    return KEDO_OK;
}

KedoError kedo_fs_cat(KedoFS *fs, const char *path) {
    char abs[KEDO_PATH_MAX];
    struct stat st;
    FILE *f;
    char buffer[512];
    size_t n;
    KedoError e = resolve_existing(fs, path, abs, sizeof abs, &st);
    if (e != KEDO_OK) return e;
    if (S_ISDIR(st.st_mode)) return KEDO_ERR_IS_DIRECTORY;
    f = fopen(abs, "rb");
    if (!f) return errno == EACCES ? KEDO_ERR_PERMISSION : KEDO_ERR_IO;
    while ((n = fread(buffer, 1, sizeof buffer, f)) > 0)
        fwrite(buffer, 1, n, stdout);
    if (ferror(f)) { fclose(f); return KEDO_ERR_IO; }
    fclose(f);
    if (n == 0) putchar('\n');
    return KEDO_OK;
}

KedoError kedo_fs_remove(KedoFS *fs, const char *path, int directory) {
    char abs[KEDO_PATH_MAX];
    struct stat st;
    KedoError e = resolve_existing(fs, path, abs, sizeof abs, &st);
    if (e != KEDO_OK) return e;
    if (directory) {
        if (!S_ISDIR(st.st_mode)) return KEDO_ERR_NOT_DIRECTORY;
        if (KEDO_RMDIR(abs) != 0) return errno == ENOTEMPTY || errno == EEXIST ? KEDO_ERR_IO : KEDO_ERR_PERMISSION;
    } else {
        if (S_ISDIR(st.st_mode)) return KEDO_ERR_IS_DIRECTORY;
        if (remove(abs) != 0) return errno == EACCES ? KEDO_ERR_PERMISSION : KEDO_ERR_IO;
    }
    return KEDO_OK;
}

static void tree_walk(const char *path, int depth) {
    DIR *d = opendir(path);
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d)) != NULL) {
        char full[KEDO_PATH_MAX];
        struct stat st;
        int i;
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (snprintf(full, sizeof full, "%s/%s", path, e->d_name) >= (int)sizeof full) continue;
        for (i = 0; i < depth; ++i) printf("  ");
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("[D] %s/\n", e->d_name);
            tree_walk(full, depth + 1);
        } else printf("[F] %s\n", e->d_name);
    }
    closedir(d);
}

KedoError kedo_fs_tree(KedoFS *fs) {
    char abs[KEDO_PATH_MAX];
    if (fs->cwd[0]) {
        if (snprintf(abs, sizeof abs, "%s/%s", fs->root, fs->cwd) >= (int)sizeof abs) return KEDO_ERR_BUFFER_TOO_SMALL;
    } else snprintf(abs, sizeof abs, "%s", fs->root);
    printf(".\n");
    tree_walk(abs, 1);
    return KEDO_OK;
}

KedoError kedo_fs_pwd(KedoFS *fs) {
    printf("/%s\n", fs->cwd);
    return KEDO_OK;
}

void kedo_fs_close(KedoFS *fs) {
    (void)fs;
}
