#include "fat.h"
#include "fat_internal.h"

bool globl_is_mounted = false;

FILE *globl_disk = NULL;

FAT_BootSector globl_bs;

uint32_t FAT_StartSector = 0;
uint32_t FAT_Sectors = 0;
uint32_t FAT_RootDirStartSector = 0;
uint32_t FAT_RootDirSectors = 0;
uint32_t FAT_DataStartSector = 0;
uint32_t FAT_DataSectors = 0;
uint32_t FAT_CountofClusters = 0;