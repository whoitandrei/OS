#define _GNU_SOURCE
#include <sys/syscall.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define SUCCESS 0
#define TRUE 1
#define WAIT_THREAD 1

void* count_loop() {
    int counter = 0;
    while (TRUE) {
        counter++; 
        //pthread_testcancel();
    }
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, count_loop, NULL);

    sleep(WAIT_THREAD);
    printf("Main [tid: %d]: cancelling thread\n", gettid());
    pthread_cancel(tid);

    pthread_join(tid, NULL);
    printf("Main [tid: %d]: thread stopped\n", gettid());
    return SUCCESS;
}
