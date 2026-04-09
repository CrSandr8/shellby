#ifndef _FAT_STRUCTS_H
#define _FAT_STRUCTS_H

#include <stdint.h>

// Error codes for function return values
#define FAT_SUCCESS 0
#define FAT_ERR_GENERIC -1 // Generic error code
#define FAT_ERR_BAD_SIG -2 // Non mountable file
#define FAT_ERR_DISK_FULL -3

// Personalized signature
#define BS_SIGNATURE 0xAC18

// Block status
#define FAT_FREE 0
#define FAT_RESERVED 0x00000001
#define FAT_INUSE(x) ((x >= 0x00000002) && (x <= 0x0FFFFFF6))
#define FAT_BADCLUSTER 0x0FFFFFF7
#define FAT_EOC 0x0FFFFFF8


#define MIN_DISK_SIZE 1024 * 1024

// Dimensional values used
#define SECTOR_SIZE 512 // bytes
#define ENTRIES_PER_SEC SECTOR_SIZE / 32

#define MAX_OPEN_FILES 12

// TODO add disk size selector/macro elsewhere

typedef struct
{
    uint16_t Signature;
    uint16_t BytsPerSec;     // Sector size in unit of byte  This value may take on only the following values: 512, 1024, 2048 or 4096
    uint32_t FATSz;          // Size of the FAT.
    uint32_t RootSec;        // First sector number of root directory
    uint32_t FSI_Free_Count; // Last known free sector count
    uint32_t FSI_Nxt_Free;   // The offset of a hinted free block
    uint8_t padding[492];

} __attribute__((packed)) FAT_Superblock; // 512 bytes

typedef struct
{
    uint8_t name[23];      // name always has 22 chars
    uint32_t first_sector; // first sector index
    uint8_t is_dir;        // is this a directory?
    uint32_t file_size;

} __attribute__((packed)) FAT_FCB; // 32 bytes

typedef struct
{
    uint8_t is_used;
    uint32_t current_offset;
    FAT_FCB cached_entry;
    uint8_t padding[27];

} __attribute__((packed)) FAT_Fd; // 64 bytes

typedef struct
{
    uint8_t disk_name[64];
    uint32_t disk_size;

    void *disk_base;
    FAT_Superblock *sb;
    uint32_t *fat; // The actual File Allocation Table
    uint8_t *data; // The data region

    FAT_Fd open_files[MAX_OPEN_FILES];
    FAT_FCB *cwd;


} FAT_Disk;

extern FAT_Disk *disk;

#endif