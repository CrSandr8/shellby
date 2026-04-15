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

// Creates a file named filename with size size and formats it to be utilized as a shellby FAT disk.
int fat_create_disk(const char *filename, int size)
{
    char final_name[256];
    int len = strlen(filename);

    if (len > 3 && strcmp(filename + len - 4, ".bin") == 0) strncpy(final_name, filename, 255);
    else snprintf(final_name, 256, "%s.bin", filename);

    int fd = open(final_name, O_RDWR | O_CREAT, 0666);

    if (fd == -1)
    {
        perror("Error while creating file");
        return FAT_ERR_GENERIC;
    }

    if (size < MIN_DISK_SIZE)
    {
        perror("Too small disk size. Please choose a bigger size");
        return FAT_ERR_GENERIC;
    }

    if (size > MAX_DISK_SIZE)
    {
        perror("Disk size is too big. Please choose a smaller size");
        return FAT_ERR_GENERIC;
    }

    //These are info needed for metadata in the superblock but depend on the actual size given
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

    //We set every single byte to 0 in case somehow it contained trash somewhere
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

    fat[0] = FAT_EOC; //The root FAT entry
    sb->FSI_Free_Count--;

    printf("Successfully formatted disk %s with size %d\n", final_name, disk_size);

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

    if (st.st_size < MIN_DISK_SIZE || st.st_size > MAX_DISK_SIZE)
    {
        perror("Error: tried to mount a disk of inappropriate size");
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

    strncpy((char *)disk->disk_name, disk_path, 63); //Insert the name in the runtime struct
    disk->disk_name[63] = '\0';

    // We now may be using a rightful disk file
    printf("Signature OK!\n");

    // At the top of the file the first thing we encounter is the superblock
    disk->sb = sb;
    // The FAT itself is located just next the superblock
    disk->fat = (uint32_t *)((uint8_t *)disk->disk_base + sizeof(FAT_Superblock));
    // The data region starts where the FAT ends
    disk->data = (uint8_t *)((uint8_t *)disk->disk_base + sizeof(FAT_Superblock) + disk->sb->FATSz);

    // Init the current working directory sector to the first of the data_region
    disk->cwd_sector = 0;
    strcpy(disk->cwd_path, "/");

    printf("Disk mapped at address: %p\n", disk->disk_base);
    printf("FAT region starts at: %p\n", disk->fat);
    printf("Data region starts at: %p (Sector 0)\n", disk->data);

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

    // We want to avoid that leftovers from our mapping cause strange behaviour with its old mapped addresses
    memset(disk, 0, sizeof(FAT_Disk));
    printf("Unmounting OK!\n");

    return FAT_SUCCESS;
}

//============================================================================//
//================================ FS API ====================================//
//============================================================================//

int fat_createdir(const char *name)
{
    if (strlen(name) > 15){
        fprintf(stderr, "Error: directory name too long\n");
        return FAT_ERR_GENERIC;
    }

    if(strchr(name, '.')){
        fprintf(stderr, "Error: directory names must not contain dots\n");
        return FAT_ERR_GENERIC;
    }

    if (find_in_dir(name, disk->cwd_sector) != NULL)
    {
        fprintf(stderr, "Directory already exists\n");
        return FAT_ERR_GENERIC;
    }

    // Locating a free slot in this sector
    FAT_FCB *new = find_free_entry(disk->cwd_sector);

    if (new == NULL)
    {
        // We search for a free sector and append it to the current chain
        uint32_t extended_sector = get_free_sector();

        if (extended_sector == FAT_ERR_DISK_FULL)
        {
            fprintf(stderr, "No more disk space for directory\n");
            return FAT_ERR_DISK_FULL;
        }

        chain_append(disk->cwd_sector, extended_sector);

        new = get_entries(extended_sector);

        memset(new, 0, SECTOR_SIZE);
    }

    uint32_t new_first_sector = get_free_sector();
    if (new_first_sector == FAT_ERR_DISK_FULL)
    {
        fprintf(stderr, "No more disk space for directory\n");
        return FAT_ERR_DISK_FULL;
    }

    // The directory FCB init
    strncpy(new->name, name, 15);
    new->name[15] = '\0';
    new->ext[0] = '\0';
    new->is_dir = 1;
    new->file_size = 0;
    disk->fat[new_first_sector] = FAT_EOC;
    new->first_sector = new_first_sector;

    FAT_FCB *entries = get_entries(new_first_sector);

    // Security cleanup before adding the must have two entries
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
    if (dir_sector == FAT_EOC)
    {
        return FAT_ERR_GENERIC;
    }

    uint32_t entry_index = 0;
    FAT_FCB *current_file;
    uint32_t file_count = 0;

    printf("\n%-15s %-5s %10s\n", "NAME", "EXT", "SIZE");
    printf("----------------------------------------------\n");

    // We cycle from here
    while ((current_file = read_dir_next(dir_sector, &entry_index)) != NULL)
    {
        // NOTE These lines of code have the only utility of making the ls look a little bit better
        if (current_file->is_dir) {

            if (strcmp(current_file->name, ".") == 0 || strcmp(current_file->name, "..") == 0){

                printf("%-15s            \n", current_file->name);
            }
            else{

                printf("%-15s %-5s %10u B\n", current_file->name, "", current_file->file_size);
            }
        }

        else {

            printf("%-15s %-5s %10u B\n", current_file->name, current_file->ext, current_file->file_size);
        }
        // This is by far more important
        file_count++;
    }

    printf("----------------------------------------------\n");
    printf("Total: %u elements.\n\n", file_count);

    return FAT_SUCCESS;
}



// This is basically the cd command
int fat_change_dir(const char *path)
{
    uint32_t target = fat_resolve_path(path); // Locate the destination sector

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

    // Just like createdir we try to find a slot in the current sector and if we don't, we lookup and append
    FAT_FCB *new = find_free_entry(disk->cwd_sector);

    if (new == NULL)
    {
        uint32_t extended_sector = get_free_sector();
        if (extended_sector == FAT_ERR_DISK_FULL)
        {
            fprintf(stderr, "No more disk space to expand directory\n");
            return FAT_ERR_DISK_FULL;
        }

        chain_append(disk->cwd_sector, extended_sector);
        FAT_FCB *extended_sector_ptr = get_entries(extended_sector);
        memset(extended_sector_ptr, 0, SECTOR_SIZE);

        new = &extended_sector_ptr[0];
    }

    char name[16];
    char ext[7];

    if (parse_filename(filename, name, ext) == FAT_ERR_GENERIC){
        fprintf(stderr, "Error while parsing file name or extension\n");
        return FAT_ERR_GENERIC;
    }

    // File FCB init
    strcpy(new->name, name);
    strcpy(new->ext, ext);
    new->is_dir = 0;
    new->file_size = 0;
    new->first_sector = FAT_EOC;

    return FAT_SUCCESS;
}



// This is basically the implementation of the ls command
int fat_readfile(const char *filename)
{
    FAT_FCB *this = find_in_dir(filename, disk->cwd_sector);

    if (this == NULL || this->is_dir == 1)
    {
        fprintf(stderr, "The file has not been found or it is a directory\n");
        return FAT_ERR_GENERIC;
    }

    uint32_t current = this->first_sector;
    uint32_t to_read = this->file_size; // This is the bytes left to read

    while (current != FAT_EOC && to_read > 0)
    {
        uint32_t to_read_in_sector = to_read > SECTOR_SIZE ? SECTOR_SIZE : to_read;

        // We choose to use fwrite to let shellby read any type of file, not only text files, for the future
        fwrite((char *)get_entries(current), 1, to_read_in_sector, stdout);

        to_read -= to_read_in_sector;

        current = get_next_sector(current);
    }

    printf("\n");
    return FAT_SUCCESS;
}



// Write the text we get in input as a string to the file filename. The append parameter is a flag. If set to 0 we are writing in the file from the start, deleting everything it contained before. If set to 1 we append to the end of the current content.
// We allocate sectors to the file only when we start writing on it. Before that if a file is blank it lives in the filesystem only as an FCB in the parent directory.
int fat_writefile(const char *filename, const char *text, int append)
{
    if (append > 1 || append < 0)
    {
        fprintf(stderr, "Invalid append flag\n");
        return FAT_ERR_GENERIC;
    }

    // Checking if the file exists
    FAT_FCB *this = find_in_dir(filename, disk->cwd_sector);
    if (this == NULL)
    {
        fprintf(stderr, "File not found\n");
        return FAT_ERR_GENERIC;
    }

    if (this->first_sector == FAT_EOC){
        uint32_t new_sector = get_free_sector();
        if (new_sector == FAT_ERR_DISK_FULL){
            fprintf(stderr, "Error: no more disk space to expand file\n");
            return FAT_ERR_DISK_FULL;
        }
        this->first_sector = new_sector;
        disk->fat[new_sector] = FAT_EOC;
    }

    uint32_t new_text_len = strlen(text);
    int size_delta;

    uint32_t sector_start = this->first_sector; // The sector we start from
    uint32_t offset_start = 0;                  // The offset we start from in sector_start

    if (append == 0)
    {
        size_delta = (int)new_text_len - (int)this->file_size;

        // Cleanup the file
        this->file_size = 0;
        uint32_t this_first_sector_copy = disk->fat[this->first_sector];
        disk->fat[this->first_sector] = FAT_EOC;
        chain_rm(this_first_sector_copy);

        offset_start = 0;
    }
    else
    {
        size_delta = (int)new_text_len;

        // We are in append mode
        while (get_next_sector(sector_start) != FAT_EOC)
        {
            sector_start = get_next_sector(sector_start);
        }

        offset_start = this->file_size % SECTOR_SIZE;
    }

    uint32_t to_write = strlen(text);

    while (to_write > 0)
    {
        // This is how many bytes we can write in the sector we are in right now
        uint32_t writable = to_write > (SECTOR_SIZE - offset_start) ? SECTOR_SIZE - offset_start : to_write;

        // strncpy((char *)get_entries(this->first_sector), text, writable);

        // This is the actual writing moment
        memcpy((char *)get_entries(sector_start) + offset_start, text, writable);

        // Here we move the pointer forward
        text += writable;

        // Update the remaining bytes to write
        to_write -= writable;

        // And the file size
        this->file_size += writable;

        if (to_write > 0)
        {
            printf("Writing %u bytes to sector %u (offset %u)...\n", writable, sector_start, offset_start);
            uint32_t new_sector = get_free_sector();

            if (new_sector == FAT_ERR_DISK_FULL)
            {
                fprintf(stderr, "No more disk space for file\n");
                return FAT_ERR_DISK_FULL;
            }

            chain_append(sector_start, new_sector);
            sector_start = new_sector;

            // Reset the current offset in the sector
            offset_start = 0;
        }
    }

    update_parent_size(disk->cwd_sector, size_delta);
    return FAT_SUCCESS;
}



// Delete the file or directory named filename. If it is a directory, must be empty or this command has to be set to recursive remove mode with its flag_recursive set to 1.
int fat_rm(const char *filename, int flag_recursive)
{

    FAT_FCB *found = find_in_dir(filename, disk->cwd_sector);

    if (found == NULL)
    {
        fprintf(stderr, "Error: file or directory not found\n");
        return FAT_ERR_GENERIC;
    }

    if ((strcmp(found->name, ".") == 0)||(strcmp(found->name, "..") == 0))
    {
        fprintf(stderr, "Error: tried to remove this folder or its parent, which is not allowed\n");
        return FAT_ERR_GENERIC;
    }

    if (found->is_dir == 1)
    {
        if (flag_recursive == 1){
            rm_recursive(found->first_sector);
        }
        else{
            uint32_t current_entry_index = 0;
            uint32_t current = found->first_sector;
            FAT_FCB *temp;
            while((temp = read_dir_next(current, &current_entry_index)) != NULL){
                if ((strcmp(temp->name, ".") != 0) && (strcmp(temp->name, "..") != 0))
                {
                    fprintf(stderr, "Error: tried to remove a folder that is not empty\n");
                    return FAT_ERR_GENERIC;
                }

            }
        }
    }

    update_parent_size(disk->cwd_sector, -(int)found->file_size);

    // It is a file
    found->name[0] = '\0';
    found->file_size = 0;

    if (chain_rm(found->first_sector) == FAT_ERR_GENERIC)
    {
        fprintf(stderr, "Error while removing the file's sector chain\n");
        return FAT_ERR_GENERIC;
    }

    return FAT_SUCCESS;
}

//============================================================================//
//============================= FCB Routines =================================//
//============================================================================//

// Look for a file named name in the current dir. In input we also take the sector that is the starting point of our search
FAT_FCB *find_in_dir(const char *filename, uint32_t sector)
{
    char name[16];
    char ext[7];

    if (parse_filename(filename, name, ext) == FAT_ERR_GENERIC){
        fprintf(stderr, "Error while parsing filename in find_in_dir\n");
        return NULL;
    }

    uint32_t current = sector;

    while (current != FAT_EOC)
    {
        FAT_FCB *entries = get_entries(current);

        // We cycle in this sector
        for (int i = 0; i < ENTRIES_PER_SEC; i++)
        {
            //
            if (entries[i].name[0] != '\0' && strcmp((char *)entries[i].name, name) == 0 && strcmp((char *)entries[i].ext, ext) == 0)
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



// Starting from the current location, look for empty space in chains of sectors. This function uses the FAT to get to the next sector of the chain
FAT_FCB *find_free_entry(uint32_t sector)
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



//Since every sector has the same capacity of FCBs, we use this function to iterate in a single sector, starting from the index specified in the entry_index variable. In the exact moment we find and entry with a name that is not empty, we return its FCB
FAT_FCB *read_dir_next(uint32_t dir_sector, uint32_t *entry_index)
{
    while (1)
    {

        uint32_t sectors_to_skip = *entry_index / ENTRIES_PER_SEC; //How many sector we have to skip before we get to the one in which the cursor is

        uint32_t in_sector_offset = *entry_index % ENTRIES_PER_SEC; //Where is the cursor placed in the sector we start from

        uint32_t current = dir_sector;

        for (uint32_t i = 0; i < sectors_to_skip; i++)
        {
            current = get_next_sector(current); //Lookup

            if (current == FAT_EOC) //We arrived at the end of the chain, so we didn't find anything
            {
                return NULL;
            }
        }

        FAT_FCB *entries = get_entries(current);

        FAT_FCB *result = &entries[in_sector_offset];

        if (result->name[0] == '\0') //The slot is free
        {

            // printf("[DEBUG] Directory scan: found '%s' at cursor %u\n", result->name, *cursor);
            (*entry_index)++;
            continue; //Restart the loop
        }

        (*entry_index)++;

        return result;
    }
}

//============================================================================//
//============================ Sector routines ===============================//
//============================================================================//

// We append the sector at offset b to the end of the chain that contains the sector at offset a.
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



// Removes the entire chain of sectors given the index of the first one
int chain_rm(uint32_t first_sector)
{
    uint32_t current = first_sector;

    if (current == FAT_FREE || current == FAT_EOC) // If sector is already labeled as free
        return FAT_SUCCESS;

    while (1)
    {
        uint32_t next = get_next_sector(current); // We save where is the next sector of the chain
        disk->fat[current] = FAT_FREE;            // And free the current one
        disk->sb->FSI_Free_Count++;
        disk->sb->FSI_Nxt_Free = current < disk->sb->FSI_Nxt_Free ? current : disk->sb->FSI_Nxt_Free;

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



// This function looks for a free sector in the data region. It starts by looking at the pointed sector in the superblock nxt_free camp, using the FAT to check if a sector is used or not.
// If the hint turns out to be not useful, we start again traversing from the root to the former starting point (nxt_free)
// This type of search is not so rare because this system has a problem of external fragmentation
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
            disk->sb->FSI_Free_Count--;
            return i;
        }
    }

    // If we are here we didn't find anything yet, the hint was incorrect
    for (i = 0; i < start; i++)
    {
        if (disk->fat[i] == FAT_FREE)
        {
            disk->sb->FSI_Nxt_Free = i + 1;
            disk->sb->FSI_Free_Count--;
            return i;
        }
    }

    // DISK FULL
    return FAT_ERR_DISK_FULL;
}



// This is a function where we again use the fact that every FCB has a fixed size in our favor.
int rm_recursive(uint32_t folder_sector)
{
    uint32_t fcb_index = 0;
    FAT_FCB *entry;
    while((entry = read_dir_next(folder_sector, &fcb_index)) != NULL){

        if ((strcmp(entry->name, ".") == 0) || (strcmp(entry->name, "..") == 0)) continue;

        if (entry->is_dir == 1) rm_recursive(entry->first_sector);

        entry->name[0] = '\0';
        entry->file_size = 0;
        chain_rm(entry->first_sector);
    }

    return FAT_SUCCESS;
}

//============================================================================//
//========================= Path and string management =======================//
//============================================================================//


// Parses the input string and divides it in name and extension. It is used for file creation or to search for a file in the current directory.
int parse_filename(const char *filename, char *name_dest, char *ext_dest)
{
    
    if (filename[0] == '\0'){
        fprintf(stderr, "Error: cannot use a blank name for file\n");
        return FAT_ERR_GENERIC;
    }

    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0){
        strcpy(name_dest, filename);
        ext_dest[0] = '\0';
        return FAT_SUCCESS;
    }

    uint32_t input_len = strlen(filename);

    if (input_len > 22){
        fprintf(stderr, "Error: tried to use a too long name for file\n");
        return FAT_ERR_GENERIC;
    }

    // The start index of the extension is the one next to the last '.' occurrence in the string
    char *ext_start = strrchr(filename, '.');

    // Case no extension given
    if (ext_start == NULL){
        strncpy(name_dest, filename, 16);
        ext_dest[0] = '\0';
        return FAT_SUCCESS;
    }

    // Case filenames that start with '.' like .gitignore .zshrc ecc
    if (ext_start == filename){
        strncpy(name_dest, filename, 15);
        name_dest[15] = '\0';
        ext_dest[0] = '\0';
        return FAT_SUCCESS;
    }

    uint32_t ext_len = strlen(ext_start + 1); // We move a step right from the dot
    uint32_t name_len = input_len - ext_len - 1; // We stop a step left from the dot

    strncpy(name_dest, filename, name_len);
    name_dest[name_len] = '\0';

    strncpy(ext_dest, ext_start + 1, 6);
    ext_dest[ext_len] = '\0';

    return FAT_SUCCESS;

}



// Finds the sector where a given path is located. It is used for directory path resolving.
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
        //printf("[DEBUG] Resolving token: '%s' in sector %u...\n", token, current_sector);
        //
        FAT_FCB *found = find_in_dir(token, current_sector);

        if (found == NULL)
        {
            fprintf(stderr, "Folder not found\n");
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



// Updates the runtime struct cwd_path string by parsing the path given. It does NOT check if the path actually exists. That is a resposnsibility of the change_dir function, that should call this function only after checking it.
int update_cwd_path(const char *path)
{
    if (path == NULL)
        return FAT_ERR_GENERIC;

    char temp[256];

    // If the path is absolute, start from the root.
    // Otherwise, start from the current working directory string.
    if (path[0] == '/')
    {
        strcpy(temp, "/");
    }
    else
    {
        strcpy(temp, disk->cwd_path);
    }

    char path_copy[256];
    strncpy(path_copy, path, 255);
    path_copy[255] = '\0';

    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        if (strcmp(token, ".") == 0)
        {
            // Current directory reference: do nothing.
        }
        else if (strcmp(token, "..") == 0)
        {
            // Parent directory reference: move one level up.
            if (strcmp(temp, "/") != 0)
            {
                // Locate the last slash to truncate the path string.
                char *last_slash = strrchr(temp, '/');
                if (last_slash == temp)
                {
                    // We are just one level below root (e.g., "/dir").
                    // Truncate right after the leading slash.
                    *(last_slash + 1) = '\0';
                }
                else
                {
                    // Truncate at the slash location.
                    *last_slash = '\0';
                }
            }
        }
        else
        {
            // Directory name: append it to the path string.
            int len = strlen(temp);

            // Add a separator slash if we are not at the root "/".
            if (temp[len - 1] != '/')
            {
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



// Uses the special ".." entry to get to the parent directory. Then iterates through its entries and sums their sizes to update the total size.
int update_parent_size(uint32_t current_sector, int size)
{
    if (current_sector == 0) return FAT_SUCCESS;

    FAT_FCB *parent = find_in_dir("..", current_sector);

    if (parent == NULL) return FAT_ERR_GENERIC;

    uint32_t parent_sector = parent->first_sector;
    uint32_t current = parent_sector;

    while(current != FAT_EOC){

        FAT_FCB *entries = get_entries(current);
        for(int i = 0; i < ENTRIES_PER_SEC; i++){

            if (entries[i].name[0] != '\0' && entries[i].first_sector == current_sector){
                entries[i].file_size += size;
                return(update_parent_size(parent_sector, size));
            }

        }
        current = get_next_sector(current);
    }

    return FAT_SUCCESS;
}



// TODO use calculate disk free space (using sb metadata)!! 