#include "my_shell.h"

void deallocate_cmd(char* argv[MAX_TOKENS]) {
	while (*argv != NULL)
		free(*argv++);
}

char* dup_string(const char* in) {
    size_t n = strlen(in);
    char* out = malloc(n + 1);
    strcpy(out, in);
    return out;
}

void get_cmd_line(char* argv[MAX_TOKENS]) {
    int argc = 0;
    char line[MAX_LINE];
    fgets(line, MAX_LINE, stdin);
    char* token = strtok(line, " \t\n");
    argc = 0;
    while (argc < MAX_TOKENS && token != NULL) {
        argv[argc++] = dup_string(token);
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
}

int cat(const char *filename)
{
    int fd = fat_open(filename, 0);

    if (fd == FAT_ERR_GENERIC){
        printf("Error: file not found");
        return -1;
    }

    char buffer[512] = {0};

    int read = fat_read(fd, buffer, sizeof(buffer) - 1);

    if (read > 0){
        prinf("%s", buffer);
    }

    fat_close(fd);

    return 0;

}

int append(const char *filename, const char *text)
{
    int fd = fat_open(filename, 1);

    if (fd == FAT_ERR_GENERIC){
        printf("Error: file not found");
        return -1;
    }

    char buffer[512] = {0};

    int size = sizeof(buffer);

    int written = fat_write(fd, buffer, size - 1);

    if (written > 0){
        printf("Append success of %d in file %s\n", size-1);
    }

    fat_close(fd);

    return 0;
}

int shell_close(const char *filename)
{
    fat_unmount(filename);

    return 1;
}