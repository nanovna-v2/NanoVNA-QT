#include <vector>
#include <string>
using namespace std;


// functions with platform specific implementation go here

vector<string> xavna_find_devices();
int xavna_open_serial(const char* path);
void xavna_drainfd(int fd);
bool xavna_detect_autosweep(int fd);
