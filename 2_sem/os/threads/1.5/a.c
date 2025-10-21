#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define SUCCESS 0
#define ERR 1
#define RECIEVED_SIG 0
#define SLEEP_TIME 1000



void* thread1() {
    sigset_t set;
    int err;
    
    err = sigfillset(&set);
    if (err != SUCCESS) {
        printf("sigfill set error");
        return NULL;
    }

    err = pthread_sigmask(SIG_SETMASK, &set, NULL);
    if (err != SUCCESS) {
        printf("sigmask error");
        return NULL;
    }
    
    printf("Thread 1 [%d]: all signals blocked\n", gettid());
    
    while(true) {
        printf("Thread 1: working...\n");
        sleep(SLEEP_TIME);
    }
    return NULL;
}

void sigint_handler(int sig) {
    if (sig != SIGINT) {
        return;
    }
    printf("Thread 2: recieved SIGINT (%d)\n", sig);
    return;
}

void* thread2() {
    sigset_t set;
    int err;
    
    err = sigemptyset(&set);
    if (err != SUCCESS) {
        printf("sigemptyset error\n");
        return NULL;
    }

    err = sigaddset(&set, SIGINT);
    if (err != SUCCESS) {
        printf("sigadd error\n");
        return NULL;
    }
    
    err = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    if (err != SUCCESS) {
        printf("sigmask error");
        return NULL;
    }
    
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    err = sigaction(SIGINT, &sa, NULL);
    if (err != SUCCESS) {
        printf("Thread 2: sigaction failed: %s\n", strerror(errno));
        return NULL;
    }
    
    printf("Thread 2 [%d]: waiting SIGINT (Ctrl+C)\n", gettid());
    
    while(true) {
        printf("Thread 2: working...\n");
        sleep(SLEEP_TIME);
    }
    return NULL;
}

void* thread3() {
    sigset_t set;
    int sig;
    int err;

    err = sigemptyset(&set);
    if (err != SUCCESS) {
        printf("sigemptyset error\n");
        return NULL;
    }

    err = sigaddset(&set, SIGQUIT);
    if (err != SUCCESS) {
        printf("sigadd error\n");
        return NULL;
    }
    
    printf("Thread 3 [%d]: waiting SIGQUIT (Ctrl+\\)\n", gettid());
    
    err = sigwait(&set, &sig);
    if (err != SUCCESS) {
        printf("sigwait error\n");
        return NULL;
    }
    printf("Thread 3: recieved SIGQUIT (%d) via sigwait()\n", sig);
    
    return NULL;
}



int main() {
    pthread_t t1, t2, t3;
    sigset_t set;
    int err;

    printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());
    
    err = sigemptyset(&set);
    if (err != SUCCESS) {
        printf("main: sigemptyset error\n");
        return ERR;
    }
    err = sigaddset(&set, SIGINT);
    if (err != SUCCESS) {
        printf("main: sigaddset error\n");
        return ERR;
    }
    err = sigaddset(&set, SIGQUIT);
    if (err != SUCCESS) {
        printf("main: sigaddset error\n");
        return ERR;
    }
    err = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_sigmask error\n");
        return ERR;
    }
    
    err = pthread_create(&t1, NULL, thread1, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_create error\n");
        return ERR;
    }
    err = pthread_create(&t2, NULL, thread2, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_create error\n");
        return ERR;
    }
    err = pthread_create(&t3, NULL, thread3, NULL);
    if (err != SUCCESS) {
        printf("main: pthread_create error\n");
        return ERR;
    }
    
    printf("All threads created. Use:\n");
    printf("- Ctrl+C for SIGINT (Thread 2)\n");
    printf("- Ctrl+\\ for SIGQUIT (Thread 3)\n");
    
    sleep(SLEEP_TIME);
    return SUCCESS;
}