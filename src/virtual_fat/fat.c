#include "fat.h"

//==================================== FS API ================================//

int fat_format(const char *filename, int size)
{
    int fd;

    //Handling file creation
    if((fd = open(filename, O_CREAT | O_RDWR, 0666)) == -1){
        perror("Error while opening");
        return FAT_ERR_GENERIC;
    }

    //Resizing the created file
    if ((ftruncate(fd, size)) == -1){
        perror("Error while truncating file");
        return FAT_ERR_GENERIC;
    }

    //Mapping
    uint8_t *base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (base == MAP_FAILED){
        perror("Error while mmapping file ");
        return FAT_ERR_GENERIC;
    }

    //Setting up default values for structs and initializing the FS

    //disk.bs = (FAT_BootSector *)(base); //The boot sector is at the top of the disk

    //Here we want to make sure all values are dynamic and there are no real addresses but offsets
    //disk.bs->BytsPerSec = SECTOR_SIZE;
    //disk.bs->SecPerClus = SECTOR_PER_CLUS;
    //disk.bs->RsvdSecCnt = SECTOR_RSVD_CNT;

    //Operations needed to calculate the data region size


    //Locating FAT table with its offsets and initializing it


    //Locating data region with its offsets

    //Setting the starting current directory and path, the root


    //Hopefully somehow we get here
    return FAT_SUCCESS;

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

    //disk.disk_size = DISK_SIZE; //TODO checkout if this has to be corrected

    disk.disk_base = mmap(NULL, disk.disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (disk.disk_base == MAP_FAILED)
    {
        perror("Error while mapping...");
        return FAT_ERR_GENERIC;
    }

    printf("Checking disk signature...\n");

    if (disk.sb->Signature != BS_SIGNATURE)
    {
        munmap(disk.disk_base, disk.disk_size);
        disk.disk_base = NULL;
        close(fd);
        return FAT_ERR_BAD_SIG;
    }

    // We now may be using a rightful disk file

    printf("Signature OK!\n");

    // NOTE: the following offsets are in bytes

    //TODO might want to rewrite this operations based on the ones of the fat_format
    //uint32_t fsi_offset = disk.bs->BytsPerSec * disk.bs->FSInfo_offset;
    //disk.fsinfo = (FAT_FSInfo *)(disk.disk_base + fsi_offset);

    //uint32_t fat_offset = disk.bs->BytsPerSec * disk.bs->RsvdSecCnt;
    //disk.fat_table = (uint32_t *)(disk.disk_base + fat_offset);
//
    //uint32_t data_offset = (disk.bs->RsvdSecCnt + disk.bs->FATSz) * disk.bs->BytsPerSec;
    //disk.data_region = (uint8_t *)(disk.disk_base + data_offset);

    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        disk.open_files[i].is_used = 0;
        disk.open_files[i].current_offset = 0;
    }

    close(fd);

    printf("Correctly mounted disk\n");

    return FAT_SUCCESS;
}


void fat_unmount(void) // TODO capire se qui ci deve essere un input o no
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


int fat_open(const char *path, int mode)
{
    // printf("[DEBUG] MOCK: fat_open chiamato con path '%s' e mode %d\n", path, mode);

    // return 3;

    int free_index = -1;

    //Looking for a free slot in the open file list
    for (int i = 0; i < MAX_OPEN_FILES; i++)
    {
        if (disk.open_files[i].is_used == 0)
        {
            free_index = i;
        }
    }

    if (free_index == -1){
        perror("Error: too many files are open, please close at least one before proceeding");
        return FAT_ERR_GENERIC;
    }

    FAT_FCB found;

    if (find_DirectoryEntry(path, &found) == FAT_ERR_GENERIC)
    {

        // TODO aggiungere controllo sulla modalità di apertura del file
        //Forse no?^

        perror("Cannot open directory: file not found");
        return FAT_ERR_GENERIC;
    }



    perror("Error: too many open files! Please close one at least before proceding...");
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

    disk.open_files[fd].current_offset = 0; //Resetting the file descriptor

    memset(&disk.open_files[fd].cached_entry, 0, sizeof(FAT_FCB)); // Clearing cached entry

    return FAT_SUCCESS;
}
int fat_read(int fd, void *buf, int size)
{
    //printf("[DEBUG] MOCK: fat_read chiamato per l'fd %d, richiedendo %d byte\n", fd, size);
    //printf("[DEBUG] MOCK: fat_read chiamato per l'fd %d, richiedendo %d byte\n", fd, size);
    //printf("[DEBUG] MOCK: fat_read chiamato per l'fd %d, richiedendo %d byte\n", fd, size);
    //// Inseriamo una finta stringa di testo nel buffer giusto per testare il comando 'cat'
    //if (buf != NULL && size > 0)
    //{
    //    strncpy((char *)buf, "Contenuto fittizio del file lettto con successo!\n", size);
    //}
    //// Fingiamo di aver letto esattamente il numero di byte richiesti (o almeno un po')
    //return size;

    FAT_Fd *to_read = &disk.open_files[fd];

    //As of now, trying to read more bytes than the file size is not allowed and blocked before even starting to read
    if (size >= to_read->cached_entry.file_size){
        perror("Errow: trying to read more bytes than the file size");
        return FAT_ERR_GENERIC;
    }

}


int fat_write(int fd, const void *buf, int size)
{
    printf("[DEBUG] MOCK: fat_write chiamato per l'fd %d. Richiesta scrittura di %d byte\n", fd, size);
    //Fingiamo di aver scritto tutti i byte con successo
    return size;
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

    if (chain_rm(out.first_sector) == FAT_ERR_GENERIC)
    {
        perror("Error while trying to remove related clusters");
        return FAT_ERR_GENERIC;
    }

    //TODO


    return FAT_SUCCESS;

}

//==================================== Helper functions from here ================================//

int tokenize_path(char *buf, char *tokens[], int max_tokens) //TODO check the max tokens control
{
    char *token = strtok(buf, "/");

    int i = 0;

    while (token != NULL && i < max_tokens - 1){
        tokens[i] = token;

        token = strtok(NULL, "/"); //Here we go on to the next word by using the weird effect of passing NULL as an arg for strtok gg
        i++;
    }

    tokens[i] = NULL; //We use NULL in the token list to specify it is terminated, like the "\0" for the strings gg

    return i;
}

//TODO add comments
FAT_FCB  *find_in_dir(const char *target, uint32_t sector)
{
    FAT_FCB *dir_fcbs = (FAT_FCB *)get_cluster_ptr(sector);

    //TODO make this number static or calculated elsewhere
    //Maybe now it is

    int i;
    for(i = 0; i < ENTRIES_PER_SEC; i++){
        if (strcmp((char*)dir_fcbs[i].name, target) == 0){
            return &dir_fcbs[i];
        }
    }

    return NULL;
}

int delete_fcb(const char *target, uint32_t sector)
{

}

//Cluster routine operations

int get_sector_number()
{
    return (disk.disk_size - sizeof(FAT_Superblock) - sizeof(disk.fat))/SECTOR_SIZE;
}

uint32_t get_root_sector()
{
    return (disk.sb->RootSec);
}

void *get_cluster_ptr(uint32_t sector)
{
    if(sector < get_root_sector()) return NULL;
    return &(disk.data[(sector - get_root_sector())*SECTOR_SIZE]);
}

uint32_t get_next_sector(uint32_t sector)
{
    return disk.fat[sector];
}

uint32_t find_free_cluster()
{
    uint32_t start = disk.sb->FSI_Nxt_Free;

    //Start from nxtfree, we just iterate untile we find
    uint32_t i;
    for(i = start;i < get_sector_number; i++){
        if(get_next_sector(i) == FAT_FREE){
            disk.sb->FSI_Nxt_Free = i + 1;
            disk.sb->FSI_Free_Count--;
            return i;
        }
    }

    //If we are here we didn't find anything yet, the hint was incorrect
    for(i = disk.sb->RootSec; i < start; i++){
        if(get_next_sector(i) == FAT_FREE){
            disk.sb->FSI_Nxt_Free = i + 1;
            disk.sb->FSI_Free_Count--;
            return i;
        }        
    }

    //DISK FULL
    return FAT_ERR_DISK_FULL;

}

uint32_t resolve_path_from_list(const char **path, uint32_t start_cluster)
{
    uint32_t current = start_cluster;
    
    int i = 0;

    while(path[i] != NULL){
        FAT_FCB *found = find_in_dir(path[i], current);

        if (found == NULL){
            return 0;
        }

        current = found->first_sector;
        i++;
    }

    return current;
    
}


//Sector chains routine operations

int chain_append(uint32_t a, uint32_t b)
{
    while(1){
        uint32_t next = get_next_sector(a); //Lookup next sector
        if (next == FAT_EOC){ //It is the last of the chain
            disk.fat[a] = b;
            disk.fat[b] = FAT_EOC; //We append b
            break;
        }

        //If we are here we have to check if the next sector is the one that has a "EOC next"
        a = next;
    }

    return FAT_SUCCESS;

}

int chain_rm(uint32_t first_sector)
{
    uint32_t current = first_sector;

    if (current == FAT_FREE) //If sector is already labeled as free
        return FAT_SUCCESS;

    while (1)
    {
        uint32_t next = get_next_sector(current); //We save where is the next sector of the chain
        disk.fat[current] = FAT_FREE;      //And free the current one

        if (next == FAT_EOC || next == FAT_FREE) //Check out the next one, it might be the the end of the chain
        {
            break; //
        }

        current = next; //If that is not the case we go on with the cycle
    }

    return FAT_SUCCESS;
}


//The idea here is we navigate the chain until the wanted size of the cut leads to the EOC. At that point we remove the chain with the current sector as the starting one using chain_rm
int chain_cut(uint32_t first_sector, int size)
{
    int i;
    uint32_t current = first_sector;
    for(i = 0; i < size; i++){
        uint32_t next = disk.fat[current];
        current = next;
    }

    uint32_t to_delete = get_next_sector(current);
    disk.fat[current] = FAT_EOC;
    return chain_rm(to_delete);

}