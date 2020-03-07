AM_CXXFLAGS = \
	-std=c++11 \
	-Wall \
	-fPIC \
	-DSSL_VERSION_1_0=${SSL_VERSION_1_0} \
	-DSSL_VERSION_1_1=${SSL_VERSION_1_1}

AM_LDFLAGS = -lresolv ${STATIC_FLAG}

lib_LTLIBRARIES = libpeco.la

# All Sources From Submodules
libpeco_la_SOURCES = \
	PEUtils/peco/src/stringutil.cpp \
	PEUtils/peco/src/hex_dump.cpp \
	PEUtils/peco/src/argparser.cpp \
	PEUtils/peco/src/jsoncpp.cpp \
	PEUtils/peco/src/sysinfo.cpp \
	PEUtils/peco/src/gzip.cpp \
	PEUtils/peco/src/filemgr.cpp \
	PECoTask/peco/src/loop.cpp \
	PECoTask/peco/src/mempage.cpp \
	PECoTask/peco/src/taskadapter.cpp \
	PECoTask/peco/src/process.cpp \
	PECoTask/peco/src/condition.cpp \
	PECoNet/peco/conet/utils/basic.cpp \
	PECoNet/peco/conet/utils/obj.cpp \
	PECoNet/peco/conet/utils/rawf.cpp \
	PECoNet/peco/conet/conns/tcp.cpp \
	PECoNet/peco/conet/conns/udp.cpp \
	PECoNet/peco/conet/conns/uds.cpp \
	PECoNet/peco/conet/conns/ssl.cpp \
	PECoNet/peco/conet/conns/netadapter.cpp \
	PECoNet/peco/conet/protos/dns.cpp \
	PECoNet/peco/conet/protos/http.cpp \
	PECoNet/peco/conet/protos/redis.cpp \
	PECoNet/peco/conet/protos/socks5.cpp \
	PECoRLog/peco/corlog.cpp

pecoSubdir=$(pkgincludedir)/peco
peutilsSubdir=$(pkgincludedir)/peco/utils
pecotaskSubdir=$(pkgincludedir)/peco/cotask
peconetutilsSubdir=$(pkgincludedir)/peco/conet/utils
peconetconnsSubdir=$(pkgincludedir)/peco/conet/conns
peconetprotosSubdir=$(pkgincludedir)/peco/conet/protos

pecoSub_HEADERS = \
	peco.h \
	PEUtils/peco/peutils.h \
	PECoTask/peco/cotask.h \
	PECoNet/peco/conet.h \
	PECoRLog/peco/corlog.h

peutilsSub_HEADERS = \
	PEUtils/peco/utils/dependency.hpp \
	PEUtils/peco/utils/lrucache.hpp \
	PEUtils/peco/utils/filemgr.h \
	PEUtils/peco/utils/gzip.h \
	PEUtils/peco/utils/mustd.h \
	PEUtils/peco/utils/sysinfo.h \
	PEUtils/peco/utils/argparser.h \
	PEUtils/peco/utils/eventtrigger.h \
	PEUtils/peco/utils/filternode.h \
	PEUtils/peco/utils/hex_dump.h \
	PEUtils/peco/utils/mustd.hpp \
	PEUtils/peco/utils/stringutil.h \
	PEUtils/peco/utils/json-forwards.h \
	PEUtils/peco/utils/json.h
pecotaskSub_HEADERS = \
	PECoTask/peco/cotask/condition.h \
	PECoTask/peco/cotask/epoll_wrapper.h \
	PECoTask/peco/cotask/loop.h \
	PECoTask/peco/cotask/process.h \
	PECoTask/peco/cotask/eventqueue.h \
	PECoTask/peco/cotask/memblock.hpp \
	PECoTask/peco/cotask/taskadapter.h \
	PECoTask/peco/cotask/cotask.hpp \
	PECoTask/peco/cotask/kevent_wrapper.h \
	PECoTask/peco/cotask/mempage.h
peconetutilsSub_HEADERS = \
	PECoNet/peco/conet/utils/basic.h \
	PECoNet/peco/conet/utils/obj.h \
	PECoNet/peco/conet/utils/rawf.h
peconetconnsSub_HEADERS = \
	PECoNet/peco/conet/conns/tcp.h \
	PECoNet/peco/conet/conns/udp.h \
	PECoNet/peco/conet/conns/uds.h \
	PECoNet/peco/conet/conns/ssl.h \
	PECoNet/peco/conet/conns/conns.hpp \
	PECoNet/peco/conet/conns/netadapter.h
peconetprotosSub_HEADERS = \
	PECoNet/peco/conet/protos/dns.h \
	PECoNet/peco/conet/protos/http.h \
	PECoNet/peco/conet/protos/socks5.h \
	PECoNet/peco/conet/protos/redis.h

# Build Test Case
bin_PROGRAMS = cotask_test1
cotask_test1_LDADD = libpeco.la
cotask_test1_SOURCES = \
	PECoTask/peco/src/task_test1.cpp
