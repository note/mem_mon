#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

using namespace std;

#define MEM_NAME "/mem"

int main(int argc, const char *argv[]){
	cout << argc << endl;
	for(int i=0; i<argc; ++i){
		cout << argv[i] << endl;
	}

	int * ptr = (int *) malloc(sizeof(int) * 10);

	int fd = open("Makefile", 0);

	void * mem;
	if((mem = mmap(NULL, 24, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *) -1)
		perror("mmap error");

	if(munmap(mem, 24) == -1)
		perror("munmap error");

	int mem_d = shm_open(MEM_NAME, O_RDWR | O_CREAT, 0600);
	ftruncate(mem_d, 2048);

	return 0;
}
