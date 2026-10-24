#include "sdk.h"
static volatile int worker_done;
static int event_handle;
static UiScene scene;
static UiCanvas large_scene;
static void worker(void){worker_done=123;api(SYS_EVENT_SET,(u32)event_handle,1,0);api(SYS_THREAD_EXIT,0,0,0);for(;;){}}
static void check(int ok,const char *s){if(!ok){Log("USER PROBE FAIL: ");Log(s);Log("\n");Exit(1);}}
int main(void){
#if MODE == 1
 *(volatile u32 *)0x100000=1;
#elif MODE == 2
 __asm__ volatile("outb %%al,%%dx"::"a"(255),"d"(0x21));
#elif MODE == 3
 for(;;)worker_done++;
#elif MODE == 4
 __asm__ volatile("ud2");
#else
 check(api(SYS_SOCKET,1,0,0)==S_DENIED,"network default deny");
 check(Read("/boot/readme.txt",(void *)0x100000,20)==S_INVALID,"kernel pointer");
 check(Write("/etc/hacked","bad",3)==S_DENIED,"protected directory");
 check(Write("/home/user/../../etc/hacked","bad",3)==S_DENIED,"canonical traversal");
 check(api(SYS_WINDOW,0xffffffff,300,200)==S_INVALID,"string overflow");
 event_handle=api(SYS_EVENT_CREATE,0,0,0);check(event_handle>0,"event create");
 check(api(SYS_THREAD,(u32)worker,0,0)>0,"thread creation");
 while(!worker_done)Yield(1);
 check(worker_done==123&&api(SYS_EVENT_WAIT,(u32)event_handle,0,0)==0,"thread shared address space/event");
 check(Close(event_handle)==0&&Close(event_handle)==S_BADHANDLE,"closed handle");
 int next=api(SYS_EVENT_CREATE,0,0,0);check(next>0&&next!=event_handle&&api(SYS_EVENT_SET,(u32)event_handle,1,0)==S_BADHANDLE,"stale generation");Close(next);
 check(Write("/home/user/probe.txt","ring3 file\n",11)==0,"write user file");
 Log("RING3 PROBE PASS: threads events pointers permissions handles VFS\n");
 int w=Window("原生用户态测试 / Ring 3",520,290);check(w>0,"window");
 large_scene.count=UI_CANVAS_COMMANDS;for(u32 i=0;i<large_scene.count;i++){UiCommand *c=&large_scene.commands[i];c->kind=UI_FILL;c->x=24;c->y=60;c->w=c->h=2;}
 check(Scene(w,(const UiScene *)&large_scene)==0,"large canvas");large_scene.count=UI_CANVAS_COMMANDS+1;check(Scene(w,(const UiScene *)&large_scene)==S_INVALID,"canvas count bound");large_scene.count=1;large_scene.commands[0].kind=UI_PANEL;large_scene.commands[0].w=0;check(Scene(w,(const UiScene *)&large_scene)==S_INVALID,"panel bounds");check(Scene(w,(const UiScene *)0xffffffffu)==S_INVALID,"canvas pointer bound");
 ui_begin(&scene);ui_text(&scene,24,60,"真正运行在 Ring 3 的独立原生应用");ui_text(&scene,24,98,"页表隔离、线程、句柄、文件 API 已通过测试。");ui_button(&scene,24,160,170,"关闭测试程序");check(Scene(w,&scene)==0,"original ABI scene");
 for(;;){UiEvent e;if(Event(&e)==0&&(ui_hit(&e,24,160,170,26)||(e.type==MSG_KEY&&e.key==27)))break;Yield(1);}
#endif
 return 0;
}
