#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread() {
    int err = pthread_detach(pthread_self());
    if (err != 0) {
        printf("prhtead_detach error");
        return NULL;
    }
    printf("mythread [%d %d %d]: Created as detached and finished\n", getpid(), getppid(), gettid());
    return NULL;
}

int main() {
    pthread_t tid;
    int err;
    int counter = 0;

    printf("main [%d %d %d]: Starting detached thread creation\n", getpid(), getppid(), gettid());

    while (1) {
        err = pthread_create(&tid, NULL, mythread, NULL);
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

    printf("main [%d %d %d]: Exiting after %d threads\n", getpid(), getppid(), gettid(), counter);
    return 0;
}