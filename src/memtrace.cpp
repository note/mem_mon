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
#include <sys/mman.h>

using namespace std;
using namespace mem_mon;

map<int, string> descriptors; //threads share descriptors table

bool is_ok(const int status){
	//Here the range [-125,-1] is reserved for errors (the constant 125 is version and architecture dependent) and other values are OK.
	//from: http://www.win.tue.nl/~aeb/linux/lk/lk-4.html (see error return section)
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
	bool active;
	void print_notification(const string & msg);
};

/********** syscall_observers **********/

class mmap_pgoff_observer : public syscall_observer{
	int size;
	int orig_esi;
	public:
	mmap_pgoff_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		size = regs->ecx;
		filename = descriptors[regs->edi];
		string msg;
		if(regs->esi & MAP_PRIVATE)
			msg = "Process is trying to create private mapping of size " + format(size);
		else
			msg = "Process is trying to create shared mapping of size " +format(size);
		
		msg += (regs->esi & MAP_ANONYMOUS) ? " (anonymous)" : " from the file " + filename;
		observed_thread->print_notification(msg);
		orig_esi = regs->esi;
	}

	void handle_return(user_regs_struct * regs){
		string msg;
		if(is_ok(regs->eax)){
			if(regs->esi & MAP_PRIVATE)
				msg = "Process has created private mapping of size " + format(size);
			else
				msg = "Process has created shared mapping of size " + format(size);
		}else{
			if(orig_esi & MAP_PRIVATE)
				msg = "Process has failed to create private mapping of size " + format(size);
			else
				msg = "Process has failed to create shared mapping of size " + format(size);
		}
			
		msg += (orig_esi & MAP_ANONYMOUS) ? " (anonymous)" : " from the file " + filename;
		observed_thread->print_notification(msg);
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
				observed_thread->print_notification("Process is trying to allocate " + format(diff));
			else
				observed_thread->print_notification("Process is trying to deallocate " + format(-diff));
		}
	}
	
	void handle_return(user_regs_struct * regs){
		int diff = regs->eax-observed_thread->current_break;
		if(observed_thread->current_break != -1 && diff!=0){
			if(diff>0)
				observed_thread->print_notification("Process has allocated " + format(diff));
			else
				observed_thread->print_notification("Process has deallocated " + format(-diff));
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
		if(descriptors[mmap_arg.fd].find("/dev/shm/") == 0)
			observed_thread->print_notification("Process is trying to map " + format(mmap_arg.len) + " from shared memory");
		else
			observed_thread->print_notification("Process is trying to map " + format(mmap_arg.len) + " from the file " + descriptors[mmap_arg.fd]);
	}

	void handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax)){
			if(descriptors[mmap_arg.fd].find("/dev/shm/") == 0)
				observed_thread->print_notification("Process has mapped " + format(mmap_arg.len) + " from shared memory");
			else
				observed_thread->print_notification("Process has mapped " + format(mmap_arg.len) + " from the file " + descriptors[mmap_arg.fd]);
		}else{
			if(descriptors[mmap_arg.fd].find("/dev/shm/") == 0)
				observed_thread->print_notification("Process has failed to map " + format(mmap_arg.len) + " from shared memory");
			else
				observed_thread->print_notification("Process has failed " + format(mmap_arg.len) + " from the file " + descriptors[mmap_arg.fd]);
		}
	}
};

class munmap_observer : public syscall_observer{
	public:
	munmap_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	void handle_enter(user_regs_struct * regs){
		observed_thread->print_notification("Process is trying to unmap " + format(regs->ecx));
	}

	void handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax))
			observed_thread->print_notification("Process has unmapped " + format(regs->ecx));
	}
};

/********** end of syscall_observers **********/

map<int, thread *> threads;

thread::thread(pid_t pid) : returning(false), observer(NULL), id(pid), current_break(-1), active(false) {}

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

void thread::print_notification(const string & notification){
	if(threads.size() > 1)
		cout << "[pid: " << id << "] " << notification << endl;
	else
		cout << notification << endl;
}

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

/**
 * @return pid of handled child
 */
pid_t handle_child(pid_t parent_id){
	unsigned long child_id;
	ptrace(PTRACE_GETEVENTMSG, parent_id, NULL, &child_id);
	threads[child_id] = new thread(child_id);
	//if(ptrace(PTRACE_ATTACH, (pid_t) child_id, NULL, NULL) == -1)
	//	perror("ptrace_attach");
	threads[parent_id]->print_notification("Started new thread with id: " + str(child_id));
	return (pid_t) child_id;
}

void handle_exiting(pid_t id){
	int status;
	ptrace(PTRACE_GETEVENTMSG, id, NULL, &status);
	threads[id]->print_notification("Process is exiting with status: " + str(status));
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

		threads[id] = new thread(id);

    		user_regs_struct regs;
		ptrace(PTRACE_SETOPTIONS, id, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXIT);

 		if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
 			perror("ptrace_syscall");
		
		int status;
		pid_t child_id;
		while(1){ // main loop
			//cout << "MAIN LOOP" << endl;
			id = waitpid(-1, &status, __WALL); // you must use the __WALL option inorder to receive notifications from threads created by the child process
			
			if(threads.find(id) == threads.end())
				threads[id] = new thread(id);

			if(status>>16 == PTRACE_EVENT_EXIT)
				handle_exiting(id);

			if(WIFEXITED(status)){
				threads[id]->print_notification("Process has exited with status:" + str(status));
				threads.erase(id);
				if(threads.empty())
					break;
				continue;
			}
			
			if(ptrace(PTRACE_GETREGS, id, NULL, &regs) == -1)
				exit_with_perror("ptrace_getregs error");
			//cout << "eax: " << regs.orig_eax << " id: " << id << endl;
			
			siginfo_t siginfo;
			ptrace(PTRACE_GETSIGINFO, id, NULL, &siginfo);
			child_id = -1;
			if(siginfo.si_signo == SIGTRAP){ // action caught by ptrace
				if(status>>16 == PTRACE_EVENT_CLONE)
					child_id = handle_child(id);

				threads[id]->received(&regs);
			}else{ // real signal was delivered
				if(siginfo.si_signo == 19 && !threads[id]->active) // new thread is initially stopped with a SIGSTOP, there's no sense in displaying info about this
					threads[id]->active = true;
				else
					threads[id]->print_notification("Received signal number " + str(siginfo.si_signo));
			}
			
			//if(child_id > 0)
			//	if(ptrace(PTRACE_SYSCALL, child_id, NULL, NULL) == -1)
			//		perror(string("ptrace_syscall2 " + str(child_id)).c_str());
			
			if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
				perror("ptrace_syscall");
		}

	}

	return 0;
}
