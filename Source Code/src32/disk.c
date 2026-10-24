/* Bounded, LBA28 ATA PIO. Only explicitly enumerated IDE disks are writable. */
#include "disk.h"
Disk disks[4];
static u16 port(int d){return d<2?0x1f0:0x170;}
static void delay(int d){u16 p=d<2?0x3f6:0x376;for(int i=0;i<4;i++)(void)inb(p);}
static int wait(int d,int data){u16 p=port(d);for(u32 n=0;n<8000000;n++){u8 s=inb(p+7);if(!s||s==255)return 0;if(!(s&128)){if(data&&(s&0x21))return 0;/* A command may begin/finish only after DRQ is clear; BSY alone
 does not establish that the previous PIO data phase has completed. */if(data?(s&8):!(s&8))return 1;}}return 0;}
static void select(int d,u32 lba){u16 p=port(d);outb(p+6,(u8)(0xe0|((d&1)<<4)|((lba>>24)&15)));delay(d);}
void disk_init(void){for(int d=0;d<4;d++){u16 p=port(d);disks[d].present=0;outb(p+6,(u8)(0xa0|((d&1)<<4)));delay(d);outb(p+2,0);outb(p+3,0);outb(p+4,0);outb(p+5,0);outb(p+7,0xec);if(!wait(d,1))continue;u16 words[256];u16 *to=words;u32 count=256;__asm__ volatile("rep insw":"+D"(to),"+c"(count):"d"(p):"memory");delay(d);u32 sectors=words[60]|((u32)words[61]<<16);if(!sectors)continue;if(sectors>0x10000000)sectors=0x10000000;disks[d].present=1;disks[d].sectors=sectors;for(int i=0;i<20;i++){disks[d].model[i*2]=(char)(words[27+i]>>8);disks[d].model[i*2+1]=(char)words[27+i];}disks[d].model[40]=0;for(int i=39;i>=0&&disks[d].model[i]==' ';i--)disks[d].model[i]=0;}}
static int transfer(int d,u32 lba,u8 *buffer,int write){if(d<0||d>=4||!disks[d].present||lba>=disks[d].sectors)return 0;u16 p=port(d);select(d,lba);if(!wait(d,0))return 0;outb(p+2,1);outb(p+3,(u8)lba);outb(p+4,(u8)(lba>>8));outb(p+5,(u8)(lba>>16));outb(p+7,write?0x30:0x20);if(!wait(d,1))return 0;u32 count=256;if(write){const u8 *from=buffer;__asm__ volatile("rep outsw":"+S"(from),"+c"(count):"d"(p):"memory");}else __asm__ volatile("rep insw":"+D"(buffer),"+c"(count):"d"(p):"memory");delay(d);return wait(d,0)&&!(inb(p+7)&0x21);}
int disk_read(int d,u32 lba,u8 *to){return transfer(d,lba,to,0);}
int disk_write(int d,u32 lba,const u8 *from){return transfer(d,lba,(u8 *)from,1);}
int disk_flush(int d){if(d<0||d>=4||!disks[d].present)return 0;select(d,0);if(!wait(d,0))return 0;outb(port(d)+7,0xe7);return wait(d,0)&&!(inb(port(d)+7)&0x21);}
