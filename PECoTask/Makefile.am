AM_CXXFLAGS = -std=c++11 -Wall -fPIC
AM_LDFLAGS = ${STATIC_FLAG}

lib_LTLIBRARIES = libcotask.la

cotaskSubdir = $(pkgincludedir)/cotask

libcotask_la_SOURCES = \
	peco/src/loop.cpp \
	peco/src/mempage.cpp \
	peco/src/taskadapter.cpp \
	peco/src/process.cpp \
	peco/src/condition.cpp
cotaskSub_HEADERS = \
	peco/cotask/condition.h \
	peco/cotask/epoll_wrapper.h \
	peco/cotask/loop.h \
	peco/cotask/process.h \
	peco/cotask/eventqueue.h \
	peco/cotask/memblock.hpp \
	peco/cotask/taskadapter.h \
	peco/cotask/cotask.hpp \
	peco/cotask/kevent_wrapper.h \
	peco/cotask/mempage.h

pkginclude_HEADERS = peco/cotask.h