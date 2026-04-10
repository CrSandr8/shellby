#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Assicurati che il percorso dell'header sia corretto in base alle tue cartelle
#include "virtual_fat/fat.h" 

#define MAX_LINE    2048
#define MAX_TOKENS  256

void listCommands(){
    printf("\nlscmd:\t\t\t lista i compandi disponibili\n");
    printf("\nhelp_ext command:\t stampa il comando \"command\" e una sua descrizione\n");
    printf("\ninit (filename | NULL):\t inizializza il filesystem (default size 1MB)\n\t\t\tse filename non viene inserito verra' assunto 'filesystem.img'\n");
    printf("\nload (filename | NULL):\t carica il filesystem da filename\n\t\t\tse filename non viene inserito verra' assunto 'filesystem.img'\n");
    printf("\nmkfile file_name:\t crea il file file_name\n");
    printf("\nmkdir dir_name:\t\t crea la directory dir_name\n");
    printf("\ncd dir_name:\t\t cambia la directory corrente\n");
    printf("\nexit:\t\t\t esci dal programma (e smonta il disco)\n");
    printf("\n(Comandi come ls, read, write e rm non sono ancora supportati dal backend)\n");
}

void help_ext(char* cmd){
    if (strcmp(cmd, "") == 0) {
        printf("help_ext command: stampa il comando \"command\" e una sua descrizione\n");
    } else if(strcmp(cmd, "init") == 0) {
        printf("init filename: inizializza il filesystem dadogli il nome filename\n");
    } else if(strcmp(cmd, "mount") == 0) {
        printf("load filename: carica il filesystem da filename\n");
    } else if(strcmp(cmd, "lscmd") == 0) {
        listCommands();
    } else if(strcmp(cmd, "touch") == 0) {
        printf("mkfile file_name: crea il file file_name\n");
    } else if (strcmp(cmd, "mkdir") == 0) {
        printf("mkdir dir_name: crea la directory dir_name\n");
    } else if (strcmp(cmd, "cd") == 0) {
        printf("cd dir_name: cambia la directory corrente\n");
    }else{
        printf("Comando per help non trovato o non supportato.\n");
    }
    return;
}

void do_cmd(char* argv[MAX_TOKENS], int argc) {
    /* HELP COMMANDS */
    if (strcmp(argv[0], "help_ext") == 0) {
        if (argv[1] == NULL) {
            help_ext("");
        } else {
            help_ext(argv[1]);
        }
    } else if (strcmp(argv[0], "lscmd") == 0) {
        listCommands();
    
    /* FS COMMANDS */
    } else if (strcmp(argv[0], "init") == 0) {
        // Usiamo una dimensione fissa di 1MB (1048576 byte) se si usa questo comando stile legacy
        char *filename = (argv[1] == NULL) ? "filesystem.img" : argv[1];
        fat_create_disk(filename, 1048576); 
    } else if (strcmp(argv[0], "mount") == 0) {
        char *filename = (argv[1] == NULL) ? "filesystem.img" : argv[1];
        fat_mount(filename);

    /* FILES COMMANDS */
    } else if(strcmp(argv[0], "touch") == 0) {
        if(argv[1] == NULL){
            printf("mkfile file_name: crea il file file_name\n");
            return;
        }
        fat_createfile(argv[1]);
        
    /* DIRECTORY COMMANDS */
    } else if (strcmp(argv[0], "mkdir") == 0) {
        if(argv[1] == NULL){
            printf("mkdir dir_name: creates a directory\n");
            return;
        }
        fat_createdir(argv[1]);
    } else if (strcmp(argv[0], "rmdir") == 0) {
        if(argv[1] == NULL){
            printf("rmdir dir_name: elimina la directory e tutto il suo contenuto \n");
            return;
        }
        fat_rmdir(argv[1]); // Questa attualmente e' vuota nel backend, ma compila!
    } else if (strcmp(argv[0], "cd") == 0) {
        if(argv[1] == NULL){
            // Senza argomenti andiamo in root
            fat_change_dir("/");
            return;
        }
        fat_change_dir(argv[1]);
        
    /* COMANDI DA IMPLEMENTARE NEL BACKEND */
    } else if (strcmp(argv[0], "rmfile") == 0 || strcmp(argv[0], "write") == 0 || 
               strcmp(argv[0], "read") == 0 || strcmp(argv[0], "seek") == 0 || 
               strcmp(argv[0], "ls") == 0) {
        printf("Backend: Il comando '%s' non e' ancora implementato in fat.c.\n", argv[0]);

    /* EXIT COMMAND */
    } else if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
        // Smontiamo il disco in sicurezza prima di uscire
        fat_unmount();
        exit(0);
    } else {
        printf("unknown command %s\n", argv[0]);
    }
}

void deallocate_cmd(char* argv[MAX_TOKENS]) {
	while (*argv != NULL)
		free(*argv++);
}

char* dup_string(const char* in) {
    size_t n = strlen(in);
    char* out = malloc(n + 1);
    strcpy(out, in);
    return out;
}

void get_cmd_line(char* argv[MAX_TOKENS], int* argc) {
    char line[MAX_LINE];
    // Aggiunto controllo per evitare crash su Ctrl+D (EOF)
    if (fgets(line, MAX_LINE, stdin) == NULL) {
        *argc = 0;
        argv[0] = NULL;
        printf("\n");
        fat_unmount(); // Salvataggio di sicurezza
        exit(0);
    }
    
    char* token = strtok(line, " \t\n");
    *argc = 0;
    while (*argc < MAX_TOKENS && token != NULL) {
        argv[(*argc)++] = dup_string(token);
        token = strtok(NULL, " \t\n");
    }
    argv[*argc] = NULL;
}

int do_shell(const char* prompt){
    printf("lscmd: lists the available commands\n");
    for (;;) {
        printf("%s", prompt);
        char* argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv, &argc);
        if (argv[0] == NULL) continue;
        
        do_cmd(argv, argc);
	    deallocate_cmd(argv);
    }
    return EXIT_SUCCESS;
}