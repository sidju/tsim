#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(tsim, 0.84, staffan.bjornesjo@gmail.com)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/Sprite.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# AC_CHECK_LIB([X11], [XCopyArea])
# AC_CHECK_LIB([Xaw], [XawDialogAddButton])
# AC_CHECK_LIB([Xext], [XShapeQueryExtension])
# AC_CHECK_LIB([Xmu], [XmuCreatePixmapFromBitmap])
# AC_CHECK_LIB([Xt], [XtCreateWindow])

libtest_failed_gen=0
libtest_failed=0
AC_CHECK_LIB([X11], [XCopyArea], [], libtest_failed=1)
if test $libtest_failed == 1
then
	AC_MSG_WARN([X11 library is missing.])
	libtest_failed_gen=1
fi

libtest_failed=0
AC_CHECK_LIB([Xaw], [XawDialogAddButton], [], libtest_failed=1)
if test $libtest_failed == 1
then
	AC_MSG_WARN([Xaw library is missing.])
	libtest_failed_gen=1
fi

libtest_failed=0
AC_CHECK_LIB([Xext], [XShapeQueryExtension], [], libtest_failed=1)
if test $libtest_failed == 1
then
	AC_MSG_WARN([Xext library is missing.])
	libtest_failed_gen=1
fi

libtest_failed=0
AC_CHECK_LIB([Xmu], [XmuCreatePixmapFromBitmap], [], libtest_failed=1)
if test $libtest_failed == 1
then
	AC_MSG_WARN([Xmu library is missing.])
	libtest_failed_gen=1
fi

libtest_failed=0
AC_CHECK_LIB([Xt], [XtCreateWindow], [], libtest_failed=1)
if test $libtest_failed == 1
then
	AC_MSG_WARN([Xt library is missing.])
	libtest_failed_gen=1
fi

if test $libtest_failed_gen == 1
then
	AC_MSG_WARN([One or more required libraries appears to be missing (see above). The compilation will probably fail. Make sure you have the libraries (and headers) installed.])
fi

# Checks for header files.
AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([stdlib.h string.h strings.h sys/ioctl.h sys/param.h unistd.h \
                  X11/Xaw/Toggle.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_WAIT3
AC_CHECK_FUNCS([strcasecmp strdup strstr])

AC_CONFIG_FILES([Makefile
                 bitmaps/Makefile
                 bitmaps/customBitmaps/Makefile
                 maps/Makefile
                 sounds/Makefile
                 src/Makefile])

AC_OUTPUT
