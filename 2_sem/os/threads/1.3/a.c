#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define SUCCESS 0
#define ERR 1


struct testStruct
{
    int number;
    char* message;
};

void *mythread(void* arg)
{
    struct testStruct* data = (struct testStruct *)arg;
    printf("mythread [tid: %d]: number = %d, message = %s\n", gettid(), data->number, data->message);
    return NULL;
}

int main()
{
    pthread_t tid;
    int err;
    struct testStruct data;
    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    data.number = 34;
    data.message = "Hello from mythread!";

    err = pthread_create(&tid, NULL, mythread, &data);
    if (err != SUCCESS)
    {
        printf("main: pthread_create() failed\n");
        return ERR;
    }

    err = pthread_join(tid, NULL);
    if (err != SUCCESS)
    {
        printf("main: pthread_join() failed\n");
        return ERR;
    }

    return SUCCESS;
}