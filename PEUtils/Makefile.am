AM_CXXFLAGS = -std=c++11 -Wall -fPIC
AM_LDFLAGS = ${STATIC_FLAG}

lib_LTLIBRARIES = libpeutils.la

utilsSubdir=$(pkgincludedir)/utils

libpeutils_la_SOURCES = \
	peco/src/stringutil.cpp \
	peco/src/hex_dump.cpp \
	peco/src/argparser.cpp \
	peco/src/jsoncpp.cpp \
	peco/src/sysinfo.cpp \
	peco/src/gzip.cpp \
	peco/src/filemgr.cpp

pkginclude_HEADERS = peco/peutils.h

utilsSub_HEADERS = \
	peco/utils/dependency.hpp \
	peco/utils/lrucache.hpp \
	peco/utils/filemgr.h \
	peco/utils/gzip.h \
	peco/utils/mustd.h \
	peco/utils/sysinfo.h \
	peco/utils/argparser.h \
	peco/utils/eventtrigger.h \
	peco/utils/filternode.h \
	peco/utils/hex_dump.h \
	peco/utils/mustd.hpp \
	peco/utils/stringutil.h \
	peco/utils/json-forwards.h \
	peco/utils/json.h
