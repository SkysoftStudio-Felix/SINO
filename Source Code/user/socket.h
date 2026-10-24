#ifndef SINO_USER_SOCKET_H
#define SINO_USER_SOCKET_H
#include "sdk.h"
#include "../src32/net.h"
static inline int socket(int type){return api(SYS_SOCKET,(u32)type,0,0);}
static inline int bind(int fd,u16 port){SockAddress a={NET_LOOPBACK,port,2};return api(SYS_BIND,(u32)fd,(u32)&a,0);}
static inline int listen(int fd){return api(SYS_LISTEN,(u32)fd,0,0);}
static inline int connect(int fd,u16 port){SockAddress a={NET_LOOPBACK,port,2};return api(SYS_CONNECT,(u32)fd,(u32)&a,0);}
static inline int accept(int fd){return api(SYS_ACCEPT,(u32)fd,0,0);}
static inline int send(int fd,const void *p,u32 n){return api(SYS_SEND,(u32)fd,(u32)p,n);}
static inline int sendto(int fd,const void *p,u32 n,const SockAddress *a){SockMessage m={(u32)p,n,*a};return api(SYS_SENDTO,(u32)fd,(u32)&m,0);}
static inline int recvfrom(int fd,void *p,u32 n,SockAddress *a){SockMessage m={(u32)p,n,{0,0,0}};int r=api(SYS_RECVFROM,(u32)fd,(u32)&m,0);if(r>=0)*a=m.address;return r;}
static inline int recv(int fd,void *p,u32 n){return api(SYS_RECV,(u32)fd,(u32)p,n);}
static inline u32 Tick(void){return (u32)api(SYS_TICKS,0,0,0);}
/* Single-descriptor select subset. Bits: readable=1 writable=2 error=4. */
static inline int select_one(int fd,u32 mask,u32 timeout){u32 begin=Tick();do{int r=api(SYS_NET_READY,(u32)fd,0,0);if(r<0||((u32)r&mask))return r;Yield(1);}while(Tick()-begin<timeout);return S_AGAIN;}
#endif
