#-------------------------------------------------
#
# Project created by QtCreator 2018-02-15T07:40:00
#
#-------------------------------------------------

QT       += widgets
CONFIG += shared

QMAKE_CXXFLAGS += -Wextra --std=c++11 -I/usr/local/include -I../../include
QMAKE_CXXFLAGS += -DEIGEN_DONT_VECTORIZE -DEIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT
android: QMAKE_CXXFLAGS += -DANDROID_WORKAROUNDS

TARGET = xavna_mock_ui
TEMPLATE = lib

DEFINES += XAVNA_MOCK_UI_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += xavna_mock_ui.C \
    xavna_mock.C \
    xavna_mock_ui_dialog.C

HEADERS += xavna_mock_ui.H\
    xavna_mock_ui_dialog.H \
    xavna_mock_ui_global.h \
    ../include/calibration.H

unix {
    target.path = /usr/lib
    INSTALLS += target
}

FORMS += \
    xavna_mock_ui_dialog.ui

LIBS += -L$$PWD/../.libs/ -lxavna
!android: LIBS += -lpthread
