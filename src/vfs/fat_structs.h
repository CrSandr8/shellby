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
    uint8_t  BS_jmpBoot[3];      // Jump instruction

    uint8_t  BS_OEMName[8];      // Ex. "MSWIN4.1"

    uint16_t BPB_BytsPerSec;     // Sector size in unit of byte
    
    uint8_t  BPB_SecPerClus;     // Sector per allocation unit

    uint16_t BPB_RsvdSecCnt;     // Number of sector that are in the reserved area

    uint8_t  BPB_NumFATS;        // How many FATS we have

    uint16_t BPB_RootEntCnt; /* From elm-chan:

    On the FAT12/16 volumes, this field indicates number of 32-byte directory entries in the root directory.
    The value should be set a value that the size of root directory is aligned to the 2-sector boundary,
    BPB_RootEntCnt * 32 becomes even multiple of BPB_BytsPerSec.
    For maximum compatibility, this field should be set to 512 on the FAT16 volume.
    For FAT32 volumes, this field must be 0.

    */

    uint16_t BPB_TotSec16;    /* From elm-chan:
    Total number of sectors of the volume in old 16-bit field.
    This value is the number of sectors including all four areas of the volume.
    When the number of sectors of the FAT12/16 volumes is 0x10000 or larger,
    an invalid value 0 is set in this field, and the true value is set to BPB_TotSec32.
    For FAT32 volumes, this field must always be 0.
    */

    uint8_t  BPB_Media;   /*
    Type of disk.
    The valid values for this field is 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE and 0xFF.
    */

    uint16_t BPB_FATSz16;        /*
     	Number of sectors occupied by a FAT. This field is used for only FAT12/16 volumes.
        On the FAT32 volumes, it must be an invalid value 0 and BPB_FATSz32 is used instead.
        The size of the FAT area becomes BPB_FATSz?? * BPB_NumFATs sectors.
    */ 

    uint16_t BPB_SecPerTrk;      /*
    Number of sectors per track. This field is relevant only for 
    media that have geometry and used for only disk BIOS of IBM PC.
    */

    uint16_t BPB_NumHeads;       /*
    Number of heads.
    This field is relevant only for media that have geometry and used for only disk BIOS of IBM PC.
    */
    uint32_t BPB_HiddSec;        /*
    Number of hidden physical sectors preceding the FAT volume.
    It is generally related to storage accessed by disk BIOS of IBM PC,
    and what kind of value is set is platform dependent.
    This field should always be 0 if the volume starts at the beginning of the storage,
    e.g. non-partitioned disks, such as floppy disk.
    */
    uint32_t BPB_TotSec32;       /*
    Total number of sectors of the FAT volume in new 32-bit field.
    This value is the number of sectors including all four areas of the volume.
    When the value on the FAT12/16 volume is less than 0x10000,
    this field must be invalid value 0 and the true value is set to BPB_TotSec16.
    On the FAT32 volume, this field is always valid and old field is not used.
    */

    // --- FAT32 Extended Boot Record (EBR) ---
    uint32_t BPB_FATSz32;        // Size of a FAT in unit of sector. 
    // The size of the FAT area is BPB_FATSz32 * BPB_NumFATs 
    uint16_t BPB_ExtFlags;       //
    uint16_t BPB_FSVer;          // FAT32 version
    uint32_t BPB_RootClus;       // First cluster number of root directory (Usually 2)
    uint16_t BPB_FSInfo;         // Sector of FSInfo structure in offset from top of the FAT32 volume (Usually 1)
    uint16_t BPB_BkBootSec;      // Sector of backup boot in offset from the top of the FAT (Usually 6)
    uint8_t  BPB_Reserved[12];   // 
    uint8_t  BS_DrvNum;          // Drive number used by disk BIOS of IBM PC.
    uint8_t  BS_Reserved1;       // Reserved in DOS and Windows 9X, 
    // used by Windows NT for volume integrity check. It should be set 0 when create the volume.
    uint8_t  BS_BootSig;         // Extended boot signature (0x29). This is a signature byte indicates
    // that the following three fields are present
    uint32_t BS_VolID;           // Volume serial number
    uint8_t  BS_VolLab[11];      // Volume label, matches the one recorded in the root dir  
    uint8_t  BS_FilSysType[8];   // Used by old drivers to determine FAT type, always "FAT32    "
    // From here we should have 512 byte padding since we had more records, useless to
    // the aim of this project RN...
    uint8_t  BS_Code[420]; 
    uint16_t BS_Signature;

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

} __attribute__((packed)) FAT_FSInfo;

// This is basically a 32 byte FCB
typedef struct
{
    // name always has 11 chars
    uint8_t dir_name[11];
    // directory attributes
    uint8_t dir_attr;
    // is directory reserved?
    uint8_t dir_res;
    // creation specs
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    // last access (read?)
    uint16_t acc_date;
    // cluster's high part
    uint16_t cluster_HI;
    // last modifications
    uint16_t write_time;
    uint16_t write_date;
    // cluster's low part
    uint16_t cluster_LO;
    // this is set to 0 if we are a directory
    uint32_t file_size;

} __attribute__((packed)) FAT_DirectoryEntry;

#endif