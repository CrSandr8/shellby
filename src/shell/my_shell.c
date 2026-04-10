#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_shell.h"


shell_state_t current_state = SHELL_STATE_UNMOUNTED;


cmd_t cmd_table[] = {


    {"init", cmd_init, "create a new directory", SHELL_STATE_UNMOUNTED},
    {"mount", cmd_mount, "create a new directory", SHELL_STATE_UNMOUNTED},
    
    {"mkdir", cmd_mkdir, "create a new directory", SHELL_STATE_MOUNTED},
    {"cd", cmd_cd, "change current working directory", SHELL_STATE_MOUNTED},
    {"touch", cmd_touch, "create a file", SHELL_STATE_MOUNTED},
    {"cat", cmd_cat, "print the content of a file", SHELL_STATE_MOUNTED},
    {"ls", cmd_ls, "list entries in this directory", SHELL_STATE_MOUNTED},
    {"append", cmd_append, "append text to a file", SHELL_STATE_MOUNTED},
    {"rm", cmd_rm, "remove a file or directory", SHELL_STATE_MOUNTED},
    {NULL, NULL, NULL}};

void deallocate_cmd(char *argv[MAX_TOKENS])
{
    while (*argv != NULL)
        free(*argv++);
}

char *dup_string(const char *in)
{
    size_t n = strlen(in);
    char *out = malloc(n + 1);
    strcpy(out, in);
    return out;
}

void get_cmd_line(char *argv[MAX_TOKENS], int *argc)
{
    char line[MAX_LINE];
    fgets(line, MAX_LINE, stdin);
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        *argc = 0;
        argv[0] = NULL;
        printf("\n"); 
        exit(0);
    }
    char *token = strtok(line, " \t\n");
    *argc = 0;
    while (*argc < MAX_TOKENS && token != NULL)
    {
        argv[(*argc)++] = dup_string(token);
        token = strtok(NULL, " \t\n");
    }
    argv[*argc] = NULL;
}

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

int do_shell(const char *prompt)
{
    printf("Welcome to Shellby!\n");
    for (;;)
    {
        printf("%s", prompt);
        char *argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv, &argc);
        if (argv[0] == NULL)
            continue;
        if (strcmp(argv[0], "quit") == 0)
            break;
        do_cmd(argv, argc);
        deallocate_cmd(argv);
    }
    return EXIT_SUCCESS;
}

int cmd_init(int argc, char **argv)
{
    return 0;
}
int cmd_mount(int argc, char **argv)
{
    return 0;
}

// Print file rows as raw text

int cmd_cat(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Uso: cat <nome_file>\n");
        return -1;
    }

    char *file_content = fat_readfile(argv[1]);

    if (file_content == NULL)
    {
        printf("cat: %s: File non trovato o è una directory\n", argv[1]);
        return -1;
    }

    //Print the result
    printf("%s\n", file_content);

    free(file_content);

    return 0;
}

// Append the inserted text to the selected file

int cmd_append(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Uso: append <nome_file> stringa/testo \n");
        return -1;
    }

    // Il comando "snello" che avevi in mente:
    //char *file_content = fat_append(argv[1]);

    //if (file_content == NULL)
    //{
    //    printf("cat: %s: File non trovato o è una directory\n", argv[1]);
    //    return -1;
    //}
//
    //// Stampa il risultato nudo e crudo
    //printf("%s\n", file_content);
//
    //// Liberiamo la memoria allocata dal backend per il buffer
    //free(file_content);

    return 0;
}

// Safely close everything

int cmd_close(int argc, char **argv)
{
    return 0;
}

// === COMANDI IMPLEMENTATI ===

int cmd_format(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Uso: format <nome_disco.img> <dimensione_in_byte>\n");
        return -1;
    }
    
    return fat_create_disk(argv[1], atoi(argv[2]));
}

int cmd_mkdir(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Uso: mkdir <nome_cartella>\n");
        return -1;
    }
    return fat_createdir(argv[1]);
}

int cmd_touch(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Uso: touch <nome_file>\n");
        return -1;
    }
    return fat_createfile(argv[1]);
}

int cmd_cd(int argc, char **argv)
{
    // If we only write "cd" we go to root folder
    if (argc < 2)
    {
        return fat_change_dir("/");
    }
    return fat_change_dir(argv[1]);
}

// === COMANDI ANCORA DA IMPLEMENTARE (per non far fallire la compilazione) ===

int cmd_ls(int argc, char **argv)
{
    printf("Comando ls ancora da implementare nel backend.\n");
    return 0;
}

int cmd_rm(int argc, char **argv)
{
    printf("Comando rm ancora da implementare nel backend.\n");
    return 0;
}