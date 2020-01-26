#include "include/platform_abstraction.H"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <stdexcept>
#include <errno.h>
#include <signal.h>

using namespace std;

// disable SIGHUP and SIGPIPE
void xavna_disableSignals() {
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; // Restart system calls if interrupted by handler
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	sa.sa_handler = SIG_DFL;
	sigaction(SIGCONT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
}

vector<string> xavna_find_devices() {
	vector<string> ret;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir ("/dev")) == NULL)
		throw runtime_error("xavna_find_devices: could not list /dev: " + 
							string(strerror(errno)));
	
	/* print all the files and directories within directory */
	while ((ent = readdir (dir)) != NULL) {
		string name = ent->d_name;
		if(name.find("ttyACM")==0)
			ret.push_back("/dev/"+name);
		if(name.find("cu.usbmodem")==0)
			ret.push_back("/dev/"+name);
	}
	closedir (dir);
	return ret;
}

int xavna_open_serial(const char* path) {
	xavna_disableSignals();
	int fd = open(path,O_RDWR);
	if(fd<0) return fd;
	struct termios tc;
	/* Set TTY mode. */
	if (tcgetattr(fd, &tc) < 0) {
		perror("tcgetattr");
		return fd;
	}
	/*tc.c_iflag &= ~(INLCR|IGNCR|ICRNL|IGNBRK|IUCLC|INPCK|ISTRIP|IXON|IXOFF|IXANY);
	tc.c_oflag &= ~OPOST;
	tc.c_cflag &= ~(CSIZE|CSTOPB|PARENB|PARODD|CRTSCTS);
	tc.c_cflag |= CS8 | CREAD | CLOCAL;
	tc.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG|IEXTEN);*/
	
	tc.c_iflag = 0;
	tc.c_oflag = 0;
	tc.c_lflag = 0;
	tc.c_cflag = CS8 | CREAD | CLOCAL;
	tc.c_cc[VMIN] = 1;
	tc.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &tc);
	return fd;
}

void xavna_drainfd(int fd) {
	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	while(poll(&pfd,1,0)>0) {
		if(pfd.revents & POLLRDHUP) break;
		if(pfd.revents & POLLERR) break;
		if(pfd.revents & POLLHUP) break;
		if(pfd.revents & POLLNVAL) break;
		if(!(pfd.revents & POLLIN)) continue;
		char buf[4096];
		if(read(fd,buf,sizeof(buf)) <= 0) break;
	}
}


bool xavna_detect_autosweep(int fd) {
	pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	if(poll(&pfd,1,100) == 0) {
		// no data was received => autosweep device
		return true;
	}
	return false;
}
