/* Persistent display and language preferences, with timed display rollback. */
#include "settings.h"
#include "graphics.h"
#include "vfs.h"
#include "storage.h"
#include "disk.h"
#define INK 0x253044u
#define BLUE 0x3265a8u
int settings_language,settings_pending;
static int tab=1,rw=1024,rh=768,rs=100,rc=100,rl;
static int ow,oh,os,oc,ol;
static int slider_drag;
static u32 deadline,last_count;
static const char *status="选择设置后点击应用。";
static const int modes[][2]={{800,600},{1024,768},{1280,720},{1280,960},{1600,900},{1920,1080}};
static const char *mode_names[]={"800 x 600","1024 x 768","1280 x 720","1280 x 960","1600 x 900","1920 x 1080"};
static const int scales[]={100,125,150,200};
typedef struct {u32 magic,version,width,height,scale,contrast,language,reserved;} Preferences;
#define PREF_MAGIC 0x32474643u
static int hit(int x,int y,int a,int b,int w,int h){return x>=a&&x<a+w&&y>=b&&y<b+h;}
static void choice(int x,int y,int w,const char *text,int selected){g_round(x,y,w,30,4,selected?0xdbe9fb:0xf0f2f6);g_box(x,y,w,30,selected?BLUE:0xc7ced8,selected?0xdbe9fb:0xf0f2f6);g_center(x,y+5,w,text,selected?BLUE:INK);}
void settings_language_set(int language){settings_language=language?1:0;if(gui_active)gui_refresh();}
static int valid(const Preferences *p){return p->magic==PREF_MAGIC&&p->version==1&&p->width>=800&&p->width<=1920&&p->height>=540&&p->height<=1200&&p->scale>=100&&p->scale<=200&&p->contrast>=60&&p->contrast<=140&&p->language<=1;}
void settings_init(void){Preferences p;int n=vfs_lookup("/home/user/.sino-settings");int found=n>=0&&vfs_read(n,(u8 *)&p,sizeof(p))==(int)sizeof(p)&&valid(&p);if(!found&&*(volatile u8 *)0x4f0c>=0x80){u8 sector[512];int d=storage_disk>=0?storage_disk:*(volatile u8 *)0x4f0c-0x80;if(d<4&&disk_read(d,1,sector)){memcpy(&p,sector,sizeof(p));found=valid(&p);}}if(found){rw=(int)p.width;rh=(int)p.height;rs=(int)p.scale;rc=(int)p.contrast;settings_language=rl=(int)p.language;}}
void settings_boot(void){if(!graphics_mode(rw,rh,rs,rc)){rw=1024;rh=768;rs=rc=100;(void)graphics_mode(rw,rh,rs,rc);}}
static void stage_current(void){rw=display_width;rh=display_height;rs=display_scale;rc=display_contrast;rl=settings_language;}
void settings_open(void){if(!settings_pending){stage_current();status="选择设置后点击应用。";}}
static void persist(void){Preferences p={PREF_MAGIC,1,(u32)display_width,(u32)display_height,(u32)display_scale,(u32)display_contrast,(u32)settings_language,0};int n=vfs_lookup("/home/user/.sino-settings");if(n<0)n=vfs_create("/home/user/.sino-settings",0,0);if(n<0||vfs_write(n,(const u8 *)&p,sizeof(p),0)<0){status="设置保存失败，请检查数据磁盘。";return;}if(!storage_ready){status="设置已应用，本次会话有效（无数据磁盘）。";return;}status=storage_sync()?"设置已保存，重启后仍然生效。":"设置保存失败，请检查数据磁盘。";}
void settings_cancel(void){if(!settings_pending)return;(void)graphics_mode(ow,oh,os,oc);settings_language=ol;settings_pending=0;stage_current();status="已恢复之前的显示设置。";gui_refresh();}
static void apply(void){if(settings_pending)return;ow=display_width;oh=display_height;os=display_scale;oc=display_contrast;ol=settings_language;if(!graphics_mode(rw,rh,rs,rc)){status="此分辨率与缩放组合不可用，请选择其他组合。";return;}settings_language=rl;settings_pending=1;deadline=ticks+1500;last_count=16;status="";gui_refresh();}
int settings_poll(void){if(!settings_pending)return 0;if((i32)(ticks-deadline)>=0){settings_cancel();return 1;}u32 count=(deadline-ticks+99)/100;if(count!=last_count){last_count=count;return 1;}return 0;}
int settings_mouse(int x,int y,int held){(void)y;if(!held){slider_drag=0;return 0;}if(!slider_drag||settings_pending)return 0;int next=60+(x-185)*80/350;if(next<60)next=60;if(next>140)next=140;if(next==rc)return 0;rc=next;return 1;}
void settings_draw(int w,int h){
 g_fill(2,30,w-4,h-32,0xf9fafc);g_fill(2,30,155,h-32,0xebeff5);g_text(1,18,48,"系统设置",INK);
 const char *tabs[]={"系统","显示","语言","关于"};for(int i=0;i<4;i++){if(tab==i)g_round(10,99+i*44,137,36,4,0xd4e3f6);g_text(0,27,108+i*44,tabs[i],tab==i?BLUE:INK);}
 int x=181;g_text(1,x,48,tabs[tab],INK);
 if(tab==0){g_text(0,x,100,"SINO 操作系统",INK);g_text(0,x,140,"版本 0.10  /  SINO32",BLUE);g_text(0,x,178,"32 位 x86，自行编写的内核、驱动和桌面。",INK);g_text(0,x,216,*(volatile u8 *)0x4f0c>=0x80?"启动来源：硬盘安装版":"启动来源：ISO 光盘 / 体验模式",INK);g_text(0,x,254,storage_ready?"个人文件：持久化数据磁盘":"个人文件：内存临时模式",INK);char b[16];number(free_pages*4,b);g_text(0,x,292,"空闲内存（KB）",INK);g_text(0,x+160,292,b,BLUE);choice(x,345,165,"显示设置",0);choice(x+180,345,165,"语言设置",0);}
 else if(tab==1){g_text(0,x,96,"屏幕分辨率",INK);for(int i=0;i<6;i++)choice(x+(i%3)*166,126+(i/3)*38,155,mode_names[i],rw==modes[i][0]&&rh==modes[i][1]);g_text(0,x,212,"缩放比例",INK);for(int i=0;i<4;i++){char b[16];number((u32)scales[i],b);u32 n=strlen(b);b[n]='%';b[n+1]=0;choice(x+i*110,242,99,b,rs==scales[i]);}g_text(0,x,296,"对比度",INK);g_fill(x+4,343,350,4,0xc4ccd7);int pos=x+4+(rc-60)*350/80;g_round(pos-6,335,13,21,5,BLUE);char b[16];number((u32)rc,b);g_text(0,x+381,335,b,INK);g_text(0,x,381,"缩放会调整文字、控件和可用桌面空间。",0x667388);}
 else if(tab==2){g_text(0,x,100,"系统显示语言",INK);choice(x,145,212,"简体中文",rl==0);choice(x+228,145,212,"English",rl==1);g_text(0,x,211,"语言切换不改变个人文件名或正文。",0x667388);g_text(0,x,257,"中文输入：Ctrl+空格切换拼音。",INK);g_text(0,x,293,"空格选首字，1-7 选字，[ ] 翻页。",INK);g_text(0,x,329,"支持单字及常用词，暂不支持整句预测。",0x667388);}
 else {g_text(1,x,104,"SINO 0.10",BLUE);g_text(0,x,153,"内核、桌面、安装程序：项目自写 C / 汇编。",INK);g_text(0,x,195,"开源组件：QEMU、Unifont、经典字库。",INK);g_text(0,x,237,"拼音单字数据：pinyin-data（MIT）。",INK);g_text(0,x,279,"BIOS / MBR 启动，SINO 自有数据文件系统。",INK);g_text(0,x,321,"原生应用在用户态，旧版内置应用仍在内核。",0x667388);g_text(0,x,359,"源代码及各组件许可证随系统保留。",0x667388);}
 if(settings_pending){int y=h-149;g_box(174,y,w-190,136,BLUE,0xe8f0fc);g_text(0,190,y+12,"保留这些显示设置？超时将自动还原。",INK);char b[12];number((deadline-ticks+99)/100,b);g_text(0,190,y+40,b,BLUE);g_text(0,227,y+40,"秒",BLUE);choice(w-266,y+85,112,"还原",0);choice(w-142,y+85,112,"保留更改",1);}
 else {g_cliptext(x,h-88,w-x-18,status,0x667388);choice(w-266,h-49,112,"恢复默认",0);choice(w-142,h-49,112,"应用",1);}
}
int settings_click(int x,int y,int w,int h){if(settings_pending){if(hit(x,y,w-266,h-64,112,30))settings_cancel();else if(hit(x,y,w-142,h-64,112,30)){persist();settings_pending=0;gui_refresh();}return 1;}for(int i=0;i<4;i++)if(hit(x,y,10,99+i*44,137,36)){tab=i;return 1;}if(hit(x,y,w-266,h-49,112,30)){rw=1024;rh=768;rs=rc=100;rl=0;status="点击应用以恢复默认设置。";return 1;}if(hit(x,y,w-142,h-49,112,30)){apply();return 1;}if(tab==0&&y>=345&&y<375){if(x>=181&&x<346)tab=1;else if(x>=361&&x<526)tab=2;}if(tab==1){for(int i=0;i<6;i++)if(hit(x,y,181+(i%3)*166,126+(i/3)*38,155,30)){rw=modes[i][0];rh=modes[i][1];if(rw*100/rs>1280||rh*100/rs>960)rs=150;}for(int i=0;i<4;i++)if(hit(x,y,181+i*110,242,99,30))rs=scales[i];if(hit(x,y,179,327,370,35)){slider_drag=1;rc=60+(x-185)*80/350;if(rc<60)rc=60;if(rc>140)rc=140;}}else if(tab==2){if(hit(x,y,181,145,212,30))rl=0;else if(hit(x,y,409,145,212,30))rl=1;}return 1;}

/* Only translate immutable UI literals. RAM-backed filenames/text are exempt. */
extern char __locale_literals_start[],__locale_literals_end[];
typedef struct {const char *zh,*en;} Translation;
#include "locale.inc"
const char *settings_text(const char *text){if(!settings_language||(u32)text<(u32)__locale_literals_start||(u32)text>=(u32)__locale_literals_end)return text;for(u32 i=0;i<sizeof(translations)/sizeof(translations[0]);i++)if(streq(text,translations[i].zh))return translations[i].en;return text;}
