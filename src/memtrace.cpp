#include <sys/types.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/user.h>
#include "common.h"
#include <map>

using namespace std;
using namespace mem_mon;

map<int, string> descriptors; //threads share descriptors table

//see section error return: http://www.win.tue.nl/~aeb/linux/lk/lk-4.html
bool is_ok(const int status){
	return status>-1 || status<-125;
}

class thread;

class syscall_observer{
	protected:
	thread * observed_thread;
	public:
	syscall_observer(thread * observed_thread) : observed_thread(observed_thread){}
	virtual void handle_enter(user_regs_struct * regs) = 0;
	virtual void handle_return(user_regs_struct * regs) = 0;
	virtual ~syscall_observer(){}
};

class thread{
	public:
	thread(pid_t pid);
	thread(){}
	void received(user_regs_struct * regs);
	bool returning;
	syscall_observer * observer;
	pid_t id;
	int current_break;
};


/********** syscall_observers **********/

class mmap_pgoff_observer : public syscall_observer{
	public:
	mmap_pgoff_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		filename = descriptors[regs->edi];
		cout << "#" << regs->edi << endl;
		cout << "$$" << filename << endl;
		cout << "Process is trying to map " << format(regs->ecx) << endl;
	}

	void handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax)){
			if(filename.find("/dev/shm/") == 0)
				cout << "Process has mapped " << format(regs->ecx) << " from shared memory" << endl;
			else
				cout << "Process has mapped " << format(regs->ecx) << " from the file " << filename << endl;
		}else{
			if(filename.find("/dev/shm/") == 0)
				cout << "Process has failed to map " << format(regs->ecx) << " from shared memory" << endl;
			else
				cout << "Process has failed to map " << format(regs->ecx) << " from the file " << filename << endl;
		}

	}

	private:
	string filename;
};

class brk_observer : public syscall_observer{
	public:
	brk_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		int diff = regs->ebx - observed_thread->current_break;
		if(regs->ebx){ //process is trying to change break
			if(diff>0)
				cout << "Process is trying to allocate " << format(diff) << endl;
			else
				cout << "Process is trying to deallocate " << format(-diff) << endl;
		}
	}
	
	void handle_return(user_regs_struct * regs){
		int diff = regs->eax-observed_thread->current_break;
		if(observed_thread->current_break != -1 && diff!=0){
			if(diff>0)
				cout << "Process has allocated " << format(diff) << endl;
			else
				cout << "Process has deallocated " << format(-diff) << endl;
		}
		observed_thread->current_break = regs->eax;

	}
};

class open_observer : public syscall_observer{
	string filename;
	public:
	open_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		filename = load_string(observed_thread->id, regs->ebx);
	}

	void handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax))
			descriptors[regs->eax] = filename;
	}

};

class old_mmap_observer : public syscall_observer{
	mmap_arg_struct mmap_arg;
	public:
	old_mmap_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		load_struct(&mmap_arg, observed_thread->id, regs->ebx);
		cout << "Process is trying to map " << format(mmap_arg.len) << endl;
	}

	void handle_return(user_regs_struct * regs){
		if(regs->eax != -1)
			cout << "Process has mapped " << format(mmap_arg.len) << endl;
	}
};

class munmap_observer : public syscall_observer{
	public:
	munmap_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		cout << "Process is trying to unmap " << format(regs->ecx) << endl;
	}

	void handle_return(user_regs_struct * regs){
		if(regs->eax == 0)
			cout << "Process has unmapped " << format(regs->ecx) << endl;
	}
};

/********** end of syscall_observers **********/

thread::thread(pid_t pid) : returning(false), observer(NULL), id(pid), current_break(-1) {}

void thread::received(user_regs_struct * regs){
	if(!returning){
		switch(regs->orig_eax){
			case 5:
				observer = new open_observer(this);
			break;
			case 45:
				observer = new brk_observer(this);
			break;
			case 90:
				observer = new old_mmap_observer(this);
			break;
			case 91:
				observer = new munmap_observer(this);
			break;
			case 192:
				observer = new mmap_pgoff_observer(this);
			break;
		}
		if(observer)
			observer->handle_enter(regs);
	}else{
		if(observer){
			observer->handle_return(regs);
			delete observer;
			observer = NULL;
		}
	}

	returning = !returning;
}

map<int, thread> threads;
typedef unsigned long long ull;

#define M_OFFSETOF(STRUCT, ELEMENT) \
	(unsigned int) &((STRUCT *)NULL)->ELEMENT;

void exit_with_msg(const char * msg){
	cout << msg << endl;
	exit(1);
}

void exit_with_perror(const char * msg){
	perror(msg);
	exit(1);
}

void view_help(){
	cout << "mem_mon" << endl << "Copyright Michal Sitko" << endl;
	exit(0);
}

void print(user_regs_struct * regs){
	cout << "eax" << regs->eax << endl << "ebx" << regs->ebx << endl << "ecx" << regs->ecx << endl << "edx" << regs->edx << endl << "esi" << regs->esi << endl << "edi" << regs->edi << endl;
}

int main(int argc, const char *argv[]){
	if(argc < 2)
		exit_with_msg("Proper invocation: ./memtrace command [args].\nFor more informations: ./memtrace -h");
	
	bool stepping_mode = false;
	int command_index = 1;

	if(argv[1][0] == '-'){
		string s(argv[1]);
		if(s.find("h") != string::npos)
			view_help();
		if(s.find("s") != string::npos)
			stepping_mode = true;

		if(argc < 3)
			exit_with_msg("Proper invocation: ./memtrace command [args].\nFor more informations: ./memtrace -h");
		command_index = 2;
	}
	
	pid_t id;
	if((id = vfork()) == -1)
		exit_with_perror("vfork error");
	else if(id == 0){
        	ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		const char ** args = new const char*[argc-command_index];
		for(int i=0; i<argc-command_index; ++i)
			args[i] = argv[i+command_index];
		execvp(argv[command_index], (char * const *) args);
		exit_with_perror("execvp error");
	}else{
		wait(NULL);

		thread th(id);
		threads[id] = th;

    		user_regs_struct regs;
		if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
			perror("ptrace_syscall");

		wait(NULL);

		int res = ptrace(PTRACE_GETREGS, id, NULL, &regs);
		bool returning = false;
		int status;
		string str;
		while(res != -1){
			ull eax_offset = M_OFFSETOF(struct user, regs.orig_eax);
			ull eax = ptrace(PTRACE_PEEKUSER, id, eax_offset, NULL);
			cout << "EAX: " << eax << endl;
			
			threads[id].received(&regs);

			if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
				perror("ptrace_syscall");

			wait(&status);
			if(WIFEXITED(status))
				break;

			res = ptrace(PTRACE_GETREGS, id, NULL, &regs);
		}

	}

	return 0;
}
