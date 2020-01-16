AC_INIT(vna, version-0.1, 54525305+nanovna@users.noreply.github.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_CXX
AC_LANG(C++)
AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_HEADERS([libxavna/include/xavna.h])
AC_CONFIG_FILES([
 Makefile
 libxavna/Makefile
])
LT_INIT([win32-dll])
AC_CANONICAL_HOST

is_windows=no
case "${host_os}" in
    cygwin*|mingw*)
        is_windows=yes
        ;;
    linux-androideabi)
        is_android=yes
        ;;
esac

AM_CONDITIONAL([WINDOWS], [test "$is_windows" = "yes"])
AM_CONDITIONAL([ANDROID], [test "$is_android" = "yes"])

AC_CHECK_HEADER(eigen3/Eigen/Dense)
if test "$ac_cv_header_eigen3_Eigen_Dense" == no
then
    AC_MSG_ERROR([eigen3 not installed (eigen3/Eigen/Dense include not found)])
fi


AC_CHECK_HEADER(fftw3.h)
if test "$ac_cv_header_fftw3_h" == no
then
    AC_MSG_ERROR([fftw3 not installed (fftw3.h include not found)])
fi



#if test -z $CXXFLAGS; then
#    CXXFLAGS='-O2 --std=c++0x -fPIC -fwrapv -fno-delete-null-pointer-checks -funsigned-char -fno-strict-aliasing -Wno-pmf-conversions'
#fi

#AC_SUBST(OBJECTS)
#OBJECTS="lib/libxavna.so"



#AC_SUBST(EXTRA_LIBRARIES)
#AC_SUBST(EXTRA_SOURCES)

#AC_ARG_VAR([QMAKE], [Path to qmake])

#AC_ARG_WITH([qt_gui],
    #[AS_HELP_STRING([--with-qt-gui],
      #[Build QT GUI @<:@default=yes@:>@])],
    #[],
    #[with_qt_gui=yes])


#if test "$with_qt_gui" == "yes"
#then
    #AC_CHECK_PROG([QMAKE], [qmake], [qmake])
    #OBJECTS+=" vna_qt/vna_qt"
#fi

AC_OUTPUT
