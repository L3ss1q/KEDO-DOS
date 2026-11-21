#ifndef KEDO_ERROR_H
#define KEDO_ERROR_H

typedef enum {
    KEDO_OK = 0,
    KEDO_ERR_INVALID_COMMAND = 1,
    KEDO_ERR_OUT_OF_MEMORY = 2,
    KEDO_ERR_NOT_FOUND = 3,
    KEDO_ERR_ALREADY_EXISTS = 4,
    KEDO_ERR_NOT_DIRECTORY = 5,
    KEDO_ERR_IS_DIRECTORY = 6,
    KEDO_ERR_INVALID_PATH = 7,
    KEDO_ERR_IO = 8,
    KEDO_ERR_PERMISSION = 9,
    KEDO_ERR_BUFFER_TOO_SMALL = 10,
    KEDO_ERR_ABORT = 134,
    KEDO_ERR_ARITHMETIC = 136,
    KEDO_ERR_BUS_ERROR = 138,
    KEDO_ERR_SEGFAULT = 139,
    KEDO_ERR_KERNEL_PANIC = 255
} KedoError;

const char *kedo_error_name(KedoError error);
const char *kedo_error_message(KedoError error);

#endif
