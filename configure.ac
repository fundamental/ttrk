# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
AC_INIT(ttrk, 0.8.2, mark.d.mccurry@gmail.com)
AC_CONFIG_SRCDIR([src/chanbar.cpp])
AM_INIT_AUTOMAKE(ttrk,0.8.2)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_MSG_CHECKING(for prefix)
if test x$prefix = xNONE; then
	ttrk_prefix=$ac_default_prefix
else
	ttrk_prefix=$prefix
fi
AC_MSG_RESULT($ttrk_prefix)

dnl Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)


AC_MSG_CHECKING(for a s-lang specific include directory)
if test -d $ttrk_prefix/include/slang; then
	CPPFLAGS="$CPPFLAGS -I$ttrk_prefix/include/slang"
	slang_specific=yes
elif test -d /usr/include/slang; then
	CPPFLAGS="$CPPFLAGS -I/usr/include/slang"
	slang_specific=yes
else
	slang_specific=no
fi
AC_MSG_RESULT($slang_specific)


dnl Check for slang, then more specifically for reinit.
AC_CHECK_LIB(slang, SLang_init_tty)
dnl AC_MSG_CHECKING(for SLsmg_reinit_smg in -lslang)
dnl AC_TRY_LINK(,[SLsmg_reinit_smg()],
dnl [AC_DEFINE(HAVE_SLSMG_REINIT_SMG) AC_MSG_RESULT(yes)],
dnl AC_MSG_RESULT(no))

AC_CHECK_LIB(z, gzopen)
AC_CHECK_LIB(rtosc, rtosc_message)
AC_CHECK_LIB(jack, jack_ringbuffer_create)

dnl Checks for header files.
AC_CHECK_HEADERS(linux/rtc.h)
AC_CHECK_HEADERS(slang.h)

dnl Misc
AC_C_CONST
AC_C_INLINE

AC_CONFIG_FILES([
	Makefile
	src/Makefile
])
AC_OUTPUT

