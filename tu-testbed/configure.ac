dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)
AC_INIT(gameswf/gameswf.h)

AC_CANONICAL_TARGET

dnl These are required by automake
AM_INIT_AUTOMAKE(gameswf, 0.2)
AM_MAINTAINER_MODE
AC_PROG_MAKE_SET

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

dnl AM_PATH_LIBXMLCPP
AM_PATH_LIBXML2

AC_LINK_FILES(gameswf/testsuite/ips.xml,gameswf/testsuite/ips.xml)

AC_OUTPUT(Makefile
base/Makefile
geometry/Makefile
gameswf/Makefile
gameswf/testsuite/Makefile
config.mk
)
