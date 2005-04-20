dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)
AC_INIT(gameswf/gameswf.h)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_TARGET

dnl These are required by automake
AM_INIT_AUTOMAKE(gameswf, 0.2)
AM_MAINTAINER_MODE
AC_PROG_MAKE_SET

dnl Use the POSIX Threads library, if specified.
AC_ARG_ENABLE(pthreads, [  --enable-pthreads            Enable support for pthreads],
[case "${enableval}" in
  yes) pthreads=yes ;;
  no)  pthreads=no ;;
  *)   AC_MSG_ERROR([bad value ${enableval} for enable-pthreads option]) ;;
esac])

if test x$pthreads = xyes; then
    AC_DEFINE([HAVE_PTHREADS], [], [Use POSIX Threads])
fi

dnl Add XML support, if specified.
AC_ARG_ENABLE(libxml, [  --disable-xml            Disable support for xml and xmlsocket],
[case "${enableval}" in
  yes) disablexml=yes ;;
  no)  disablexml=no ;;
  *)   AC_MSG_ERROR([bad value ${enableval} for disable-xml option]) ;;
esac])

if test -z $enable_xml; then
    AC_DEFINE([HAVE_LIBXML], [], [Use XML])
    AM_PATH_LIBXML2
fi

AC_PROG_CC
AC_PROG_CXX
AC_EXEEXT
AM_COMPILER_LIB
AC_PROG_INSTALL

AM_PATH_SDL
AM_PATH_SDL_MIXER
AM_PATH_OPENGL
AM_PATH_ZLIB
AM_PATH_JPEG
AM_PATH_PNG

dnl Define winsock if we're on wodows. We could do something complicated,
dnl but since AC_EXEEXT does it for us, we'll do this the easy way.
if test x"$EXEEXT" == "exe"; then
  AC_DEFINE(HAVE_WINSOCK)
fi

dnl Look for pthreads
if test x"$pthreads" = x"yes"; then
  AC_CHECK_LIB(pthread, main, PTHREAD_LIB=-lpthread,PTHREAD_LIB="")
else
  PTHREAD_LIB=""
fi
AC_SUBST(PTHREAD_LIB)

AC_OUTPUT(Makefile
base/Makefile
geometry/Makefile
gameswf/Makefile
config.mk
)
