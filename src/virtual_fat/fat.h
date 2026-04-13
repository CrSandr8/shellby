#ifndef _FAT_H
#define _FAT_H

#include "fat_structs.h"

// Global pointer to the mapped disk structure
extern FAT_Disk *disk;
extern int optind; //this 


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
int fat_change_dir(const char *path);
int fat_readdir(uint32_t dir_sector);

int fat_createfile(const char *filename);
int fat_readfile(const char *filename);
int fat_writefile(const char *filename, const char *text, int append);
int fat_rm(const char *filename, int flag_recursive);

//============================================================================//
//============================= Backend Routines =============================//
//============================================================================//

FAT_FCB *find_in_dir(const char *name, uint32_t sector);
FAT_FCB *find_free_entry(uint32_t sector);
FAT_FCB *read_dir_next(uint32_t dir_sector, uint32_t *cursor);

int chain_append(uint32_t a, uint32_t b);
int chain_rm(uint32_t first_sector);
int chain_cut(uint32_t first_sector, int size);
int rm_recursive(uint32_t folder_sector);

uint32_t get_free_sector(void);

// Helper macros for sector and entry management
#define get_total_disk_size(a, b) (a) + (b) + (sizeof(FAT_Superblock))
#define get_num_sectors(a) (a) / SECTOR_SIZE
#define get_fat_size(a) (a) * (sizeof(uint32_t))
#define get_next_sector(a) (disk->fat[(a)])
#define get_entries(a) ((FAT_FCB *)(disk->data + ((a) * SECTOR_SIZE)))

// Path and string management
int parse_filename(const char *filename, char *name_dest, char *ext_dest);
uint32_t fat_resolve_path(const char *path);
int update_cwd_path(const char *path);

// Update the size of the parent directories when a file changes size. Note that this quantity can be positive or negative, since a file can be extended when something is appended or be truncated/deleted
int update_parent_size(uint32_t current_sector, int size);

#endif