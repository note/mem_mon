#ifndef MEM_MON_H
#define MEM_MON_H

#include <mem_mon/process.h>
#include <mem_mon/system.h>
#include <malloc.h>


namespace mem_mon{

static void my_init_hook(void){
	__malloc_hook = malloc_hooker::malloc_notify;
	__realloc_hook = malloc_hooker::realloc_notify;
	__free_hook = malloc_hooker::free_notify;
}

}

#endif //MEM_MON_H
