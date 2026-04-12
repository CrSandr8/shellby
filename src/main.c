#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virtual_fat/fat.h"
#include "shell/my_shell.h"


int main(int argc, char **argv) {
    // 1. Allocate space for the disk state structure
    disk = malloc(sizeof(FAT_Disk));
    if (disk == NULL) {
        perror("Memory allocation error");
        return 1;
    }

    // 2. Clear structure memory
    memset(disk, 0, sizeof(FAT_Disk));
    
    // 3. Start the shell
    return do_shell("shellby>>");
}