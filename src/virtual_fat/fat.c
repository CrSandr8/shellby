#include "fat.h"

FAT_Disk disk;

int fat_format(const char *filename)
{

}
int fat_mount(const char *filename)
{
    printf("Starting mount operations...\n");
    int fd;
    if ((fd = open(filename, O_RDWR)) == -1){
        perror("Error in opening file during mount...");
        return FAT_ERR_GENERIC;
    }

    disk.disk_size = DISK_SIZE;

    disk.disk_base = mmap(NULL, disk.disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk.disk_base == MAP_FAILED){
        perror("Error while mapping...");
        return FAT_ERR_GENERIC;
    }

    disk.bs = (FAT_BootSector *) disk.disk_base;

    printf("Checking disk signature...\n");

    if (disk.bs->Signature != BS_SIGNATURE){
        munmap(disk.disk_base, disk.disk_size);
        disk.disk_base = NULL;
        close(fd);
        return FAT_ERR_BAD_SIG;
    }

    // We now may be using a rightful disk file

    printf("Signature OK!\n");

    uint32_t fsi_offset = disk.bs->BytsPerSec * disk.bs->FSInfo_offset; //Bytes
    disk.fsinfo = (FAT_FSInfo *) (disk.disk_base + fsi_offset);

    uint32_t fat_offset = disk.bs->BytsPerSec * disk.bs->RsvdSecCnt;
    disk.fat_table = (uint32_t *) (disk.disk_base + fat_offset);

    uint32_t data_offset = (disk.bs->RsvdSecCnt + (disk.bs->NumFATS * disk.bs->FATSz)) * disk.bs->BytsPerSec;
    disk.data_region = (uint32_t *) (disk.disk_base + data_offset);

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        disk.open_files[i].is_used = 0;
        disk.open_files[i].current_offset = 0;
    }

    close(fd);

    printf("Correctly mounted disk\n");

    return FAT_SUCCESS;
}
void fat_unmount(FAT_Disk disk)
{

    printf("Starting unmount process...\n");
    if (disk.disk_base == NULL) return;

    if (msync(disk.disk_base, disk.disk_size, MS_SYNC) == -1){
        perror("Error while saving data on disk");
        return FAT_ERR_GENERIC;
    }

    if (munmap(disk.disk_base, disk.disk_size) == -1){
        perror("Error while unmapping disk");
        return FAT_ERR_GENERIC;
    }

    memset(&disk, 0, sizeof(FAT_Disk));
    printf("Unmounting OK!\n");

    return FAT_SUCCESS;

}
int fat_init(const char *filename)
{
    if (fat_mount(filename) == FAT_SUCCESS) return FAT_SUCCESS;

    //might want to format or do some

}
int fat_open(const char *path, int mode)
{
    // TODO
}
int fat_close(int fd)
{
    // TODO
}
int fat_read(int fd, void *buf, int size)
{
    // TODO
}
int fat_write(int fd, const void *buf, int size)
{
    // TODO
}
int fat_lseek(int fd, int offset, int whence)
{
    // TODO
}
int fat_mkdir(const char *path)
{
    // TODO
}
int fat_rm(const char *path)
{
    // TODO
}