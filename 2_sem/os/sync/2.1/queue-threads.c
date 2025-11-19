#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>

#include "queue.h"

#define RED "\033[41m"
#define NOCOLOR "\033[0m"
#define SUCCESS 0
#define TRUE 1
#define ERR 1
#define WORKING_TIME 30
#define QUEUE_SIZE 10000000


void set_cpu(int core) {
	int err;
	cpu_set_t cpuset;
	pthread_t tid = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(core, &cpuset);

	err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (err != SUCCESS) {
		printf("set_cpu: pthread_setaffinity failed for cpu %d\n", core);
		return;
	}

	printf("set_cpu: set cpu %d\n", core);
}

void *reader(void *arg) {
	int expected = 0;
	queue_t *q = (queue_t *)arg;
	printf("reader [%d %d %d]\n", getpid(), getppid(), gettid());

	set_cpu(1);

	while (TRUE) {
		int val = -1;
		int ok = queue_get(q, &val);
		if (!ok)
			continue;

		if (expected != val)
			printf(RED"ERROR: get value is %d but expected - %d" NOCOLOR "\n", val, expected);

		expected = val + 1;
		pthread_testcancel(); 
	}

	return NULL;
}

void *writer(void *arg) {
	int i = 0;
	queue_t *q = (queue_t *)arg;
	printf("writer [%d %d %d]\n", getpid(), getppid(), gettid());

	set_cpu(2);

	while (TRUE) {
		int ok = queue_add(q, i);
		if (!ok)
			continue;
		i++;
		pthread_testcancel();
	}

	return NULL;
}

int main() {
	pthread_t reader_tid, writer_tid;
	queue_t *q;
	int err;

	printf("main [%d %d %d]\n", getpid(), getppid(), gettid());

	q = queue_init(QUEUE_SIZE);
	set_cpu(3);

	err = pthread_create(&reader_tid, NULL, reader, q);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}

	//sched_yield();

	err = pthread_create(&writer_tid, NULL, writer, q);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}

	sleep(WORKING_TIME);

	err = pthread_cancel(writer_tid);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}

	err = pthread_cancel(reader_tid);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}

	err = pthread_join(writer_tid, NULL);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}
	err = pthread_join(reader_tid, NULL);
	if (err != SUCCESS) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return ERR;
	}

	queue_destroy(q);
	return SUCCESS;
}
