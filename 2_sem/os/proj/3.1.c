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
#include <stdatomic.h>

atomic_ulong copied_files = 0;

#define TRUE 1
#define ERROR -1
#define SUCCESS 0
#define REQUIRES_ARGS 3
#define PATH_SEEN 1
#define PATH_NOT_SEEN 0

#define BUFFER_SIZE 4096
#define PATH_MAX 4096
#define MAX_THREADS 8
#define SLEEP_TIMEOUT 3
#define QUEUE_TIMEOUT_US 100000

#define ACCESS_RIGHTS_COPYING (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MANIP_RIGHTS_COPYING (O_WRONLY | O_CREAT | O_TRUNC)
#define DIR_ACCESS_RIGHTS (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define CURRENT_DIR "."
#define PARENT_DIR ".."

typedef struct task_data
{
    char *src_path;
    char *dst_path;
    struct task_data *next;
} task_data_queue;

typedef struct
{
    task_data_queue *head;
    task_data_queue *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_cond_t cond_done;
    int stop;
    int pending;
} task_queue_t;

task_queue_t discovery_queue;
task_queue_t work_queue;

int err;

void queue_init(task_queue_t *q)
{
    q->head = q->tail = NULL;
    q->stop = 0;
    q->pending = 0;

    err = pthread_mutex_init(&q->mutex, NULL);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_init(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    err = pthread_cond_init(&q->cond, NULL);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_cond_init(): %s\n", strerror(errno));
        pthread_mutex_destroy(&q->mutex);
        exit(EXIT_FAILURE);
    }
    err = pthread_cond_init(&q->cond_done, NULL);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_cond_init(done): %s\n", strerror(errno));
        pthread_cond_destroy(&q->cond);
        pthread_mutex_destroy(&q->mutex);
        exit(EXIT_FAILURE);
    }
}

void queue_push(task_queue_t *q, task_data_queue *task)
{
    if (!task)
        return;
    task->next = NULL;

    err = pthread_mutex_lock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() in queue_push\n");
        return;
    }

    if (q->tail != NULL)
        q->tail->next = task;
    else
        q->head = task;
    q->tail = task;

    q->pending++;
    err = pthread_cond_signal(&q->cond);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_cond_signal() in queue_push\n");
    }

    err = pthread_mutex_unlock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_unlock() in queue_push\n");
    }
}

task_data_queue *queue_pop(task_queue_t *q)
{
    if (pthread_mutex_lock(&q->mutex) != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() in queue_pop\n");
        return NULL;
    }

    while (q->head == NULL && q->stop == 0)
    {
        err = pthread_cond_wait(&q->cond, &q->mutex);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_cond_wait() in queue_pop\n");
            pthread_mutex_unlock(&q->mutex);
            return NULL;
        }
    }

    if (q->stop)
    {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }

    task_data_queue *task = q->head;
    q->head = task->next;
    if (q->head == NULL)
        q->tail = NULL;

    err = pthread_mutex_unlock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_unlock() in queue_pop\n");
    }

    return task;
}

void queue_task_done(task_queue_t *q)
{
    err = pthread_mutex_lock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() in queue_task_done\n");
        return;
    }

    q->pending--;
    if (q->pending < 0)
        q->pending = 0;

    if (q->pending == 0)
    {
        err = pthread_cond_signal(&q->cond_done);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_cond_signal(cond_done)\n");
        }
    }

    err = pthread_mutex_unlock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_unlock() in queue_task_done\n");
    }
}

void queue_wait_empty(task_queue_t *q)
{
    err = pthread_mutex_lock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() in queue_wait_empty\n");
        return;
    }

    while (q->pending > 0)
    {
        err = pthread_cond_wait(&q->cond_done, &q->mutex);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_cond_wait(cond_done)\n");
            break;
        }
    }

    err = pthread_mutex_unlock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_unlock() in queue_wait_empty\n");
    }
}

void queue_destroy(task_queue_t *q)
{
    err = pthread_mutex_lock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() in queue_destroy\n");
        return;
    }

    q->stop = 1;

    err = pthread_cond_broadcast(&q->cond);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_cond_broadcast() in queue_destroy\n");
    }
    pthread_cond_broadcast(&q->cond_done);

    err = pthread_mutex_unlock(&q->mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_unlock() in queue_destroy\n");
    }

    pthread_cond_destroy(&q->cond);
    pthread_cond_destroy(&q->cond_done);
    pthread_mutex_destroy(&q->mutex);
}

void task_data_queue_destructor(task_data_queue *context)
{
    if (context == NULL)
        return;
    free(context->src_path);
    free(context->dst_path);
    free(context);
}

void copy_file(task_data_queue *context)
{
    if (!context)
        return;

    int src_fd = -1, dst_fd = -1;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;

    src_fd = open(context->src_path, O_RDONLY);
    while (src_fd == ERROR && errno == EMFILE)
    {
        sleep(SLEEP_TIMEOUT);
        src_fd = open(context->src_path, O_RDONLY);
    }
    if (src_fd < 0)
    {
        fprintf(stderr, "<ERROR> open(src='%s'): %s\n", context->src_path, strerror(errno));
        return;
    }

    dst_fd = open(context->dst_path, MANIP_RIGHTS_COPYING, ACCESS_RIGHTS_COPYING);
    while (dst_fd == ERROR && errno == EMFILE)
    {
        sleep(SLEEP_TIMEOUT);
        dst_fd = open(context->dst_path, MANIP_RIGHTS_COPYING, ACCESS_RIGHTS_COPYING);
    }
    if (dst_fd < 0)
    {
        fprintf(stderr, "<ERROR> open(dst='%s'): %s\n", context->dst_path, strerror(errno));
        close(src_fd);
        return;
    }

    bytes_read = read(src_fd, buffer, sizeof(buffer));
    while (bytes_read > 0)
    {
        ssize_t to_write = bytes_read;
        char *p = buffer;
        while (to_write > 0)
        {
            bytes_written = write(dst_fd, p, to_write);
            if (bytes_written < 0)
            {
                if (errno == EINTR)
                    continue;
                fprintf(stderr, "<ERROR> write(dst='%s'): %s\n", context->dst_path, strerror(errno));
                close(src_fd);
                close(dst_fd);
                return;
            }
            to_write -= bytes_written;
            p += bytes_written;
        }
        bytes_read = read(src_fd, buffer, sizeof(buffer));
    }

    if (bytes_read < 0)
    {
        fprintf(stderr, "<ERROR> read(src='%s'): %s\n", context->src_path, strerror(errno));
    }

    err = close(src_fd);
    if (err < 0)
    {
        fprintf(stderr, "<ERROR> close(src): %s\n", strerror(errno));
    }
    err = close(dst_fd);
    if (err < 0)
    {
        fprintf(stderr, "<ERROR> close(dst): %s\n", strerror(errno));
    }
    atomic_fetch_add(&copied_files, 1);
}

void *worker_thread(void *arg)
{
    (void)arg;

    while (TRUE)
    {
        task_data_queue *task = queue_pop(&work_queue);
        if (task == NULL)
            break;

        struct stat st;
        err = lstat(task->src_path, &st);
        if (err == SUCCESS && S_ISREG(st.st_mode))
        {
            copy_file(task);
        }

        task_data_queue_destructor(task);
        queue_task_done(&work_queue);
    }

    return NULL;
}

int process_directory(task_data_queue *task)
{
    err = mkdir(task->dst_path, DIR_ACCESS_RIGHTS);
    if (err != SUCCESS && errno != EEXIST)
    {
        fprintf(stderr, "<ERROR> mkdir('%s'): %s\n",
                task->dst_path, strerror(errno));
        task_data_queue_destructor(task);
        queue_task_done(&discovery_queue);
        return EXIT_FAILURE;
    }

    DIR *dir = opendir(task->src_path);
    if (dir == NULL)
    {
        fprintf(stderr, "<ERROR> opendir('%s'): %s\n",
                task->src_path, strerror(errno));
        task_data_queue_destructor(task);
        queue_task_done(&discovery_queue);
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    entry = readdir(dir);
    while (entry != NULL)
    {
        if (strcmp(entry->d_name, CURRENT_DIR) == SUCCESS ||
            strcmp(entry->d_name, PARENT_DIR) == SUCCESS)
        {
            entry = readdir(dir);
            continue;
        }

        char src[PATH_MAX];
        char dst[PATH_MAX];

        err = snprintf(src, sizeof(src), "%s/%s",
                       task->src_path, entry->d_name);
        if (err < 0 || err >= (int)sizeof(src))
        {
            entry = readdir(dir);
            continue;
        }

        err = snprintf(dst, sizeof(dst), "%s/%s",
                       task->dst_path, entry->d_name);
        if (err < 0 || err >= (int)sizeof(dst))
        {
            entry = readdir(dir);
            continue;
        }

        task_data_queue *child = malloc(sizeof(task_data_queue));
        if (child == NULL)
        {
            entry = readdir(dir);
            continue;
        }

        child->src_path = strdup(src);
        child->dst_path = strdup(dst);
        child->next = NULL;

        if (child->src_path == NULL || child->dst_path == NULL)
        {
            task_data_queue_destructor(child);
            entry = readdir(dir);
            continue;
        }

        struct stat st;
        err = lstat(child->src_path, &st);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<WARN> lstat('%s'): %s\n",
                    task->src_path, strerror(errno));
            task_data_queue_destructor(task);
            queue_task_done(&discovery_queue);
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            queue_push(&discovery_queue, child);
        }
        else if (S_ISREG(st.st_mode))
        {
            queue_push(&work_queue, child);
        }

        entry = readdir(dir);
    }

    err = closedir(dir);
    if (err != SUCCESS)
    {
        printf("closedir error");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void *task_creator_thread(void *arg)
{
    (void)arg;

    while (TRUE)
    {
        task_data_queue *task = queue_pop(&discovery_queue);
        if (task == NULL)
            break;


        err = process_directory(task);
        if (err == EXIT_FAILURE)
        {
            task_data_queue_destructor(task);
            queue_task_done(&discovery_queue);
            continue;
        }

        task_data_queue_destructor(task);
        queue_task_done(&discovery_queue);
    }

    queue_wait_empty(&discovery_queue);

    return NULL;
}

/* --- main --- */
int main(int argc, char *argv[])
{
    if (argc != REQUIRES_ARGS)
    {
        fprintf(stderr, "Usage: %s <src> <dst>\n", argv[0]);
        return EXIT_FAILURE;
    }

    queue_init(&discovery_queue);
    queue_init(&work_queue);

    task_data_queue *root_task = malloc(sizeof(task_data_queue));
    if (!root_task)
    {
        fprintf(stderr, "<ERROR> malloc() for root task failed\n");
        queue_destroy(&discovery_queue);
        return EXIT_FAILURE;
    }
    root_task->src_path = strdup(argv[1]);
    root_task->dst_path = strdup(argv[2]);
    root_task->next = NULL;
    if (!root_task->src_path || !root_task->dst_path)
    {
        fprintf(stderr, "<ERROR> strdup() for root paths failed\n");
        task_data_queue_destructor(root_task);
        queue_destroy(&discovery_queue);
        queue_destroy(&work_queue);
        return EXIT_FAILURE;
    }

    queue_push(&discovery_queue, root_task);

    pthread_t creator;
    int err = pthread_create(&creator, NULL, task_creator_thread, NULL);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_create(task_creator): %s\n", strerror(err));
        queue_destroy(&discovery_queue);
        queue_destroy(&work_queue);
        return EXIT_FAILURE;
    }

    pthread_t workers[MAX_THREADS];
    int created = 0;
    for (int i = 0; i < MAX_THREADS; i++)
    {
        err = pthread_create(&workers[i], NULL, worker_thread, NULL);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_create(worker %d): %s\n", i, strerror(err));
            for (int j = 0; j < created; j++)
                pthread_cancel(workers[j]);
            for (int j = 0; j < created; j++)
                pthread_join(workers[j], NULL);
            pthread_cancel(creator);
            pthread_join(creator, NULL);
            queue_destroy(&discovery_queue);
            queue_destroy(&work_queue);
            return EXIT_FAILURE;
        }
        created++;
    }

    queue_wait_empty(&discovery_queue);
    queue_wait_empty(&work_queue);

    err = pthread_mutex_lock(&work_queue.mutex);
    if (err != SUCCESS)
    {
        fprintf(stderr, "<ERROR> pthread_mutex_lock() before stop work_queue\n");
    }
    else
    {
        work_queue.stop = 1;
        err = pthread_cond_broadcast(&work_queue.cond);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_cond_broadcast() work_queue\n");
        }
        err = pthread_mutex_unlock(&work_queue.mutex);
        if (err != SUCCESS)
        {
            fprintf(stderr, "<ERROR> pthread_mutex_unlock() work_queue\n");
        }
    }

    for (int i = 0; i < created; i++)
    {
        pthread_join(workers[i], NULL);
    }

    queue_destroy(&discovery_queue);
    queue_destroy(&work_queue);

    printf("Copied files: %lu\n", atomic_load(&copied_files));
    printf("copying end\n");
    return EXIT_SUCCESS;
}
