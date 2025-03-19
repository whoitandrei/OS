#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH 4096
#define FILE_PROPS S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH



void reverse_string(char* str, char* result) {
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        result[i] = str[len - 1 - i];
    }
    result[len] = '\0';
}

int reverse_text(const char* source_path, const char* dest_path) {
    FILE* source = fopen(source_path, "rb");
    if (!source) {
        perror("Ошибка открытия исходного файла");
        return -1;
    }

    FILE* dest = fopen(dest_path, "wb");
    if (!dest) {
        fclose(source);
        perror("Ошибка создания целевого файла");
        return -1;
    }

    fseek(source, 0, SEEK_END);
    long size = ftell(source);
    
    const long BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    for (long pos = size; pos > 0; pos -= BUFFER_SIZE) {
        long chunk_size = (pos >= BUFFER_SIZE) ? BUFFER_SIZE : pos;
        long read_pos = pos - chunk_size;
        
        if (fseek(source, read_pos, SEEK_SET) != 0) {
            fprintf(stderr, "Ошибка позиционирования в файле %s\n", source_path);
            fclose(source);
            fclose(dest);
            return -1;
        }

        long bytes_read = fread(buffer, 1, chunk_size, source);
        if (bytes_read != chunk_size) {
            fprintf(stderr, "Ошибка чтения: прочитано %zu байт вместо %zu\n", bytes_read, chunk_size);
            fclose(source);
            fclose(dest);
            return -1;
        }
        
        for (long i = 0; i < chunk_size / 2; i++) {
            char tmp = buffer[i];
            buffer[i] = buffer[chunk_size - 1 - i];
            buffer[chunk_size - 1 - i] = tmp;
        }
        
        long bytes_written = fwrite(buffer, 1, chunk_size, dest);
        if (bytes_written != chunk_size) {
            fprintf(stderr, "Ошибка записи: записано %zu байт вместо %zu\n", bytes_written, chunk_size);
            fclose(source);
            fclose(dest);
            return -1;
        }
    }
    
    fclose(source);
    fclose(dest);
    return 0;
}


void reverse_parts(const char *input, char *output) {
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

    snprintf(tmp, sizeof(tmp), "%s", path);

    for (p = tmp; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';  
            if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
                return -1;
            }
            *p = '/'; 
        }
    }

    if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

int reverse_file(struct dirent* entry, struct stat statbuf, char* reversed_name, char* source_path, char* dest_path) {
    if (stat(source_path, &statbuf) == -1) {
        perror("Ошибка получения информации о файле");
        return 0;
    }

    if (S_ISDIR(statbuf.st_mode)) {
        return 1;
    }

    if (S_ISREG(statbuf.st_mode)) {
        char reversed_file[MAX_PATH];
        reverse_string(entry->d_name, reversed_file);

        size_t reversed_name_len = strlen(reversed_name);
        size_t reversed_file_len = strlen(reversed_file);
        if (reversed_name_len + 1 + reversed_file_len >= MAX_PATH) {
            fprintf(stderr, "Ошибка: путь %s/%s слишком длинный\n", reversed_name, reversed_file);
            return 0;
        }

        strcpy(dest_path, reversed_name);
        strcat(dest_path, "/");
        strcat(dest_path, reversed_file);

        printf("Копирую %s -> %s\n", source_path, dest_path);
        if (reverse_text(source_path, dest_path) == -1) {
            fprintf(stderr, "Ошибка при копировании файла %s\n", entry->d_name);
        }
    }

    return 0;
}

int process_directory(const char* source_dir) {
    DIR* dir;
    struct dirent* entry;
    struct stat statbuf;
    char reversed_name[MAX_PATH];
    char source_path[MAX_PATH];
    char dest_path[MAX_PATH];

    dir = opendir(source_dir);
    if (!dir) {
        perror("Ошибка открытия каталога");
        return -1;
    }

    reverse_parts((char*)source_dir, reversed_name);
    if (strlen(reversed_name) >= MAX_PATH) {
        fprintf(stderr, "Ошибка: имя целевого каталога слишком длинное\n");
        closedir(dir);
        return -1;
    }

    
    if (mkdir_p(reversed_name, FILE_PROPS) == -1) {
        if (errno != EEXIST) {
            perror("Ошибка создания каталога");
            closedir(dir);
            return -1;
        }
    }

    printf("Создан каталог: %s\n", reversed_name);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t source_dir_len = strlen(source_dir);
        size_t entry_name_len = strlen(entry->d_name);
        if (source_dir_len + 1 + entry_name_len >= MAX_PATH) { 
            fprintf(stderr, "Ошибка: путь к файлу %s слишком длинный\n", entry->d_name);
            continue;
        }
        
        strcpy(source_path, source_dir);
        strcat(source_path, "/");
        strcat(source_path, entry->d_name);
        
        if (reverse_file(entry, statbuf, reversed_name, source_path, dest_path) == 0) {
            continue;
        }
        
        if (process_directory(source_path) == -1) {
            fprintf(stderr, "Ошибка обработки каталога %s", source_path);
            return -1;
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <путь_к_каталогу>\n", argv[0]);
        return 1;
    }

    if (process_directory(argv[1]) == -1) {
        return 1;
    }

    return 0;
}