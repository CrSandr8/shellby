#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtual_fat/fat.h"
#include "shell/my_shell.h"

int main(int argc, char **argv) {
    // 1. Allochiamo lo spazio per il sistema operativo
    disk = malloc(sizeof(FAT_Disk));
    if (disk == NULL) {
        perror("Errore di allocazione memoria");
        return 1;
    }

    // 2. Inizializziamo a zero per sicurezza
    memset(disk, 0, sizeof(FAT_Disk));
    
    return do_shell("shellby: $ ");
}