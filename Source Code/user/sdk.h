#ifndef SINO_SDK_H
#define SINO_SDK_H
#include "../src32/api.h"
void *memcpy(void *,const void *,u32);
void *memset(void *,int,u32);
u32 strlen(const char *);
int streq(const char *,const char *);
void number(u32,char *);
static inline int api(u32 n,u32 a,u32 b,u32 c){__asm__ volatile("int $0x80":"+a"(n):"b"(a),"c"(b),"d"(c):"memory","cc");return (int)n;}
static inline void Exit(int n){api(SYS_EXIT,(u32)n,0,0);for(;;){}}
static inline void Yield(u32 ticks){api(SYS_YIELD,ticks,0,0);}
static inline int Read(const char *p,void *b,u32 n){return api(SYS_READ,(u32)p,(u32)b,n);}
static inline int Write(const char *p,const void *b,u32 n){return api(SYS_WRITE,(u32)p,(u32)b,n);}
static inline int Mkdir(const char *p){return api(SYS_MKDIR,(u32)p,0,0);}
static inline int Remove(const char *p,int dir){return api(SYS_REMOVE,(u32)p,(u32)dir,0);}
static inline int List(const char *p,FileInfo *b,u32 n){return api(SYS_LIST,(u32)p,(u32)b,n);}
static inline int Spawn(const char *p){return api(SYS_SPAWN,(u32)p,0,0);}
static inline int SpawnArgs(const char *p,const char *args){return api(SYS_SPAWN,(u32)p,(u32)args,0);}
static inline const char *Executable(void){return (const char *)(USER_ARGUMENTS+256);}
static inline const char *Arguments(void){return (const char *)USER_ARGUMENTS;}
/* A failed snapshot leaves the previous committed bank intact. Retry transient
   device completion failures, never acknowledge until SYS_SYNC confirms. */
static inline int Sync(void){int r=S_IO;for(int i=0;i<3;i++){r=api(SYS_SYNC,0,0,0);if(r!=S_IO)return r;if(i<2)Yield(1);}return r;}
static inline int Window(const char *p,int w,int h){return api(SYS_WINDOW,(u32)p,(u32)w,(u32)h);}
static inline int Scene(int h,const UiScene *s){return api(SYS_SCENE,(u32)h,(u32)s,0);}
static inline int Event(UiEvent *e){return api(SYS_EVENT,(u32)e,0,0);}
static inline int Close(int h){return api(SYS_CLOSE,(u32)h,0,0);}
void copy(char *,const char *);
int text_encode(u32,char *);
u32 text_previous(const char *,u32);
void Log(const char *);
void ui_begin(UiScene *);
void ui_text(UiScene *,int,int,const char *);
void ui_mono(UiScene *,int,int,const char *,u32);
void ui_mono_clip(UiScene *,int,int,int,const char *,u32);
int ui_dialog(int,UiScene *,const char *,char *,int);
void ui_button(UiScene *,int,int,int,const char *);
void ui_fill(UiScene *,int,int,int,int,u32);
void ui_panel(UiScene *,int,int,int,int,u32,int);
int ui_hit(UiEvent *,int,int,int,int);
#endif
