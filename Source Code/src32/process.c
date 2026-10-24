/* Preemption only at user boundaries. Kernel/desktop remain non-preemptible. */
#include "process.h"
#include "object.h"
#include "vfs.h"
#include "gui.h"
#include "storage.h"
#include "net.h"
#define NP 8
#define NT 4
typedef struct {Frame frame;u32 useresp,ss;} UserFrame;
typedef struct {int state;u32 wake;UserFrame saved;} Thread;
typedef struct {u32 pid,state,caps;int code;u32 *pd,*pt;void *kernel_stack;Thread thread[NT];Handle handles[HANDLES];UiEvent queue[16];u32 head,tail;char name[32];} Process;
static Process processes[NP];
static Process *current;
static int current_thread;static u32 cursor;
static u32 next_pid=1,quantum_end;
static struct {u32 pid;char name[64];} leases[8];
volatile u32 user_switch_out;
u32 process_faults,process_switches,process_exits;
static u32 gdt[12],tss[26];
extern void user_enter(UserFrame *);
static void descriptor(int i,u32 base,u32 limit,u32 access,u32 flags){gdt[i*2]=(limit&65535)|(base<<16);gdt[i*2+1]=(base&0xff000000)|((base>>16)&255)|(access<<8)|(limit&0xf0000)|(flags<<20);}
void process_init(void){memset(gdt,0,sizeof(gdt));descriptor(1,0,0xfffff,0x9a,12);descriptor(2,0,0xfffff,0x92,12);descriptor(3,0,0xfffff,0xfa,12);descriptor(4,0,0xfffff,0xf2,12);tss[2]=16;tss[25]=sizeof(tss)<<16;descriptor(5,(u32)tss,sizeof(tss)-1,0x89,0);struct PACKED {u16 limit;u32 base;} p={sizeof(gdt)-1,(u32)gdt};__asm__ volatile("lgdt %0; mov $0x28,%%ax; ltr %%ax"::"m"(p):"eax","memory");object_tests();debug("USER GDT/TSS ready, CPL3 selectors 1B/23, I/O denied\n");}
static void cr3(u32 a){__asm__ volatile("mov %0,%%cr3"::"r"(a):"memory");}
static void release(Process *p){for(int i=0;i<8;i++)if(leases[i].pid==p->pid)leases[i].pid=0;handles_close(p->handles);if(p->pt){for(u32 i=0;i<1024;i++)if(p->pt[i]&1)page_free((void *)(p->pt[i]&~4095u));page_free(p->pt);}if(p->pd)page_free(p->pd);if(p->kernel_stack)page_free(p->kernel_stack);memset(p,0,sizeof(*p));}
static int map_page(Process *p,u32 index,u32 flags){void *page=page_alloc();if(!page)return 0;p->pt[index]=(u32)page|flags;return 1;}
static void initial(Thread *t,u32 entry,u32 stack){memset(t,0,sizeof(*t));t->state=1;t->wake=ticks;t->saved.frame.eip=entry;t->saved.frame.cs=0x1b;t->saved.frame.eflags=0x202;t->saved.useresp=stack;t->saved.ss=0x23;}
int process_launch(const char *path,u32 caps){return process_launch_args(path,caps,"");}
int process_launch_args(const char *path,u32 caps,const char *arguments){int n=vfs_lookup(path);if(n<0)return S_NOTFOUND;if(nodes[n].dir||nodes[n].size<sizeof(NativeHeader)||nodes[n].size>MAX_FILE)return S_INVALID;/* Scratch is reserved; this call never yields. */
 u8 *bytes=(u8 *)0xf00000;if(vfs_read(n,bytes,MAX_FILE)<0)return S_IO;NativeHeader *h=(NativeHeader *)bytes;
 if(h->magic!=NATIVE_MAGIC||h->abi!=1||h->image_bytes!=nodes[n].size||h->entry<32||h->entry>=h->data_offset||h->data_offset>h->image_bytes||(h->data_offset&4095)||h->memory_bytes<h->image_bytes||h->memory_bytes>USER_BYTES||h->reserved[0]||h->reserved[1])return S_INVALID;
 Process *p=0;for(int i=0;i<NP;i++)if(!processes[i].state){p=&processes[i];break;}if(!p||next_pid>=0x7fffffu||strlen(arguments)>255)return S_LIMIT;memset(p,0,sizeof(*p));p->pd=page_alloc();p->pt=page_alloc();p->kernel_stack=page_alloc();if(!p->pd||!p->pt||!p->kernel_stack){release(p);return S_NOMEM;}memcpy(p->pd,(void *)0x90000,4096);p->pd[USER_BASE>>22]=(u32)p->pt|7;
 for(u32 i=0;i<(h->memory_bytes+4095)/4096;i++){if(!map_page(p,i,i*4096<h->data_offset?5:7)){release(p);return S_NOMEM;}u32 offset=i*4096,length=h->image_bytes>offset?h->image_bytes-offset:0;if(length>4096)length=4096;if(length)memcpy((void *)(p->pt[i]&~4095u),bytes+offset,length);}
 if(!map_page(p,48,5)){release(p);return S_NOMEM;}memcpy((void *)(p->pt[48]&~4095u),arguments,strlen(arguments)+1);char executable[128];vfs_path(n,executable);memcpy((void *)((p->pt[48]&~4095u)+256),executable,strlen(executable)+1);
 for(u32 i=60;i<64;i++)if(!map_page(p,i,7)){release(p);return S_NOMEM;}
 p->pid=next_pid++;if(!next_pid)next_pid=1;p->state=1;p->caps=caps&7;u32 j=0;while(nodes[n].name[j]&&j<31){p->name[j]=nodes[n].name[j];j++;}initial(&p->thread[0],USER_BASE+h->entry,USER_STACK(0));debug("PROCESS start ");debug(p->name);debug(" pid=");debug_hex(p->pid);debug("\n");return (int)p->pid;
}
int process_kill(u32 pid,i32 code){for(int i=0;i<NP;i++)if(processes[i].state&&processes[i].pid==pid){processes[i].state=2;processes[i].code=code;return 0;}return S_NOTFOUND;}
void process_message(u32 pid,UiEvent e){for(int i=0;i<NP;i++)if(processes[i].state==1&&processes[i].pid==pid){Process *p=&processes[i];u32 previous=(p->head-1)&15;if(e.type==MSG_MOUSE_MOVE&&p->head!=p->tail&&p->queue[previous].type==MSG_MOUSE_MOVE){p->queue[previous]=e;return;}u32 next=(p->head+1)&15;if(next==p->tail)return;p->queue[p->head]=e;p->head=next;}}
static int range(u32 address,u32 length,int write){if(address<USER_BASE||length>0x60000u||address-USER_BASE>0x60000u-length)return 0;if(!length)return 1;u32 a=(address-USER_BASE)>>12,b=(address+length-1-USER_BASE)>>12;for(;a<=b;a++)if((current->pt[a]&(write?7u:5u))!=(write?7u:5u))return 0;return 1;}
static int string(u32 address,char *dest,u32 size){for(u32 i=0;i<size;i++){if(!range(address+i,1,0))return 0;dest[i]=*(const char *)(address+i);if(!dest[i])return 1;}return 0;}
static int homepath(u32 address,char *path,int writing){char raw[VFS_PATH];if(!string(address,raw,sizeof(raw))||raw[0]!='/')return S_INVALID;if(vfs_canonical(raw,path)<0)return S_INVALID;if(!writing)return 0;if(!(current->caps&CAP_HOME))return S_DENIED;const char *home="/home/user/";for(u32 i=0;home[i];i++)if(path[i]!=home[i])return S_DENIED;return 0;}
static void leave(Frame *f){Thread *t=&current->thread[current_thread];memcpy(&t->saved,f,sizeof(UserFrame));user_switch_out=1;cr3(0x90000);}
static int syscall(Frame *f){u32 a=f->ebx,b=f->ecx,c=f->edx;char path[VFS_PATH];int n,r;Object *o;
 if(f->eax>=SYS_SOCKET&&f->eax<=SYS_RECVFROM&&!(current->caps&CAP_NETWORK))return S_DENIED;
 switch(f->eax){
 case SYS_LEASE:{char name[64];if(!string(a,name,sizeof(name))||!name[0])return S_INVALID;int free=-1;for(int i=0;i<8;i++){if(!leases[i].pid)free=i;else if(streq(leases[i].name,name)){if(leases[i].pid!=current->pid)return S_BUSY;if(b)leases[i].pid=0;return 0;}}if(b)return S_NOTFOUND;if(free<0)return S_LIMIT;leases[free].pid=current->pid;memcpy(leases[free].name,name,strlen(name)+1);return 0;}
 case SYS_SENDTO:case SYS_RECVFROM:if(!range(b,sizeof(SockMessage),f->eax==SYS_RECVFROM))return S_INVALID;o=handle_get(current->handles,a,OBJ_SOCKET,f->eax==SYS_SENDTO?2:1);if(!o)return S_BADHANDLE;{SockMessage *m=(SockMessage *)b;if(m->size>1024||!range(m->buffer,m->size,f->eax==SYS_RECVFROM))return S_INVALID;return f->eax==SYS_SENDTO?net_sendto(o->value,(const u8 *)m->buffer,m->size,&m->address):net_recvfrom(o->value,(u8 *)m->buffer,m->size,&m->address);}
 case SYS_SOCKET:n=net_socket((int)a);if(n<0)return n;r=object_new(OBJ_SOCKET,n);if(r<0){net_close(n);return r;}n=handle_open(current->handles,r,3);if(n<0){net_close(objects[r].value);objects[r].refs=0;}return n;
 case SYS_BIND:case SYS_CONNECT:o=handle_get(current->handles,a,OBJ_SOCKET,2);if(!o)return S_BADHANDLE;if(!range(b,sizeof(SockAddress),0))return S_INVALID;return f->eax==SYS_BIND?net_bind(o->value,(const SockAddress *)b):net_connect(o->value,(const SockAddress *)b);
 case SYS_LISTEN:o=handle_get(current->handles,a,OBJ_SOCKET,2);return o?net_listen(o->value):S_BADHANDLE;
 case SYS_ACCEPT:o=handle_get(current->handles,a,OBJ_SOCKET,1);if(!o)return S_BADHANDLE;n=net_accept(o->value);if(n<0)return n;r=object_new(OBJ_SOCKET,n);if(r<0){net_close(n);return r;}n=handle_open(current->handles,r,3);if(n<0){net_close(objects[r].value);objects[r].refs=0;}return n;
 case SYS_SEND:case SYS_RECV:o=handle_get(current->handles,a,OBJ_SOCKET,f->eax==SYS_SEND?2:1);if(!o)return S_BADHANDLE;if(c>1024||!range(b,c,f->eax==SYS_RECV))return S_INVALID;return f->eax==SYS_SEND?net_send(o->value,(const u8 *)b,c):net_recv(o->value,(u8 *)b,c);
 case SYS_NET_READY:o=handle_get(current->handles,a,OBJ_SOCKET,1);return o?net_ready(o->value):S_BADHANDLE;
 case SYS_NET_INFO:if(!range(a,sizeof(NetStats),1))return S_INVALID;*(NetStats *)a=net_loop.stats;return 0;
 case SYS_PING:return net_ping(a,b);
 case SYS_DUP:return handle_duplicate(current->handles,a,b);
 case SYS_WINDOW_POLICY:o=handle_get(current->handles,a,OBJ_WINDOW,2);return o?gui_user_policy(o->value,b):S_BADHANDLE;
 case SYS_MOVE:r=homepath(a,path,1);if(r)return r;{char dest[VFS_PATH];r=homepath(b,dest,1);if(r)return r;r=vfs_move(path,dest);if(r<0)return S_IO;gui_files_changed();return 0;}
 case SYS_CLIP_GET:if(b>16384||!range(a,b,1))return S_INVALID;return gui_clip_read((u8 *)a,b);
 case SYS_CLIP_SET:if(b>16384||!range(a,b,0))return S_INVALID;return gui_clip_write((const u8 *)a,b);
 case SYS_EXIT:current->state=2;current->code=(int)a;process_exits++;leave(f);return 0;
 case SYS_YIELD:current->thread[current_thread].wake=ticks+(a>1000?1000:a);leave(f);return 0;
 case SYS_TICKS:return (int)ticks;
 case SYS_LOG:if(c>1024||!range(a,c,0))return S_INVALID;for(u32 i=0;i<c;i++)outb(0xe9,((const u8 *)a)[i]);return (int)c;
 case SYS_WINDOW:{char title[64];if(!string(a,title,sizeof(title))||b<200||b>1000||c<120||c>714)return S_INVALID;int slot=-1;for(int i=0;i<HANDLES;i++)if(!current->handles[i].generation){slot=i;break;}if(slot<0)return S_LIMIT;int obj=object_new(OBJ_WINDOW,-1);if(obj<0)return obj;int id=gui_user_open(current->pid,title,(int)b,(int)c);if(id<0){objects[obj].refs=0;return S_LIMIT;}objects[obj].value=id;return handle_open(current->handles,obj,3);}
 case SYS_SCENE:o=handle_get(current->handles,a,OBJ_WINDOW,2);if(!o)return S_BADHANDLE;if(!range(b,4,0))return S_INVALID;{u32 count=*(const u32 *)b;if(count>UI_CANVAS_COMMANDS||!range(b,4+count*sizeof(UiCommand),0))return S_INVALID;}return gui_user_scene(o->value,(const UiScene *)b);
 case SYS_EVENT:if(!range(a,sizeof(UiEvent),1))return S_INVALID;if(current->head==current->tail)return S_AGAIN;*(UiEvent *)a=current->queue[current->tail];current->tail=(current->tail+1)&15;return 0;
 case SYS_CLOSE:return handle_close(current->handles,a);
 case SYS_READ:r=homepath(a,path,0);if(r)return r;n=vfs_lookup(path);if(n<0)return S_NOTFOUND;if(c>MAX_FILE||!range(b,c,1))return S_INVALID;if(nodes[n].size>c)return S_LIMIT;return vfs_read(n,(u8 *)b,c)<0?S_IO:(int)nodes[n].size;
 case SYS_WRITE:r=homepath(a,path,1);if(r)return r;if(c>MAX_FILE||!range(b,c,0))return S_INVALID;n=vfs_lookup(path);if(n>=0&&nodes[n].dir)return S_INVALID;int created=n<0;if(created)n=vfs_create(path,0,0);if(n<0)return S_IO;if(vfs_write(n,(const u8 *)b,c,0)<0){if(created)vfs_remove(path,0);return S_IO;}gui_files_changed();return 0;
 case SYS_MKDIR:gui_files_changed();r=homepath(a,path,1);if(r)return r;return vfs_create(path,1,1)<0?S_IO:0;
 case SYS_REMOVE:gui_files_changed();r=homepath(a,path,1);if(r)return r;return vfs_remove(path,(int)b)<0?S_IO:0;
 case SYS_LIST:r=homepath(a,path,0);if(r)return r;n=vfs_lookup(path);if(n<0||!nodes[n].dir)return S_NOTFOUND;if(c>VFS_NODES||!range(b,c*sizeof(FileInfo),1))return S_INVALID;{u32 count=0;FileInfo *out=(FileInfo *)b;for(int i=1;i<VFS_NODES&&count<c;i++)if(nodes[i].used&&nodes[i].parent==n){memcpy(out[count].name,nodes[i].name,32);out[count].size=nodes[i].size;out[count++].dir=(u32)nodes[i].dir;}return (int)count;}
 case SYS_SPAWN:r=homepath(a,path,0);if(r)return r;char args[256];if(b){if(!string(b,args,sizeof(args)))return S_INVALID;}else args[0]=0;return process_launch_args(path,current->caps&CAP_HOME,args);/* no implicit network grant */
 case SYS_KILL:if(a!=current->pid&&!(current->caps&CAP_MANAGE))return S_DENIED;return process_kill(a,(int)b);
 case SYS_PROCESSES:if(c>NP||!range(b,c*sizeof(ProcessInfo),1))return S_INVALID;{u32 count=0;ProcessInfo *out=(ProcessInfo *)b;for(int i=0;i<NP&&count<c;i++)if(processes[i].state){Process *p=&processes[i];ProcessInfo *v=&out[count++];memset(v,0,sizeof(*v));v->pid=p->pid;v->state=p->state;v->exit_code=p->code;memcpy(v->name,p->name,32);for(int j=0;j<NT;j++)if(p->thread[j].state)v->threads++;for(int j=0;j<1024;j++)if(p->pt[j]&1)v->pages++;}return (int)count;}
 case SYS_SYNC:if(storage_sync())return 0;debug("SYS_SYNC failed: ");debug(storage_error);debug("\n");return S_IO;
 case SYS_THREAD:if(a<USER_BASE+32||a>=USER_BASE+USER_BYTES||!range(a,1,0)||current->pt[(a-USER_BASE)>>12]&2)return S_INVALID;for(int i=1;i<NT;i++)if(!current->thread[i].state){for(u32 j=60+i*8;j<64+(u32)i*8;j++)if(!map_page(current,j,7)){for(u32 k=60+i*8;k<j;k++){page_free((void *)(current->pt[k]&~4095u));current->pt[k]=0;}return S_NOMEM;}initial(&current->thread[i],a,USER_STACK(i));return i;}return S_LIMIT;
 case SYS_THREAD_EXIT:current->thread[current_thread].state=0;/* pages reclaimed with process; thread slots not reused until their stack is freed */int live=0;for(int i=0;i<NT;i++)if(current->thread[i].state)live=1;if(!live)current->state=2;for(u32 j=60+current_thread*8;j<64+(u32)current_thread*8;j++){page_free((void *)(current->pt[j]&~4095u));current->pt[j]=0;}leave(f);return 0;
 case SYS_EVENT_CREATE:n=object_new(OBJ_EVENT,0);if(n<0)return n;r=handle_open(current->handles,n,3);if(r<0)objects[n].refs=0;return r;
 case SYS_EVENT_SET:o=handle_get(current->handles,a,OBJ_EVENT,2);if(!o)return S_BADHANDLE;o->value=!!b;return 0;
 case SYS_EVENT_WAIT:o=handle_get(current->handles,a,OBJ_EVENT,1);if(!o)return S_BADHANDLE;return o->value?0:S_AGAIN;
 default:return S_INVALID;
 }}
int process_interrupt(Frame *f){if((f->cs&3)!=3)return 0;if(!current)panic("Orphan user frame",f->eip,0);if(f->vector<32){process_faults++;current->code=-(int)f->vector;current->state=2;debug("USER fault contained pid=");debug_hex(current->pid);debug(" vector=");debug_hex(f->vector);debug("\n");leave(f);return 1;}if(f->vector==0x80){if(current->caps&CAP_NETWORK)net_poll();int r=syscall(f);f->eax=(u32)r;if(user_switch_out)current->thread[current_thread].saved.frame.eax=(u32)r;return 1;}if(f->vector==32&&(i32)(ticks-quantum_end)>=0)leave(f);return 0;}
void process_step(void){for(int i=0;i<NP;i++)if(processes[i].state==2)release(&processes[i]);for(int i=0;i<NP*NT;i++){int which=cursor++%(NP*NT);Process *p=&processes[which/NT];Thread *t=&p->thread[which%NT];if(p->state!=1||t->state!=1||(i32)(ticks-t->wake)<0)continue;current=p;current_thread=which%NT;user_switch_out=0;quantum_end=ticks+2;tss[1]=(u32)p->kernel_stack+4096;cli();cr3((u32)p->pd);process_switches++;user_enter(&t->saved);cr3(0x90000);current=0;sti();break;}}
void process_tests(void){debug("PROCESS address spaces initialized (8 processes x 4 threads)\n");}

int process_list(ProcessInfo *out,u32 cap){u32 count=0;for(int i=0;i<NP&&count<cap;i++)if(processes[i].state){Process *p=&processes[i];ProcessInfo *v=&out[count++];memset(v,0,sizeof(*v));v->pid=p->pid;v->state=p->state;v->exit_code=p->code;memcpy(v->name,p->name,32);for(int j=0;j<NT;j++)if(p->thread[j].state)v->threads++;for(int j=0;j<1024;j++)if(p->pt[j]&1)v->pages++;}return (int)count;}
