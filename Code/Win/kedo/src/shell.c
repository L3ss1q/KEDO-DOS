#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kedo.h"

static char *skip_spaces(char *p) {
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

static void report(KedoError e, const char *ctx) {
    if (e != KEDO_OK) kedo_print_error(e, ctx);
}

static void show_help(void) {
    puts("KEDO commands:");
    puts("  help                         show this help");
    puts("  ver                          show version");
    puts("  hello                        test output");
    puts("  pwd                          show current directory");
    puts("  ls [PATH]                    list files and folders");
    puts("  cd [PATH]                    change directory (cd / returns to root)");
    puts("  mkdir NAME                   create a directory");
    puts("  touch NAME                   create an empty file");
    puts("  write NAME TEXT              create/overwrite a text file");
    puts("  append NAME TEXT             append text to a file");
    puts("  cat NAME                     print a file");
    puts("  rm NAME                      delete a file");
    puts("  rmdir NAME                   remove an empty directory");
    puts("  tree                         show directory tree");
    puts("  cls                          clear terminal");
    puts("  panic MESSAGE                deliberately trigger panic diagnostics");
    puts("  exit                         shut down KEDO");
    puts("");
    puts("Hosted files are stored persistently inside ./kedo_data.");
}

void kedo_shell_run(KedoFS *fs) {
    char line[KEDO_LINE_MAX];
    puts("KEDO DOS " KEDO_VERSION " (hosted edition)");
    puts("Type 'help' for commands. Files are sandboxed in ./kedo_data.");
    for (;;) {
        char *cmd, *arg, *rest;
        printf("KEDO> ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) {
            puts("\nInput closed. KEDO stopped.");
            break;
        }
        line[strcspn(line, "\r\n")] = '\0';
        cmd = skip_spaces(line);
        if (!*cmd) continue;
        arg = cmd;
        while (*arg && *arg != ' ' && *arg != '\t') ++arg;
        if (*arg) *arg++ = '\0';
        arg = skip_spaces(arg);
        rest = arg;

        if (!strcmp(cmd, "help")) show_help();
        else if (!strcmp(cmd, "ver")) printf("kedo dos version %s\n", KEDO_VERSION);
        else if (!strcmp(cmd, "hello")) puts("Hello from KEDO!");
        else if (!strcmp(cmd, "pwd")) report(kedo_fs_pwd(fs), "pwd");
        else if (!strcmp(cmd, "ls")) report(kedo_fs_list(fs, arg), "ls");
        else if (!strcmp(cmd, "cd")) report(kedo_fs_cd(fs, arg), "cd");
        else if (!strcmp(cmd, "mkdir")) report(kedo_fs_mkdir(fs, arg), "mkdir");
        else if (!strcmp(cmd, "touch")) report(kedo_fs_touch(fs, arg), "touch");
        else if (!strcmp(cmd, "cat")) report(kedo_fs_cat(fs, arg), "cat");
        else if (!strcmp(cmd, "rm")) report(kedo_fs_remove(fs, arg, 0), "rm");
        else if (!strcmp(cmd, "rmdir")) report(kedo_fs_remove(fs, arg, 1), "rmdir");
        else if (!strcmp(cmd, "tree")) report(kedo_fs_tree(fs), "tree");
        else if (!strcmp(cmd, "write") || !strcmp(cmd, "append")) {
            char *text = rest;
            while (*text && *text != ' ' && *text != '\t') ++text;
            if (!*rest || !*text) {
                puts("Usage: write NAME TEXT");
                continue;
            }
            *text++ = '\0';
            text = skip_spaces(text);
            if (!*text) { puts("Usage: write NAME TEXT"); continue; }
            report(kedo_fs_write(fs, rest, text, !strcmp(cmd, "append")), cmd);
        }
        else if (!strcmp(cmd, "cls")) {
#ifdef _WIN32
            system("cls");
#else
            fputs("\033[2J\033[H", stdout);
#endif
        }
        else if (!strcmp(cmd, "panic")) {
            kedo_panic(KEDO_ERR_KERNEL_PANIC, *arg ? arg : "manual panic command", __FILE__, __LINE__);
        }
        else if (!strcmp(cmd, "exit") || !strcmp(cmd, "fr")) {
            puts("KEDO halted.");
            break;
        }
        else {
            kedo_print_error(KEDO_ERR_INVALID_COMMAND, cmd);
            puts("Type 'help' for a command list.");
        }
    }
}
