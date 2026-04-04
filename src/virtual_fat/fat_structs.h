#ifndef _FAT_STRUCTS_H
#define _FAT_STRUCTS_H

#include <stdint.h>

// Error codes for function return values
#define FAT_SUCCESS           0
#define FAT_ERR_GENERIC      -1   // Generic error code 
#define FAT_ERR_BAD_SIG      -2   // Non mountable file

// Dimensional values used
#define SECTOR_SIZE 512

// Personalized signature
#define BS_SIGNATURE 0xAC18

// File attributes, as bitmask
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10    // Is this a directory?

// Block status
#define FAT_FREE 0
#define FAT_RESERVED 0x00000001
#define FAT_INUSE(x) ((x>=0x00000002) && (x<=0x0FFFFFF6)) 
#define FAT_BADCLUSTER 0x0FFFFFF7
#define FAT_EOC 0x0FFFFFF8

//
#define MAX_OPEN_FILES 12

//
#define  DISK_SIZE 66666666


// BOOT SECTOR

typedef struct
{

    uint16_t BytsPerSec;     // Sector size in unit of byte  This value may take on only the following values: 512, 1024, 2048 or 4096
    uint8_t  SecPerClus;     // Sector per allocation unit This value must be a power of 2 that is greater than 0. The legal values are 1, 2, 4, 8, 16, 32, 64, and 128
    uint16_t RsvdSecCnt;     //TODO explain 
    uint8_t  NumFATS;        // How many FATS we have
    uint32_t FATSz;        // Size of a FAT in unit of sector.
    uint32_t RootClus;       // First cluster number of root directory (Usually 2)
    uint16_t FSInfo_offset;  // Sector of FSInfo structure in offset from top of the FAT volume (Usually 1)
    uint8_t padding[494];
    uint16_t Signature;

} __attribute__((packed)) FAT_BootSector; // 512 bytes 

typedef struct
{
    uint32_t FSI_LeadSig;        // 0x41615252 || Whatever
    uint8_t  FSI_Reserved1[480]; // 0
    uint32_t FSI_StrucSig;       // 0x61417272 || Whatever
    uint32_t FSI_Free_Count;     // Last known free cluster count
    uint32_t FSI_Nxt_Free;       // A hint for the next free cluster
    uint8_t  FSI_Reserved2[12];  // 0
    uint32_t FSI_TrailSig;       // 0xAA550000 || Whatever
    
} __attribute__((packed)) FAT_FSInfo;  // 512 bytes

typedef struct
{
    uint8_t name[22];     // name always has 22 chars
    uint32_t first_cluster; // first cluster index
    uint8_t attr;    // directory attributes
    uint8_t is_res;    // is directory reserved?
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
    uint8_t *disk_base;         
    uint32_t disk_size;         
    
    FAT_BootSector *bs;         
    FAT_FSInfo *fsinfo;         
    uint32_t *fat_table;        
    uint8_t *data_region;
    
    FAT_Fd open_files[MAX_OPEN_FILES];

} FAT_Disk;

// uint32_t first_data_sector = fs_state.bs->RsvdSecCnt + (fs_state.bs->NumFATS * fs_state.bs->FATSz);


#endif