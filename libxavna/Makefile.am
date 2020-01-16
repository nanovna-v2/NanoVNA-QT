AM_CPPFLAGS = -I../include -O2 --std=c++0x -fPIC -fwrapv -fno-delete-null-pointer-checks -funsigned-char -fno-strict-aliasing -Wno-pmf-conversions
AM_LDFLAGS = -no-undefined
lib_LTLIBRARIES = libxavna.la
libxavna_la_SOURCES = xavna.C xavna_cpp.C calibration.C


AM_CPPFLAGS += -DEIGEN_DONT_VECTORIZE -DEIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT

if !ANDROID
AM_LDFLAGS += -lpthread
endif

if WINDOWS
libxavna_la_SOURCES += platform_abstraction_windows.C
else
libxavna_la_SOURCES += platform_abstraction.C
endif
