#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libpeco], [2.0.0], [littlepush@gmail.com], [], [https://github.com/littlepush/libpeco])
AC_CONFIG_AUX_DIR([build])
AM_MAINTAINER_MODE([disable])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_PROG_AR
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB
LT_INIT

AC_SUBST([STATIC_FLAG], "")
AS_IF([test "x$enable_shared" == "xno"], [
    AC_SUBST([STATIC_FLAG], "-static")
])

# Check if this is the source dir
AC_CONFIG_SRCDIR([PEUtils/peco/src/gzip.cpp])
AC_CONFIG_SRCDIR([PEUtils/peco/src/argparser.cpp])
AC_CONFIG_SRCDIR([PEUtils/peco/src/filemgr.cpp])

AS_IF([test "x${prefix}" != "xNONE"], [
    CPPFLAGS="-I${prefix}/include ${CPPFLAGS}"
    LDFALGS="-L${prefix}/lib ${LDFALGS}"
])

AS_IF([test "x${libssl}" != "x"], [
    CPPFLAGS="-I${libssl}/include ${CPPFLAGS}"
    LDFLAGS="-L${libssl}/lib ${LDFALGS}"
])

CPPFLAGS="-I./PEUtils -I./PECoTask -I./PECoNet -I./PECoRLog ${CPPFLAGS}"

AC_CHECK_LIB(crypto, CRYPTO_new_ex_data, [], [AC_MSG_ERROR([library 'crypto' is required for OpenSSL])])

FOUND_SSL_LIB_1_0="yes"
FOUND_SSL_LIB_1_1="yes"
AC_CHECK_LIB(ssl, SSL_library_init, [], [
    FOUND_SSL_LIB_1_0="no"
])
AC_CHECK_LIB(ssl, OPENSSL_init_ssl, [], [
    FOUND_SSL_LIB_1_1="no"
])
AS_IF([test "x${FOUND_SSL_LIB_1_1}" == "xyes"], [
    AC_SUBST([SSL_VERSION_1_0], 0)
    AC_SUBST([SSL_VERSION_1_1], 1)
], [
    AS_IF([test "x${FOUND_SSL_LIB_1_0}" == "xyes"], [
        AC_SUBST([SSL_VERSION_1_0], 1)
        AC_SUBST([SSL_VERSION_1_1], 0)
    ], [
        AC_MSG_FAILURE([library 'ssl' is required for OpenSSL])        
    ])
])
AC_CHECK_LIB([z], [gzread], [], [AC_MSG_FAILURE([could not find libz])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
