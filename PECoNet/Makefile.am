AM_CXXFLAGS = \
	-std=c++11 \
	-Wall \
	-fPIC \
	-DSSL_VERSION_1_0=${SSL_VERSION_1_0} \
	-DSSL_VERSION_1_1=${SSL_VERSION_1_1}
	
AM_LDFLAGS = -lresolv ${STATIC_FLAG}

lib_LTLIBRARIES = libconet.la

libconet_la_SOURCES = \
	peco/conet/utils/basic.cpp \
	peco/conet/utils/obj.cpp \
	peco/conet/utils/rawf.cpp \
	peco/conet/conns/tcp.cpp \
	peco/conet/conns/udp.cpp \
	peco/conet/conns/uds.cpp \
	peco/conet/conns/ssl.cpp \
	peco/conet/conns/netadapter.cpp \
	peco/conet/protos/dns.cpp \
	peco/conet/protos/http.cpp \
	peco/conet/protos/redis.cpp \
	peco/conet/protos/socks5.cpp

utilsSubdir=$(pkgincludedir)/conet/utils
connsSubdir=$(pkgincludedir)/conet/conns
protosSubdir=$(pkgincludedir)/conet/protos

pkginclude_HEADERS = \
	peco/conet.h
utilsSub_HEADERS = \
	peco/conet/utils/basic.h \
	peco/conet/utils/obj.h \
	peco/conet/utils/rawf.h
connsSub_HEADERS = \
	peco/conet/conns/tcp.h \
	peco/conet/conns/udp.h \
	peco/conet/conns/uds.h \
	peco/conet/conns/ssl.h \
	peco/conet/conns/conns.hpp \
	peco/conet/conns/netadapter.h
protosSub_HEADERS = \
	peco/conet/protos/dns.h \
	peco/conet/protos/http.h \
	peco/conet/protos/socks5.h \
	peco/conet/protos/redis.h
