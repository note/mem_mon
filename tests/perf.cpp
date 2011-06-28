#include "mem_mon.h"
#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace mem_mon;

void config_fast(){
	mallopt(M_TRIM_THRESHOLD, -1);
}

void config_fastest(){
	config_fast();
	mallopt(M_TOP_PAD, 4096*1000);
}

void config_smallest(){
	mallopt(M_TRIM_THRESHOLD, 100);
}


void config_small(){
	mallopt(M_TRIM_THRESHOLD, 1024*40);
}

int main(int argc, const char *argv[]){
	//mallopt(M_MMAP_MAX, 0);
	if(argc > 1){
		string cmd(argv[1]);
		if(cmd == "fast")
			config_fast();
		if(cmd == "fastest")
			config_fastest();
		if(cmd == "small")
			config_small();
		if(cmd == "smallest")
			config_smallest();
	}
	
	const int chunks = 400000;
	const int step = 200;
	const int chunk_size = 512;
	int * ptrs[chunks+2];

	for(int i=0; i<chunks; ++i)
		ptrs[i] = NULL;

	int cnt = 0;
	while(cnt < chunks-step){
		for(int i=0; i<step; ++i, ++cnt)
			ptrs[cnt] = (int *) malloc(chunk_size);
		int tmp = cnt;
		for(int i=0; i<step/2; ++i){
			--cnt;
			free(ptrs[cnt]);
			ptrs[cnt] = NULL;
		}
		cnt = tmp;
	}

// 	for(int i=0; i<chunks; ++i)
// 		if(ptrs[i])
// 			free(ptrs[i]);

	for(int i=0; i<chunks; ++i)
		if(!ptrs[i])
			ptrs[i] = (int *) malloc(chunk_size*8);

	process proc;
	proc.update();
	cout << proc;

// 	for(int i=0; i<chunks; ++i)
// 		free(ptrs[i]);

	return 0;
}
