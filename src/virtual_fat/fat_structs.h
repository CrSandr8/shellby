#ifndef _FAT_STRUCTS_H
#define _FAT_STRUCTS_H

#include <stdint.h>

// Error codes for function return values
#define FAT_SUCCESS 0
#define FAT_ERR_GENERIC -1 
#define FAT_ERR_BAD_SIG -2 
#define FAT_ERR_DISK_FULL -3

// Personalized signature (checked during mount)
#define BS_SIGNATURE 0xAC18

// FAT Block status definitions
#define FAT_FREE 0
#define FAT_RESERVED 0x00000001
#define FAT_BADCLUSTER 0x0FFFFFF7
#define FAT_EOC 0x0FFFFFF8

// Fundamental dimensional values
#define SECTOR_SIZE 512 
#define ENTRIES_PER_SEC (SECTOR_SIZE / 32) // Each FCB is 32 bytes
#define MIN_DISK_SIZE (1024 * 1024)

/**
 * Superblock: Located at the start of the disk file (Sector 0)
 */
typedef struct
{
    uint16_t Signature;      // Magic number
    uint16_t BytsPerSec;     // Sector size (usually 512)
    uint32_t FATSz;          // Total size of the FAT region in bytes
    uint32_t RootSec;        // Starting sector of the root directory
    uint32_t FSI_Free_Count; // Number of free sectors remaining
    uint32_t FSI_Nxt_Free;   // Hint for the next free sector search
    uint8_t padding[492];    // Pad to 512 bytes

} __attribute__((packed)) FAT_Superblock;

/**
 * File Control Block (FCB): Represents a file or directory entry (32 bytes)
 */
typedef struct
{
    uint8_t name[16];      // File name string (null-terminated)
    uint8_t ext[7];        // File extension
    uint32_t first_sector; // Index of the first sector in the FAT
    uint8_t is_dir;        // Boolean flag: 1 if directory, 0 if file
    uint32_t file_size;    // Size of the file in bytes

} __attribute__((packed)) FAT_FCB;

/**
 * FAT_Disk: Runtime structure containing mapping info and state
 */
typedef struct
{
    uint8_t disk_name[64];
    uint32_t disk_size;

    void *disk_base;       // Base address of the mmap'ed file
    FAT_Superblock *sb;    // Pointer to the superblock in memory
    uint32_t *fat;         // Pointer to the FAT table region
    uint8_t *data;         // Pointer to the start of the data region

    uint32_t cwd_sector;   // Current sector of the working directory
    char cwd_path[256];    // String representation of current path

} FAT_Disk;

#endif