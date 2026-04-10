#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_shell.h"


cmd_t cmd_table[] = {
    {"format", cmd_format, "format file as a shellby fs file with the given size"},
    {"mkdir", cmd_mkdir, ""},
    {"cd", cmd_cd, ""},
    {"touch", cmd_touch, ""},
    {"cat", cmd_cat, ""},
    {"ls", cmd_ls, ""},
    {"append", cmd_append, ""},
    {"rm", cmd_rm, ""},
    {"close", cmd_close, ""},
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
            return cmd_table[i].func(argc, argv);
        }
    }

    printf("Shellby: command not found: %s\n", argv[0]);
    return -1;
}

int do_shell(const char *prompt)
{
    printf("lscmd: lists the available commands\n");
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

// Print file rows as raw text

int cmd_cat(int argc, char **argv)
{
    return 0;
}

// Append the inserted text to the selected file

int cmd_append(int argc, char **argv)
{
    return 0;
}

// Safely close everything

int cmd_shell_close(int argc, char **argv)
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