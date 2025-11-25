#include "fat.h"
#include "fat_internal.h"
#include <stdio.h>

int fat_mount(const char *filename){
    globl_disk = fopen(filename, "rb+");

    if (!globl_disk) return -1; //ERR

    fseek(globl_disk, 0, SEEK_SET);

    if (fread(&globl_bs, sizeof(FAT_BootSector), 1, globl_disk) != 1){
        fclose(&globl_disk);
        return -2;
    }

    FAT_StartSector = globl_bs.BPB_RsvdSecCnt;

    FAT_Sectors = globl_bs.BPB_FATSz32 * globl_bs.BPB_NumFATS;

    FAT_RootDirStartSector = FAT_StartSector + FAT_Sectors;

    FAT_RootDirSectors = (32 * globl_bs.BPB_RootEntCnt + globl_bs.BPB_BytsPerSec - 1) / globl_bs.BPB_BytsPerSec;

    FAT_DataStartSector = FAT_RootDirStartSector  + FAT_RootDirSectors;

    FAT_DataSectors = globl_bs.BPB_TotSec32 - FAT_DataStartSector;

    FAT_CountofClusters = FAT_DataSectors / globl_bs.BPB_SecPerClus;

    globl_is_mounted = true;

    return 0;
}
void fat_unmount(){
// TODO    
}
int fat_open(const char *path, int mode){
// TODO
}
int fat_close(int fd){
// TODO    
}
int fat_read(int fd, void *buf, int size){
// TODO    
}
int fat_write(int fd, const void *buf, int size){
// TODO    
}
int fat_lseek(int fd, int offset, int whence){
// TODO    
}
int fat_mkdir(const char *path){
// TODO    
}
int fat_rm(const char *path){
// TODO    
}