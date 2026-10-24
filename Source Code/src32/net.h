#ifndef SINO_NET_H
#define SINO_NET_H
#include "api.h"
#define NET_LOOPBACK 0x7f000001u
#define NET_MSS 512u
enum {NET_UDP=1,NET_TCP=2};
typedef struct {u32 address;u16 port,family;} SockAddress;
typedef struct {u32 buffer,size;SockAddress address;} SockMessage;
int net_sendto(int,const u8 *,u32,const SockAddress *);
int net_recvfrom(int,u8 *,u32,SockAddress *);
typedef struct {u32 tx,rx,dropped,bad_checksum,malformed,retransmits,connections,icmp_replies;} NetStats;
typedef struct {const char *name;u32 address,mask,mtu,flags;int (*transmit)(const u8 *,u32);void (*poll)(void);NetStats stats;} NetDevice;
extern NetDevice net_loop;
void net_init(void);
void net_poll(void);
int net_socket(int);
int net_bind(int,const SockAddress *);
int net_listen(int);
int net_connect(int,const SockAddress *);
int net_accept(int);
int net_send(int,const u8 *,u32);
int net_recv(int,u8 *,u32);
int net_ready(int);
void net_close(int);
int net_ping(u32,u32);
void net_tests(void);
#endif
