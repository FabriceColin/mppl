AC_INIT([mppl], [0.8.0])
AC_CONFIG_SRCDIR([src/mpconv.cc])
AM_INIT_AUTOMAKE

AC_PROG_CXX

AC_CHECK_FUNCS(strptime)

PKG_CHECK_MODULES(JSON, json11 >= 1.0 )
AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)

PKG_CHECK_MODULES(TAGLIB, taglib >= 1.4 )
AC_SUBST(TAGLIB_CFLAGS)
AC_SUBST(TAGLIB_LIBS)

dnl DEBUG mode
CXXFLAGS="-fPIC $CXXFLAGS"
AC_MSG_CHECKING(whether to enable DEBUG mode)
AC_ARG_ENABLE(debug,
   [AS_HELP_STRING([--enable-debug], [enable debug [default=no]])],
   ,[enable_debug=no])
if test "x$enable_debug" = "xyes"; then
   if test "x$GCC" = "xyes"; then
      CXXFLAGS="$CXXFLAGS -ggdb3 -O0 -DDEBUG"
   fi
else
   CXXFLAGS="$CXXFLAGS -DNDEBUG"
   enable_debug="no"
fi
AC_MSG_RESULT($enable_debug)


AC_OUTPUT(
Makefile
src/Makefile
)
