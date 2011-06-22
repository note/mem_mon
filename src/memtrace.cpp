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
map<int, string> descriptors;
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

//see section error return: http://www.win.tue.nl/~aeb/linux/lk/lk-4.html
bool is_ok(const int status){
	return status>-1 || status<-125;
}

void print(user_regs_struct * regs){
	cout << "eax" << regs->eax << endl << "ebx" << regs->ebx << endl << "ecx" << regs->ecx << endl << "edx" << regs->edx << endl << "esi" << regs->esi << endl << "edi" << regs->edi << endl;
}

void handle_mmap_pgoff(user_regs_struct * regs, bool returning){
	static string filename;
	if(!returning){
		filename = descriptors[regs->edi];
		cout << "#" << regs->edi << endl;
		cout << "$$" << filename << endl;
		cout << "Process is trying to map " << format(regs->ecx) << endl;
	}else{
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

    		user_regs_struct regs;
		if(ptrace(PTRACE_SYSCALL, id, NULL, NULL) == -1)
			perror("ptrace_syscall");

		wait(NULL);

		int res = ptrace(PTRACE_GETREGS, id, NULL, &regs);
		bool returning = false;
		int current_break = -1, status;
		string str;
		while(res != -1){

			ull eax_offset = M_OFFSETOF(struct user, regs.orig_eax);
			ull start_offset = M_OFFSETOF(struct user, start_stack);
			ull eax = ptrace(PTRACE_PEEKUSER, id, eax_offset, NULL);
			ull start = ptrace(PTRACE_PEEKUSER, id, start_offset, NULL);
			cout << "EAX: " << eax << endl;
			cout << "stack: " << start << endl;


			char in[consts::max_line];
			//cout << regs.orig_eax << endl;
			switch(regs.orig_eax){
				case 5:
				if(!returning)
					str = load_string(id, regs.ebx);
				else{
					//print(&regs);
					cout << str << endl;
					if(regs.eax > 0)
						descriptors[regs.eax] = str;
				}
				break;
				case 45:

				if(returning){
					int diff = regs.eax-current_break;
					if(current_break != -1 && diff!=0){
						if(diff>0)
							cout << "Process has allocated " << format(diff) << endl;
						else
							cout << "Process has deallocated " << format(-diff) << endl;
					}
					current_break = regs.eax;
				}else{
					int diff = regs.ebx-current_break;
					if(regs.ebx){ //process is trying to change break
						if(diff>0)
							cout << "Process is trying to allocate " << format(diff) << endl;
						else
							cout << "Process is trying to deallocate " << format(-diff) << endl;
					}else //process just checks current break
						break;
				}
				if(stepping_mode)
					cin.getline(in, consts::max_line);
				break;

				case 90:
				mmap_arg_struct mmap_arg;
				if(!returning){
					load_struct(&mmap_arg, id, regs.ebx);
					cout << "Process is trying to map " << format(mmap_arg.len) << endl;
				}else{
					if(regs.eax != -1)
						cout << "Process has mapped " << format(mmap_arg.len) << endl;
				}
				break;

				case 91:
				if(!returning){
					cout << "Process is trying to unmap " << format(regs.ecx) << endl;
				}else{
					if(regs.eax == 0)
						cout << "Process has unmapped " << format(regs.ecx) << endl;
				}
				break;

				case 192:
				handle_mmap_pgoff(&regs, returning);
				break;

			}

			returning = !returning;

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
