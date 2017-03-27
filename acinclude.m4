# TS_DEFAULT_FLAGS
# ----------
# Set our default FLAGS variables.
# Remember to call before the AC_PROG_ macros, otherwise those
# defaults will be used instead of ours.
AC_DEFUN([TS_DEFAULT_FLAGS],
[
  if test x"$CFLAGS" = "x"; then
    CFLAGS="-O2 -Wall -W"
  fi
]) # TS_DEFAULT_FLAGS
