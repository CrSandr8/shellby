#include "fat.h"

//============================================================================//
//==================================== FS API ================================//
//============================================================================//

int fat_format(const char *filename, int size)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0666);

    if (fd == -1){
        perror("Error while opening file");
        return FAT_ERR_GENERIC;
    }

    if (ftruncate(fd, size) == -1){
        perror("Error while truncating file");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    void *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (base == MAP_FAILED){
        perror("Error while mapping file");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    memset(base, 0, size);

    FAT_Superblock *sb = (FAT_Superblock *)base;

    sb->Signature = BS_SIGNATURE;
    sb->BytsPerSec = SECTOR_SIZE;
    sb->FATSz = sizeof(disk->fat)*sizeof(uint32_t);
    sb->RootSec = sizeof(FAT_Superblock) + sb->FATSz;
    sb->FSI_Free_Count = 0;
    sb->FSI_Nxt_Free = 0;

    uint32_t *fat = (uint32_t *)(base + sizeof(FAT_Superblock));
    uint8_t *data = (uint8_t *)(base + sizeof(FAT_Superblock) + sb->FATSz);

    printf("Successfully formatted disk %s with size %d\n", filename, size);

    close(fd);

    munmap(base, size);

    return FAT_SUCCESS;
}

int fat_mount(const char *disk_path)
{
    printf("Starting mount operations...\n");
    int fd;
    if ((fd = open(disk_path, O_RDWR)) == -1)
    {
        perror("Error in opening file during mount...");
        return FAT_ERR_GENERIC;
    }

    disk->disk_base = mmap(NULL, disk->disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk->disk_base == MAP_FAILED)
    {
        perror("Error while mapping...");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    printf("Checking disk signature...\n");

    disk->sb = (FAT_Superblock *)disk->disk_base;

    if (disk->sb->Signature != BS_SIGNATURE)
    {
        munmap(disk->disk_base, disk->disk_size);
        disk->disk_base = NULL;
        close(fd);
        return FAT_ERR_BAD_SIG;
    }

    // We now may be using a rightful disk file

    printf("Signature OK!\n");

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        disk->open_files[i].is_used = 0;
        disk->open_files[i].current_offset = 0;
    }

    close(fd);

    disk->fat = (uint32_t *)(disk->disk_base + sizeof(FAT_Superblock));
    disk->data = (uint8_t *)(disk->disk_base + sizeof(FAT_Superblock) + disk->sb->FATSz);

    disk->cwd = (FAT_FCB *) disk->data;

    printf("Correctly mounted disk\n");

    return FAT_SUCCESS;
}

int fat_unmount(void)
{

    printf("Starting unmount process...\n");
    if (disk->disk_base == NULL)
        return;

    if (msync(disk->disk_base, disk->disk_size, MS_SYNC) == -1)
    {
        perror("Error while saving data on disk");
        return FAT_ERR_GENERIC;
    }

    if (munmap(disk->disk_base, disk->disk_size) == -1)
    {
        perror("Error while unmapping disk");
        return FAT_ERR_GENERIC;
    }

    memset(&disk, 0, sizeof(FAT_Disk));
    printf("Unmounting OK!\n");

    return FAT_SUCCESS;
}


int fat_rmdir(const char *path)
{

    FAT_FCB *found = find_in_dir(path);
}

//==================================== Helper functions from here ================================//

int tokenize_path(char *buf, char *tokens[], int max_tokens) // TODO check the max tokens control
{
    char *token = strtok(buf, "/");

    int i = 0;

    while (token != NULL && i < max_tokens - 1)
    {
        tokens[i] = token;

        token = strtok(NULL, "/"); // Here we go on to the next word by using the weird effect of passing NULL as an arg for strtok gg
        i++;
    }

    tokens[i] = NULL; // We use NULL in the token list to specify it is terminated, like the "\0" for the strings gg

    return i;
}

FAT_FCB *find_in_dir(const char *target)
{}

int delete_fcb(const char *target, uint32_t sector)
{}


int get_sector_number() // Get the total number of sectors in this disk
{}

uint32_t get_root_sector() // Get the sector offset of the root directory
{}

uint32_t get_next_sector(uint32_t sector)
{
    return disk->fat[sector];
}

uint32_t find_free_sector()
{
    uint32_t start = disk->sb->FSI_Nxt_Free;

    // Start from nxtfree, we just iterate untile we find
    uint32_t i;
    for (i = start; i < get_sector_number; i++)
    {
        if (get_next_sector(i) == FAT_FREE)
        {
            disk->sb->FSI_Nxt_Free = i + 1;
            disk->sb->FSI_Free_Count--;
            return i;
        }
    }

    // If we are here we didn't find anything yet, the hint was incorrect
    for (i = get_root_sector(); i < start; i++)
    {
        if (get_next_sector(i) == FAT_FREE)
        {
            disk->sb->FSI_Nxt_Free = i + 1;
            disk->sb->FSI_Free_Count--;
            return i;
        }
    }

    // DISK FULL
    return FAT_ERR_DISK_FULL;
}

//================================================================
//=============================== Sector chains routine operations
//================================================================

int chain_append(uint32_t a, uint32_t b)
{
    while (1)
    {
        uint32_t next = get_next_sector(a); // Lookup next sector
        if (next == FAT_EOC)
        { // It is the last of the chain
            disk->fat[a] = b;
            disk->fat[b] = FAT_EOC; // We append b
            break;
        }

        // If we are here we have to check if the next sector is the one that has a "EOC next"
        a = next;
    }

    return FAT_SUCCESS;
}

int chain_rm(uint32_t first_sector)
{
    uint32_t current = first_sector;

    if (current == FAT_FREE) // If sector is already labeled as free
        return FAT_SUCCESS;

    while (1)
    {
        uint32_t next = get_next_sector(current); // We save where is the next sector of the chain
        disk->fat[current] = FAT_FREE;             // And free the current one

        if (next == FAT_EOC || next == FAT_FREE) // Check out the next one, it might be the the end of the chain
        {
            break; //
        }

        current = next; // If that is not the case we go on with the cycle
    }

    return FAT_SUCCESS;
}

// The idea here is we navigate the chain until the wanted size of the cut leads to the EOC. At that point we remove the chain with the current sector as the starting one using chain_rm
int chain_cut(uint32_t first_sector, int size)
{
    int i;
    uint32_t current = first_sector;
    for (i = 0; i < size; i++)
    {
        uint32_t next = disk->fat[current];
        current = next;
    }

    uint32_t to_delete = get_next_sector(current);
    disk->fat[current] = FAT_EOC;
    return chain_rm(to_delete);
}