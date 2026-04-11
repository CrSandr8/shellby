#ifndef _FAT_H
#define _FAT_H

#include "fat_structs.h"

// Global pointer to the mapped disk structure
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
int fat_readdir(uint32_t dir_sector);

int fat_createfile(const char *filename);
int fat_readfile(const char *filename);
int fat_writefile(const char *filename, const char *text, int append);
int fat_rmfile(const char *filename);

//============================================================================//
//============================= Backend Routines =============================//
//============================================================================//

FAT_FCB *find_in_dir(const char *name, uint32_t sector);
FAT_FCB *find_free_slot(uint32_t sector);
FAT_FCB *read_dir_next(uint32_t dir_sector, uint32_t *cursor);

int chain_append(uint32_t a, uint32_t b);
int chain_rm(uint32_t first_sector);
int chain_cut(uint32_t first_sector, int size);

uint32_t get_free_sector(void);

// Helper macros for sector and entry management
#define get_next_sector(a) (disk->fat[(a)])
#define get_entries(a) ((FAT_FCB *)(disk->data + ((a) * SECTOR_SIZE)))

// Disk size calculation helpers
int get_num_sectors(int data_size);
int get_fat_size(int num_sectors);
int get_total_disk_size(int fat_size, int data_size);

// Path and string management
uint32_t fat_resolve_path(const char *path);
int update_cwd_path(const char *path);

#endif