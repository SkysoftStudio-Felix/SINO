#ifndef SINO_KERNEL_H
#define SINO_KERNEL_H
#include "types.h"
#define RAMDISK ((const u8 *)0x200000)
#define RAMDISK_SIZE 1474560u
#define MAX_FILE 65536u
static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void outw(u16 p,u16 v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static inline u16 inw(u16 p){u16 v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void cli(void){__asm__ volatile("cli":::"memory");}
static inline void sti(void){__asm__ volatile("sti":::"memory");}
static inline void halt(void){__asm__ volatile("hlt");}
void *memcpy(void *,const void *,u32);
void *memset(void *,int,u32);
u32 strlen(const char *);
int streq(const char *,const char *);
void debug(const char *);
void debug_hex(u32);
void number(u32,char *);
void panic(const char *,u32,u32);
typedef struct {u32 edi,esi,ebp,esp,ebx,edx,ecx,eax,vector,error,eip,cs,eflags;} Frame;
extern volatile u32 ticks, keyboard_irqs, mouse_irqs, mouse_packets;
extern volatile i32 mouse_x, mouse_y;
extern volatile u8 mouse_buttons, mouse_dirty;
extern u32 free_pages, usable_memory, fault_resume, fault_expected, fault_count;
extern int mouse_present;
void arch_init(void);
void memory_init(void);
void *page_alloc(void);
void *page_alloc_run(u32);
int page_free_run(void *,u32);
int page_free(void *);
int key_read(void);
typedef struct {i32 x,y;u32 time;u8 buttons,changed;i8 wheel;} MouseEvent;
int mouse_read(MouseEvent *);
void mouse_poll(void);
extern int mouse_absolute;
extern volatile u32 mouse_overflows;
void ps2_byte(int,u8);
void native_write(const char *,u32);
void run_native(u32);
void native_exit(void);
void test_page_fault(void);
void test_invalid_opcode(void);
typedef struct {char name[13];u32 size;u16 cluster;u16 reserved;} File;
extern File files[224];
extern u32 file_count;
int fs_init(void);
int fs_read(u32,u8 *,u32);
void shell_init(void);
void shell_step(void);
void shell_text(const char *);
void shell_key(int);
void console_panic(const char *,u32,u32);
void kernel_tests(void);
void shell_tests(void);
#endif
