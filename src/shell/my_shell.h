#ifndef _MY_SHELL_H
#define _MY_SHELL_H

#include "virtual_fat/fat.h"
#include "virtual_fat/fat_structs.h"

#define MAX_LINE    1024
#define MAX_TOKENS  64

typedef int (*cmd_func_t)(int argc, char **argv);

typedef struct {
    char *name;
    cmd_func_t func;
    char *help;
} cmd_t;

void deallocate_cmd(char* argv[MAX_TOKENS]);
char* dup_string(const char* in);
void get_cmd_line(char* argv[MAX_TOKENS], int *argc);
int do_shell(const char* prompt);

int cmd_format(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_touch(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_ls(int argc, char **argv);
int cmd_append(int argc, char **argv);
int cmd_rm(int argc, char **argv);

int cmd_close(int argc, char **argv);

/*
   Comandi usuali di shell (e di "libreria)
   - format <fs_filename> <size>
   - mkdir
   - cd
   - touch (crea un file vuoto)
   - cat (stampa il contenuto di un file su schermo)
   - ls
   - append <file> <testo>
   - rm <dir/file>
   - close
*/

#endif
