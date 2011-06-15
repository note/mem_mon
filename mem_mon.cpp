#include "mem_mon.h"
#include <fstream>
#include <string>
#include <iostream>
#include <stdlib.h>

using namespace std;
const int max_line = 256;

int get_from_proc_meminfo(const string & name){
	ifstream ifs("/proc/meminfo", ifstream::in);
	char buf[max_line];
	while (ifs.good()){
		ifs.getline(buf, max_line);
		if(string(buf).find(name) != string::npos){
			return atoi(&(buf[name.size()]));
		}
	}
	return -1;

}

int get_total_system_memory(){
	return get_from_proc_meminfo("MemTotal:");
}

int get_free_system_memory(){
	return get_from_proc_meminfo("MemFree:");
}

int get_total_swap_size(){
	return get_from_proc_meminfo("SwapTotal:");
}

int get_free_swap_size(){
	return get_from_proc_meminfo("SwapFree:");
}

int main(int argc, const char *argv[]){
	cout << get_total_system_memory() << endl;
	cout << get_total_swap_size() << endl;
	return 0;
}
