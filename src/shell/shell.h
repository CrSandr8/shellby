#ifndef _SHELL_H
#define _SHELL_H

#define MAX_LINE    2048
#define MAX_TOKENS  256

typedef enum{
    SHELL_STATE_UNMOUNTED, //When we start the app
    SHELL_STATE_MOUNTED,  //After we have mounted a (virtual) disk
    SHELL_STATE_ANY, //Only for help command
} shell_state_t;

typedef int (*cmd_func_t)(int argc, char **argv);

typedef struct {
    char *name;
    cmd_func_t func;
    shell_state_t required_state; //A command can be executed only in its shell state
} cmd_t;

// Shell functions
int do_shell(const char* prompt);
int do_cmd(char* argv[MAX_TOKENS], int argc);
void get_cmd_line(char* argv[MAX_TOKENS], int* argc);

int cmd_help(int argc, char **argv);

int cmd_format(int argc, char **argv);
int cmd_mount(int argc, char **argv);
int cmd_unmount(int argc, char **argv);

int cmd_import(int argc, char **argv);
int cmd_export(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_touch(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_ls(int argc, char **argv);
int cmd_write(int argc, char **argv);
int cmd_append(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_clear(int argc, char **argv);

#endif
