#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat/fat.h"
#include "shell/shell.h"


int main(int argc, char **argv) {
    disk = malloc(sizeof(FAT_Disk));
    if (disk == NULL) {
        perror("Memory allocation error");
        return 1;
    }

    memset(disk, 0, sizeof(FAT_Disk));
    
    return do_shell("shellby>>");
}