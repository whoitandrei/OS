#define _GNU_SOURCE
#include <sys/syscall.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define SUCCESS 0
#define TRUE 1
#define EXEC TRUE
#define WAIT_THREAD 5
#define WAIT_PRINT 1
#define NULL_TERM_CHAR 1
#define STR "hello world"

void cleanup(void* arg) {
    char* str = (char*)arg;
    printf("Cleaning up memory\n");
    free(str);
}

void* print_hello() {
    char *message = malloc((strlen(STR) + NULL_TERM_CHAR) * sizeof(char));
    if (message == NULL) {
        printf("malloc error");
        return NULL;
    }

    strcpy(message, STR);

    pthread_cleanup_push(cleanup, message); 

    printf("address - %p\n", &message);
    while (TRUE) {
        printf("%s\n", message);
        sleep(WAIT_PRINT);
    }

    pthread_cleanup_pop(EXEC);
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, print_hello, NULL);

    sleep(WAIT_THREAD);
    printf("Main [tid: %d]: cancelling thread\n", gettid());
    pthread_cancel(tid);

    pthread_join(tid, NULL);
    printf("Main [tid: %d]: thread stopped\n", gettid());
    return SUCCESS;
}
