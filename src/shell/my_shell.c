#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "my_shell.h"

// Note: Ensure the path to fat.h matches your directory structure
#include "../virtual_fat/fat.h" 

void listCommands(){
    printf("\n--- Available Commands ---\n");
    printf("init [name]   : Format a new disk (default 1MB)\n");
    printf("mount [name]  : Mount an existing disk image\n");
    printf("ls            : List files in current directory\n");
    printf("cd [dir]      : Change directory\n");
    printf("mkdir [name]  : Create a directory\n");
    printf("touch [name]  : Create an empty file\n");
    printf("cat [file]   : Display file content (cat)\n");
    printf("write [file] [text] : Overwrite file with text\n");
    printf("append [file] [text]: Append text to file\n");
    printf("rmfile [name] : Delete a file\n");
    printf("rmdir [name]  : Delete a directory\n");
    printf("exit          : Unmount and quit\n");
}

void do_cmd(char* argv[MAX_TOKENS], int argc) {
    if (strcmp(argv[0], "lscmd") == 0 || strcmp(argv[0], "help") == 0) {
        listCommands();
    } 
    /* DISK OPS */
    else if (strcmp(argv[0], "init") == 0) {
        char *filename = (argv[1] == NULL) ? "filesystem.img" : argv[1];
        fat_create_disk(filename, atoi(argv[2])); 
    } else if (strcmp(argv[0], "mount") == 0) {
        char *filename = (argv[1] == NULL) ? "filesystem.img" : argv[1];
        fat_mount(filename);
    } 
    /* DIRECTORY OPS */
    else if (strcmp(argv[0], "ls") == 0) {
        fat_readdir(disk->cwd_sector);
    } else if (strcmp(argv[0], "mkdir") == 0) {
        if (argv[1]) fat_createdir(argv[1]);
    } else if (strcmp(argv[0], "cd") == 0) {
        fat_change_dir(argv[1] ? argv[1] : "/");
    } else if (strcmp(argv[0], "rmdir") == 0) {
        if (argv[1]) fat_rmdir(argv[1]);
    }
    /* FILE OPS */
    else if (strcmp(argv[0], "touch") == 0) {
        if (argv[1]) fat_createfile(argv[1]);
    } else if (strcmp(argv[0], "cat") == 0) {
        if (argv[1]) fat_readfile(argv[1]);
    } else if (strcmp(argv[0], "write") == 0) {
        if (argc >= 3) fat_writefile(argv[1], argv[2], 0); // Overwrite
        else printf("Usage: write <file> <text>\n");
    } else if (strcmp(argv[0], "append") == 0) {
        if (argc >= 3) fat_writefile(argv[1], argv[2], 1); // Append mode
    } else if (strcmp(argv[0], "rmfile") == 0) {
        if (argv[1]) fat_rmfile(argv[1]);
    }
    /* EXIT */
    else if (strcmp(argv[0], "exit") == 0) {
        fat_unmount();
        exit(0);
    } else {
        printf("Unknown command: %s\n", argv[0]);
    }
}

// ... keep dup_string and deallocate_cmd as they were ...

void get_cmd_line(char* argv[MAX_TOKENS], int* argc) {
    char line[MAX_LINE];
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        printf("\n");
        fat_unmount();
        exit(0);
    }
    char* token = strtok(line, " \t\n");
    *argc = 0;
    while (*argc < MAX_TOKENS && token != NULL) {
        argv[(*argc)++] = strdup(token); // Simplified with strdup
        token = strtok(NULL, " \t\n");
    }
    argv[*argc] = NULL;
}

int do_shell(const char* prompt_base) {
    printf("Welcome to Shellby, a FAT based and portable shell\n");
    for (;;) {
        // Dynamic prompt: "shellby:/path/to/dir$ "
        printf("%s%s$ ", prompt_base, disk->disk_base ? disk->cwd_path : "");
        
        char* argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv, &argc);
        if (argc > 0) {
            do_cmd(argv, argc);
            for(int i=0; i<argc; i++) free(argv[i]);
        }
    }
    return 0;
}