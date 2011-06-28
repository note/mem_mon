#include "mem_mon.h"
#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace mem_mon;

void config_fast(){
	mallopt(M_TRIM_THRESHOLD, -1);
}

void config_small(){
	mallopt(M_TRIM_THRESHOLD, 100);
}

int main(int argc, const char *argv[]){
	//mallopt(M_MMAP_MAX, 0);
	config_fast();
	const int mul = 400000;
	const int step = 200;
	int * ptrs[mul+2];

	for(int i=0; i<mul; ++i)
		ptrs[i] = NULL;

	int cnt = 0;
	while(cnt < mul-step){
		for(int i=0; i<step; ++i, ++cnt)
			ptrs[cnt] = (int *) malloc(4096);
		for(int i=0; i<step/2; ++i){
			--cnt;
			free(ptrs[cnt]);
			ptrs[cnt] = NULL;
		}
	}

	for(int i=0; i<mul; ++i)
		if(ptrs[i])
			free(ptrs[i]);

	process proc;
	proc.update();
	cout << proc;

// 	for(int i=0; i<mul; ++i)
// 		free(ptrs[i]);

	return 0;
}
