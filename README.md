# NanoVNA-QT
PC GUI software for NanoVNA V2, based on the software for the xaVNA.

__Directory layout__
* libxavna: C & C++ library for accessing the hardware, see README.md in subdirectory for more info
* vna_qt: QT GUI


# Downloads
For pre-compiled executables go to:
https://github.com/nanovna/NanoVNA-QT/releases


# Screenshots

##### Open circuited coax stub

<img src="pictures/coax.png" width="500">

##### Antenna

<img src="pictures/angtenna.png" width="500">

##### Time to fault (measuring coax cable)

<img src="pictures/ttf.png" width="500">


# Building the software

__Building on Linux__

Build libxavna (required for QT GUI):
```bash
sudo apt-get install automake libtool make g++ libeigen3-dev libfftw3-dev
cd /PATH/TO/NanoVNA-QT
autoreconf --install
./configure
make
cd libxavna/xavna_mock_ui/
/PATH/TO/qmake
make
```

Build & run QT GUI:
```bash
sudo apt-get install libqt5charts5-dev
cd /PATH/TO/NanoVNA-QT
cd vna_qt
/PATH/TO/qmake
make
export QT=/PATH/TO/QT # optional, e.g. ~/qt/5.10.1/gcc_64
../run ./vna_qt
```

__Building on mac os__
```bash
brew install automake libtool make eigen fftw
cd /PATH/TO/NanoVNA-QT
./deploy_macos.sh
# result is in ./vna_qt/vna_qt.app
```

__Cross-compile for windows (from Linux)__

Download and build MXE:
```bash
cd ~/
git clone https://github.com/mxe/mxe.git
cd mxe
export QT_MXE_ARCH=386
make qt5 qtcharts cc eigen fftw pthreads
```
Edit mxe/settings.mk and add i686-w64-mingw32.shared to MXE_TARGETS.

Build
```bash
cd /PATH/TO/NanoVNA-QT
export PATH="/PATH/TO/MXE/usr/bin:$PATH"
./deploy_windows.sh
```
