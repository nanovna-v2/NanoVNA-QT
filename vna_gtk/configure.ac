AC_INIT(vna, version-0.1, private0x01@gmail.com)

if test -z $CXXFLAGS; then
    CXXFLAGS='-O2 -std=c++0x -fPIC -fwrapv -fno-delete-null-pointer-checks -funsigned-char -fno-strict-aliasing -Wno-pmf-conversions'
fi

AC_PROG_CXX
AC_LANG(C++)

AC_SUBST(EXTRA_LIBRARIES)
AC_SUBST(EXTRA_SOURCES)

PKG_CHECK_MODULES([GTK], [gtkmm-3.0])
PKG_CHECK_MODULES([FFTW], [fftw3])

AC_OUTPUT(Makefile)
