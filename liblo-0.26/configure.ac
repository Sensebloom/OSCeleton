AC_PREREQ(2.57)

AC_INIT([liblo], [0.26], [liblo-devel@lists.sourceforge.net])

# libtool version: current:revision:age
#
# If the library source code has changed at all since the last update, then
# increment revision (`c:r:a' becomes `c:r+1:a').
#
# If any interfaces have been added, removed, or changed since the last update,
# increment current, and set revision to 0.
#
# If any interfaces have been added since the last public release, then
# increment age.
#
# If any interfaces have been removed since the last public release, then set
# age to 0.
LO_SO_VERSION=7:0:0

AC_CONFIG_SRCDIR([src/address.c])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_SUBST(LO_SO_VERSION)

# disable support for ipv6.
AC_ARG_ENABLE(ipv6, [  --enable-ipv6           Enable ipv6 support],want_ipv6=yes,)
if test "$want_ipv6" = "yes"; then
	AC_DEFINE(ENABLE_IPV6, 1, Define this to enable ipv6.)
fi

# Checks for programs.
AC_PROG_CC
AM_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_CHECK_PROG([DOXYGEN], [doxygen], [doc], [])
AC_SUBST(DOXYGEN)

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_SEARCH_LIBS([recvfrom], [socket])
AC_CHECK_FUNC([log], [], [AC_CHECK_LIB([m],[log])])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Check for features
AC_CHECK_FUNC([select], [AC_DEFINE(HAVE_SELECT, [1], [Define to 1 if select() is available.])], [
     AC_MSG_CHECKING([for select in ws2_32])
     LIBS="$LIBS -lws2_32"
     # some winsock2 functions require XP, so WINNT=0x501
     CFLAGS="$CFLAGS -DWIN32 -D_WIN32_WINNT=0x501"
     AC_TRY_LINK([#include <winsock2.h>],
                 [select(0,0,0,0,0)],
                 [AC_MSG_RESULT(yes)
                 AC_DEFINE(HAVE_SELECT, [1], [Define to 1 if select() is available.])],
                 [AC_MSG_RESULT(no)])
])
AC_CHECK_FUNC([poll], [AC_DEFINE(HAVE_POLL, [1], [Define to 1 if poll() is available.])])
AC_CHECK_FUNC([inet_aton], [AC_DEFINE(HAVE_INET_ATON, [1], [Define to 1 if inet_aton() is available.])])

AC_C_BIGENDIAN([LO_BIGENDIAN="1"], [LO_BIGENDIAN="0"])
AC_DEFINE_UNQUOTED(LO_BIGENDIAN, "$LO_BIGENDIAN", [If machine is bigendian])
AC_SUBST(LO_BIGENDIAN)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/tools/Makefile
	examples/Makefile
	lo/Makefile
	lo/lo_endian.h
	liblo.pc
	doc/Makefile
	doc/reference.doxygen
	build/Makefile
])
AC_OUTPUT()
