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
    int err;
    
    err = pthread_mutex_lock(&output_mutex);
    if (err != 0) {
        printf("lock error");
        return NULL;
    }
    
    printf("Thread %d IDs: PID=%d, PPID=%d, TID=%d, TID(self) = %lu\n", thread_num, pid, ppid, tid, self_pthr);
    printf("Thread %d addresses: local=%p, static=%p, const=%p, global=%p\n",
           thread_num, &local_var, &local_static_var, &local_const_var, &global_var);
    
    err = pthread_mutex_unlock(&output_mutex);
    if (err != 0) {
        printf("ulock error");
        return NULL;
    }
    
    local_var += thread_num;
    
    err = pthread_mutex_lock(&global_var_mutex);
    if (err != 0) {
        printf("lock error");
        return NULL;
    }
    
    global_var += thread_num;
    int current_global = global_var;
    
    err = pthread_mutex_unlock(&global_var_mutex);
    if (err != 0) {
        printf("unlock error");
        return NULL;
    }
    
    err = pthread_mutex_lock(&output_mutex);
    if (err != 0) {
        printf("lock error");
        return NULL;
    }
    
    printf("Thread %d: local_var=%d, global_var=%d\n", thread_num, local_var, current_global);
    
    err = pthread_mutex_unlock(&output_mutex);
    if (err != 0) {
        printf("unlock error");
        return NULL;
    }
    
    return (void*)self_pthr;
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
        printf("\nCreated thread %d, tid: %lu\n", i+1, threads[i]);
        pthread_mutex_unlock(&output_mutex);
    }

    pthread_t results[5];
    for (int i = 0; i < 5; i++) {
        void* thread_return;
        err = pthread_join(threads[i], &thread_return);
        if (err) {
            printf("main: pthread_join() failed: %s\n", strerror(err));
            return -1;
        }

        results[i] = (pthread_t)thread_return;
    }

    for (int i = 0; i < 5; ++i) {
        pthread_mutex_lock(&output_mutex);
        char res = (pthread_equal((pthread_t)results[i], threads[i]) != 0) ? 'Y' : 'N';
        printf("pthread_equal() in MAIN result %d: %c\n", i+1, res);
        pthread_mutex_unlock(&output_mutex);
    }    

    pthread_mutex_destroy(&global_var_mutex);
    pthread_mutex_destroy(&output_mutex);

    return 0;
}