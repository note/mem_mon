#include "mallinfo_wrapper.h"

using namespace std;

namespace mem_mon{

ostream & operator<<(ostream &os, const mallinfo_wrapper & info){

	os << "Total space in arena: " << format(info.info.arena) << endl;
	os << "Number of ordinary blocks: " << info.info.ordblks << endl;
	os << "Number of small blocks: " << info.info.smblks << endl;
	os << "Space in holding block headers: " << info.info.hblkhd << endl;
	os << "Number of holding blocks: " << info.info.hblks << endl;
	os << "Space in small blocks in use: " << format(info.info.usmblks) << endl;
	os << "Space in free small blocks: " << format(info.info.fsmblks) << endl;
	os << "Space in ordinary blocks in use: " << format(info.info.uordblks) << endl;
	os << "Space in free ordinary blocks: " << format(info.info.fordblks) << endl;

	return os;
}

}
