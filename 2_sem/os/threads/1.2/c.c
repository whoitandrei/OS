#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#define SUCCESS 0
#define ERR -1
#define STR "hello world"

void *mythread() {
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    
    char *message = malloc((strlen(STR) + 1) * sizeof(char));
    if (message == NULL) {
        printf("malloc error");
        return NULL;
    }

    strcpy(message, STR);
    
    printf("mythread [%d %d %d]: Returning string: %s\n", getpid(), getppid(), gettid(), message);
    return (void*)message;
}

int main() {
    pthread_t tid;
    int err;
    char *thread_message;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return ERR;
    }

    err = pthread_join(tid, (void**)&thread_message);
    if (err != SUCCESS) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return ERR;
    }

    printf("main [%d %d %d]: Received string: %s\n", getpid(), getppid(), gettid(), thread_message);
    
    free(thread_message);
    printf("main [%d %d %d]: Memory freed\n", getpid(), getppid(), gettid());
    return SUCCESS;
}