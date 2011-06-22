#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

void handler(int signo){
	open("signal_handler_open", 0);
}

int main(int argc, const char *argv[]){
	signal(SIGTSTP, handler);
	
	int n;
	cin >> n;

	return 0;
}
