/*
    conet.h
    PECoNet
    2019-05-23
    Push Chen

    Copyright 2015-2018 MeetU Infomation and Technology Inc. All rights reserved.
*/

#pragma once

#ifndef PE_CO_NET_CONET_H__
#define PE_CO_NET_CONET_H__

/*
    Include all libconet's header files
*/
#include <peco/conet/utils/basic.h>
#include <peco/conet/utils/obj.h>
#include <peco/conet/utils/rawf.h>
#include <peco/conet/conns/tcp.h>
#include <peco/conet/conns/udp.h>
#include <peco/conet/conns/uds.h>
#include <peco/conet/conns/ssl.h>
#include <peco/conet/conns/netadapter.h>
#include <peco/conet/protos/dns.h>
#include <peco/conet/protos/http.h>
#include <peco/conet/protos/socks5.h>
#include <peco/conet/protos/redis.h>

extern "C" {
    int PECoNet_Autoconf();
}

#endif

// Push Chen
