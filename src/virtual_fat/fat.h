#ifndef _FAT_H
#define _FAT_H

#include "fat_structs.h"

extern FAT_Disk *disk;

//============================================================================//
//============================== Disk Manipulation ===========================//
//============================================================================//
int fat_create_disk(const char *filename, int size);
int fat_mount(const char *disk_path);
int fat_unmount(void);

//============================================================================//
//================================ FS API ====================================//
//============================================================================//
int fat_createdir(const char *name);
int fat_rmdir(const char *path);
int fat_change_dir(const char *path);
int fat_createfile(const char *filename);

//============================================================================//
//============================= Backend Routines =============================//
//============================================================================//

FAT_FCB *find_in_dir(const char *name, uint32_t sector);
FAT_FCB *find_free_slot(uint32_t sector);

int chain_append(uint32_t a, uint32_t b);
int chain_rm(uint32_t first_sector);
int chain_cut(uint32_t first_sector, int size);

uint32_t get_free_sector(void);

#define get_next_sector(a) (disk->fat[(a)])
#define get_entries(a) ((FAT_FCB *)(disk->data + ((a) * SECTOR_SIZE)))

int get_num_sectors(int data_size);
int get_fat_size(int num_sectors);
int get_total_disk_size(int fat_size, int data_size);

uint32_t fat_resolve_path(const char *path);
int update_cwd_path(const char *path);

#endif