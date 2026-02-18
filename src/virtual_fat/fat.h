#ifndef _FAT_H
#define _FAT_H

#include <stdint.h>

#define MAX_OPEN_FILES 12

// FILE API

int fat_open(const char *path, int mode);
int fat_close(int fd);
int fat_read(int fd, void *buf, int size);
int fat_write(int fd, const void *buf, int size);
int fat_lseek(int fd, int offset, int whence);

// DIRECTORY API

int fat_mkdir(const char *path);
int fat_rm(const char *path);

// Mount && Unmount

int fat_mount(const char *filename);
void fat_unmount();

#endif