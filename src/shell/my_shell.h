#ifndef _MY_SHELL_H
#define _MY_SHELL_H

#include "virtual_fat/fat.h"
#include "virtual_fat/fat_structs.h"

#define MAX_LINE    1024
#define MAX_TOKENS  64

void deallocate_cmd(char* argv[MAX_TOKENS]);
char* dup_string(const char* in);
void get_cmd_line(char* argv[MAX_TOKENS]);

int format(const char *name, int size);
int mkdir(const char *filename);
int cd(const char *path);
int touch(const char *filename);
int cat(const char *filename);
int ls();
int append(const char *filename, const char *text);
int rm(const char *path);
int shell_close(const char *filename);

#endif
