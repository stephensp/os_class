#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

unsigned int share; /* Global variable everyone will try and read */
unsigned int resource_counter = 0; /* Variable used to determine # readers */
unsigned int NUM_ACCESS; /* Number of access each thread must read/write */

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; /* Mutex for share */ 
pthread_cond_t c_write = PTHREAD_COND_INITIALIZER; /* Writers wait on this */
pthread_cond_t c_read =  PTHREAD_COND_INITIALIZER; /* Readers wait on this */

void *readerFunc(void *param);
void *writeFunc(void *param) ;
void waitRandomtime();

int main() {
	int num_read, num_write, num_threads, i;

	/* Create random number of reader and writer threads */
	num_read = (rand()+2) % 6;
	num_write = rand() % 6;
	num_threads = num_read + num_write;

	pthread_t tid[num_threads];
	unsigned int thread_num[num_threads];

	printf("creating %08x readers and %08x writers\n", 
		num_read, num_write);

	NUM_ACCESS = rand() % 0x800;
	printf("NUM_ACCESS=%08x\n", NUM_ACCESS);

	/* First create the reader threads */
	for(i = 0; i < num_read; i++) {
		thread_num[i] = i;
		if(pthread_create(&tid[i], NULL, readerFunc, &thread_num[i]) != 0) {
			fprintf(stderr, "Unable to create reader thread");
			exit(1);
		}
	}
	
	/* Now create the writer threads */
	for(i = num_read; i < num_threads; i++) {
		thread_num[i] = i;
		if(pthread_create(&tid[i], NULL, writeFunc, &thread_num[i]) != 0) {
			fprintf(stderr, "Unable to create reader thread");
			exit(1);
		}
	}

	/* Wait for all threads to join before exiting */
	for(i = 0; i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
		
	return 0;
}

void *readerFunc(void *param) {
	unsigned int *threadNum, access;

	threadNum = (unsigned int*)param;
	printf("I am reader thread %08x\n", *threadNum);
	
	access = 0;
	while(access < NUM_ACCESS) {
		pthread_mutex_lock(&m);
			while(resource_counter == -1) {
				pthread_cond_wait (&c_read, &m);
			}
			resource_counter++;
		pthread_mutex_unlock(&m);
#if 0
		printf("Reading %08x\n", share);
#endif

		pthread_mutex_lock(&m);
			resource_counter--;
			if(resource_counter == 0) {
				/* Signal writers if there are no readers */
				pthread_cond_signal(&c_write);
			}
		pthread_mutex_unlock(&m);
		access++;

		waitRandomtime();
	}
}

void *writeFunc(void *param) {
	unsigned int *threadNum, access;

	threadNum = (unsigned int*)param;
	printf("I am writer thread %08x\n", *threadNum);

	access = 0;
	while(access < NUM_ACCESS) {
		pthread_mutex_lock(&m);
			while(resource_counter != 0) {
				pthread_cond_wait (&c_write, &m);
			}
			/* Set to -1 so that no one will read while we are
			 * writing */
			resource_counter = -1;
		pthread_mutex_unlock(&m);

		share++;
#if 0
		printf("------------Writing %08x-----------\n", share);
#endif

		pthread_mutex_lock(&m);
			resource_counter = 0;
			pthread_cond_broadcast(&c_read); /* Broadcast to readers */
			pthread_cond_signal(&c_write); /* Signal to one writer */
		pthread_mutex_unlock(&m);
		access++;

		waitRandomtime();
	}
}

void waitRandomtime() {
	struct timespec request, remain; 

	/* Now wait a random amount of time before trying again */
	request.tv_sec = 0;
	request.tv_nsec = rand();

	if(nanosleep(&request, &remain) != 0) {
	/* This returns -1 , don't know why */
#if 0
		printf("Nanosleep failed\n");
#endif
	}
}

