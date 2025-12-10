#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#define TRUE 1
#define ERROR -1
#define SUCCESS 0
#define REQUIRES_ARGS 3

#define BUFFER_SIZE 4096
#define PATH_MAX 4096
#define MAX_THREADS 8
#define SLEEP_TIMEOUT 3
#define QUEUE_TIMEOUT 100000

#define ACCESS_RIGHTS_COPYING (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MANIP_RIGHTS_COPYING (O_WRONLY | O_CREAT | O_TRUNC)
#define DIR_ACCESS_RIGHTS (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define CURRENT_DIR "."
#define PARENT_DIR ".."

typedef struct thread_context {
    char *src_path;
    char *dst_path;
    struct thread_context *next;
} thread_context_t;

typedef struct {
    thread_context_t *head;
    thread_context_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int stop;
} task_queue_t;

task_queue_t queue;

void queue_init(task_queue_t *q) {
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    q->stop = 0;
}

void queue_push(task_queue_t *q, thread_context_t *task) {
    pthread_mutex_lock(&q->mutex);
    task->next = NULL;
    if (q->tail != NULL) q->tail->next = task;
    else q->head = task;
    q->tail = task;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

thread_context_t* queue_pop(task_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL && q->stop != TRUE) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    if (q->stop == TRUE) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }
    thread_context_t *task = q->head;
    q->head = task->next;
    if (q->head == NULL) q->tail = NULL;
    pthread_mutex_unlock(&q->mutex);
    return task;
}

void queue_destroy(task_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    q->stop = TRUE;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void thread_context_t_destructor(thread_context_t *context) {
    if (context == NULL) return;
    free(context->src_path);
    free(context->dst_path);
    free(context);
}

void copy_file(thread_context_t *context) {
    int src_fd, dst_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    src_fd = open(context->src_path, O_RDONLY);
    while (src_fd == ERROR && errno == EMFILE) {
        sleep(SLEEP_TIMEOUT);
        src_fd = open(context->src_path, O_RDONLY);
    }
    if (src_fd < 0) {
        fprintf(stderr, "<ERROR>:  open() failed: can not open file in src_path\n");
        return;
    }

    dst_fd = open(context->dst_path, MANIP_RIGHTS_COPYING, ACCESS_RIGHTS_COPYING);
    while (dst_fd == ERROR && errno == EMFILE) {
        sleep(SLEEP_TIMEOUT);
        dst_fd = open(context->dst_path, MANIP_RIGHTS_COPYING, ACCESS_RIGHTS_COPYING);
    }
    if (dst_fd < 0) {
        fprintf(stderr, "<ERROR>: open() failed: can not open file in dst_path\n");
        close(src_fd);
        return;
    }

    bytes_read = read(src_fd, buffer, sizeof(buffer));
    while (bytes_read > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written < 0 || bytes_written != bytes_read) {
            fprintf(stderr, "<ERROR>: write() failed: Error writing to destination file\n");
            break;
        }
        bytes_read = read(src_fd, buffer, sizeof(buffer));
    }

    if (bytes_read < 0) {
        fprintf(stderr, "<ERROR>: %s(): read() failed: %s\n", __func__, strerror(errno));
    }

    close(src_fd);
    close(dst_fd);
}


void copy_directory(thread_context_t *context) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char src_path[PATH_MAX], dst_path[PATH_MAX];

    if (strcmp(context->src_path, context->dst_path) == SUCCESS) return;

    dir = opendir(context->src_path);
    while (dir == NULL && errno == EMFILE) { 
        sleep(SLEEP_TIMEOUT); 
        dir = opendir(context->src_path); 
    }
    if (dir == NULL) {
        fprintf(stderr, "<ERROR> opendir() failed: cannot open source directory\n");
        return;
    }

    mkdir(context->dst_path, DIR_ACCESS_RIGHTS);

    entry = readdir(dir);
    while (entry != NULL) {
        if (strcmp(entry->d_name, CURRENT_DIR) == SUCCESS || strcmp(entry->d_name, PARENT_DIR) == SUCCESS) {
            entry = readdir(dir);
            continue;
        }

        snprintf(src_path, sizeof(src_path), "%s/%s", context->src_path, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", context->dst_path, entry->d_name);

        if (lstat(src_path, &statbuf) != SUCCESS) {
            entry = readdir(dir);
            continue;
        }

        thread_context_t *new_task = malloc(sizeof(thread_context_t));
        new_task->src_path = strdup(src_path);
        new_task->dst_path = strdup(dst_path);

        queue_push(&queue, new_task);
        entry = readdir(dir);
    }
    closedir(dir);
}


void* worker_thread() {
    while (TRUE) {
        thread_context_t *task = queue_pop(&queue);
        if (task == NULL) break; 

        struct stat st;
        if (lstat(task->src_path, &st) != SUCCESS) { 
            thread_context_t_destructor(task); 
            continue; 
        }

        if (S_ISDIR(st.st_mode)) 
            copy_directory(task);
        else if (S_ISREG(st.st_mode)) 
            copy_file(task);

        thread_context_t_destructor(task);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != REQUIRES_ARGS) { 
        fprintf(stderr, "Usage: %s <src> <dst>\n", argv[0]); 
        return EXIT_FAILURE; 
    }

    queue_init(&queue);

    thread_context_t *root_task = malloc(sizeof(thread_context_t));
    root_task->src_path = strdup(argv[1]);
    root_task->dst_path = strdup(argv[2]);
    queue_push(&queue, root_task);

    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    while (TRUE) {
        pthread_mutex_lock(&queue.mutex);
        int empty = (queue.head == NULL ? 1 : 0);
        pthread_mutex_unlock(&queue.mutex);
        if (empty) break;
        usleep(QUEUE_TIMEOUT);
    }

    queue_destroy(&queue);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("copying end\n");
    return EXIT_SUCCESS;
}
