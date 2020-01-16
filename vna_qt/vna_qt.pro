#-------------------------------------------------
#
# Project created by QtCreator 2017-12-16T02:35:03
#
#-------------------------------------------------

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


#CONFIG += static
#CONFIG -= import_plugins

QT += svg
QTPLUGIN.iconengines += svgicon
QTPLUGIN.imageformats += svg

#QMAKE_LFLAGS += --static -lexpat -lz -lXext -lXau -lbsd -lXdmcp
#QMAKE_LFLAGS += -L../lib -lxavna
QMAKE_CXXFLAGS += -Wextra --std=c++11
QMAKE_CXXFLAGS += -DEIGEN_DONT_VECTORIZE -DEIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT
android: QMAKE_CXXFLAGS += -I../android_include -DANDROID_WORKAROUNDS

TARGET = vna_qt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES +=\
    polarview.C \
    mainwindow.C \
    main.C \
    markerslider.C \
    impedancedisplay.C \
    frequencydialog.C \
    graphpanel.C \
    configureviewdialog.C \
    touchstone.C \
    calkitsettingsdialog.C \
    calkitsettings.C \
    networkview.C \
    dtfwindow.C

HEADERS  += \
    polarview.H \
    mainwindow.H \
    markerslider.H \
    impedancedisplay.H \
    utility.H \
    frequencydialog.H \
    graphpanel.H \
    configureviewdialog.H \
    touchstone.H \
    calkitsettingsdialog.H \
    calkitsettings.H \
    networkview.H \
    dtfwindow.H

FORMS    += mainwindow.ui \
    markerslider.ui \
    impedancedisplay.ui \
    frequencydialog.ui \
    graphpanel.ui \
    configureviewdialog.ui \
    calkitsettingsdialog.ui \
    calkitsettingswidget.ui \
    dtfwindow.ui \
    graphlimitsdialog.ui

RESOURCES += \
    resources.qrc

LIBS += -L$$PWD/../libxavna/.libs/ -L/usr/local/lib/ -lxavna -lfftw3
android: LIBS += -L$$PWD/../lib

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../libxavna/xavna_mock_ui/release/ -lxavna_mock_ui
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../libxavna/xavna_mock_ui/debug/ -lxavna_mock_ui
else:unix: LIBS += -L$$PWD/../libxavna/xavna_mock_ui/ -lxavna_mock_ui

INCLUDEPATH += $$PWD/../include /usr/local/include
DEPENDPATH += $$PWD/../include

#INCLUDEPATH += ../libxavna/xavna_mock_ui
#PRE_TARGETDEPS += ../libxavna/xavna_mock_ui/libxavna_mock_ui.so

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_EXTRA_LIBS = \
        /persist/vna/vna_qt/../libxavna/.libs/libxavna.so \
        $$PWD/../libxavna/xavna_mock_ui/libxavna_mock_ui.so
}
