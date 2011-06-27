#include "common.h"

namespace mem_mon{
using namespace std;

string load_string(const pid_t pid, int addr){
	int in, tmp;
	char buf[sizeof(int)];
	string res;

	do{
		tmp = res.length();
		in = ptrace(PTRACE_PEEKDATA, pid, (void *) addr, NULL);
		strncpy(buf, (char *) &in, sizeof(int));
		res.append(buf, 0, sizeof(int));
		addr += sizeof(int);
		memset(buf, 0, sizeof(int));
	}while(res.length()-tmp == sizeof(int));

	return res;
}

vector<string> split(const string & str, const string & separator){
	vector<string> res;
	string tmp(str);
	int pos = tmp.find(separator);
	while(pos != string::npos){
		if(pos != 0)
			res.push_back(tmp.substr(0, pos));

		if(pos+1 == tmp.length())
			return res;
		tmp = tmp.substr(pos+1);
		pos = tmp.find(separator);
	}
	res.push_back(tmp);
	return res;
}


}
