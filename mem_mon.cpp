#include "mem_mon.h"
#include <fstream>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <sys/resource.h>

using namespace std;
const int max_line = 256;

void stack_consuming(char * arg, int n){
	char buf[256];
	for(int i=0; i<256; ++i)
		buf[i] = arg[i];

	if(n>0)
		stack_consuming(buf, n-1);
	else
		cout << "stack_consuming, stack size: " << get_stack_size() << endl;
}

int main(int argc, const char *argv[]){
	//cout << get_total_system_mem() << endl;
	//cout << get_total_swap_size() << endl;
	
	cout << "Stack limit: " << get_stack_limit() << endl;
	cout << "Stack size: " << get_stack_size() << endl;

	char buf[256];
	stack_consuming(buf, 12);

	return 0;
}
