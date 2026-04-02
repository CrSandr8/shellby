#include "fat.h"
#include "fat_structs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define  DISK_SIZE 66666666

char * disk_base;

int fat_format(const char * filename, int size_mb){
    if (!filename){
        filename = "disk_name.img";
    }
    int fd;
    if ((fd = open(filename, O_CREAT | O_RDWR, 0666)) == -1){
        perror("Cannot properly format virtual disk");
        return -1;
    }
    if (ftruncate(fd, DISK_SIZE) == -1){
        perror("Cannot properly format virtual disk");
        return -1;       
    }

    // mmap and init

    disk_base = (char *) mmap(NULL, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk_base == MAP_FAILED){
        perror("Error while mapping...");
        close(fd);
        return -1;
    }

    FAT_BootSector * bs = (FAT_BootSector * )disk_base;

    bs->BytsPerSec = SECTOR_SIZE;
    bs->SecPerClus = 8;
    bs->NumFATS = 2;
    bs->FATSz = 2;
    bs->RootClus = 2;
    bs->FSInfo_offset = 1;
    bs->Signature = BS_SIGNATURE;

    FAT_FSInfo * fsinfo = (FAT_FSInfo * ) disk_base + SECTOR_SIZE; // fsinfo is typically at offset 1

    memset(fsinfo, 0, sizeof(FAT_FSInfo));

    fsinfo->FSI_LeadSig = 0x41615252;
    fsinfo->FSI_StrucSig  = 0x61417272;
    fsinfo->FSI_TrailSig = 0xAA550000;

    fsinfo->FSI_Nxt_Free = 3;
    fsinfo->FSI_Free_Count = 0;


    return fd;

}
int fat_mount(const char *filename){
    disk_base = filename;
    int fd = open(disk_base, O_RDWR, 0666);
    if (fd){
        
    }
    else{
        perror("FATAL: Cannot open virtual disk");
        return -1;
    }
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