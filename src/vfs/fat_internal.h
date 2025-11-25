#ifndef _FAT_INTERNAL_H
#define _FAT_INTERNAL_H

#include "fat_structs.h"
#include <stdio.h>
#include <stdbool.h>

extern bool globl_is_mounted;

extern FAT_BootSector globl_bs;

extern FILE *globl_disk;

typedef struct
{
    bool is_used;

    uint32_t current_offset;

    FAT_DirectoryEntry cached_entry;
} FAT_Fd;

extern FAT_Fd global_fd_table[];

extern uint32_t FAT_StartSector;
extern uint32_t FAT_Sectors;
extern uint32_t FAT_RootDirStartSector;
extern uint32_t FAT_RootDirSectors;
extern uint32_t FAT_DataStartSector;
extern uint32_t FAT_DataSectors;
extern uint32_t FAT_CountofClusters;

#endif