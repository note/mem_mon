#include "mem_mon.h"
#include <stdlib.h>
#include <iostream>

using namespace std;
using namespace mem_mon;

int main(int argc, const char *argv[]){
	void * ptr = malloc(4096);

	mallinfo_wrapper mallinfo;
	cout << mallinfo;

	malloc_stats();

	return 0;
}
