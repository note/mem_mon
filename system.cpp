#include "system.h"
#include "common.h"
#include <fstream>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <sys/resource.h>

using namespace std;
namespace mem_mon{

system * system::instance = NULL;

system & system::get_instance(){
	if(instance == NULL)
		instance = new system();
	return *instance;
}

ostream & operator<<(std::ostream &os, const system & sys){
	os << "*** Output from mem_mon::system ***" << endl << "Page size: " << sys.get_page_size() << endl << "Total RAM: " << sys.get_total_ram() << endl << "RAM in use: " << sys.get_used_ram() << endl << "Free RAM: " << sys.get_free_ram() << endl << "Total swap: " << sys.get_total_swap() << endl << "Swap in use: " << sys.get_used_swap() << endl << "Free swap: " << sys.get_free_swap() << endl;
	os << "*** End of output from mem_mon::system ***" << endl;
	return os;
}

void system::get_from_proc_meminfo(system_info & info){
	ifstream ifs("/proc/meminfo", ifstream::in);
	char buf[consts::max_line];
	string mem_total("MemTotal:"), mem_free("MemFree:"), swap_total("SwapTotal:"), swap_free("SwapFree:");
	while (ifs.good()){
		ifs.getline(buf, consts::max_line);
		string line(buf);
		if(line.find(mem_total) != string::npos)
			info.total_ram = atoi(&(buf[mem_total.size()]));

		if(line.find(mem_free) != string::npos)
			info.free_ram = atoi(&(buf[mem_free.size()]));

		if(line.find(swap_total) != string::npos)
			info.total_swap = atoi(&(buf[swap_total.size()]));

		if(line.find(swap_free) != string::npos)
			info.free_swap = atoi(&(buf[swap_free.size()]));
	}
}

void system::update(){
	get_from_proc_meminfo(res);
}

}
