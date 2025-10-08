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

void *mythread() {
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    
    int *result = malloc(sizeof(int));
    if (result == NULL) {
	printf("malloc error");
	return NULL;
    }

    *result = 42;
    
    printf("mythread [%d %d %d]: Returning value: %d\n", getpid(), getppid(), gettid(), *result);
    return (void*)result;
}

int main() {
    pthread_t tid;
    int err;
    int *thread_result;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return ERR;
    }

    err = pthread_join(tid, (void**)&thread_result);
    if (err != SUCCESS) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return ERR;
    }

    printf("main [%d %d %d]: Received value: %d\n", getpid(), getppid(), gettid(), *thread_result);
    
    free(thread_result);
    printf("main [%d %d %d]: Memory freed\n", getpid(), getppid(), gettid());
    return SUCCESS;
}
