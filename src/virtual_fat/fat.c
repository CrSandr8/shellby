#include "fat.h"

extern FAT_Disk disk;

int find_DirectoryEntry(const char *path, FAT_FCB *output)
{
    
}

int chain_append() {}

int chain_rm(uint32_t first_cluster)
{
    uint32_t current = first_cluster;

    if (current == FAT_FREE)
        return FAT_SUCCESS;

    while (1)
    {
        uint32_t next = disk.fat_table[current]; //
        disk.fat_table[current] = FAT_FREE;      //

        if (next == FAT_EOC || next == FAT_FREE)
        {
            break; //
        }

        current = next; //
    }

    return FAT_SUCCESS;
}

int fat_format(const char *filename, int size)
{
}

int fat_mount(const char *filename)
{
    printf("Starting mount operations...\n");
    int fd;
    if ((fd = open(filename, O_RDWR)) == -1)
    {
        perror("Error in opening file during mount...");
        return FAT_ERR_GENERIC;
    }

    disk.disk_size = DISK_SIZE;

    disk.disk_base = mmap(NULL, disk.disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk.disk_base == MAP_FAILED)
    {
        perror("Error while mapping...");
        return FAT_ERR_GENERIC;
    }

    disk.bs = (FAT_BootSector *)disk.disk_base;

    printf("Checking disk signature...\n");

    if (disk.bs->Signature != BS_SIGNATURE)
    {
        munmap(disk.disk_base, disk.disk_size);
        disk.disk_base = NULL;
        close(fd);
        return FAT_ERR_BAD_SIG;
    }

    // We now may be using a rightful disk file

    printf("Signature OK!\n");

    // NOTE: the following offsets are in bytes

    uint32_t fsi_offset = disk.bs->BytsPerSec * disk.bs->FSInfo_offset;
    disk.fsinfo = (FAT_FSInfo *)(disk.disk_base + fsi_offset);

    uint32_t fat_offset = disk.bs->BytsPerSec * disk.bs->RsvdSecCnt;
    disk.fat_table = (uint32_t *)(disk.disk_base + fat_offset);

    uint32_t data_offset = (disk.bs->RsvdSecCnt + (disk.bs->NumFATS * disk.bs->FATSz)) * disk.bs->BytsPerSec;
    disk.data_region = (uint8_t *)(disk.disk_base + data_offset);

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        disk.open_files[i].is_used = 0;
        disk.open_files[i].current_offset = 0;
    }

    close(fd);

    printf("Correctly mounted disk\n");

    return FAT_SUCCESS;
}
void fat_unmount(FAT_Disk disk) // TODO capire se qui ci deve essere un input o no
{

    printf("Starting unmount process...\n");
    if (disk.disk_base == NULL)
        return;

    if (msync(disk.disk_base, disk.disk_size, MS_SYNC) == -1)
    {
        perror("Error while saving data on disk");
        return FAT_ERR_GENERIC;
    }

    if (munmap(disk.disk_base, disk.disk_size) == -1)
    {
        perror("Error while unmapping disk");
        return FAT_ERR_GENERIC;
    }

    memset(&disk, 0, sizeof(FAT_Disk));
    printf("Unmounting OK!\n");

    return FAT_SUCCESS;
}
int fat_init(const char *filename)
{
    if (fat_mount(filename) == FAT_SUCCESS)
        return FAT_SUCCESS;

    // might want to format or do some
}
int fat_open(const char *path, int mode)
{
    // printf("[DEBUG] MOCK: fat_open chiamato con path '%s' e mode %d\n", path, mode);

    // return 3;

    FAT_FCB found;

    if (find_DirectoryEntry(path, &found) == FAT_ERR_GENERIC)
    {

        // TODO aggiungere controllo sulla modalità di apertura del file

        perror("Cannot open directory: file not found");
        return FAT_ERR_GENERIC;
    }

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (disk.open_files[i].is_used == 0)
        {
            disk.open_files[i].is_used = 1;
            disk.open_files[i].cached_entry = found;
            disk.open_files[i].current_offset = 0;
            return i;
        }
    }

    perror("Error: too many open files!");
    return FAT_ERR_GENERIC;
}
int fat_close(int fd)
{
    // printf("[DEBUG] MOCK: fat_close chiamato per chiudere l'fd %d\n", fd);
    // return FAT_SUCCESS;

    if (fd > MAX_OPEN_FILES || fd <= 0)
    {
        perror("Error: file not open");
        return FAT_ERR_GENERIC;
    }

    disk.open_files[fd].is_used = 0;

    disk.open_files[fd].current_offset = 0;

    memset(&disk.open_files[fd].cached_entry, 0, sizeof(FAT_FCB)); // Clearing cached entry

    return FAT_SUCCESS;
}
int fat_read(int fd, void *buf, int size)
{
    printf("[DEBUG] MOCK: fat_read chiamato per l'fd %d, richiedendo %d byte\n", fd, size);

    // Inseriamo una finta stringa di testo nel buffer giusto per testare il comando 'cat'
    if (buf != NULL && size > 0)
    {
        strncpy((char *)buf, "Contenuto fittizio del file lettto con successo!\n", size);
    }

    // Fingiamo di aver letto esattamente il numero di byte richiesti (o almeno un po')
    return size;
}
int fat_write(int fd, const void *buf, int size)
{
    // printf("[DEBUG] MOCK: fat_write chiamato per l'fd %d. Richiesta scrittura di %d byte\n", fd, size);
    //  Fingiamo di aver scritto tutti i byte con successo
    // return size;
}
int fat_lseek(int fd, int offset, int whence)
{
    printf("[DEBUG] MOCK: fat_lseek chiamato per l'fd %d, offset %d, whence %d\n", fd, offset, whence);
    // Ritorniamo il nuovo offset fittizio
    return offset;
}
int fat_mkdir(const char *path)
{
    printf("[DEBUG] MOCK: fat_mkdir chiamato per creare la directory '%s'\n", path);
    return FAT_SUCCESS;
}
int fat_rm(const char *path)
{
    // printf("[DEBUG] MOCK: fat_rm chiamato per eliminare il file/dir '%s'\n", path);
    // return FAT_SUCCESS;
    FAT_FCB out;

    if (find_DirectoryEntry(path, &out) == FAT_ERR_GENERIC)
    {
        perror("Error: file or directory with input path not found");
        return FAT_ERR_GENERIC;
    }

    if (chain_rm(out.first_cluster) == FAT_ERR_GENERIC)
    {
        perror("Error while trying to remove related clusters");
        return FAT_ERR_GENERIC;
    }
}