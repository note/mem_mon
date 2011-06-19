#include "process.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <sys/resource.h>


using namespace std;

namespace mem_mon{

std::vector<abstract_malloc_observer *> malloc_hooker::observers;

void process::update(){
	res.read_proc_stat();
	res.get_by_getrlimit();
}

void process::process_info::get_by_getrlimit(){
	struct rlimit rlim;
	if(getrlimit(RLIMIT_DATA, &rlim) == -1)
		data_segment_limit = -1;
	else
		data_segment_limit = rlim.rlim_cur;

	if(getrlimit(RLIMIT_STACK, &rlim) == -1)
		stack_limit = -1;
	else
		stack_limit = rlim.rlim_cur;
}

ostream & operator<<(ostream &os, const process & proc){
	os << "*** Output from mem_mon::process ***" << endl;
	os << "Stack size: " << proc.get_stack_size() << endl;
	os << "*** End of output from mem_mon::process ***" << endl;
	return os;
}

void process::process_info::read_proc_stat(){
	ifstream ifs("/proc/self/stat", ifstream::in);
	string thrash;
	for(int i=0; i<27; ++i)
		ifs >> thrash;

	long unsigned start_stack, current_esp;
	ifs >> start_stack;
	ifs >> current_esp;
	stack_size = start_stack - current_esp; //because stack is expanding towards lower addresses
}

void process::process_info::read_proc_statm(){
	ifstream ifs("/proc/self/statm", ifstream::in);
	cin >> virtual_mem_size;
	cin >> rss;
	cin >> shared_mem_size;
	cin >> code_size;
	cin >> lib_size;
}

}
