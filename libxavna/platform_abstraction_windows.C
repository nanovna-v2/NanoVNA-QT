#include "include/platform_abstraction.H"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <windows.h>

using namespace std;
vector<string> xavna_find_devices() {
    vector<string> ret;
    for(int i=0;i<256;i++)
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "\\\\.\\COM%i", i);

        HANDLE h = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if(h == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            if(err == ERROR_SHARING_VIOLATION || err == ERROR_ACCESS_DENIED ||
               err == ERROR_GEN_FAILURE || err == ERROR_SEM_TIMEOUT) {
                ret.push_back(buf);
            }
        } else {
            ret.push_back(buf);
            CloseHandle(h);
        }
    }
    return ret;
}

int xavna_open_serial(const char* path) {
    HANDLE hComm;
    hComm = CreateFile(path,                //port name
                  GENERIC_READ | GENERIC_WRITE, //Read/Write
                  0,                            // No Sharing
                  NULL,                         // No Security
                  OPEN_EXISTING,// Open existing port only
                  0,            // Non Overlapped I/O
                  NULL);        // Null for Comm Devices

    if (hComm == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error in opening serial port");
        return -1;
    }
    DCB dcbSerialParams = { 0 }; // Initializing DCB structure
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    GetCommState(hComm, &dcbSerialParams);
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fParity = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fAbortOnError = FALSE;
    dcbSerialParams.Parity = NOPARITY;
    SetCommState(hComm, &dcbSerialParams);

    return _open_osfhandle((intptr_t)hComm, 0);
}

void xavna_drainfd(int fd) {
    HANDLE hComm = (HANDLE) _get_osfhandle(fd);
    COMMTIMEOUTS comTimeOut = {};
    comTimeOut.ReadIntervalTimeout = MAXDWORD;
    comTimeOut.ReadTotalTimeoutMultiplier = MAXDWORD;
    comTimeOut.ReadTotalTimeoutConstant = 1;
    SetCommTimeouts(hComm,&comTimeOut);

    char buf[1024];
    read(fd, buf, sizeof(buf));

    comTimeOut.ReadTotalTimeoutConstant = 200;
    SetCommTimeouts(hComm,&comTimeOut);
}

bool xavna_detect_autosweep(int fd) {
    bool ret = true;
    HANDLE hComm = (HANDLE) _get_osfhandle(fd);
    COMMTIMEOUTS comTimeOut = {};
    comTimeOut.ReadIntervalTimeout = MAXDWORD;
    comTimeOut.ReadTotalTimeoutMultiplier = MAXDWORD;
    comTimeOut.ReadTotalTimeoutConstant = 300;
    SetCommTimeouts(hComm,&comTimeOut);

    char buf[128];
    if(read(fd, buf, sizeof(buf)) > 0) {
        ret = false;
    }
    // no data was received => autosweep device

    comTimeOut.ReadTotalTimeoutConstant = 200;
    SetCommTimeouts(hComm,&comTimeOut);
    return ret;
}
