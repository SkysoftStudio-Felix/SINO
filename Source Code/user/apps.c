/* Application manager is a separate ring-3 executable. */
#include "package.h"
static FileInfo list[128];static UiScene scene;static AppRecord record;static int selected,count,window,first;
static void refresh(void){count=List("/home/user/.sino/apps",list,128);if(count<0)count=0;if(selected>=count)selected=count-1;}
static int load(void){if(selected<0||selected>=count)return 0;char path[128];copy(path,"/home/user/.sino/apps/");copy(path+strlen(path),list[selected].name);return Read(path,&record,sizeof(record))==(int)sizeof(record)&&record.magic==APP_RECORD_MAGIC;}
static void draw(void){ui_begin(&scene);ui_text(&scene,22,52,"软件库 / 已安装应用");
 if(selected<first)first=selected;if(selected>=first+9)first=selected-8;if(first<0)first=0;
 for(int row=0;row<9&&row+first<count;row++){int i=row+first;char path[128];copy(path,"/home/user/.sino/apps/");copy(path+strlen(path),list[i].name);if(Read(path,&record,sizeof(record))!=(int)sizeof(record)||record.magic!=APP_RECORD_MAGIC)continue;
 if(i==selected)ui_fill(&scene,18,94+row*28,600,26,0xdce8f8);ui_text(&scene,25,96+row*28,record.manifest.name);ui_text(&scene,502,96+row*28,record.manifest.version);}
 if(!count)ui_text(&scene,25,110,"尚无已安装应用。F8 打开软件安装程序。");
 ui_button(&scene,22,405,100,"启动");ui_button(&scene,134,405,100,"修复");ui_button(&scene,246,405,100,"卸载");ui_button(&scene,358,405,100,"刷新");
 if(load()){ui_text(&scene,22,355,record.manifest.name);ui_text(&scene,320,355,record.manifest.version);}Scene(window,&scene);}

int main(void){window=Window("SINO 应用管理器",640,460);if(window<0)return 1;selected=0;refresh();draw();for(;;){UiEvent e;if(Event(&e)!=0){Yield(1);continue;}if(e.type==MSG_KEY&&e.key==27)break;if(e.type==MSG_KEY&&(e.key==0x148||e.key==0x150)){selected+=e.key==0x148?-1:1;if(selected<0)selected=0;if(selected>=count)selected=count-1;draw();}if(e.type==MSG_CLICK&&e.y>=94&&e.y<346){selected=first+(e.y-94)/28;draw();}if(ui_hit(&e,358,405,100,26)){refresh();draw();}if(load()){char path[128];if(ui_hit(&e,22,405,100,26)){copy(path,record.root);copy(path+strlen(path),"/");copy(path+strlen(path),record.manifest.entry);Spawn(path);}if(ui_hit(&e,134,405,100,26)){copy(path,"/repair:");copy(path+strlen(path),record.manifest.id);SpawnArgs("/boot/setup.snx",path);}if(ui_hit(&e,246,405,100,26)){copy(path,"/remove:");copy(path+strlen(path),record.manifest.id);SpawnArgs("/boot/setup.snx",path);}}}return 0;}
