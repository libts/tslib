dnl use: TSLIB_CHECK_MODULE(module, default, description)
AC_DEFUN([TSLIB_CHECK_MODULE],
[
m4_pushdef([UP], m4_translit([$1], [-a-z], [_A-Z]))dnl

AC_MSG_CHECKING([whether $1 module is requested]) 

AC_ARG_ENABLE([$1],
   AC_HELP_STRING([--enable-$1], [$3 (default=$2)]),
   [enable_module=$enableval],
   [enable_module=$2])
AC_MSG_RESULT($enable_module)

AM_CONDITIONAL(ENABLE_[]UP[]_MODULE, test "x$enable_module" = "xyes")
AM_CONDITIONAL(ENABLE_STATIC_[]UP[]_MODULE, test "x$enable_module" = "xstatic")
if test "x$enable_module" = "xstatic" ; then
	AC_DEFINE(TSLIB_STATIC_[]UP[]_MODULE, 1, whether $1 should be build as part of libts or as a 
		separate shared library which is dlopen()-ed at runtime)
fi

m4_popdef([UP])
])
