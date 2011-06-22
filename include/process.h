#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <vector>

namespace mem_mon{
	class abstract_malloc_observer{
		public:
		virtual void * malloc_update(size_t size, const void * caller) = 0;
		virtual void * realloc_update(void * ptr, size_t size, const void * caller) = 0;
		virtual void free_update(void * ptr, const void * caller) = 0;
		virtual ~abstract_malloc_observer();
	};

	class abstract_timer{
		public:
		virtual void start() = 0;
		virtual void stop() = 0;
		virtual ~abstract_timer();
	};

	class malloc_hooker{
		static std::vector<abstract_malloc_observer *> observers;

		public:
		void static add_observer(abstract_malloc_observer * observer){
			observers.push_back(observer);
		}

		static void * malloc_notify(size_t size, const void * caller){
			for(size_t i=0; i<observers.size(); ++i)
				observers[i]->malloc_update(size, caller);
			return NULL;
		}

		static void * realloc_notify(void * ptr, size_t size, const void * caller){
			for(size_t i=0; i<observers.size(); ++i)
				observers[i]->realloc_update(ptr, size, caller);
			return NULL;
		}

		static void free_notify(void * ptr, const void * caller){
			for(size_t i=0; i<observers.size(); ++i)
				observers[i]->free_update(ptr, caller);
		}

		~malloc_hooker(){
			for(size_t i=0; i<observers.size(); ++i)
				delete observers[i];
		}
	};

	class process{
		struct process_info{
			int stack_limit, data_segment_limit;
			signed long rss;
			unsigned int shared_mem_size, code_size, lib_size;
			long unsigned virtual_mem_size, stack_size;

			void read_proc_stat();
			void read_proc_statm();
			void get_by_getrlimit();
		};

		public:
		process() : timer(NULL){}

		void update();
		
		int get_virtual_mem_size() const{
			return res.virtual_mem_size;
		}

		int get_rss() const{
			return res.rss;
		}

		int get_shared_mem_size() const{
			return res.shared_mem_size;
		}

		int get_code_size() const{
			return res.code_size;
		}

		int get_lib_size() const{
			return res.lib_size;
		}

		int get_mapped_mem_size() const;

		int get_stack_limit() const{
			return res.stack_limit;
		}

		/*
		 * Returns the maximum size of the process's data segment (initialized data, uninitalized data and heap).
		 */
		int get_data_segment_limit() const{
			return res.data_segment_limit;
		}

		/*
		 * It can works only with known allocators
		 */
		int get_heap_in_use_size() const;
		int get_stack_size() const{
			return res.stack_size;
		}

		int get_malloc_calls_number() const;
		int get_realloc_calls_number() const;
		int get_free_calls_number() const;

		void set_malloc_observer(abstract_malloc_observer * observer){
			malloc_hooker::add_observer(observer);
		}

		void set_timer(abstract_timer * timer){
			this->timer = timer;
		}

		abstract_timer * get_timer() const{
			return timer;
		}

		friend std::ostream & operator<<(std::ostream &os, const process & proc);

		private:
		process_info res;
		abstract_timer * timer;
	};
}

#endif //PROCESS_H
