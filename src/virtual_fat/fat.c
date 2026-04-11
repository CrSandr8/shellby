#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"

FAT_Disk *disk = NULL;

//============================================================================//
//============================== Disk Manipulation ===========================//
//============================================================================//

int fat_create_disk(const char *filename, int size)
{
    int fd = open(filename, O_RDWR | O_CREAT, 0666);

    if (fd == -1)
    {
        perror("Error while creating file");
        return FAT_ERR_GENERIC;
    }

    int num_sectors = get_num_sectors(size);
    int fat_size = get_fat_size(num_sectors);
    int disk_size = get_total_disk_size(fat_size, size);

    if (ftruncate(fd, disk_size) == -1)
    {
        perror("Error while truncating created file");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    void *base = mmap(NULL, disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (base == MAP_FAILED)
    {
        perror("Error while mapping created file");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    memset(base, 0, disk_size);

    FAT_Superblock *sb = (FAT_Superblock *)base;

    sb->Signature = BS_SIGNATURE;
    sb->BytsPerSec = SECTOR_SIZE;
    sb->FATSz = fat_size;
    sb->RootSec = sizeof(FAT_Superblock) + sb->FATSz;
    sb->FSI_Free_Count = num_sectors;
    sb->FSI_Nxt_Free = 1; // We put the root at index 0

    uint32_t *fat = (uint32_t *)((uint8_t *)base + sizeof(FAT_Superblock));
    uint8_t *data = (uint8_t *)((uint8_t *)base + sizeof(FAT_Superblock) + sb->FATSz);

    fat[0] = FAT_EOC;
    sb->FSI_Free_Count--;

    printf("Successfully formatted disk %s with size %d\n", filename, disk_size);

    close(fd);

    munmap(base, disk_size);

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

    struct stat st;

    if (fstat(fd, &st) == -1)
    {
        perror("Error getting file size");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    disk->disk_size = st.st_size;

    disk->disk_base = mmap(NULL, disk->disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk->disk_base == MAP_FAILED)
    {
        perror("Error while mapping...");
        close(fd);
        return FAT_ERR_GENERIC;
    }

    printf("Checking disk signature...\n");

    FAT_Superblock *sb = (FAT_Superblock *)disk->disk_base;

    if (sb->Signature != BS_SIGNATURE)
    {
        perror("Error, file has bad signature, aborting...");
        munmap(disk->disk_base, disk->disk_size);
        close(fd);
        return FAT_ERR_BAD_SIG;
    }

    // We now may be using a rightful disk file
    printf("Signature OK!\n");


    disk->sb = sb;
    disk->fat = (uint32_t *)((uint8_t *)disk->disk_base + sizeof(FAT_Superblock));
    disk->data = (uint8_t *)((uint8_t *)disk->disk_base + sizeof(FAT_Superblock) + disk->sb->FATSz);

    disk->cwd_sector = 0;
    strcpy(disk->cwd_path, "/");

    // for (int i = 0; i < MAX_OPEN_FILES; i++)
    //{
    //     disk->open_files[i].is_used = 0;
    //     disk->open_files[i].current_offset = 0;
    // }
    
    printf("[DEBUG] Disk mapped at address: %p\n", disk->disk_base);
    printf("[DEBUG] FAT region starts at: %p\n", disk->fat);
    printf("[DEBUG] Data region starts at: %p (Sector 0)\n", disk->data);


    close(fd);

    printf("Correctly mounted disk\n");

    return FAT_SUCCESS;
}

int fat_unmount(void)
{

    printf("Starting unmount process...\n");
    if (disk->disk_base == NULL)
        return FAT_SUCCESS;

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

    memset(disk, 0, sizeof(FAT_Disk));
    printf("Unmounting OK!\n");

    return FAT_SUCCESS;
}

//============================================================================//
//================================ FS API ====================================//
//============================================================================//

int fat_createdir(const char *name)
{
    if (find_in_dir(name, disk->cwd_sector) != NULL)
    {
        fprintf(stderr, "Directory already exists");
        return FAT_ERR_GENERIC;
    }

    FAT_FCB *new = find_free_slot(disk->cwd_sector);

    if (new == NULL)
    {
        //perror("Directory is full");
        //return FAT_ERR_GENERIC;
        uint32_t extended_sector = get_free_sector();
        chain_append(disk->cwd_sector, extended_sector);
        FAT_FCB *extended_sector_ptr = get_entries(extended_sector);
        memset(extended_sector_ptr, 0, SECTOR_SIZE);
        new = &extended_sector_ptr[0];
    }

    uint32_t new_first_sector = get_free_sector();
    if (new_first_sector == FAT_ERR_DISK_FULL)
    {
        fprintf(stderr, "No more disk space for directory");
        return FAT_ERR_DISK_FULL;

    }

    strcpy(new->name, name);
    new->is_dir = 1;
    new->file_size = 0;
    disk->fat[new_first_sector] = FAT_EOC;
    new->first_sector = new_first_sector;

    FAT_FCB *entries = get_entries(new_first_sector);

    memset(entries, 0, SECTOR_SIZE);

    strcpy((char *)entries[0].name, ".");
    entries[0].is_dir = 1;
    entries[0].first_sector = new_first_sector; // Points to itself

    strcpy((char *)entries[1].name, "..");
    entries[1].is_dir = 1;
    entries[1].first_sector = disk->cwd_sector; // Points to the directory in which we are creating the new one, basically the parent directory

    return FAT_SUCCESS;

}

int fat_readdir(uint32_t dir_sector) 
{
    // Checking if input sector is valid
    if (dir_sector == FAT_EOC) {
        return FAT_ERR_GENERIC; 
    }

    uint32_t cursor = 0;
    FAT_FCB *current_file;
    uint32_t file_count = 0;

    printf("\nNAME                 SIZE\n");
    printf("-------------------------------\n");

    // We cycle from here
    while ((current_file = read_dir_next(dir_sector, &cursor)) != NULL) {
        printf("%-20s %8u B\n", current_file->name, current_file->file_size);
        file_count++;
    }

    printf("-------------------------------\n");
    printf("Total: %u elements.\n\n", file_count);

    return FAT_SUCCESS; 
}

int fat_change_dir(const char *path)
{
    uint32_t target = fat_resolve_path(path);

    if (target == FAT_ERR_GENERIC)
    {
        fprintf(stderr, "Error: cannot resolve path\n");
        return FAT_ERR_GENERIC;
    }

    if (update_cwd_path(path) != FAT_SUCCESS)
    {
        fprintf(stderr, "Something went wrong while changing current path in shell\n");
        return FAT_ERR_GENERIC;
    }

    disk->cwd_sector = target;

    return FAT_SUCCESS;
}

int fat_createfile(const char *filename)
{
    if (find_in_dir(filename, disk->cwd_sector) != NULL)
    {
        fprintf(stderr, "File already exists\n");
        return FAT_ERR_GENERIC;
    }

    FAT_FCB *new = find_free_slot(disk->cwd_sector);

    if (new == NULL)
    {
        uint32_t extended_sector = get_free_sector();
        if (extended_sector == FAT_ERR_DISK_FULL) {
            fprintf(stderr, "No more disk space to expand directory\n");
            return FAT_ERR_DISK_FULL;
        }

        chain_append(disk->cwd_sector, extended_sector);
        FAT_FCB *extended_sector_ptr = get_entries(extended_sector);
        memset(extended_sector_ptr, 0, SECTOR_SIZE);
        
        new = &extended_sector_ptr[0];
    }

    uint32_t new_first_sector = get_free_sector();
    if (new_first_sector == FAT_ERR_DISK_FULL)
    {
        fprintf(stderr, "No more disk space for file\n");
        return FAT_ERR_DISK_FULL;
    }

    strcpy(new->name, filename);
    new->is_dir = 0;
    new->file_size = 0;
    disk->fat[new_first_sector] = FAT_EOC;
    new->first_sector = new_first_sector;

    return FAT_SUCCESS;
}

int fat_readfile(const char *filename)
{
    FAT_FCB *this = find_in_dir(filename, disk->cwd_sector);

    if (this == NULL || this->is_dir == 1){
        fprintf(stderr, "The file has not been found or it is a directory\n");
        return FAT_ERR_GENERIC;
    }

    uint32_t current = this->first_sector;
    uint32_t to_read = this->file_size; //This is the bytes left to read

    while(current != FAT_EOC && to_read > 0){
        uint32_t to_read_in_sector = to_read > SECTOR_SIZE ? SECTOR_SIZE : to_read;

        //
        fwrite((char *)get_entries(current), 1, to_read_in_sector, stdout);

        to_read -= to_read_in_sector;
        current = get_next_sector(current);

    }

    printf("\n");
    return FAT_SUCCESS;
    
}

int fat_writefile(const char *filename, const char *text, int append)
{
    if (append > 1 || append < 0){
        fprintf(stderr, "Invalid append flag\n");
        return FAT_ERR_GENERIC;
    }

    //Checking if the file exists
    FAT_FCB *this = find_in_dir(filename, disk->cwd_sector);
    if (this == NULL){
        fprintf(stderr, "File not found");
        return FAT_ERR_GENERIC;
    }

    uint32_t sector_start = this->first_sector; //The sector we start from
    uint32_t offset_start = 0; //The offset we start from in sector_start

    if (append == 0){

        //Cleanup the file
        this->file_size = 0;
        uint32_t this_first_sector_copy = disk->fat[this->first_sector];
        disk->fat[this->first_sector] = FAT_EOC;
        chain_rm(this_first_sector_copy);

        offset_start = 0;

    }
    else{

        //We are in append mode
        while(get_next_sector(sector_start) != FAT_EOC){
            sector_start = get_next_sector(sector_start);
            
        }

        offset_start = this->file_size % SECTOR_SIZE;
    }

    uint32_t to_write = strlen(text);   

    while (to_write > 0){
        //This is how many bytes we can write in the sector we are in right now
        uint32_t writable = to_write > (SECTOR_SIZE - offset_start) ? SECTOR_SIZE - offset_start : to_write;

        //strncpy((char *)get_entries(this->first_sector), text, writable);

        //This is the actual writing moment
        memcpy((char *)get_entries(sector_start) + offset_start, text, writable);

        //Here we move the pointer forward
        text += writable;

        //Update the remaining bytes to write
        to_write -= writable;

        //And the file size
        this->file_size += writable;

        if(to_write > 0){
            printf("Writing %u bytes to sector %u (offset %u)...\n", writable, sector_start, offset_start);
            uint32_t new_sector = get_free_sector();

            if (new_sector == FAT_ERR_DISK_FULL){
                fprintf(stderr, "No more disk space for file");
                return FAT_ERR_DISK_FULL;
            }

            chain_append(sector_start, new_sector);
            sector_start = new_sector;

            //Reset the current offset in the sector
            offset_start = 0;
            
        }
    }

    return FAT_SUCCESS;

}

int fat_rm(const char *filename)
{
    // Placeholder for testing purposes
    printf("[DEBUG] fat_rmfile called for filename: '%s'\n", filename);

    /* TODO: 
       1. Find the file in the current directory.
       2. Free the FAT chain using chain_rm.
       3. Mark the FCB as deleted (name[0] = '\0').
    */

    FAT_FCB *found = find_in_dir(filename, disk->cwd_sector);

    if (found == NULL){
        fprintf(stderr, "Error: file not found\n");
        return FAT_ERR_GENERIC;
    }

    //if(found->is_dir == 1){
    //    if (get_entries(found))
    //}

    found->name[0] = '\0';
    found->file_size = 0;

    if (chain_rm(found->first_sector) == FAT_ERR_GENERIC){
        fprintf(stderr, "Error while removing this file sector chain\n");
        return FAT_ERR_GENERIC;
    }

    return FAT_SUCCESS;
}

//============================================================================//
//============================= FCB Routines =================================//
//============================================================================//


FAT_FCB *find_in_dir(const char *name, uint32_t sector) // Look for a file named name in the current dir
{
    uint32_t current = sector;

    while (current != FAT_EOC)
    {
        FAT_FCB *entries = get_entries(current);

        // We cycle in this sector
        for (int i = 0; i < ENTRIES_PER_SEC; i++)
        {
            //
            if (entries[i].name[0] != '\0' && strcmp((char *)entries[i].name, name) == 0)
            {
                return &entries[i]; // Found it!
            }
        }

        // Go on with the chain
        current = disk->fat[current];
    }

    // If we arrive here nothing has been found
    return NULL;
}

FAT_FCB *find_free_slot(uint32_t sector) // Starting from the current location, look for empty space in sectors
{
    uint32_t current = sector;

    while (current != FAT_EOC)
    {
        FAT_FCB *entries = get_entries(current);

        for (int i = 0; i < ENTRIES_PER_SEC; i++)
        {
            if (entries[i].name[0] == '\0')
            {
                return &entries[i];
            }
        }

        current = disk->fat[current];
    }

    return NULL;
}

FAT_FCB *read_dir_next(uint32_t dir_sector, uint32_t *cursor)
{
    while(1){

        uint32_t sectors_to_skip = *cursor / ENTRIES_PER_SEC;
        
        uint32_t in_sector_offset = *cursor % ENTRIES_PER_SEC;

        uint32_t current = dir_sector;

        for (uint32_t i = 0; i < sectors_to_skip; i++){
            current = get_next_sector(current);

            if (current == FAT_EOC){
                return NULL;
            }
        
        }

        FAT_FCB *entries = get_entries(current);

        FAT_FCB *result = &entries[in_sector_offset];

        if (result->name[0] == '\0'){

            //printf("[DEBUG] Directory scan: found '%s' at cursor %u\n", result->name, *cursor);
            (*cursor)++;
            continue;
        }

        (*cursor)++;

        return result;
    }
}

//============================================================================//
//============================ Sector routines ===============================//
//============================================================================//

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
        disk->fat[current] = FAT_FREE;            // And free the current one
        disk->sb->FSI_Free_Count++;

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
    for (i = 0; i < size - 1; i++)
    {
        uint32_t next = disk->fat[current];
        if (next == FAT_EOC)
            break;
        current = next;
    }

    uint32_t to_delete = disk->fat[current];

    if (to_delete != FAT_EOC && to_delete != FAT_FREE)
    {
        disk->fat[current] = FAT_EOC;
        return chain_rm(to_delete);
    }

    return FAT_SUCCESS;
}

uint32_t get_free_sector()
{
    if (disk->sb->FSI_Free_Count == 0)
        return FAT_ERR_DISK_FULL;

    uint32_t start = disk->sb->FSI_Nxt_Free;

    // Start from nxtfree, we just iterate untile we find
    uint32_t i;                                        
    uint32_t max = disk->sb->FATSz / sizeof(uint32_t);

    for (i = start; i < max; i++)
    {
        if (disk->fat[i] == FAT_FREE)
        {
            printf("[DEBUG] Allocated free sector: %u\n", i);
            disk->sb->FSI_Nxt_Free = i + 1;
            return i;
        }
    }

    // If we are here we didn't find anything yet, the hint was incorrect
    for (i = 0; i < start; i++)
    {
        if (disk->fat[i] == FAT_FREE)
        {
            disk->sb->FSI_Nxt_Free = i + 1;
            return i;
        }
    }

    // DISK FULL
    return FAT_ERR_DISK_FULL;

}

//============================================================================//
//================================== IDK =====================================//
//============================================================================//

int get_num_sectors(int data_size)
{
    return data_size / SECTOR_SIZE;

}

int get_fat_size(int num_sectors)
{
    return num_sectors * sizeof(uint32_t);

}

int get_total_disk_size(int fat_size, int data_size)
{
    return fat_size + data_size + sizeof(FAT_Superblock);

}

//============================================================================//
//========================= Path related string management ===================//
//============================================================================//
uint32_t fat_resolve_path(const char *path)
{
    // If given path is empty we remain where we are
    if (path == NULL || strlen(path) == 0)
        return disk->cwd_sector;

    // We save a copy of the string since strtok modifies it
    char path_copy[256];
    strncpy(path_copy, path, 256);

    // The starting point
    uint32_t current_sector = disk->cwd_sector;
    if (path_copy[0] == '/')
    {
        current_sector = 0; // This means we have an absolute path so we start from root
    }

    // We break the path into a token list. Every token in order is the directory where we have to move first
    char *token = strtok(path_copy, "/");

    while (token != NULL)
    {
        printf("[DEBUG] Resolving token: '%s' in sector %u...\n", token, current_sector);
        //
        FAT_FCB *found = find_in_dir(token, current_sector);

        if (found == NULL)
        {
            fprintf(stderr, "Folder not found");
            return FAT_ERR_GENERIC;
        }

        // Checking if it actually is a directory and not a file
        if (found->is_dir == 0)
        {
            return FAT_ERR_GENERIC;
        }

        // We go on to the next sector
        current_sector = found->first_sector;

        token = strtok(NULL, "/"); // Remember that strtok ""saves its state""
    }

    return current_sector;

}

int update_cwd_path(const char *path)
{
    if (path == NULL)
        return FAT_ERR_GENERIC;

    char temp[256];
    
    // If the path is absolute, start from the root.
    // Otherwise, start from the current working directory string.
    if (path[0] == '/') {
        strcpy(temp, "/");
    } else {
        strcpy(temp, disk->cwd_path);
    }

    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';

    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        if (strcmp(token, ".") == 0) {
            // Current directory reference: do nothing.
        } 
        else if (strcmp(token, "..") == 0) {
            // Parent directory reference: move one level up.
            if (strcmp(temp, "/") != 0) {
                // Locate the last slash to truncate the path string.
                char *last_slash = strrchr(temp, '/');
                if (last_slash == temp) {
                    // We are just one level below root (e.g., "/dir").
                    // Truncate right after the leading slash.
                    *(last_slash + 1) = '\0';
                } else {
                    // Truncate at the slash location.
                    *last_slash = '\0';
                }
            }
        } 
        else {
            // Directory name: append it to the path string.
            int len = strlen(temp);
            
            // Add a separator slash if we are not at the root "/".
            if (temp[len - 1] != '/') {
                strcat(temp, "/");
            }
            
            strcat(temp, token);
        }

        token = strtok(NULL, "/");
    }

    // Update the actual CWD path in the disk structure.
    strncpy(disk->cwd_path, temp, 255);
    disk->cwd_path[255] = '\0';

    return FAT_SUCCESS;
}
