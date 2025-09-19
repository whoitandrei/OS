#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg) {
	printf("mythread [%d %d %d]: Hello from mythread!\n", getpid(), getppid(), gettid());
	return NULL;
}

int main() {
	int err;

	printf("main [%d %d %d]: Hello from main!\n", getpid(), getppid(), gettid());

    pthread_t threads_id[5];
    int thread_nums[5];

	
    for (int i = 0; i < 5; i++) {
        thread_nums[i] = i + 1;
        err = pthread_create(&threads_id[i], NULL, mythread, &thread_nums[i]);
        if (err) {
            printf("pthread_create() failed: %s\n", strerror(err));
            return -1;
        }
    }

    for (int i = 0; i < 5; i++) {
        err = pthread_join(threads_id[i], NULL);
        if (err) {
            printf("main: pthread_join() failed: %s\n", strerror(err));
            return -1;
	    }
    }
	return 0;
}

