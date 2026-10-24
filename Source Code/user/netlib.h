#ifndef SINO_NETLIB_H
#define SINO_NETLIB_H
#include "socket.h"
int DnsResolve(const char *,u32 *);
int HttpGet(u16,const char *,char *,u32);
#endif
