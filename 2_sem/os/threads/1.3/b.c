#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#define SUCCESS 0
#define ERR 1
#define STR "hello world"
#define STRCPY_OFFSET 1


struct testStruct
{
    int number;
    char* message;
};

void *mythread(void* arg) {
    struct testStruct* data = (struct testStruct*)arg;

    printf("mythread [tid: %d]: number = %d, message = %s\n", gettid(), data->number, data->message);
    free(data->message);
    free(data);
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_attr_t attr;
    int err;

    struct testStruct *data = malloc(sizeof(struct testStruct));
    if (data == NULL) {
        printf("malloc error");
        return SUCCESS;
    }

    data->number = 34;
    
    data->message = malloc(strlen(STR) * sizeof(char) + STRCPY_OFFSET);
    if (data->message == NULL) {
        printf("data->message malloc err");
        return SUCCESS;
    }
    strcpy(data->message, STR);


    printf("message pointer addr - %p, message addr - %p, pid - %d\n", &(data->message), (data->message), getpid());
    //sleep(1000);
    
    err = pthread_attr_init(&attr);
    if (err != SUCCESS) {
        printf("main: pthread_attr_init() failed: %s\n", strerror(err));
        return ERR;
    }

    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err != SUCCESS) {
        printf("main: pthread_attr_setdetachstate() failed: %s\n", strerror(err));
        pthread_attr_destroy(&attr);
        return ERR;
    }

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
    err = pthread_create(&tid, &attr, mythread, data);
    if (err != SUCCESS)
    {
        fprintf(stderr, "pthread_create() failed: %s\n", strerror(err));
        pthread_attr_destroy(&attr);
        free(data);
        return ERR;
    }


    err = pthread_attr_destroy(&attr);
    if (err != SUCCESS) {
        printf("main: pthread_attr_destroy() failed: %s\n", strerror(err));
    }

    pthread_exit(NULL);
}
