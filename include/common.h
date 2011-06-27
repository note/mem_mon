#ifndef COMMOH_H
#define COMMON_H

#include <string>
#include <sstream>
#include <string.h>
#include <vector>
#include <sys/ptrace.h>

namespace mem_mon{
	typedef unsigned int uint;
	namespace consts{
		const int max_line = 256;
	}

	template<class T>
	std::string str(const T & n){
		std::stringstream sstm;
		sstm << n;
		return sstm.str();
	}

	template<class T>
	void load_struct(T * obj, const pid_t pid, int addr){
		size_t i = sizeof(T);
		while(i>0){
			int in = ptrace(PTRACE_PEEKDATA, pid, (void *) addr, NULL);
			memcpy(obj, (void *) &in, (i>=sizeof(int)) ? sizeof(int) : i);
			obj += sizeof(int);
			addr += sizeof(int);
			i -= sizeof(int);
		}
	}

	std::string load_string(const pid_t pid, int addr);
	std::vector<std::string> split(const std::string & str, const std::string & separator);
	
	template<class T>
	inline std::string format(const T size){
		if(size < 1024)
			return str(size) + "B";
		if(size < 1048576) // 1024*1024=1048576
			return str(size/1024) + "KB";
		return str(size/(1024*1024)) + "MB";
	}
	
	//actually it's kernel structue but I couldn't find its header file. I copied it from:
	//http://lxr.free-electrons.com/source/mm/mmap.c#L1132
	struct mmap_arg_struct {
		unsigned long addr;
		unsigned long len;
		unsigned long prot;
		unsigned long flags;
		unsigned long fd;
		unsigned long offset;
	};
}

#endif //COMMON_H
