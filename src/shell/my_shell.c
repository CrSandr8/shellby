#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "my_shell.h"

#include "../virtual_fat/fat.h" 

cmd_t cmd_table[] = {

    {"help", cmd_help, "get help for usage", SHELL_STATE_UNMOUNTED},
    {"format", cmd_format, "create a new directory", SHELL_STATE_UNMOUNTED},
    {"mount", cmd_mount, "create a new directory", SHELL_STATE_UNMOUNTED},
    
    {"mkdir", cmd_mkdir, "create a new directory", SHELL_STATE_MOUNTED},
    {"cd", cmd_cd, "change current working directory", SHELL_STATE_MOUNTED},
    {"touch", cmd_touch, "create a file", SHELL_STATE_MOUNTED},
    {"cat", cmd_cat, "print the content of a file", SHELL_STATE_MOUNTED},
    {"ls", cmd_ls, "list entries in this directory", SHELL_STATE_MOUNTED},
    {"write", cmd_write, "overwrite a file with new text", SHELL_STATE_MOUNTED},
    {"append", cmd_append, "append text to a file", SHELL_STATE_MOUNTED},
    {"rm", cmd_rm, "remove a file or directory", SHELL_STATE_MOUNTED},
    {"unmount", cmd_unmount, "unmount the current disk", SHELL_STATE_MOUNTED},
    {"clear", cmd_clear, "clear the terminal screen", SHELL_STATE_MOUNTED},

    {NULL, NULL, NULL}};

shell_state_t current_state = SHELL_STATE_UNMOUNTED;

int do_cmd(char *argv[MAX_TOKENS], int argc)
{
    if (argc == 0 || argv[0] == NULL)
        return 0;

    for (int i = 0; cmd_table[i].name != NULL; i++)
    {
        if (strcmp(argv[0], cmd_table[i].name) == 0)
        {
            if(current_state != cmd_table[i].required_state){
                printf("Error: wrong shell state\n");
                return -1;
            }
            return cmd_table[i].func(argc, argv);
        }
    }

    printf("Shellby: command not found: %s\n", argv[0]);
    return -1;
}
// ... keep dup_string and deallocate_cmd as they were ...

void get_cmd_line(char* argv[MAX_TOKENS], int* argc) {
    static char line[MAX_LINE];
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        printf("\n");
        fat_unmount();
        exit(0);
    }
    *argc = 0;
    char *p = line;
    int in_quotes = 0;

    while(*p != '\0' && *argc < MAX_TOKENS){

        while(*p == ' ' || *p == '\t' || *p == '\n') p++;

        if (*p == '\0') break;

        if (*p == '"'){
            in_quotes = 1;
            p++;
        }

        argv[*argc] = p;
        (*argc)++;

        while(*p != '\0'){
            if (in_quotes && *p == '"'){
                in_quotes = 0;
                *p = '\0';
                p++;
                break;
            }
            if (!in_quotes && (*p == ' ' || *p == '\t' || *p == '\n')){
                *p = '\0';
                p++;
                break;
            }
            p++;
        }
    }
    argv[*argc] = NULL;
}

int do_shell(const char* prompt_base) {
    printf("Welcome to Shellby, a FAT based and portable shell\n");
    for (;;) {

        if (disk->disk_base != NULL) {
        // Disk is mounted: show shellby[disk_name]:/pathto/dir$ 
        printf("%s[%s]:%s$ ", prompt_base, disk->disk_name, disk->cwd_path);
        } else {
        // No disk mounted: show shellby:$ 
        printf("%s$ ", prompt_base);
}
        
        char* argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv, &argc);
        if (argc > 0) {
            if(strcmp(argv[0], "close") == 0) break;
            do_cmd(argv, argc);
        }
    }
    return 0;
}

int cmd_help(int argc, char **argv)
{
    return 0;
}

int cmd_format(int argc, char **argv)
{
    char *final_name = "filesystem.bin";
    int final_size = 1000000;

    if (argc == 2) {
        /* * Logic for a single extra argument: format <input>
         * We check if the first character of the input is a digit.
         */
        if (isdigit(argv[1][0])) {
            // Case 1: Input is a number (e.g., "format 500000")
            // Keep default name, update size.
            final_size = atoi(argv[1]);
        } else {
            // Case 2: Input is a string (e.g., "format mydisk")
            // Update name, keep default size.
            final_name = argv[1];
        }
    } 
    else if (argc >= 3) {
        /* * Case 4: Complete input provided (e.g., "format mydisk 2000000")
         */
        final_name = argv[1];
        final_size = atoi(argv[2]);
    }
    // If argc == 1 (only "format"), the code falls through and uses defaults.

    // Call the core function to format the virtual disk image
    return fat_create_disk(final_name, final_size);

}

int cmd_mount(int argc, char **argv)
{
    int res = fat_mount(argv[1]);

    if(res == FAT_SUCCESS) current_state = SHELL_STATE_MOUNTED;

    return res;
}

int cmd_unmount(int argc, char **argv)
{
    int res = fat_unmount();

    if (res == FAT_SUCCESS) current_state = SHELL_STATE_UNMOUNTED;

    return res;
}

int cmd_mkdir(int argc, char **argv)
{
    return fat_createdir(argv[1]);
}

int cmd_cd(int argc, char **argv)
{
    return fat_change_dir(argv[1]);
}

int cmd_touch(int argc, char **argv)
{
    return fat_createfile(argv[1]);
}

int cmd_cat(int argc, char **argv)
{
    return fat_readfile(argv[1]);
}

int cmd_ls(int argc, char **argv)
{
    return fat_readdir(disk->cwd_sector);
}

int cmd_write(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: write <filename> \"text\"\n");
        return -1;
    }
    return fat_writefile(argv[1], argv[2], 0);
}

int cmd_append(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: append <filename> \"text\"\n");
        return -1;
    }
    return fat_writefile(argv[1], argv[2], 1);
}

int cmd_rm(int argc, char **argv)
{
    int opt;
    int flag = 0;

    optind = 0;

    while ((opt = getopt(argc, argv, "r")) != -1){
        if (opt == 'r') flag = 1;
        else return -1;
    }

    if (optind >= argc){
        fprintf(stderr, "Err");
        return -1;
    }

    char *arg = argv[optind];

    return fat_rm(arg, flag);
}

int cmd_clear(int argc, char **argv)
{
    // \033[2J: Clears the entire screen
    // \033[H: Moves the cursor to the top-left corner (Home)
    printf("\033[2J\033[H");
    
    // Ensure the output is printed immediately
    fflush(stdout); 
    
    return 0; // Success
}