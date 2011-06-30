#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/user.h>
#include "mem_mon.h"
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

template <class T>
struct mixin{
	static int get_calls_amount(){
		return calls_amount;
	}

	protected:
	static int calls_amount;
	static void increase(){
		++calls_amount;
	}
};
template <class T> int mixin<T>::calls_amount = 0;

class syscall_observer{
	protected:
	thread * observed_thread;
	public:
	syscall_observer(thread * observed_thread) : observed_thread(observed_thread){}
	virtual bool handle_enter(user_regs_struct * regs) = 0;
	virtual bool handle_return(user_regs_struct * regs) = 0;
	virtual ~syscall_observer(){}
};

class thread{
	public:
	thread(pid_t pid);
	thread(){}
	bool received(user_regs_struct * regs);
	bool returning;
	syscall_observer * observer;
	pid_t id;
	int current_break;
	bool active;
	void print_notification(const string & msg);
	void print_mem_info(){
		proc.update();
		cerr << proc;
	}
	process proc;
};

/********** syscall_observers **********/

class mmap_pgoff_observer : public syscall_observer, public mixin<mmap_pgoff_observer>{
	int size;
	int orig_esi;
	public:
	mmap_pgoff_observer(thread * observed_thread) : syscall_observer(observed_thread){}

	bool handle_enter(user_regs_struct * regs){
		increase();
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
		return true;
	}

	bool handle_return(user_regs_struct * regs){
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
		return true;
	}

	private:
	string filename;
};

class brk_observer : public syscall_observer, public mixin<brk_observer>{
	static int allocs_amount;
	static int deallocs_amount;
	public:
	brk_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	
	bool handle_enter(user_regs_struct * regs){
		increase();
		int diff = regs->ebx - observed_thread->current_break;
		if(regs->ebx){ //process is trying to change break
			if(diff>0)
				observed_thread->print_notification("Process is trying to allocate " + format(diff));
			else
				observed_thread->print_notification("Process is trying to deallocate " + format(-diff));
			return true;
		}
		return false;
	}
	
	bool handle_return(user_regs_struct * regs){
		int diff = regs->eax-observed_thread->current_break;
		if(observed_thread->current_break != -1 && diff!=0){
			if(diff>0){
				++allocs_amount;
				observed_thread->print_notification("Process has allocated " + format(diff));
			}else{
				++deallocs_amount;
				observed_thread->print_notification("Process has deallocated " + format(-diff));
			}
			observed_thread->current_break += diff;
			return true;
		}
		observed_thread->current_break = regs->eax;
		return false;
	}

	static int get_extending_amount(){
		return brk_observer::allocs_amount;
	}

	static int get_reducing_amount(){
		return brk_observer::deallocs_amount;;
	}
};
int brk_observer::allocs_amount = 0, brk_observer::deallocs_amount = 0;

class open_observer : public syscall_observer, mixin<open_observer>{
	string filename;
	public:

	open_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	bool handle_enter(user_regs_struct * regs){
		increase();
		filename = load_string(observed_thread->id, regs->ebx);
		return false;
	}

	bool handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax))
			descriptors[regs->eax] = filename;
		return false;
	}
};

class old_mmap_observer : public syscall_observer, public mixin<old_mmap_observer>{
	mmap_arg_struct mmap_arg;
	static int calls_amount;
	public:

	old_mmap_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	bool handle_enter(user_regs_struct * regs){
		increase();
		load_struct(&mmap_arg, observed_thread->id, regs->ebx);
		if(descriptors[mmap_arg.fd].find("/dev/shm/") == 0)
			observed_thread->print_notification("Process is trying to map " + format(mmap_arg.len) + " from shared memory");
		else
			observed_thread->print_notification("Process is trying to map " + format(mmap_arg.len) + " from the file " + descriptors[mmap_arg.fd]);
		return true;
	}

	bool handle_return(user_regs_struct * regs){
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
		return true;
	}
};

class munmap_observer : public syscall_observer, public mixin<munmap_observer>{
	static int calls_amount;
	public:
	
	munmap_observer(thread * observed_thread) : syscall_observer(observed_thread){}
	bool handle_enter(user_regs_struct * regs){
		increase();
		observed_thread->print_notification("Process is trying to unmap " + format(regs->ecx));
		return true;
	}

	bool handle_return(user_regs_struct * regs){
		if(is_ok(regs->eax))
			observed_thread->print_notification("Process has unmapped " + format(regs->ecx));
		return true;
	}
};

/********** end of syscall_observers **********/

map<int, thread *> threads;

thread::thread(pid_t pid) : returning(false), observer(NULL), id(pid), current_break(-1), active(false), proc(pid) {}

/**
 * @return true if any action that user should be notified about was performed
 */
bool thread::received(user_regs_struct * regs){
	bool ret = false;
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
			ret = observer->handle_enter(regs);
	}else{
		if(observer){
			ret = observer->handle_return(regs);
			delete observer;
			observer = NULL;
		}
	}

	returning = !returning;
	return ret;
}

void thread::print_notification(const string & notification){
	if(threads.size() > 1)
		cerr << "[pid: " << id << "] " << notification << endl;
	else
		cerr << notification << endl;
}

typedef unsigned long long ull;

#define M_OFFSETOF(STRUCT, ELEMENT) \
	(unsigned int) &((STRUCT *)NULL)->ELEMENT;

void exit_with_msg(const char * msg){
	cout << msg << endl;
	exit(1);
}

void exit_with_msg(const string & msg){
	cout << msg << endl;
	exit(1);
}

void exit_with_perror(const char * msg){
	perror(msg);
	exit(1);
}

void view_console_help(){
	cout << "memtrace console commands:" << endl << " next (abbraviated: n) - step execution" << endl << " system (sys s) - system memory info" << endl <<
	" process (proc p) [id] - process memory info. Id specifies which thread memory checks. If no id specified check main thread." << endl << 
	" continue (cont c) - exit stopping mode, continue tracing, but not in stepping mode" << endl <<
	" detach (d) - stop tracing mode, process which is observed will continue execution" << endl << " quit (q) - exit memtrace and process which is observed" << endl <<
	" number (num) - print number of each observed syscall" << endl <<
	" help (h) - display help information" << endl;
}

void view_help(){
	cout << "memtrace" << endl << "Copyright Michal Sitko" << endl << endl;
	cout << "usage: memtrace [-hsfo] command [arg ...]" << endl << "-h display help information" << endl
	<< "-s stepping mode, memtrace will stop executing and enter memtrace console after calling and returning every syscall" << endl << "-f follow threads created by process being observed" << endl
	<< "-o file - send memtrace output to file instead of stderr" << endl
	<< "-n at the end print number of each observed syscalls" << endl << endl;
	view_console_help();
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
	threads[parent_id]->print_notification("Started new thread with id: " + str(child_id));
	return (pid_t) child_id;
}

void handle_exiting(pid_t id){
	int status;
	ptrace(PTRACE_GETEVENTMSG, id, NULL, &status);
	threads[id]->print_notification("Process is exiting with status: " + str(status));
}

void print_syscall_stats(){
	cerr << "*** syscalls statistics: ***" << endl;
	cerr << "jrk: " << brk_observer::get_calls_amount() << " calls, amongst them:" << endl;
	cerr << "  " << brk_observer::get_extending_amount() << " extending" << endl;
	cerr << "  " << brk_observer::get_reducing_amount() << " reducing" << endl;
	cerr << "mmap_pgoff: " << mmap_pgoff_observer::get_calls_amount() << " calls" << endl;
	cerr << "old_mmap: " << old_mmap_observer::get_calls_amount() << " calls" << endl;
	cerr << "munmap: " << munmap_observer::get_calls_amount() << " calls" << endl;
}

/**
 * @param [in] main_thread_id main thread id
 * @param [out] stepping_mode flag which determine whether memtrace runs in stepping mode
 */
void console_loop(pid_t main_thread_id, bool * stepping_mode, bool show_stats){
	string in;
	mem_mon::system sys = mem_mon::system::get_instance();
	while(1){
		cout << "(memtrace) ";
		cin >> in;
		
		if(in == "help" || in == "h"){
			view_console_help();
			continue;
		}

		if(in == "system" || in == "sys" || in == "s"){
			sys.update();
			cout << sys;
			continue;
		}

		if(in == "continue" || in == "cont" || in == "c"){
			*stepping_mode = false;
			break;
		}

		if(in == "detach" || in == "d"){
			ptrace(PTRACE_DETACH, main_thread_id, NULL, NULL); //todo: do i have to detach all threads manually? maybe user should be able to specify pid?
			if(show_stats)
				print_syscall_stats();
			continue;
		}

		if(in == "next" || in == "n")
			break;

		if(in == "number" || in == "num")
			print_syscall_stats();

		if(in == "quit" || in =="q"){
			kill(main_thread_id, SIGINT);
			if(show_stats)
				print_syscall_stats();
			exit(0);
		}

		vector<string> cmds = split(in, " ");
		if(cmds[0] == "process" || cmds[0] == "proc" || cmds[0] == "p"){
			int tmp_id;
			if(cmds.size() > 1){
				tmp_id = atoi(cmds[1].c_str());
				if(threads.find(tmp_id) == threads.end()){
					cout << "Thread with id " + cmds[1] + " does not exist" << endl;
					continue;
				}
			}else
				tmp_id = main_thread_id;
			
			threads[tmp_id]->print_mem_info();
			continue;
		}

		cout << "Undefinded command: \"" << in << "\". Try \"help\"" << endl;
	}
}


int main(int argc, const char *argv[]){
	if(argc < 2)
		exit_with_msg("Proper invocation: ./memtrace command [args].\nFor more information: ./memtrace -h");
	
	bool stepping_mode = false, follow_threads = false, show_stats;
	int command_index = 1;
	
	int i = 1;
	while(i<argc){
		if(argv[i][0] == '-'){
			string s(argv[i]);
			if(s.find("h") != string::npos)
				view_help(); // view_help does not return

			if(i == argc-1)
				exit_with_msg("!Proper invocation: ./memtrace command [args].\nFor more information: ./memtrace -h");

			if(s.find("s") != string::npos)
				stepping_mode = true;

			if(s.find("f") != string::npos)
				follow_threads = true;

			if(s.find("n") != string::npos)
				show_stats = true;

			if(s.find("o") != string::npos){
				if(i >= argc-2)
					exit_with_msg("You must specify filename when using -o option");
				int fd;
				if((fd = open(argv[i+1], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
					perror("open error");
					exit_with_msg("Cannot open file " + string(argv[i+1]));
				}
				if(dup2(fd, 2) == -1){ // redirect stderr
					perror("dup2 error");
					exit_with_msg("Cannot redirect output to file " + string(argv[i+1]));
				}
				i += 2; // add 2 to i, because we omit next argument, which is filename
			}else
				++i;
			
			command_index = i;
		}else
			break;
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
		
		pid_t main_thread_id = id;
		threads[id] = new thread(id);
		threads[id]->active = true;

    		user_regs_struct regs;

		if(follow_threads)
			ptrace(PTRACE_SETOPTIONS, id, NULL, PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXIT);
		else
			ptrace(PTRACE_SETOPTIONS, id, NULL, PTRACE_O_TRACEEXIT);

 		if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
 			perror("ptrace_syscall");
		

		int status;
		bool console = false;
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
				if(follow_threads && status>>16 == PTRACE_EVENT_CLONE)
					child_id = handle_child(id);

				console = threads[id]->received(&regs); //console will be true if thread::received() does something that user should be informed about
			}else{ // real signal was delivered
				if(siginfo.si_signo == 19 && !threads[id]->active) // new thread is initially stopped with a SIGSTOP, there's no sense in displaying info about this
					threads[id]->active = true;
				else
					threads[id]->print_notification("Received signal number " + str(siginfo.si_signo));
			}

			if(console && stepping_mode)
				console_loop(main_thread_id, &stepping_mode, show_stats);
			
			if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
				perror("ptrace_syscall");
		}

	}

	if(show_stats)
		print_syscall_stats();

	return 0;
}
