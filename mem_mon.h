#ifndef MEM_MON_H
#define MEM_MON_H

#include <stdio.h>
#include <unistd.h>

/*
 * Returns size of page in bytes
 */
inline int get_page_size(){
	return sysconf(_SC_PAGESIZE);
}


int get_reserved_pages_number();

/*
 * Returns heap size (memory reserved with brk)
 */
int get_heap_size();

/*
 * It can works only with known allocators
 */
int get_heap_in_use_size();

int get_stack_limit();
int get_stack_size();
int get_stack_in_use_size();

int get_mapped_memory_size();
int get_shared_memory_size();

/*
 * Returns the same as get_reserved_memory() + get_shared_memory() + get_mapped_memory()
 */
int get_total_memory();

/*
 * Returns total amount of free and used physical memory in the system. Returned amount is in kB.
 */
int get_total_system_memory();

/*
 * Returns the amound of physical RAM unused by the system.
 */
int get_free_system_memory();

/*
 * Returns total amount of physical swap memory.
 */
int get_total_swap_size();

/*
 * Returns total amound of swap memory free.
 */
int get_free_swap_size();
//todo add other function that informs about system memory

int get_malloc_calls_number();
int get_realloc_calls_number();
int get_free_calls_number();

#endif //MEM_MON_H
