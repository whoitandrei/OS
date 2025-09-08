#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>      
#include <sys/syscall.h>  

int global_var = 100;
pthread_mutex_t global_var_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

void *mythread(void *arg) {
    int thread_num = *(int*)arg;
    
    int local_var = 42;
    static int local_static_var = 0;
    const int local_const_var = 99;

    pid_t pid = getpid();
    pid_t ppid = getppid();
    pid_t tid = gettid();
    pthread_t self_pthr = pthread_self();
    
    pthread_mutex_lock(&output_mutex);
    printf("Thread %d IDs: PID=%d, PPID=%d, TID=%d\n", 
           thread_num, pid, ppid, tid);
    
    char res = (pthread_equal((pthread_t)tid, self_pthr) == 0) ? 'Y' : 'N';
    printf("pthread_equal() result: %c\n", res);
    
    printf("Thread %d addresses: local=%p, static=%p, const=%p, global=%p\n",
           thread_num, &local_var, &local_static_var, &local_const_var, &global_var);
    pthread_mutex_unlock(&output_mutex);
    
    local_var += thread_num;
    
    pthread_mutex_lock(&global_var_mutex);
    global_var += thread_num;
    int current_global = global_var;
    pthread_mutex_unlock(&global_var_mutex);
    
    pthread_mutex_lock(&output_mutex);
    printf("Thread %d: local_var=%d, global_var=%d\n", 
           thread_num, local_var, current_global);
    pthread_mutex_unlock(&output_mutex);
    
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
        
        pthread_mutex_lock(&output_mutex);
        printf("\nCreated thread %d\n", i+1);
        pthread_mutex_unlock(&output_mutex);
    }

    for (int i = 0; i < 5; i++) {
        err = pthread_join(threads[i], NULL);
        if (err) {
            printf("main: pthread_join() failed: %s\n", strerror(err));
            return -1;
        }
    }
    

    pthread_mutex_destroy(&global_var_mutex);
    pthread_mutex_destroy(&output_mutex);

    return 0;
}