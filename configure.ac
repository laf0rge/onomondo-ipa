dnl Process this file with autoconf to produce a configure script
AC_INIT([ipa],
	[1.0],
	[info@sysmocom.de])

dnl *This* is the root dir, even if an install-sh exists in ../ or ../../
AC_CONFIG_AUX_DIR([.])

AM_INIT_AUTOMAKE([dist-bzip2])
AC_CONFIG_TESTDIR(tests)

CFLAGS="$CFLAGS -std=c99"

dnl kernel style compile messages
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl checks for programs
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT

dnl checks for libraries
PKG_CHECK_MODULES(LIBCURL, libcurl >= 7.64.0)
PKG_CHECK_MODULES(PCSC, libpcsclite)
AC_SEARCH_LIBS([dlopen], [dl dld], [LIBRARY_DL="$LIBS";LIBS=""])
AC_SUBST(LIBRARY_DL)

dnl checks for header files
AC_HEADER_STDC

dnl Checks for typedefs, structures and compiler characteristics

AC_ARG_ENABLE(sanitize,
	[AS_HELP_STRING(
		[--enable-sanitize],
		[Compile with address sanitizer enabled],
	)],
	[sanitize=$enableval], [sanitize="no"])
if test x"$sanitize" = x"yes"
then
	CFLAGS="$CFLAGS -fsanitize=address -fsanitize=undefined"
	CPPFLAGS="$CPPFLAGS -fsanitize=address -fsanitize=undefined"
fi

AC_ARG_ENABLE(werror,
	[AS_HELP_STRING(
		[--enable-werror],
		[Turn all compiler warnings into errors, with exceptions:
		 a) deprecation (allow upstream to mark deprecation without breaking builds);
		 b) "#warning" pragmas (allow to remind ourselves of errors without breaking builds)
		]
	)],
	[werror=$enableval], [werror="no"])
if test x"$werror" = x"yes"
then
	WERROR_FLAGS="-Werror"
	WERROR_FLAGS+=" -Wno-error=deprecated -Wno-error=deprecated-declarations"
	WERROR_FLAGS+=" -Wno-error=cpp" # "#warning"
	CFLAGS="$CFLAGS $WERROR_FLAGS"
	CPPFLAGS="$CPPFLAGS $WERROR_FLAGS"
fi

# The following test is taken from WebKit's webkit.m4
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden "
AC_MSG_CHECKING([if ${CC} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([char foo;])],
      [ AC_MSG_RESULT([yes])
        SYMBOL_VISIBILITY="-fvisibility=hidden"],
        AC_MSG_RESULT([no]))
CFLAGS="$saved_CFLAGS"
AC_SUBST(SYMBOL_VISIBILITY)

AC_ARG_ENABLE(profile,
		[AS_HELP_STRING([--enable-profile], [Compile with profiling support enabled], )],
		[profile=$enableval], [profile="no"])
if test x"$profile" = x"yes"
then
	CFLAGS="$CFLAGS -pg"
	CPPFLAGS="$CPPFLAGS -pg"
fi

AC_MSG_RESULT([CFLAGS="$CFLAGS"])
AC_MSG_RESULT([CPPFLAGS="$CPPFLAGS"])

dnl Generate the output
AM_CONFIG_HEADER(config.h)

AC_OUTPUT(
    include/Makefile
    include/onomondo/Makefile
    include/onomondo/ipa/Makefile
    src/Makefile
    src/ipa/Makefile
    src/ipa/libipa/Makefile
    src/ipa/libasn/Makefile
    tests/Makefile
    tests/atlocal
    tests/utils/Makefile
    tests/activation_code/Makefile
    Makefile)
