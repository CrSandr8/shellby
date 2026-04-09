#ifndef _FAT_H
#define _FAT_H

#include "fat_structs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fat_mount(const char *filename);
int fat_unmount(void);
int fat_format(const char *filename, int size);


#endif