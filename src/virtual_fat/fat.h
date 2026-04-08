#ifndef _FAT_H
#define _FAT_H

#include "fat_structs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// File operations

int fat_open(const char *path, int mode);
int fat_close(int fd);
int fat_read(int fd, void *buf, int size);
int fat_write(int fd, const void *buf, int size);
int fat_lseek(int fd, int offset, int whence);

// Dir operations

int fat_mkdir(const char *path);
int fat_rm(const char *path);

// Mount && Unmount

int fat_mount(const char *filename);
void fat_unmount(void);

// Format && Init

int fat_format(const char *filename, int size);
int fat_init(const char *filename);

// "Routine Operations"

int find_DirectoryEntry(const char *filename, FAT_FCB *output);

int chain_append();
int chain_rm(uint32_t first_cluster);
int chain_resize(uint32_t first_cluster, int size);

void *get_cluster_ptr(uint32_t cluster);
uint32_t find_free_cluster();


#endif