#define _GNU_SOURCE
#include <sys/syscall.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define SUCCESS 0
#define TRUE 1
#define SLEEP_PRINT 1
#define WAIT_PRINT 5

void* print_loop() {
    while (TRUE) {
        printf("Thread %d is running\n", gettid());
        sleep(SLEEP_PRINT); 
    }
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, print_loop, NULL);

    sleep(WAIT_PRINT); 
    printf("Main [tid: %d]: cancelling thread\n", gettid());
    pthread_cancel(tid);

    pthread_join(tid, NULL);
    printf("Main [tid: %d]: thread stopped\n", gettid());
    return SUCCESS;
}
