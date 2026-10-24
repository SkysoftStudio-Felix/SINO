#include "kernel.h"
/* Bounded IPv4 loopback transport. Frames really traverse the receive queue,
   IPv4 validator and UDP/TCP/ICMP decoders; no socket-to-socket copy shortcut. */
#include "net.h"
#define NS 16
#define RX_CAP 1024u
#define SYN 2
#define ACK 16
#define FIN 1
#define RST 4
enum {IDLE=1,BOUND,LISTEN,SYN_SENT,SYN_RCVD,ESTABLISHED,FIN_WAIT,CLOSE_WAIT,CLOSED,FAILED};
typedef struct {int type,state,parent,accepted,closing,peer_closed;u16 local,peer,last_source;u32 tx_next,tx_start,rx_next,rx_size,peer_window,deadline,retries;u32 pending,tx_size;u8 rx[RX_CAP],tx[NET_MSS];} Socket;
static Socket *sockets[NS];
static u8 *packets[8];static u32 lengths[8],head,tail;
static u16 ephemeral=49152,ip_id;
static u32 last_ping,ping_reply,ping_id;
#ifdef SELFTEST
static int drop_ack;
#endif
NetDevice net_loop;
static u16 get16(const u8 *p){return (u16)(((u16)p[0]<<8)|p[1]);}
static u32 get32(const u8 *p){return ((u32)get16(p)<<16)|get16(p+2);}
static void put16(u8 *p,u16 n){p[0]=(u8)(n>>8);p[1]=(u8)n;}
static void put32(u8 *p,u32 n){put16(p,(u16)(n>>16));put16(p+2,(u16)n);}
static u32 sum(const u8 *p,u32 size,u32 value){while(size>=2){value+=get16(p);p+=2;size-=2;}if(size)value+=(u16)*p<<8;while(value>>16)value=(value&65535)+(value>>16);return value;}
static u16 check(const u8 *p,u32 size){return (u16)~sum(p,size,0);}
static u16 transport(const u8 *p,u32 size,u8 protocol){u8 pseudo[12];put32(pseudo,NET_LOOPBACK);put32(pseudo+4,NET_LOOPBACK);pseudo[8]=0;pseudo[9]=protocol;put16(pseudo+10,(u16)size);return (u16)~sum(p,size,sum(pseudo,12,0));}
static int loop_tx(const u8 *p,u32 size){if(size<20||size>1500)return S_INVALID;
#ifdef SELFTEST
 if(drop_ack&&size>=40&&p[9]==6&&p[33]==ACK){drop_ack=0;net_loop.stats.dropped++;return 0;}
#endif
 u32 next=(head+1)&7;if(next==tail){net_loop.stats.dropped++;return S_AGAIN;}u8 *page=page_alloc();if(!page)return S_NOMEM;memcpy(page,p,size);packets[head]=page;lengths[head]=size;head=next;net_loop.stats.tx++;return 0;}
void net_init(void){net_loop=(NetDevice){"lo",NET_LOOPBACK,0xff000000u,1500,1,loop_tx,0,{0}};debug("NETWORK lo 127.0.0.1 IPv4/ICMP/UDP/TCP initialized; no external NIC enabled\n");}
static Socket *valid(int n){return n>=0&&n<NS?sockets[n]:0;}
int net_socket(int type){if(type!=NET_UDP&&type!=NET_TCP)return S_INVALID;for(int i=0;i<NS;i++)if(!sockets[i]){Socket *s=page_alloc();if(!s)return S_NOMEM;s->type=type;s->state=IDLE;s->parent=-1;s->peer_window=RX_CAP;sockets[i]=s;return i;}return S_LIMIT;}
static void destroy(int n){if(valid(n)){page_free(sockets[n]);sockets[n]=0;}}
static int occupied(int type,u16 port){for(int i=0;i<NS;i++)if(sockets[i]&&sockets[i]->type==type&&sockets[i]->local==port&&sockets[i]->parent<0)return 1;return 0;}
static int autobind(Socket *s){for(u32 i=0;i<16384;i++){u16 port=ephemeral++;if(ephemeral<49152)ephemeral=49152;if(!occupied(s->type,port)){s->local=port;return 0;}}return S_LIMIT;}
int net_bind(int n,const SockAddress *a){Socket *s=valid(n);if(!s||s->state!=IDLE||!a->port||a->family!=2||(a->address&&a->address!=NET_LOOPBACK))return S_INVALID;if(occupied(s->type,a->port))return S_EXISTS;s->local=a->port;s->state=BOUND;return 0;}
static int ip_output(u8 protocol,const u8 *payload,u32 size){if(size>1480)return S_LIMIT;u8 frame[1500];memset(frame,0,20);frame[0]=0x45;put16(frame+2,(u16)(size+20));put16(frame+4,++ip_id);frame[8]=64;frame[9]=protocol;put32(frame+12,NET_LOOPBACK);put32(frame+16,NET_LOOPBACK);put16(frame+10,check(frame,20));memcpy(frame+20,payload,size);return net_loop.transmit(frame,size+20);}
static int tcp_emit(Socket *s,u32 sequence,u8 flags,const u8 *data,u32 size){u8 packet[NET_MSS+24];u32 hdr=(flags&SYN)?24:20;memset(packet,0,hdr);put16(packet,s->local);put16(packet+2,s->peer);put32(packet+4,sequence);put32(packet+8,s->rx_next);packet[12]=(u8)(hdr/4<<4);packet[13]=flags;put16(packet+14,(u16)(RX_CAP-s->rx_size));if(flags&SYN){packet[20]=2;packet[21]=4;put16(packet+22,NET_MSS);}if(size)memcpy(packet+hdr,data,size);put16(packet+16,transport(packet,size+hdr,6));return ip_output(6,packet,size+hdr);}
static void transmit_pending(Socket *s){tcp_emit(s,s->tx_start,(u8)s->pending,s->tx,s->tx_size);s->deadline=ticks+50;}
int net_listen(int n){Socket *s=valid(n);if(!s||s->type!=NET_TCP||s->state!=BOUND)return S_INVALID;s->state=LISTEN;return 0;}
int net_connect(int n,const SockAddress *a){Socket *s=valid(n);if(!s||a->address!=NET_LOOPBACK||a->family!=2||!a->port)return S_INVALID;if(s->state==ESTABLISHED||(s->type==NET_UDP&&s->peer))return s->peer==a->port?0:S_BUSY;if(s->state==FAILED)return S_IO;if(s->state==SYN_SENT)return S_AGAIN;if(s->state!=IDLE&&s->state!=BOUND)return S_INVALID;if(!s->local&&autobind(s)<0)return S_LIMIT;s->peer=a->port;if(s->type==NET_UDP){s->state=BOUND;return 0;}s->tx_next=0x100000u+ticks*67+(u32)n*8192;s->tx_start=s->tx_next++;s->state=SYN_SENT;s->pending=SYN;s->tx_size=0;transmit_pending(s);return S_AGAIN;}
int net_accept(int n){Socket *s=valid(n);if(!s||s->state!=LISTEN)return S_INVALID;for(int i=0;i<NS;i++)if(sockets[i]&&sockets[i]->parent==n&&sockets[i]->state==ESTABLISHED&&!sockets[i]->accepted){sockets[i]->accepted=1;return i;}return S_AGAIN;}
int net_send(int n,const u8 *data,u32 size){Socket *s=valid(n);if(!s||!size||size>NET_MSS)return S_INVALID;if(!s->peer)return S_INVALID;if(s->type==NET_UDP){u8 packet[NET_MSS+8];put16(packet,s->local);put16(packet+2,s->peer);put16(packet+4,(u16)(size+8));put16(packet+6,0);memcpy(packet+8,data,size);u16 c=transport(packet,size+8,17);put16(packet+6,c?c:65535);int r=ip_output(17,packet,size+8);return r<0?r:(int)size;}
 if(s->state==FAILED||s->state==CLOSED)return S_IO;if(s->state!=ESTABLISHED)return S_AGAIN;if(s->pending||size>s->peer_window)return S_AGAIN;memcpy(s->tx,data,size);s->tx_start=s->tx_next;s->tx_next+=size;s->tx_size=size;s->pending=ACK|8;s->retries=0;transmit_pending(s);return (int)size;}
int net_recv(int n,u8 *out,u32 cap){Socket *s=valid(n);if(!s||!cap)return S_INVALID;if(!s->rx_size){if(s->state==CLOSE_WAIT||s->state==CLOSED)return 0;return s->state==FAILED?S_IO:S_AGAIN;}if(s->type==NET_UDP&&cap<s->rx_size)return S_LIMIT;u32 bytes=s->rx_size<cap?s->rx_size:cap;memcpy(out,s->rx,bytes);for(u32 i=bytes;i<s->rx_size;i++)s->rx[i-bytes]=s->rx[i];s->rx_size-=bytes;if(s->type==NET_TCP)tcp_emit(s,s->tx_next,ACK,0,0);return (int)bytes;}
int net_ready(int n){Socket *s=valid(n);if(!s)return S_BADHANDLE;int bits=s->rx_size||s->state==CLOSE_WAIT||s->state==CLOSED?1:0;if(s->state==FAILED)bits|=4;if(s->type==NET_UDP||(s->state==ESTABLISHED&&!s->pending&&s->peer_window))bits|=2;if(s->state==LISTEN)for(int i=0;i<NS;i++)if(sockets[i]&&sockets[i]->parent==n&&!sockets[i]->accepted&&sockets[i]->state==ESTABLISHED)bits|=1;return bits;}
static void fin_start(Socket *s){s->tx_start=s->tx_next++;s->tx_size=0;s->pending=FIN|ACK;s->state=FIN_WAIT;s->retries=0;transmit_pending(s);}
void net_close(int n){Socket *s=valid(n);if(!s)return;if(s->state==LISTEN){for(int i=0;i<NS;i++)if(sockets[i]&&sockets[i]->parent==n){if(!sockets[i]->accepted)destroy(i);else sockets[i]->parent=-1;}destroy(n);return;}if(s->type==NET_TCP&&(s->state==ESTABLISHED||s->state==CLOSE_WAIT)){s->closing=1;if(!s->pending)fin_start(s);return;}destroy(n);}
static void tcp_input(const u8 *p,u32 size){if(size<20){net_loop.stats.malformed++;return;}u32 hdr=(p[12]>>4)*4;if(hdr<20||hdr>size||size-hdr>NET_MSS){net_loop.stats.malformed++;return;}if(transport(p,size,6)){net_loop.stats.bad_checksum++;return;}u16 source=get16(p),dest=get16(p+2);u8 flags=p[13];u32 sequence=get32(p+4),ack=get32(p+8),bytes=size-hdr;Socket *s=0;int listener=-1;
 for(int i=0;i<NS;i++)if(sockets[i]&&sockets[i]->type==NET_TCP&&sockets[i]->local==dest){Socket *v=sockets[i];if(v->state==LISTEN)listener=i;else if(v->peer==source){s=v;break;}}
 if(!s){if(listener<0||(flags&(SYN|ACK))!=SYN)return;int i=net_socket(NET_TCP);if(i<0){net_loop.stats.dropped++;return;}s=sockets[i];s->local=dest;s->peer=source;s->parent=listener;s->state=SYN_RCVD;s->rx_next=sequence+1;s->tx_start=0x800000u+ticks*89+(u32)i*8192;s->tx_next=s->tx_start+1;s->pending=SYN|ACK;s->peer_window=get16(p+14);transmit_pending(s);return;}
 if(flags&RST){s->state=FAILED;s->pending=0;return;}
 if(s->state==SYN_SENT){if((flags&(SYN|ACK))!=(SYN|ACK)||ack!=s->tx_next)return;s->rx_next=sequence+1;s->pending=0;s->state=ESTABLISHED;s->peer_window=get16(p+14);tcp_emit(s,s->tx_next,ACK,0,0);net_loop.stats.connections++;return;}
 if((flags&(SYN|ACK))==SYN&&s->state==SYN_RCVD){transmit_pending(s);return;}
 if(s->state==SYN_RCVD){if(!(flags&ACK)||ack!=s->tx_next||sequence!=s->rx_next)return;s->state=ESTABLISHED;s->pending=0;net_loop.stats.connections++;}
 /* A peer may retransmit SYN+ACK after our final ACK was lost. */
 if((flags&(SYN|ACK))==(SYN|ACK)&&sequence+1==s->rx_next){tcp_emit(s,s->tx_next,ACK,0,0);return;}
 if(sequence!=s->rx_next){tcp_emit(s,s->tx_next,ACK,0,0);return;}
 if(flags&ACK){s->peer_window=get16(p+14);if(s->pending&&ack==s->tx_next){u32 pending=s->pending;s->pending=0;s->retries=0;if(s->closing&&s->peer_closed&&(pending&FIN))s->state=CLOSED;if(s->closing&&s->state==CLOSED)return;}}
 if(bytes){if(bytes>RX_CAP-s->rx_size){tcp_emit(s,s->tx_next,ACK,0,0);return;}memcpy(s->rx+s->rx_size,p+hdr,bytes);s->rx_size+=bytes;s->rx_next+=bytes;tcp_emit(s,s->tx_next,ACK,0,0);}
 if(flags&FIN){s->rx_next++;s->peer_closed=1;s->state=s->closing&&(s->state==FIN_WAIT||s->state==CLOSED)?CLOSED:CLOSE_WAIT;tcp_emit(s,s->tx_next,ACK,0,0);}
}
static void input(u8 *p,u32 size){if(size<20||p[0]!=0x45||get16(p+2)!=size||(get16(p+6)&0x3fff)||!p[8]||get32(p+12)!=NET_LOOPBACK||get32(p+16)!=NET_LOOPBACK){net_loop.stats.malformed++;return;}if(check(p,20)){net_loop.stats.bad_checksum++;return;}net_loop.stats.rx++;u8 protocol=p[9];p+=20;size-=20;
 if(protocol==1){if(size<8||check(p,size)){net_loop.stats.bad_checksum++;return;}if(p[0]==8&&p[1]==0){p[0]=0;put16(p+2,0);put16(p+2,check(p,size));ip_output(1,p,size);}else if(p[0]==0&&get16(p+4)==ping_id&&get16(p+6)==last_ping){ping_reply=last_ping;net_loop.stats.icmp_replies++;}return;}
 if(protocol==6){tcp_input(p,size);return;}if(protocol!=17)return;if(size<8||get16(p+4)!=size||size-8>NET_MSS){net_loop.stats.malformed++;return;}if(get16(p+6)&&transport(p,size,17)){net_loop.stats.bad_checksum++;return;}u16 source=get16(p),dest=get16(p+2);for(int i=0;i<NS;i++){Socket *s=sockets[i];if(s&&s->type==NET_UDP&&s->local==dest&&(!s->peer||s->peer==source)){if(s->rx_size){net_loop.stats.dropped++;return;}s->last_source=source;memcpy(s->rx,p+8,size-8);s->rx_size=size-8;return;}}net_loop.stats.dropped++;}
void net_poll(void){for(int i=0;i<8&&tail!=head;i++){u8 *p=packets[tail];u32 size=lengths[tail];tail=(tail+1)&7;input(p,size);page_free(p);}for(int i=0;i<NS;i++){Socket *s=sockets[i];if(!s)continue;if(s->closing&&!s->pending&&(s->state==ESTABLISHED||s->state==CLOSE_WAIT))fin_start(s);if(s->closing&&((s->state==CLOSED&&!s->pending)||(!s->pending&&(i32)(ticks-s->deadline)>500))){destroy(i);continue;}if(s->pending&&(i32)(ticks-s->deadline)>=0){if(++s->retries>5){s->pending=0;s->state=FAILED;if(s->closing||(s->parent>=0&&!s->accepted))destroy(i);}else{net_loop.stats.retransmits++;transmit_pending(s);}}}if(net_loop.poll)net_loop.poll();}
int net_ping(u32 address,u32 sequence){if(address!=NET_LOOPBACK||!sequence||sequence>65535)return S_INVALID;if(sequence==ping_reply)return 0;if(sequence==last_ping)return S_AGAIN;u8 packet[12]={8,0,0,0,0,0,0,0,'S','I','N','O'};ping_id=0x534e;last_ping=sequence;put16(packet+4,(u16)ping_id);put16(packet+6,(u16)sequence);put16(packet+2,check(packet,12));int r=ip_output(1,packet,12);return r<0?r:S_AGAIN;}
void net_tests(void){
#ifdef SELFTEST
 u8 malformed[20]={0};u32 before=net_loop.stats.malformed;input(malformed,20);if(net_loop.stats.malformed!=before+1)panic("Network bounds",0,0);int a=net_socket(NET_UDP),b=net_socket(NET_UDP);SockAddress p={NET_LOOPBACK,7000,2},q={NET_LOOPBACK,7001,2};if(a<0||b<0||net_bind(a,&p)||net_bind(b,&q)||net_connect(a,&q)||net_connect(b,&p))panic("UDP setup",0,0);u8 data[4];if(net_send(a,(const u8 *)"udp!",4)!=4)panic("UDP send",0,0);net_poll();if(net_recv(b,data,3)!=S_LIMIT||net_recv(b,data,4)!=4||data[0]!='u')panic("UDP receive",0,0);net_close(a);net_close(b);
 u8 bad[20]={0x45};put16(bad+2,20);bad[8]=64;put32(bad+12,NET_LOOPBACK);put32(bad+16,NET_LOOPBACK);before=net_loop.stats.bad_checksum;input(bad,20);if(net_loop.stats.bad_checksum!=before+1)panic("IP checksum",0,0);
 u32 free_before=free_pages;int listener=net_socket(NET_TCP);a=net_socket(NET_TCP);if(net_bind(listener,&p)||net_listen(listener)||net_connect(a,&p)!=S_AGAIN)panic("TCP setup",0,0);
 /* Drop the first SYN in the real receive queue and expire its RTO. */
 if(tail==head)panic("TCP SYN queue",0,0);page_free(packets[tail]);tail=(tail+1)&7;net_loop.stats.dropped++;ticks+=50;net_poll();net_poll();net_poll();if(net_connect(a,&p))panic("TCP SYN retransmission",0,0);b=net_accept(listener);if(b<0)panic("TCP accept",0,0);
 u8 payload[NET_MSS],out[RX_CAP];memset(payload,'x',sizeof(payload));drop_ack=1;if(net_send(a,payload,NET_MSS)!=(int)NET_MSS)panic("TCP payload",0,0);net_poll();ticks+=50;net_poll();net_poll();if(sockets[b]->rx_size!=NET_MSS||sockets[a]->pending)panic("TCP duplicate/ACK loss",0,0);
 if(net_send(a,payload,NET_MSS)!=(int)NET_MSS)panic("TCP second payload",0,0);net_poll();if(net_send(a,payload,1)!=S_AGAIN||net_recv(b,out,NET_MSS)!=(int)NET_MSS)panic("TCP window full",0,0);net_poll();if(net_send(a,payload,1)!=1)panic("TCP window reopen",0,0);net_poll();if(net_recv(b,out,sizeof(out))!=513)panic("TCP stream",0,0);net_close(a);net_poll();net_close(b);net_poll();net_poll();net_close(listener);net_poll();for(int i=0;i<8;i++)net_poll();if(free_pages!=free_before){for(int i=0;i<NS;i++)if(sockets[i]){debug("NET remaining socket/state/pending: ");debug_hex((u32)i);debug_hex((u32)sockets[i]->state);debug_hex(sockets[i]->pending);debug("\n");}panic("Network resources",free_pages,free_before);}
 debug("NETWORK kernel checksum/SYN-loss/ACK-loss/duplicate/window/resource PASS\n");
#endif
}

int net_sendto(int n,const u8 *data,u32 bytes,const SockAddress *a){Socket *s=valid(n);if(!s||s->type!=NET_UDP||a->address!=NET_LOOPBACK||a->family!=2||!a->port)return S_INVALID;if(!s->local&&autobind(s)<0)return S_LIMIT;u16 old=s->peer;s->peer=a->port;int r=net_send(n,data,bytes);s->peer=old;return r;}
int net_recvfrom(int n,u8 *out,u32 cap,SockAddress *a){Socket *s=valid(n);if(!s||s->type!=NET_UDP)return S_INVALID;int r=net_recv(n,out,cap);if(r>=0)*a=(SockAddress){NET_LOOPBACK,s->last_source,2};return r;}
