#include <stdio.h>
#include "kedo.h"

int main(void) {
    KedoFS fs;
    if (!kedo_fs_init(&fs, "kedo_data")) {
        fputs("KEDO fatal: could not create/open kedo_data directory.\n", stderr);
        return (int)KEDO_ERR_IO;
    }
    kedo_shell_run(&fs);
    kedo_fs_close(&fs);
    return 0;
}
