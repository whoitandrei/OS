#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void *mythread(void *arg) {
    printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
    
    char *message = malloc(20 * sizeof(char));
    strcpy(message, "hello world");
    
    printf("mythread [%d %d %d]: Returning string: %s\n", getpid(), getppid(), gettid(), message);
    return (void*)message;
}

int main() {
    pthread_t tid;
    int err;
    char *thread_message;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err) {
        printf("main: pthread_create() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_join(tid, (void**)&thread_message);
    if (err) {
        printf("main: pthread_join() failed: %s\n", strerror(err));
        return -1;
    }

    printf("main [%d %d %d]: Received string: %s\n", getpid(), getppid(), gettid(), thread_message);
    
    free(thread_message);
    printf("main [%d %d %d]: Memory freed\n", getpid(), getppid(), gettid());
    return 0;
}