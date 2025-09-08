#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH 4096
#define ERR -1
#define IS_DIRECTORY 1
#define DIR_PERMS (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define CURRENT_DIR "."
#define PARENT_DIR ".."
int process_directory(const char* source_dir);

void reverse_string(char* str, char* result) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        result[i] = str[len - 1 - i];
    }
    result[len] = '\0';
}

int do_text_job(long size, FILE *source, const char *source_path, FILE *dest)
{
    const long BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    long bytes_read = 0;
    int returned_fseek;

    for (long pos = size; pos > 0; pos -= bytes_read) {
        long chunk_size = (pos >= BUFFER_SIZE) ? BUFFER_SIZE : pos;
        long read_pos = pos - chunk_size;

        returned_fseek = fseek(source, read_pos, SEEK_SET);
        if (returned_fseek != 0)
        {
            fprintf(stderr, "Error positioning in file %s\n", source_path);
            fclose(source);
            fclose(dest);
            return ERR;
        }

        bytes_read = fread(buffer, 1, chunk_size, source);
        if (bytes_read != chunk_size && ferror(source) != 0) {
            fprintf(stderr, "error reading file: read %ld instead of %ld\n", bytes_read, chunk_size);
            fclose(source);
            fclose(dest);
            return ERR;
        }

        for (long i = 0; i < bytes_read / 2; i++)
        {
            char tmp = buffer[i];
            buffer[i] = buffer[bytes_read - 1 - i];
            buffer[bytes_read - 1 - i] = tmp;
        }

        long bytes_written = fwrite(buffer, 1, bytes_read, dest);
        if (bytes_written != bytes_read)
        {
            fprintf(stderr, "Write error: wrote %zu bytes instead of %zu\n", bytes_written, chunk_size);
            fclose(source);
            fclose(dest);
            return ERR;
        }
    }
    return 0;
}

int reverse_text(const char* source_path, const char* dest_path) {
    FILE* source = fopen(source_path, "rb");
    if (source == NULL) {
        perror("[in reverse_text] Error opening source file");
        return ERR;
    }

    FILE* dest = fopen(dest_path, "wb");
    if (dest == NULL) {
        fclose(source);
        perror("[in reverse_text] Error creating target file");
        return ERR;
    }

    int returned_fseek = fseek(source, 0, SEEK_END);
    if (returned_fseek != 0) {
        fprintf(stderr, "Error positioning in file %s\n", source_path);
        fclose(source);
        fclose(dest);
        return ERR;
    }

    long size = ftell(source);
    if (size == ERR) {
        fclose(source);
        fclose(dest);
        perror("[in reverse_text] Error positioning");
        return ERR;
    }

    int retVal = do_text_job(size, source, source_path, dest);
    if (retVal == 1) {
        fclose(source);
        fclose(dest);
        return ERR;
    }

    fclose(source);
    fclose(dest);
    return 0;
}


void reverse_parts(const char *input, char *output)
{
    char buffer[MAX_PATH];
    strncpy(buffer, input, MAX_PATH); 

    char *part = strtok(buffer, "/");  
    output[0] = '\0';

    while (part != NULL) {
        char reversed_part[MAX_PATH];
        reverse_string(part, reversed_part);  

        strcat(output, reversed_part);  
        part = strtok(NULL, "/");  

        if (part != NULL) {
            strcat(output, "/");
        }
    }
}

int mkdir_p(const char *path, mode_t mode) {
    char tmp[MAX_PATH];
    char *p = NULL;
    int mkdir_result;

    snprintf(tmp, sizeof(tmp), "%s", path);

    for (p = tmp; *p != '\0'; p++) {
        if (*p != '/') {
            continue;
        }

        *p = '\0';  
        mkdir_result = mkdir(tmp, mode);
        if (mkdir_result == ERR && errno != EEXIST) {
            return ERR;
        }
        *p = '/'; 
    }

    mkdir_result = mkdir(tmp, mode);
    if (mkdir_result == ERR && errno != EEXIST) {
        return ERR;
    }

    return 0;
}

int reverse_file(struct dirent* entry, char* reversed_name, char* source_path) {
    struct stat statbuf;
    char dest_path[MAX_PATH];
    
    int lstat_result = lstat(source_path, &statbuf);
    if (lstat_result == ERR) {
        perror("[in reverse_file] Error getting file info");
        return 0;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        return IS_DIRECTORY;
    }

    if (S_ISREG(statbuf.st_mode) == 0) { 
        return 0;
    }

    printf("file: %s mode: %d\n", entry->d_name, statbuf.st_mode);
    char reversed_file[MAX_PATH];
    reverse_string(entry->d_name, reversed_file);

    size_t size_of_all_path = strlen(reversed_name) + 1 + strlen(reversed_file);
    if (size_of_all_path >= MAX_PATH) {
        fprintf(stderr, "Error: path %s/%s is too long\n", reversed_name, reversed_file);
        return 0;
    }

    strcpy(dest_path, reversed_name);
    strcat(dest_path, "/");
    strcat(dest_path, reversed_file);

    printf("Copying %s -> %s\n", source_path, dest_path);

    int reverse_result = reverse_text(source_path, dest_path);
    if (reverse_result == ERR) {
        fprintf(stderr, "Error copying file %s\n", entry->d_name);
        return 0;
    }
    return 0;
}

int read_directory(DIR *dir, const char *source_dir, char reversed_name[4096]) {
    char source_path[MAX_PATH];

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, CURRENT_DIR) == 0 || strcmp(entry->d_name, PARENT_DIR) == 0) {
            continue;
        }

        size_t entry_name_len = strlen(source_dir) + 1 + strlen(entry->d_name);
        if (entry_name_len >= MAX_PATH) {
            fprintf(stderr, "Error: file path %s is too long\n", entry->d_name);
            continue;
        }

        strcpy(source_path, source_dir);
        strcat(source_path, "/");
        strcat(source_path, entry->d_name);

        int reverse_result = reverse_file(entry, reversed_name, source_path);
        if (reverse_result == 0) {
            continue;
        }

        int process_result = process_directory(source_path);
        if (process_result == ERR) {
            fprintf(stderr, "Error processing directory %s", source_path);
            return ERR;
        }
    }
    return 0;
}

int process_directory(const char* source_dir) {
    DIR* dir;
    char reversed_name[MAX_PATH];

    dir = opendir(source_dir);
    if (dir == NULL) {
        perror("[in process_directory] Error opening directory");
        return ERR;
    }

    size_t size_of_source_dir = strlen(source_dir);
    if (size_of_source_dir >= MAX_PATH) {
        fprintf(stderr, "Error: target directory name is too long\n");
        closedir(dir);
        return ERR;
    }

    reverse_parts((char*)source_dir, reversed_name);

    int mkdir_result = mkdir_p(reversed_name, DIR_PERMS);
    if (mkdir_result == ERR && errno != EEXIST) {
        perror("[in process_directory] Error creating directory");
        closedir(dir);
        return ERR;       
    }
    printf("Created directory: %s\n", reversed_name);

    int retVal = read_directory(dir, source_dir, reversed_name);
    if (retVal == ERR) {
        closedir(dir);
        return ERR;
    }

    closedir(dir);
    return 0;
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (process_directory(argv[1]) == ERR) {
        return EXIT_FAILURE;
    }
    
    return 0;
}