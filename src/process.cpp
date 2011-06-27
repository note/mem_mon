#include "process.h"
#include "system.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <sys/resource.h>


using namespace std;

namespace mem_mon{

std::vector<abstract_malloc_observer *> malloc_hooker::observers;

void process::update(){
	if(thread_id == -1)
		res.read_proc_stat();
	else
		res.read_proc_stat(thread_id);
	res.get_by_getrlimit();
	res.read_proc_statm();
}

void process::process_info::get_by_getrlimit(){
	struct rlimit rlim;
	if(getrlimit(RLIMIT_DATA, &rlim) == -1){
		perror("getrlimit error");
		data_segment_limit = -1;
	}else
		data_segment_limit = (rlim.rlim_cur == RLIM_INFINITY) ? -2 : rlim.rlim_cur; // -2 has special meaning - no limit, while -1 means cannot read

	if(getrlimit(RLIMIT_STACK, &rlim) == -1){
		perror("getrlimit error");
		stack_limit = -1;
	}else
		stack_limit = (rlim.rlim_cur == RLIM_INFINITY) ? -2 : rlim.rlim_cur;
}

ostream & operator<<(ostream &os, const process & proc){
	os << endl << "*** Output from mem_mon::process ***" << endl;
	os << "Virtual memory size: " << format(proc.get_virtual_mem_size()) << endl;
	os << "RSS: " << format(proc.get_rss()) << endl;
	os << "Shared memory size: " << format(proc.get_shared_mem_size()) << endl;
	os << "Code size: " << format(proc.get_code_size()) << endl;
	os << "Library size: " << format(proc.get_lib_size()) << endl;
	
	if(proc.get_stack_limit() == -2)
		os << "Stack limit: No limit" << endl;
	else
		os << "Stack limit: " << format(proc.get_stack_limit()) << endl;
	os << "Stack size: " << proc.get_stack_size() << endl;
	
	if(proc.get_data_segment_limit() == -2)
		os << "Data segment limit: No limit" << endl;
	else
		os << "Data segment limit: " << format(proc.get_data_segment_limit()) << endl;

	os << "*** End of output from mem_mon::process ***" << endl;
	return os;
}

void process::process_info::read_proc_stat(){
	read_proc_stat("/proc/self/stat");
}

void process::process_info::read_proc_stat(pid_t id){
	read_proc_stat("/proc/self/task/" + str(id) + "/stat");
}

void process::process_info::read_proc_stat(const string & filename){
	ifstream ifs(filename.c_str(), ifstream::in);

	if(ifs.good()){
		string thrash;
		for(int i=0; i<27; ++i)
			ifs >> thrash;

		long unsigned start_stack, current_esp;
		ifs >> start_stack;
		ifs >> current_esp;
		stack_size = start_stack - current_esp; //because stack is expanding towards lower addresses
	}
}

void process::process_info::read_proc_statm(){
	ifstream ifs("/proc/self/statm", ifstream::in);

	//all values in /proc/self/statm are expressed in number of pages
	ifs >> virtual_mem_size;
	virtual_mem_size *= system::get_page_size();

	ifs >> rss;
	rss *= system::get_page_size();

	ifs >> shared_mem_size;
	shared_mem_size *= system::get_page_size();

	ifs >> code_size;
	code_size *= system::get_page_size();

	ifs >> lib_size;
	lib_size *= system::get_page_size();
}

}
