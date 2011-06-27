#ifndef MALLINFO_WRAPPER_H
#define MALLINFO_WRAPPER_H

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <ostream>
#include "common.h"

namespace mem_mon{

struct mallinfo_wrapper{
	struct mallinfo info;
	mallinfo_wrapper() : info(mallinfo()){}

	inline void update(){
		info = mallinfo();
	}

	friend std::ostream & operator<<(std::ostream &os, const mallinfo_wrapper & info);
};

}

#endif //MALLINFO_WRAPPER_H
