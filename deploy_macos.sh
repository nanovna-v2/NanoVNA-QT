#!/bin/bash

cd "$(dirname $0)"
. env.cfg
if [ ! -e "$QT" ]; then
    echo "please edit env.cfg and set \$QT"
    exit 1
fi
QMAKE="$QT/bin/qmake"

autoreconf --install
./configure
make -j8

pushd libxavna/xavna_mock_ui
$QMAKE
make -j8
popd

pushd vna_qt
rm -rf *.app
$QMAKE
make -j8
"$QT"/bin/macdeployqt vna_qt.app -libpath=../libxavna/xavna_mock_ui
cp -a ../libxavna/.libs/libxavna.0.dylib vna_qt.app/Contents/Frameworks

pushd vna_qt.app/Contents
install_name_tool -add_rpath "@executable_path/../Frameworks" MacOS/vna_qt
install_name_tool -change libxavna_mock_ui.1.dylib @executable_path/../Frameworks/libxavna_mock_ui.1.dylib MacOS/vna_qt
install_name_tool -change /usr/local/lib/libxavna.0.dylib @executable_path/../Frameworks/libxavna.0.dylib MacOS/vna_qt
install_name_tool -change /usr/local/lib/libxavna.0.dylib @executable_path/../Frameworks/libxavna.0.dylib Frameworks/libxavna_mock_ui.1.dylib
popd

rm -rf dmg_contents ../xaVNA_MacOS.dmg tmp.dmg
mkdir dmg_contents
cp -a vna_qt.app dmg_contents/

hdiutil create tmp.dmg -ov -volname "xaVNA GUI" -fs HFS+ -srcfolder dmg_contents
hdiutil convert tmp.dmg -format UDZO -o ../xaVNA_MacOS.dmg

