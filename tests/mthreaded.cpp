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

void * thread_function(void * arg){
	sleep(3);

	int fd = open("other", 0);
	void * mem;
	if((mem = mmap(NULL, 24, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *) -1)
		perror("mmap error");


	return NULL;
}

int main(int argc, const char *argv[]){
	pthread_t th;

	pthread_create((&th), NULL, thread_function, NULL);

	int in;
	cin >> in;

	open("Makefile", 0);

	pthread_join(th, NULL);

	return 0;
}
