#ifndef _FAT_STRUCTS_H
#define _FAT_STRUCTS_H

#include <stdint.h>

// Dimensional values used
#define SECTOR_SIZE 512
#define DIR_ENTRY_SIZE 32

// File attributes, a bitmask
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10    // Is this a directory?
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  0x0F

#define FAT_FREE 0
#define FAT_RESERVED 0x00000001
#define FAT_INUSE(x) ((x>=0x00000002) && (x<=0x0FFFFFF6)) 
#define FAT_BADCLUSTER 0x0FFFFFF7
#define FAT_EOC 0x0FFFFFF8

// BOOT SECTOR

typedef struct
{

    // --- Standard BPB ---
    uint16_t BytsPerSec;     // Sector size in unit of byte  This value may take on only the following values: 512, 1024, 2048 or 4096
    uint8_t  SecPerClus;     // Sector per allocation unit This value must be a power of 2 that is greater than 0. The legal values are 1, 2, 4, 8, 16, 32, 64, and 128
    uint8_t  NumFATS;        // How many FATS we have

    // --- Extended Boot Record (EBR) ---
    uint32_t FATSz;        // Size of a FAT in unit of sector.
    uint32_t RootClus;       // First cluster number of root directory (Usually 2)
    uint16_t FSInfo_offset;  // Sector of FSInfo structure in offset from top of the FAT volume (Usually 1)
    uint16_t Signature;

} __attribute__((packed)) FAT_BootSector;

typedef struct
{
    uint32_t FSI_LeadSig;        // 0x41615252
    uint8_t  FSI_Reserved1[480]; // 0
    uint32_t FSI_StrucSig;       // 0x61417272
    uint32_t FSI_Free_Count;     // Last known free cluster count
    uint32_t FSI_Nxt_Free;       // A hint for the next free cluster
    uint8_t  FSI_Reserved2[12];  // 0
    uint32_t FSI_TrailSig;       // 0xAA550000
    
} __attribute__((packed)) FAT_FSInfo;  // 512 bytes

typedef struct
{
    uint8_t name[21];     // name always has 21 chars
    uint32_t first_cluster; // first cluster index
    uint8_t attr;    // directory attributes
    uint8_t is_res;    // is directory reserved?
    uint8_t is_dir;    // is this a directory?
    uint32_t file_size;

} __attribute__((packed)) FAT_DirectoryEntry;  // 32 bytes

typedef struct
{
    uint8_t is_used;
    uint32_t current_offset;
    FAT_DirectoryEntry cached_entry;
    uint8_t padding[27];

} __attribute__((packed)) FAT_Fd;  //64 bytes

typedef struct
{
    FAT_BootSector bs;
    FAT_Fd fdTable[];

} FAT_GlobalInfo;

typedef struct
{
    FILE * name;
    uint8_t is_mounted;

} Disk_Fd;


#endif