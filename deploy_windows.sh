MXE=~/mxe
export PATH="$MXE/usr/bin:$PATH"
HOST="i686-w64-mingw32.shared"
QMAKE="$HOST-qmake-qt5"

autoreconf --install
./configure --host "$HOST" CXXFLAGS=-O2
make -j8

pushd libxavna/xavna_mock_ui
"$QMAKE"
make -j8
popd

pushd vna_qt
"$QMAKE"
make -j8
for x in Qt5Charts Qt5Core Qt5Gui Qt5Widgets Qt5Svg; do
    cp "$MXE/usr/$HOST/qt5/bin/$x.dll" release/
done
mkdir -p release/platforms
mkdir -p release/styles
mkdir -p release/imageformats
mkdir -p release/iconengines

for x in platforms/qwindows styles/qwindowsvistastyle \
         imageformats/qsvg iconengines/qsvgicon; do
    cp "$MXE/usr/$HOST/qt5/plugins/$x.dll" release/"$x".dll
done;

for x in libgcc_s_sjlj-1 libstdc++-6 libpcre2-16-0 zlib1 libharfbuzz-0 \
            libpng16-16 libfreetype-6 libglib-2.0-0 libbz2 libintl-8 libpcre-1\
            libiconv-2 libwinpthread-1 libjasper libjpeg-9 libmng-2 libtiff-5\
            libwebp-7 libwebpdemux-2 liblcms2-2 liblzma-5 libfftw3-3 libzstd; do
    cp "$MXE/usr/$HOST/bin/$x.dll" release/
done
cp ../libxavna/.libs/libxavna-0.dll release/
cp ../libxavna/xavna_mock_ui/release/xavna_mock_ui.dll release/
rm release/*.cpp release/*.o release/*.h 
rm ../*.zip
zip -r ../vna_qt_windows.zip release
