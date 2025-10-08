#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#define SUCCESS 0
#define ERR -1
#define COUNT_THREAD 10

void *mythread() {
    pthread_t self_pthr = pthread_self();
	printf("mythread [%d %d %d %lu]: Hello from mythread!\n", getpid(), getppid(), gettid(), self_pthr);

    char res = (pthread_equal((pthread_t)gettid(), self_pthr) != 0) ? 'Y' : 'N';
    printf("pthread_equal() result: %c\n", res);
	return (void*)self_pthr;
}

int main() {

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    pthread_t threads[5];
    int thread_nums[5];
    int err;
    
    for (int i = 0; i < 5; i++) {
        thread_nums[i] = i + 1;
        err = pthread_create(&threads[i], NULL, mythread, &thread_nums[i]);
        if (err != SUCCESS) {
            printf("pthread_create() failed: %s\n", strerror(err));
            return ERR;
        }
    
        printf("\nCreated thread %d, tid: %lu\n", i+1, threads[i]);
    }

       
	pthread_exit(NULL);
}

