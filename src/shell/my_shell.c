#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "my_shell.h"

#include "../virtual_fat/fat.h" 

cmd_t cmd_table[] = {

    {"help", cmd_help, SHELL_STATE_ANY},
    {"format", cmd_format, SHELL_STATE_UNMOUNTED},
    {"mount", cmd_mount, SHELL_STATE_UNMOUNTED},
    
    {"mkdir", cmd_mkdir, SHELL_STATE_MOUNTED},
    {"cd", cmd_cd, SHELL_STATE_MOUNTED},
    {"touch", cmd_touch, SHELL_STATE_MOUNTED},
    {"cat", cmd_cat, SHELL_STATE_MOUNTED},
    {"ls", cmd_ls, SHELL_STATE_MOUNTED},
    {"write", cmd_write, SHELL_STATE_MOUNTED},
    {"append", cmd_append, SHELL_STATE_MOUNTED},
    {"rm", cmd_rm, SHELL_STATE_MOUNTED},
    {"unmount", cmd_unmount, SHELL_STATE_MOUNTED},
    {"clear", cmd_clear, SHELL_STATE_MOUNTED}};


    

shell_state_t current_state = SHELL_STATE_UNMOUNTED;




int do_cmd(char *argv[MAX_TOKENS], int argc)
{
    if (argc == 0 || argv[0] == NULL)
        return 0;

    for (int i = 0; cmd_table[i].name != NULL; i++)
    {
        if (strcmp(argv[0], cmd_table[i].name) == 0)
        {
            if(current_state != cmd_table[i].required_state && cmd_table[i].required_state != SHELL_STATE_ANY){
                printf("Error: wrong shell state\n");
                return -1;
            }
            return cmd_table[i].func(argc, argv);
        }
    }

    printf("Shellby: command not found: %s\n", argv[0]);
    return -1;
}




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

    while(*p != '\0' && *argc < (MAX_TOKENS - 1)){ // This could cause trouble if a command has exactly 256 tokens so we leave one

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
    printf( "   ::::::::  :::    ::: :::::::::: :::        :::        :::::::::  :::   :::  \n");
    printf( "  :+:    :+: :+:    :+: :+:        :+:        :+:        :+:    :+: :+:   :+:  \n");
    printf( "  +:+        +:+    +:+ +:+        +:+        +:+        +:+    +:+  +:+ +:+   \n");
    printf( "  +#++:++#++ +#++:++#++ +#++:++#   +#+        +#+        +#++:++#+    +#++:    \n");
    printf( "         +#+ +#+    +#+ +#+        +#+        +#+        +#+    +#+    +#+     \n");
    printf( "  #+#    #+# #+#    #+# #+#        #+#        #+#        #+#    #+#    #+#     \n");
    printf( "   ########  ###    ### ########## ########## ########## #########     ###     \n");      
    
    printf("\n");

    printf("Welcome to shellby! Type help to get a glance at the commands \n");

    printf("\n");
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
            if(strcmp(argv[0], "close") == 0)
            {
                if (current_state == SHELL_STATE_MOUNTED) 
                {
                    fprintf(stderr, "Error: please unmount before closing the shell\n");
                    continue;
                }

                break;
                    
            } 
            do_cmd(argv, argc);
        }
    }
    return 0;
}




int cmd_help(int argc, char **argv)
{
    
    if (current_state == SHELL_STATE_UNMOUNTED) {
        printf("\n================================================================================\n");
        printf("             SHELLBY - STATE: UNMOUNTED\n");
        printf("==================================================================================\n");
        printf("Available commands:\n\n");
        
        printf("  help                    Show this help menu\n");
        printf("  format [name] [size]    Format a new virtual disk.\n");
        printf("                          (Default: filesystem.bin 1048576)\n");
        printf("  mount <filename>        Mount an existing virtual disk\n");
        printf("  close                   Close the application\n");
        
        printf("==================================================================================\n\n");
    } 
    else {
        printf("\n================================================================================\n");
        printf("              SHELLBY - STATE: MOUNTED\n");
        printf("==================================================================================\n");
        printf("Available commands:\n\n");
        
        printf("  help                    Show this help menu\n");
        printf("  ls                      List files in the current directory\n");
        printf("  cd <path>               Change directory (e.g., cd folder, cd ..)\n");
        printf("  mkdir <name>            Create a new directory\n");
        printf("  touch <filename>        Create a new empty file\n");
        printf("  cat <filename>          Print the content of a file\n");
        printf("  write <file> \"text\"     Overwrite a file with the specified text\n");
        printf("  append <file> \"text\"    Append text to the end of a file\n");
        printf("  rm [-r] <name>          Remove a file or directory.\n");
        printf("                          (Use -r to remove non-empty folders)\n");
        printf("  clear                   Clear the terminal screen\n");
        printf("  unmount                 Unmount the current disk and save changes\n");
        printf("  close                   Close the application (manual unmount recommended)\n");
        
        printf("===================================================================================\n\n");
    }

    return 0;
}

int cmd_format(int argc, char **argv)
{
    char *final_name = "filesystem.bin";
    int final_size = 1024*1024; // This is the default value

    if (argc == 2) {

        if (isdigit(argv[1][0])) {
            // Case 1: Input is a number
            final_size = atoi(argv[1]);
            
        } else {
            // Case 2: Input is a string
            final_name = argv[1];
        }

    } 
    else if (argc >= 3) {
        // Case 3: Complete input provided
        final_name = argv[1];
        final_size = atoi(argv[2]);
    }

    if (final_size > MAX_DISK_SIZE || final_size < MIN_DISK_SIZE){
                fprintf(stderr, "Error: tried to create a disk file of inappropriate size\n");
                return -1;
    }
    
    return fat_create_disk(final_name, final_size);

}

int cmd_mount(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: mount <filename>\n");
        return -1;
    }

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
    if (argc < 2) {
        fprintf(stderr, "Usage: mkdir <name>\n");
        return -1;
    }
    return fat_createdir(argv[1]);
}

int cmd_cd(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: cd <path>\n");
        return -1;
    }
    return fat_change_dir(argv[1]);
}

int cmd_touch(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: touch <filename>\n");
        return -1;
    }
    return fat_createfile(argv[1]);
}

int cmd_cat(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: cat <filename>\n");
        return -1;
    }
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
    
    uint32_t data_size = strlen(argv[2]);

    return fat_writefile(argv[1], (const void *)argv[2], data_size, 0);
}

int cmd_append(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: append <filename> \"text\"\n");
        return -1;
    }
    uint32_t data_size = strlen(argv[2]);

    return fat_writefile(argv[1], (const void *)argv[2], data_size, 1);
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
        fprintf(stderr, "Usage: rm [-r] <name>\n");
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