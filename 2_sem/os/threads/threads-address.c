#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>      
#include <sys/syscall.h>  


int global_var = 100;


void *mythread(void *arg) {
    int thread_num = *(int*)arg;
    
    int local_var = 42;
    static int local_static_var = 0;
    const int local_const_var = 99;

    // они всегда успешны
    pid_t pid = getpid();
    pid_t ppid = getppid();
    pid_t tid = gettid();
    pthread_t self_pthr = pthread_self();
    
    
    printf("Thread %d IDs: PID=%d, PPID=%d, TID=%d\n", 
           thread_num, pid, ppid, tid);
    
    char res = (pthread_equal((pthread_t)tid, self_pthr) == 0) ? 'Y' : 'N';
    printf("pthread_equal() result: %c\n", res);
    
    
    
    printf("Thread %d addresses: local=%p, static=%p, const=%p, global=%p\n",
           thread_num, &local_var, &local_static_var, &local_const_var, &global_var);
    
    local_var += thread_num;
    global_var += thread_num;
    
    printf("Thread %d: local_var=%d, global_var=%d\n", 
           thread_num, local_var, global_var);
    
    return NULL;
}

int main() {
    pthread_t threads[5];
    int thread_nums[5];
    int err;
    
    for (int i = 0; i < 5; i++) {
        thread_nums[i] = i + 1;
        err = pthread_create(&threads[i], NULL, mythread, &thread_nums[i]);
        if (err) {
            printf("pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
        printf("\nCreated thread %d\n", i+1);
    }


    for (int i = 0; i < 5; i++) {
        err = pthread_join(threads[i], NULL);
        if (err) {
            printf("main: pthread_join() failed: %s\n", strerror(err));
            return -1;
        }
    }

    getchar();

    return 0;
}