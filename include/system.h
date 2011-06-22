#ifndef SYSTEM_H
#define SYSTEM_H

#include "system.h"
#include <unistd.h>
#include <string>

namespace mem_mon{

	class system{
		typedef unsigned int uint;
		typedef unsigned long long ull;
		struct system_info{
			ull total_ram, free_ram, total_swap, free_swap;
		};
		
		public:
		static system& get_instance();
		void update();
	
		/*
		 * Returns size of page in bytes
		 */
		inline int get_page_size() const{
			return sysconf(_SC_PAGESIZE);
		}

		/*
		 * Returns total amount of free and used physical memory in the system. Returned amount is in kB.
		 */
		ull get_total_ram() const{
			return res.total_ram;
		}

		ull get_used_ram() const{
			return res.total_ram-res.free_ram;
		}

		/*
		 * Returns the amount of physical RAM unused by the system.
		 */
		ull get_free_ram() const{
			return res.free_ram;
		}

		/*
		 * Returns total amount of physical swap memory.
		 */
		ull get_total_swap() const{
			return res.total_swap;
		}

		ull get_used_swap() const{
			return res.total_swap - res.free_swap;
		}

		/*
		 * Returns total amount of swap memory free.
		 */
		ull get_free_swap() const{
			return res.free_swap;
		}

		//todo add other function that informs about system memory

		
		friend std::ostream & operator<<(std::ostream &os, const system & sys);
		private:
		system_info res;	

		static system * instance;
		system(){}

		void get_from_proc_meminfo(system_info & inf);

	};
}

#endif //SYSTEM_H
