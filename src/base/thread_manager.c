/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "thread_manager.h"
 
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base/global_settings.h"

#define THREADS_STEP_SIZE 16

static unsigned max_threads;
static unsigned thread_count;
static unsigned threads_size; /* the size of */
static pthread_t *threads = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned umin(unsigned a, unsigned b) {
	return a > b ? b : a;
}

int thread_manager_setup(config_t config) {
	const char *max_child_threads_s = config_get(config, "max-child-threads");
	if (max_child_threads_s) {
		if (sscanf(max_child_threads_s, "%u", &max_threads) != 1 || max_threads == 0) {
			fprintf(stderr, "\x1b[31m[Config] Invalid number: \"%s\" (it should be 1 to 2147483648)\x1b[0m\n", max_child_threads_s);
			return 0;
		}
	} else {
		max_threads = 100;
		fprintf(stderr, "\x1b[33m[Config] Config option 'max-child-threads' is missing! Setting to default: %u\x1b[0m\n", max_threads);
	}

	thread_count = 0;
	threads_size = umin(max_threads, THREADS_STEP_SIZE);
	threads = calloc(threads_size, sizeof(pthread_t));

	return 1;
}

void thread_manager_wait_or_kill(void) {
	struct timespec wait_time;
	wait_time.tv_sec = 0;
	wait_time.tv_nsec = 10000;
	size_t retries = 0;
	while (thread_count > 0) {
		nanosleep(&wait_time, NULL);
		wait_time.tv_nsec = 100000;
		if (++retries == 10) {
			pthread_mutex_lock(&mutex);
			if (thread_count > 0) {
				printf("ThreadManager: Killing all %u thread(s) remaining...\n", thread_count);
				pthread_t *the_threads = threads;
				threads = NULL;
				size_t i;
				for (i = 0; i < thread_count; i++) {
					pthread_cancel(the_threads[i]);
					nanosleep(&wait_time, NULL);
					pthread_kill(the_threads[i], SIGQUIT);
				}
				free(the_threads);
			}
			pthread_mutex_unlock(&mutex);
			break;
		}
	}
	free(threads);
}

/*  -2 = pthread error
	-1 = allocation error
	 0 = thread pool full.
     1 = success.
*/
int thread_manager_add(void *(*start_routine) (void *), void *arguments) {
	pthread_mutex_lock(&mutex);
	if (thread_count == threads_size) {
		if (threads_size != max_threads) {
			pthread_t *new_threads = realloc(threads, umin(threads_size + THREADS_STEP_SIZE, max_threads));
			if (new_threads == NULL) {
				pthread_mutex_unlock(&mutex);
				puts("ThreadManager: allocation error.");
				return -1;
			}
			threads = threads;
		} else {
			pthread_mutex_unlock(&mutex);
			puts("ThreadManager: thread pool full.");
			return 0;
		}
	}

	if (pthread_create(&threads[thread_count], NULL, start_routine, arguments) != 0) {
		pthread_mutex_unlock(&mutex);
		puts("ThreadManager: pthread_create error.");
		return -2;
	}

	thread_count += 1;
	pthread_mutex_unlock(&mutex);
	return 1;
}

void thread_manager_finished(void) {
	pthread_t current = pthread_self();
	pthread_mutex_lock(&mutex);
	size_t i;
	for (i = 0; i < thread_count; i++) {
		if (threads[i] == current) {
			if (i == thread_count - 1) {
				threads[i] = 0;
			} else {
				memcpy(threads+i, threads+i+1, thread_count - i - 1);
			}
			thread_count -= 1;
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
			return;
		}
	}
	fputs("ThreadManager: Couldn't find thread in thread pool[?][!]\n", stderr);
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}
