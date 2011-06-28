#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/mman.h>

using namespace std;

pthread_t threads[8];
int current_thread_id = 0;
pthread_mutex_t create_mutex = PTHREAD_MUTEX_INITIALIZER;

void * thread_function(void * arg){
	int depth = *((int *) arg);
	if(depth == 0){
		depth++;
		for(int i=0; i<3; ++i){
			pthread_mutex_lock(&create_mutex);
			pthread_create(&(threads[current_thread_id]), NULL, thread_function, (void*) &depth);
			current_thread_id++;
			pthread_mutex_unlock(&create_mutex);
		}
	}

	sleep(3);

	int fd = open("other", 0);

	void * mem;
	if((mem = mmap(NULL, 24, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *) -1)
		perror("mmap error");

	return NULL;
}

int main(int argc, const char *argv[]){
	int tmp = 0;

	pthread_mutex_lock(&create_mutex);
	for(; current_thread_id<2; ++current_thread_id)
		pthread_create(&(threads[current_thread_id]), NULL, thread_function, (void*) &tmp);

	pthread_mutex_unlock(&create_mutex);
	
	open("Makefile", 0);
	
	while(current_thread_id < 8)
		sleep(1);
	
	for(int i=0; i<8; ++i)
		pthread_join(threads[i], NULL);

	return 0;
}
