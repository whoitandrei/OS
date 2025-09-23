#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread() {
    printf("mythread [%d %d %d]: Created as detached and finished\n", getpid(), getppid(), gettid());
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_attr_t attr;
    int err;
    int counter = 0;

    printf("main [%d %d %d]: Starting thread creation with DETACHED attribute\n", getpid(), getppid(), gettid());
    
    err = pthread_attr_init(&attr);
    if (err) {
        printf("main: pthread_attr_init() failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err) {
        printf("main: pthread_attr_setdetachstate() failed: %s\n", strerror(err));
        pthread_attr_destroy(&attr);
        return -1;
    }


    while (1) {
        err = pthread_create(&tid, &attr, mythread, NULL);
        if (err) {
            printf("main: pthread_create() failed: %s\n", strerror(err));
            break;
        }

        counter++;
        if (counter % 100 == 0) {
            printf("main [%d %d %d]: Created %d threads\n", getpid(), getppid(), gettid(), counter);
        }

        //sleep(1);
    }

    err = pthread_attr_destroy(&attr);
    if (err) {
        printf("main: pthread_attr_destroy() failed: %s\n", strerror(err));
    }

    printf("main [%d %d %d]: Exiting after %d threads\n", getpid(), getppid(), gettid(), counter);
    return 0;
}
