#include "mem_mon.h"
#include <iostream>

using namespace mem_mon;
using namespace std;

void stack_consuming(char * arg, int n){
	char buf[256];
	for(int i=0; i<256; ++i)
		buf[i] = arg[i];

	if(n>0)
		stack_consuming(buf, n-1);
	else{
		process proc;
		proc.update();
		cout << "stack_consuming, stack size: " << proc << endl;
	}
}

int main(int argc, const char *argv[]){
	system sys = system::get_instance();
	sys.update();
//	cout << "Page size: " << sys_res.get_page_size() << endl << "Total RAM: " << sys_res.get_total_ram() << endl << "RAM in use: " << sys_res.get_used_ram() << endl << "Free RAM: " << sys_res.get_free_ram() << endl;

	cout << sys;

	process proc;
	proc.update();
	cout << proc;

	char buf[256];
	stack_consuming(buf, 12);
	pause();

	return 0;
}
