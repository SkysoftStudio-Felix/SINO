#include "kernel.h"
#include "gui.h"
#include "storage.h"
#include "disk.h"
#include "settings.h"
#include "process.h"
#include "vfs.h"
#include "net.h"
#include "keyboard.h"
void *memcpy(void *d,const void *s,u32 n){void *result=d;u32 words=n/4,tail=n&3;__asm__ volatile("rep movsl":"+D"(d),"+S"(s),"+c"(words)::"memory");u8 *p=d;const u8 *q=s;while(tail--)*p++=*q++;return result;}
void *memset(void *d,int v,u32 n){void *result=d;u32 words=n/4,tail=n&3,pattern=(u8)v*0x01010101u;__asm__ volatile("rep stosl":"+D"(d),"+c"(words):"a"(pattern):"memory");u8 *p=d;while(tail--)*p++=(u8)v;return result;}
u32 strlen(const char *s){u32 n=0;while(s[n])n++;return n;}
int streq(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
void debug(const char *s){while(*s)outb(0xe9,(u8)*s++);}
void debug_hex(u32 v){const char *h="0123456789ABCDEF";debug("0x");for(int n=28;n>=0;n-=4)outb(0xe9,h[(v>>n)&15]);}
void number(u32 v,char *s){char t[11];u32 n=0;do{t[n++]=(char)('0'+v%10);v/=10;}while(v);u32 j=0;while(n)s[j++]=t[--n];s[j]=0;}
void panic(const char *s,u32 v,u32 e){cli();debug("KERNEL PANIC: ");debug(s);debug(" ");debug_hex(v);debug(" ");debug_hex(e);debug("\n");console_panic(s,v,e);for(;;)halt();}

volatile u32 ticks,keyboard_irqs,mouse_irqs,mouse_packets;
volatile i32 mouse_x=512,mouse_y=384;
volatile u8 mouse_buttons,mouse_dirty;
int mouse_present;
int mouse_absolute;
u32 fault_resume,fault_expected,fault_count;
static volatile u8 keys[128];
static volatile u32 key_head,key_tail;
static volatile MouseEvent mouse_events[128];
static volatile u32 mouse_head,mouse_tail;
volatile u32 mouse_overflows;
static u8 packet[3],packet_pos;
typedef struct PACKED {u16 low,selector;u8 zero,flags;u16 high;} Gate;
typedef struct PACKED {u16 limit;u32 base;} TablePointer;
static Gate idt[256];
extern void *isr_table[256];
static void wait_input(void){for(u32 n=0;n<100000;n++)if(!(inb(0x64)&2))return;}
static void controller(u8 v){wait_input();outb(0x64,v);}
static void data(u8 v){wait_input();outb(0x60,v);}
static int device_command(int mouse,u8 cmd){
    if(mouse)controller(0xd4);data(cmd);
    for(u32 n=0;n<200000;n++){u8 s=inb(0x64);if(s&1){u8 v=inb(0x60);if(!!(s&0x20)==mouse)return v==0xfa;}}
    return 0;
}
static void mouse_emit(i32 x,i32 y,u8 buttons,i8 wheel){
    u8 changed=mouse_buttons^buttons;mouse_buttons=buttons;mouse_x=x<0?0:x>=GUI_WIDTH?GUI_WIDTH-1:x;mouse_y=y<0?0:y>=GUI_HEIGHT?GUI_HEIGHT-1:y;
    u32 previous=(mouse_head-1)&127,next=(mouse_head+1)&127;
    if(!changed&&!wheel&&mouse_head!=mouse_tail&&!mouse_events[previous].changed&&!mouse_events[previous].wheel&&mouse_events[previous].buttons==buttons){mouse_events[previous].x=mouse_x;mouse_events[previous].y=mouse_y;mouse_events[previous].time=ticks;}
    else{if(next==mouse_tail){mouse_tail=(mouse_tail+1)&127;mouse_overflows++;}mouse_events[mouse_head]=(MouseEvent){mouse_x,mouse_y,ticks,buttons,changed,wheel};mouse_head=next;}mouse_dirty=1;
}
typedef struct {u32 a,b,c,d;} VmReply;
static VmReply vm_call(u32 command,u32 argument){u32 a=0x564d5868,b=argument,c=command,d=0x5658;__asm__ volatile("inl %%dx,%%eax":"+a"(a),"+b"(b),"+c"(c),"+d"(d)::"memory");return(VmReply){a,b,c,d};}
static void vm_mouse_init(void){
    VmReply r=vm_call(10,0);if(r.b!=0x564d5868)return;
    (void)vm_call(41,0x45414552);r=vm_call(40,0);if((r.a>>16)||!(r.a&65535))return;r=vm_call(39,1);if(r.a!=0x3442554a)return;
    (void)vm_call(41,0x53424152);mouse_absolute=1;debug("VMMOUSE absolute pointer ready\n");
}
void mouse_poll(void){
    if(!mouse_absolute)return;u32 flags;__asm__ volatile("pushfl; popl %0":"=r"(flags));cli();
    for(int i=0;i<64;i++){VmReply r=vm_call(40,0);if((r.a>>16)||(r.a&65535)<4)break;r=vm_call(39,4);u8 buttons=(r.a&0x20?1:0)|(r.a&0x10?2:0)|(r.a&8?4:0);mouse_packets++;mouse_emit((i32)((r.b&65535)*(GUI_WIDTH-1)/65535),(i32)((r.c&65535)*(GUI_HEIGHT-1)/65535),buttons,(i8)r.d);}
    if(flags&0x200)sti();
}
static void mouse_byte(u8 v){
    if(!packet_pos&&!(v&8))return;
    packet[packet_pos++]=v;if(packet_pos!=3)return;packet_pos=0;mouse_packets++;
    i32 x=mouse_x,y=mouse_y;
    if(!(packet[0]&0xc0)){
        x+=packet[1]-((packet[0]&0x10)?256:0);y-=packet[2]-((packet[0]&0x20)?256:0);
    }
    /* Coalesce motion only. Preserve button edges even during a slow repaint. */
    mouse_emit(x,y,packet[0]&7,0);
}
int mouse_read(MouseEvent *event){
    u32 flags;__asm__ volatile("pushfl; popl %0":"=r"(flags));cli();int ready=mouse_head!=mouse_tail;
    if(ready){*event=mouse_events[mouse_tail];mouse_tail=(mouse_tail+1)&127;}mouse_dirty=mouse_head!=mouse_tail;
    if(flags&0x200)sti();return ready;
}
void irq_dispatch(Frame *f){
    if(f->vector<32||f->vector==0x80)if(process_interrupt(f))return;
    if(f->vector<32){
        if(f->vector==3){debug("BREAKPOINT exception handled\n");return;}
        if(fault_expected==f->vector&&fault_resume){
            fault_count++;f->eip=fault_resume;fault_expected=0;fault_resume=0;
            debug("EXPECTED exception handled: ");debug_hex(f->vector);debug("\n");return;
        }
        debug("Fault EIP=");debug_hex(f->eip);debug(" CR2=");u32 cr2;__asm__ volatile("mov %%cr2,%0":"=r"(cr2));debug_hex(cr2);debug("\n");
        panic("CPU exception",f->vector,f->error);
    }
    if(f->vector==0x80){
        if(f->eax==2){f->eax=32;return;}
        extern u32 native_base,native_size;extern int native_running;
        if(!native_running){f->eax=0xffffffffu;return;}
        if(f->eax==0){f->eip=(u32)native_exit;return;}
        if(f->eax==1&&f->ebx>=native_base&&f->edx<=native_size&&f->ebx-native_base<=native_size-f->edx){
            native_write((const char *)f->ebx,f->edx);f->eax=f->edx;return;
        }f->eax=0xffffffffu;return;
    }
    if(f->vector>=32&&f->vector<48){
        u32 irq=f->vector-32;
        if(irq==0){ticks++;process_interrupt(f);}
        if(irq==1||irq==12){
            u8 s=inb(0x64);if(s&1){u8 v=inb(0x60);
                if(s&0x20){mouse_irqs++;if(mouse_absolute)mouse_poll();else mouse_byte(v);}
                else{keyboard_irqs++;u32 next=(key_head+1)&127;if(next!=key_tail){keys[key_head]=v;key_head=next;}}
            }
        }
        if(irq>=8)outb(0xa0,0x20);outb(0x20,0x20);return;
    }
    panic("Unexpected interrupt",f->vector,f->eip);
}
void arch_init(void){
    cli();outb(0x21,0xff);outb(0xa1,0xff);
    for(u32 i=0;i<256;i++){u32 a=(u32)isr_table[i];idt[i]=(Gate){(u16)a,8,0,0x8e,(u16)(a>>16)};}
    idt[0x80].flags=0xee;
    TablePointer p={sizeof(idt)-1,(u32)idt};__asm__ volatile("lidt %0"::"m"(p));
    outb(0x20,0x11);outb(0x80,0);outb(0xa0,0x11);outb(0x80,0);
    outb(0x21,0x20);outb(0x80,0);outb(0xa1,0x28);outb(0x80,0);
    outb(0x21,4);outb(0xa1,2);outb(0x21,1);outb(0xa1,1);
    outb(0x21,0xff);outb(0xa1,0xff);
    outb(0x43,0x36);outb(0x40,(u8)(1193182/100));outb(0x40,(u8)((1193182/100)>>8));
    controller(0xad);controller(0xa7);
    for(u32 i=0;i<64&&(inb(0x64)&1);i++)(void)inb(0x60);
    controller(0x60);data(0x43);controller(0xae);controller(0xa8);
    if(!device_command(0,0xf4))debug("Keyboard ACK unavailable; continuing\n");
    mouse_present=device_command(1,0xf6)&&device_command(1,0xf4);
    outb(0x21,0xf8);outb(0xa1,mouse_present?0xef:0xff);
    debug("IDT 256 gates, PIC remapped, PIT 100 Hz\n");
    debug(mouse_present?"32-bit PS/2 mouse IRQ12 ready\n":"PS/2 mouse unavailable\n");
    if(mouse_present)vm_mouse_init();
}
int key_read(void){
    while(key_head!=key_tail){u8 v=keys[key_tail];key_tail=(key_tail+1)&127;
        int key=keyboard_decode(v);if(key)return key;
    }return 0;
}
void ps2_byte(int aux,u8 value){controller(aux?0xd3:0xd2);data(value);for(u32 n=0;n<200000&&(inb(0x64)&1);n++)__asm__ volatile("nop");}

static u8 pages[16384];
u32 free_pages,usable_memory;
typedef struct PACKED {u32 base,base_hi,length,length_hi,type,attributes;} MemoryRange;
void memory_init(void){
    memset(pages,2,sizeof(pages));
    u32 count=*(volatile u16 *)0x4f00;const MemoryRange *map=(const MemoryRange *)0x5000;
    if(!count||count>32)panic("Invalid E820 memory map",count,0);
    for(u32 i=0;i<count;i++)if(map[i].type==1&&!map[i].base_hi){
        u32 start=map[i].base,end=map[i].length_hi?0x4000000:start+map[i].length;
        if(end<start||end>0x4000000)end=0x4000000;
        if(end>usable_memory)usable_memory=end;
        /* GUI: backbuffer 8-14 MB, assets 7 MB, copy scratch 15 MB,
           desktop 16-22 MB, front 22-28 MB, window caches 28-52 MB. */
        start=(start+4095)&~4095u;if(start<0x3400000)start=0x3400000;
        for(u32 a=start;a+4096<=end&&a<0x4000000;a+=4096)if(pages[a>>12]){pages[a>>12]=0;free_pages++;}
    }
    if(!free_pages)panic("At least 64 MB RAM required",usable_memory,0);
    u32 *pd=(u32 *)0x90000;u32 *pt=(u32 *)0x1e0000;memset(pd,0,4096);
    for(u32 i=0;i<16384;i++)pt[i]=(i*4096)|3;
    pt[0]=0;for(u32 i=0;i<16;i++)pd[i]=(0x1e0000+i*4096)|3;
    const u8 *vbe=(const u8 *)0x5800;u32 fb=*(const u32 *)(vbe+40);
    if(!fb||*(const u16 *)(vbe+18)!=GUI_WIDTH||*(const u16 *)(vbe+20)!=GUI_HEIGHT||vbe[25]!=32||vbe[27]!=6||vbe[31]!=8||vbe[32]!=16||vbe[33]!=8||vbe[34]!=8||vbe[35]!=8||vbe[36]!=0)panic("VBE direct-color mode unavailable",fb,vbe[25]);
    if((fb>>22)<16||(fb>>22)>1022)panic("Invalid VBE framebuffer",fb,0);
    for(u32 bank=0;bank<3;bank++){u32 *video=(u32 *)(0x1f0000+bank*4096);for(u32 i=0;i<1024;i++)video[i]=((fb&0xffc00000u)+(bank*1024+i)*4096)|0x1b;pd[(fb>>22)+bank]=(u32)video|3;}
    __asm__ volatile("mov %0,%%cr3"::"r"(0x90000):"memory");
    u32 cr0;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));cr0=(cr0&~0x60000000u)|0x80010000u;
    __asm__ volatile("mov %0,%%cr0"::"r"(cr0):"memory");
    debug("PAGING enabled: CR0=");debug_hex(cr0);debug(" CR3=0x00090000, null page unmapped\n");
    debug("VBE framebuffer mapped: ");debug_hex(fb);debug(" 1024x768x32\n");
}
void *page_alloc(void){for(u32 i=13312;i<16384;i++)if(!pages[i]){pages[i]=1;free_pages--;void *p=(void *)(i*4096);memset(p,0,4096);return p;}return (void *)0;}
void *page_alloc_run(u32 count){if(!count||count>64)return 0;for(u32 i=13312;i+count<=16384;i++){u32 j=0;while(j<count&&!pages[i+j])j++;if(j==count){for(j=0;j<count;j++)pages[i+j]=1;free_pages-=count;void *p=(void *)(i*4096);memset(p,0,count*4096);return p;}i+=j;}return 0;}
 int page_free_run(void *p,u32 count){u32 a=(u32)p;if(!count||count>64||(a&4095)||a<0x3400000||a>=0x4000000||count>(0x4000000-a)/4096)return 0;for(u32 i=0;i<count;i++)if(pages[(a>>12)+i]!=1)return 0;for(u32 i=0;i<count;i++)pages[(a>>12)+i]=0;free_pages+=count;return 1;}
 int page_free(void *p){u32 a=(u32)p;if((a&4095)||a<0x3400000||a>=0x4000000||pages[a>>12]!=1)return 0;pages[a>>12]=0;free_pages++;return 1;}

File files[224];u32 file_count;
static u32 fat_offset,root_offset,data_offset,cluster_count,root_count;
static u32 le16(const u8 *p){return p[0]|((u32)p[1]<<8);}
static u32 le32(const u8 *p){return le16(p)|(le16(p+2)<<16);}
int fs_init(void){
    if(le16(RAMDISK+510)!=0xaa55||le16(RAMDISK+11)!=512||RAMDISK[13]!=1)return 0;
    u32 reserved=le16(RAMDISK+14),fats=RAMDISK[16],fat_size=le16(RAMDISK+22);root_count=le16(RAMDISK+17);
    fat_offset=reserved*512;root_offset=(reserved+fats*fat_size)*512;data_offset=root_offset+((root_count*32+511)/512)*512;
    if(root_count>224||data_offset>=RAMDISK_SIZE||fat_size<1)return 0;
    cluster_count=(RAMDISK_SIZE-data_offset)/512;if(cluster_count>=4085)return 0;
    file_count=0;
    for(u32 i=0;i<root_count;i++){const u8 *e=RAMDISK+root_offset+i*32;if(!e[0])break;if(e[0]==0xe5||(e[11]&0x18))continue;
        File *f=&files[file_count++];u32 n=0;for(u32 j=0;j<8;j++)if(e[j]!=' ')f->name[n++]=(char)e[j];
        if(e[8]!=' '){f->name[n++]='.';for(u32 j=8;j<11&&e[j]!=' ';j++)f->name[n++]=(char)e[j];}f->name[n]=0;f->size=le32(e+28);f->cluster=(u16)le16(e+26);
    }debug("FAT12 RAM disk ready, full 1.44 MB preloaded\n");return 1;
}
int fs_read(u32 index,u8 *dest,u32 cap){
    if(index>=file_count||files[index].size>cap)return 0;
    u32 left=files[index].size,c=files[index].cluster,steps=0;
    while(left){if(c<2||c>=cluster_count+2||++steps>cluster_count)return 0;
        u32 off=data_offset+(c-2)*512,n=left>512?512:left;if(off+n>RAMDISK_SIZE)return 0;memcpy(dest,RAMDISK+off,n);dest+=n;left-=n;
        u32 fat=fat_offset+c+c/2;if(fat+1>=root_offset)return 0;u32 next=le16(RAMDISK+fat);c=(c&1)?next>>4:next&0xfff;
    }return 1;
}

void kernel_tests(void){
    u32 before=free_pages;u32 *a=page_alloc(),*b=page_alloc();if(!a||!b||a==b)panic("Page allocation",0,0);
    *a=0x46454c49;*b=0x58333221;if(*a!=0x46454c49||*b!=0x58333221)panic("Memory read/write",0,0);
    if(!page_free(a)||!page_free(b)||page_free(a)||page_free((void *)0x200000)||free_pages!=before)panic("Page free validation",free_pages,before);
    void *run=page_alloc_run(2);if(!run||!page_free_run(run,2)||page_free_run(run,2)||free_pages!=before)panic("Page run",0,0);
    debug("PAGE allocator self-check passed\n");
    __asm__ volatile("int3");
    u32 v=2;__asm__ volatile("int $0x80":"+a"(v)::"memory");if(v!=32)panic("INT80 ABI",v,32);
#ifdef SELFTEST
    test_invalid_opcode();test_page_fault();if(fault_count!=2)panic("Exception recovery",fault_count,2);
    debug("FAULT recovery self-check passed\n");
#endif
}
void kmain(void){
    debug("SINO 0.10 native 32-bit kernel entered, CS=0x08\n");
    arch_init();memory_init();if(!fs_init())panic("FAT12 mount failed",0,0);
    process_init();net_init();kernel_tests();net_tests();shell_init();disk_init();storage_init();settings_init();gui_init();sti();
    u32 begin=ticks;while(ticks-begin<3)halt();debug("TIMER IRQ0 delivered in protected mode\n");
#ifdef SELFTEST
    shell_tests();
    gui_tests();
#endif
    if(vfs_lookup("/home/user/.sino/rollback/journal")>=0)process_launch_args("/boot/setup.snx",CAP_HOME,"/recover-only");
    debug("SINO desktop ready\n");
    for(;;){gui_step();net_poll();process_step();cli();sti();halt();}
}
