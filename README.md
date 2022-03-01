# NanoVNA-QT
PC GUI software for NanoVNA V2 series (V2 Plus4/V2 Plus4 Pro).

- Supports hardware versions listed here: https://nanorfe.com/nanovna-v2.html
- https://www.tindie.com/products/hcxqsgroup/nanovna-v2/


NanoVNA V2配套的PC软件，可用于实时网络分析和测量。


# Downloads / 下载
https://nanorfe.com/nanovna-v2-software.html


# Screenshots / 截图

##### Open circuited coax stub / 开路同轴电缆

<img src="pictures/coax.png" width="500">

##### Antenna / 天线

<img src="pictures/antenna.png" width="500">

##### Time to fault / 障碍距离

<img src="pictures/ttf.png" width="500">


# 编译 / Building the software

__Linux系统下编译 / Building on Linux__

编译 libxavna:
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

编译 QT 界面:
```bash
sudo apt-get install libqt5charts5-dev
cd /PATH/TO/NanoVNA-QT
cd vna_qt
/PATH/TO/qmake
make
export QT=/PATH/TO/QT # optional, e.g. ~/qt/5.10.1/gcc_64
../run ./vna_qt
```

__Mac系统下编译 / Building on mac os__

 MacPorts:
```bash
sudo port install NanoVNA-QT
# result in /Applications/MacPorts/NanoVNA-QT.app
```

 Homebrew:
```bash
brew install automake libtool make eigen fftw
cd /PATH/TO/NanoVNA-QT
./deploy_macos.sh
# result is in ./vna_qt/vna_qt.app
```

__Linux系统下编译Windows目标 / Cross-compile for windows (from Linux)__

Prerequisites:
```bash
sudo apt-get install python make autoconf automake autopoint bison flex gperf intltool libtool libtool-bin lzip ruby unzip p7zip-full libgdk-pixbuf2.0-dev libssl-dev libeigen3-dev fftw3-dev mingw-w64
```

下载与编译 MXE:
```bash
cd ~/
git clone https://github.com/mxe/mxe.git
cd mxe
echo "MXE_TARGETS := i686-w64-mingw32.shared" >> settings.mk
export QT_MXE_ARCH=386
make qt5 qtcharts cc eigen fftw pthreads
```

编译 Application:
```bash
cd ~/
git clone https://github.com/nanovna/NanoVNA-QT.git
cd NanoVNA-QT
export PATH="~/mxe/usr/bin:$PATH"
./deploy_windows.sh
# Result is in ./vna_qt_windows.zip
```

Tested on a fresh install of Ubuntu 18.04 LTS.
