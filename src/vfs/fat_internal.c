#include "fat.h"
#include "fat_internal.h"

bool globl_is_mounted = false;

FILE *globl_disk = NULL;

FAT_BootSector globl_bs;

uint32_t FAT_StartSector = 0; // BPB_ResvdSecCnt
uint32_t FAT_Sectors = 0; // BPB_FATSz * BPB_NumFATs
uint32_t FAT_RootDirStartSector = 0; // FatStartSector + FatSectors
uint32_t FAT_RootDirSectors = 0; // (32 * BPB_RootEntCnt + BPB_BytsPerSec - 1) / BPB_BytsPerSec
uint32_t FAT_DataStartSector = 0; // RootDirStartSector + RootDirSectors
uint32_t FAT_DataSectors = 0; // BPB_TotSec - DataStartSector
uint32_t FAT_CountofClusters = 0; // DataSectors / BPB_SecPerClus