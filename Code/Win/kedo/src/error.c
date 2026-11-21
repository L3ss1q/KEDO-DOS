#include <stdio.h>
#include <stdlib.h>
#include "kedo.h"

const char *kedo_error_name(KedoError e) {
    switch (e) {
        case KEDO_OK: return "KEDO_OK";
        case KEDO_ERR_INVALID_COMMAND: return "KEDO_ERR_INVALID_COMMAND";
        case KEDO_ERR_OUT_OF_MEMORY: return "KEDO_ERR_OUT_OF_MEMORY";
        case KEDO_ERR_NOT_FOUND: return "KEDO_ERR_NOT_FOUND";
        case KEDO_ERR_ALREADY_EXISTS: return "KEDO_ERR_ALREADY_EXISTS";
        case KEDO_ERR_NOT_DIRECTORY: return "KEDO_ERR_NOT_DIRECTORY";
        case KEDO_ERR_IS_DIRECTORY: return "KEDO_ERR_IS_DIRECTORY";
        case KEDO_ERR_INVALID_PATH: return "KEDO_ERR_INVALID_PATH";
        case KEDO_ERR_IO: return "KEDO_ERR_IO";
        case KEDO_ERR_PERMISSION: return "KEDO_ERR_PERMISSION";
        case KEDO_ERR_BUFFER_TOO_SMALL: return "KEDO_ERR_BUFFER_TOO_SMALL";
        case KEDO_ERR_ABORT: return "KEDO_ERR_ABORT";
        case KEDO_ERR_ARITHMETIC: return "KEDO_ERR_ARITHMETIC";
        case KEDO_ERR_BUS_ERROR: return "KEDO_ERR_BUS_ERROR";
        case KEDO_ERR_SEGFAULT: return "KEDO_ERR_SEGFAULT";
        case KEDO_ERR_KERNEL_PANIC: return "KEDO_ERR_KERNEL_PANIC";
        default: return "KEDO_ERR_UNKNOWN";
    }
}

const char *kedo_error_message(KedoError e) {
    switch (e) {
        case KEDO_OK: return "success";
        case KEDO_ERR_INVALID_COMMAND: return "invalid command";
        case KEDO_ERR_OUT_OF_MEMORY: return "out of memory";
        case KEDO_ERR_NOT_FOUND: return "file or directory not found";
        case KEDO_ERR_ALREADY_EXISTS: return "file or directory already exists";
        case KEDO_ERR_NOT_DIRECTORY: return "not a directory";
        case KEDO_ERR_IS_DIRECTORY: return "is a directory";
        case KEDO_ERR_INVALID_PATH: return "invalid or out-of-sandbox path";
        case KEDO_ERR_IO: return "input/output operation failed";
        case KEDO_ERR_PERMISSION: return "permission denied";
        case KEDO_ERR_BUFFER_TOO_SMALL: return "path or input exceeds supported size";
        case KEDO_ERR_ABORT: return "operation aborted";
        case KEDO_ERR_ARITHMETIC: return "arithmetic error";
        case KEDO_ERR_BUS_ERROR: return "bus error";
        case KEDO_ERR_SEGFAULT: return "segmentation fault";
        case KEDO_ERR_KERNEL_PANIC: return "kernel panic";
        default: return "unknown error";
    }
}

void kedo_print_error(KedoError e, const char *context) {
    if (e == KEDO_OK) return;
    fprintf(stderr, "KEDO error [%s]: %s", kedo_error_name(e), kedo_error_message(e));
    if (context && *context) fprintf(stderr, " (%s)", context);
    fputc('\n', stderr);
}

void kedo_panic(KedoError code, const char *message, const char *file, int line) {
    fprintf(stderr, "\n*** KEDO KERNEL PANIC ***\n");
    fprintf(stderr, "code: %d (%s)\n", (int)code, kedo_error_name(code));
    fprintf(stderr, "message: %s\n", message ? message : "(no message)");
    fprintf(stderr, "location: %s:%d\n", file ? file : "(unknown)", line);
    fprintf(stderr, "KEDO has stopped.\n");
    fflush(stderr);
    exit((int)KEDO_ERR_KERNEL_PANIC);
}
