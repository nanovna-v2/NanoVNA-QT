#ifndef FIRMWAREUPDATER_H
#define FIRMWAREUPDATER_H
#include <string>
#include <functional>
#include <pthread.h>
#include <cstdint>

using namespace std;

class FirmwareUpdater
{
public:
    FirmwareUpdater();
    ~FirmwareUpdater();

    // open a tty device. Throws logic_error if device is not in DFU mode.
    void open(const char *dev);
    void close();

    // upload firmware asynchronously.
    // reader is a function that will read up to len bytes into buf, similar to read(2).
    // cb is called at a set interval to indicate progress.
    // progress is bytes sent. When cb is called with progress = -1, the operation is complete,
    // and you can call endUploadFirmware() from another thread to check for errors.
    void beginUploadFirmware(uint32_t dstAddr,
                             const function<int(uint8_t* buf, int len)>& reader,
                             const function<void(int progress)>& cb);

    // returns the first exception encountered during firmware upload, if any.
    // if return value is non-null, the user must call delete on it.
    exception* endUploadFirmware();

    // sets the argument passed to the user application upon reboot
    void setUserArgument(uint32_t arg);

    // reboot the device
    void reboot();

    // returns -1 on error
    int readRegister(uint8_t addr);
    int writeRegister(uint8_t addr, uint8_t val);
    int writeRegister32(uint8_t addr, uint32_t val);


    // internal functions
    int ttyFD = -1;
    int _sendBytes(const uint8_t* data, int len);
    int _waitSend();

private:
    function<void(int progress)> _cb;
    function<int(uint8_t*, int)> _reader;
    pthread_t _pth;
    static void* _flashThread(void* v);
    void* flashThread();
};

#endif // FIRMWAREUPDATER_H
