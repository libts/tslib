# TS_CXX_SUPPORTS_HIDDEN_VISIBILITY_INLINES
# ----------
# Check the C++ compiler for support for -fvisibility-inlines-hidden.
AC_DEFUN([TS_CXX_SUPPORTS_HIDDEN_VISIBILITY_INLINES],
[AC_REQUIRE([AC_PROG_CXX])
  bb_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="-fvisibility-inlines-hidden $bb_save_CXXFLAGS"
  AC_CACHE_CHECK([whether the C++ compiler supports -fvisibility-inlines-hidden],
                 [bb_cv_cc_supports_hidden_visibility_inlines], [
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])], [bb_cv_cc_supports_hidden_visibility_inlines=yes], [bb_cv_cc_supports_hidden_visibility_inlines=no])
    AC_LANG_POP(C++)
  ])
  CXXFLAGS="$bb_save_CXXFLAGS"
]) # TS_CXX_SUPPORTS_HIDDEN_VISIBILITY_INLINES


# TS_CXX_HIDDEN_VISIBILITY_INLINES
# ----------
# Decide whether or not to use -fvisibility-inlines-hidden for c++ applications.
AC_DEFUN([TS_CXX_HIDDEN_VISIBILITY_INLINES],
[AC_REQUIRE([TS_CXX_SUPPORTS_HIDDEN_VISIBILITY_INLINES])
  AC_MSG_CHECKING([whether to use -fvisibility-inlines-hidden])
  AC_ARG_WITH([hidden_visibility_inlines],
              AC_HELP_STRING([--with-hidden_visibility_inlines=VAL],
                             [use -fvisibility-inlines-hidden (default VAL is 'auto')]),
              [bb_with_hidden_visibility_inlines=$withval], [bb_with_hidden_visibility_inlines=auto])

  if test "x$bb_with_hidden_visibility_inlines" != "xno" && \
     test "x$bb_cv_cc_supports_hidden_visibility_inlines" != "xno"; then
     AC_MSG_RESULT([yes])
     VIS_CXXFLAGS="-fvisibility-inlines-hidden"
  else
     AC_MSG_RESULT([no])
  fi
  AC_SUBST(VIS_CXXFLAGS)
]) # TS_CXX_HIDDEN_VISIBILITY_INLINES


# TS_CC_SUPPORTS_HIDDEN_VISIBILITY
# ----------
# Check the C compiler for support for -fvisibility=hidden.
AC_DEFUN([TS_CC_SUPPORTS_HIDDEN_VISIBILITY],
[AC_REQUIRE([AC_PROG_CC])
  bb_save_CFLAGS="$CFLAGS"
  CFLAGS="-fvisibility=hidden $bb_save_CFLAGS"
  AC_CACHE_CHECK([whether the C compiler supports -fvisibility=hidden],
                 [bb_cv_cc_supports_hidden_visibility], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])], [bb_cv_cc_supports_hidden_visibility=yes], [bb_cv_cc_supports_hidden_visibility=no])
  ])
  CFLAGS="$bb_save_CFLAGS"
  if test "x$bb_cv_cc_supports_hidden_visibility" != "xno"; then
     CFLAGS="-DGCC_HASCLASSVISIBILITY $CFLAGS"
  fi
]) # TS_CC_SUPPORTS_HIDDEN_VISIBILITY


# TS_CC_HIDDEN_VISIBILITY
# ----------
# Decide whether or not to use -fvisibility=hidden.
AC_DEFUN([TS_CC_HIDDEN_VISIBILITY],
[AC_REQUIRE([TS_CC_SUPPORTS_HIDDEN_VISIBILITY])
  AC_MSG_CHECKING([whether to use -fvisibility=hidden])
  AC_ARG_WITH([hidden_visibility],
              AC_HELP_STRING([--with-hidden_visibility=VAL],
                             [use -fvisibility=hidden (default VAL is 'auto')]),
              [bb_with_hidden_visibility=$withval], [bb_with_hidden_visibility=auto])

  if test "x$bb_with_hidden_visibility" != "xno" && \
     test "x$bb_cv_cc_supports_hidden_visibility" != "xno"; then
     AC_MSG_RESULT([yes])
     VIS_CFLAGS="-fvisibility=hidden"
  else
     AC_MSG_RESULT([no])
  fi
  AC_SUBST(VIS_CFLAGS)
]) # TS_CC_HIDDEN_VISIBILITY
