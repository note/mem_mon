#include "mem_mon.h"
#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace mem_mon;

int main(int argc, const char *argv[]){
	//mallopt(M_MMAP_MAX, 0);
	const int mul = 30;
	void * ptrs[mul];
	for(int i=0; i<mul; ++i)
		ptrs[i] = malloc(40960);

// 	for(int i=0; i<mul; ++i)
// 		free(ptrs[i]);

	return 0;
}
