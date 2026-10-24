#include "sdk.h"
/* GCC's MinGW C entry hook: no host CRT or C++ constructors in native ABI 1. */
void __main(void){}
void *memcpy(void *d,const void *s,u32 n){u8 *a=d;const u8 *b=s;while(n--)*a++=*b++;return d;}
void *memset(void *d,int c,u32 n){u8 *a=d;while(n--)*a++=(u8)c;return d;}
u32 strlen(const char *s){u32 n=0;while(s[n])n++;return n;}
int streq(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
void copy(char *a,const char *b){while((*a++=*b++)){} }
void number(u32 v,char *s){char b[12];int i=0;do{b[i++]=(char)('0'+v%10);v/=10;}while(v);while(i)*s++=b[--i];*s=0;}
void Log(const char *s){api(SYS_LOG,(u32)s,0,strlen(s));}
static UiCommand *command(UiScene *s,int kind,int x,int y,int w,int h,const char *text,u32 color){if(s->count>=UI_COMMANDS)return 0;UiCommand *c=&s->commands[s->count++];memset(c,0,sizeof(*c));c->kind=kind;c->x=x;c->y=y;c->w=w;c->h=h;c->color=color;u32 i=0;if(text){while(text[i]&&i<79){c->text[i]=text[i];i++;}}return c;}
void ui_begin(UiScene *s){memset(s,0,sizeof(*s));}
void ui_text(UiScene *s,int x,int y,const char *t){command(s,UI_TEXT,x,y,0,0,t,0x24252a);}
void ui_mono(UiScene *s,int x,int y,const char *t,u32 color){command(s,UI_TEXT,x,y,0,2,t,color);}
void ui_mono_clip(UiScene *s,int x,int y,int width,const char *t,u32 color){if(width>0)command(s,UI_TEXT,x,y,width,2,t,color);}
void ui_button(UiScene *s,int x,int y,int w,const char *t){command(s,UI_BUTTON,x,y,w,26,t,0);}
void ui_fill(UiScene *s,int x,int y,int w,int h,u32 color){command(s,UI_FILL,x,y,w,h,0,color);}
void ui_panel(UiScene *s,int x,int y,int w,int h,u32 color,int down){command(s,UI_PANEL,x,y,w,h,0,color|((u32)(down!=0)<<24));}
int ui_hit(UiEvent *e,int x,int y,int w,int h){return e->type==MSG_CLICK&&e->x>=x&&e->y>=y&&e->x<x+w&&e->y<y+h;}

int ui_dialog(int window,UiScene *scene,const char *prompt,char *out,int input){out[0]=0;for(;;){ui_begin(scene);ui_fill(scene,24,65,460,260,0xe5e9ef);ui_text(scene,40,86,prompt);if(input){ui_fill(scene,40,150,420,30,0xffffff);ui_text(scene,44,154,out);}ui_button(scene,330,265,120,"OK / 回车");ui_text(scene,40,220,"Esc 取消");Scene(window,scene);UiEvent e;while(Event(&e)!=0)Yield(1);if(ui_hit(&e,330,265,120,26)||(e.type==MSG_KEY&&e.key==13))return 1;if(e.type==MSG_KEY&&e.key==27)return 0;if(input&&e.type==MSG_TEXT){char encoded[4];int bytes=text_encode((u32)e.key,encoded);u32 n=strlen(out);if(n+(u32)bytes<100){memcpy(out+n,encoded,(u32)bytes);out[n+bytes]=0;}}if(input&&e.type==MSG_KEY){u32 n=strlen(out);if(e.key==8&&n){n--;while(n&&((u8)out[n]&0xc0)==0x80)n--;out[n]=0;}else if(e.key==1)out[0]=0;else if(e.key>=32&&e.key<127&&n<100){out[n]=(char)e.key;out[n+1]=0;}}}}

int text_encode(u32 cp,char *s){if(cp<128){s[0]=(char)cp;return 1;}if(cp<2048){s[0]=(char)(0xc0|(cp>>6));s[1]=(char)(0x80|(cp&63));return 2;}if(cp<65536&&!(cp>=0xd800&&cp<=0xdfff)){s[0]=(char)(0xe0|(cp>>12));s[1]=(char)(0x80|((cp>>6)&63));s[2]=(char)(0x80|(cp&63));return 3;}if(cp<=0x10ffff){s[0]=(char)(0xf0|(cp>>18));s[1]=(char)(0x80|((cp>>12)&63));s[2]=(char)(0x80|((cp>>6)&63));s[3]=(char)(0x80|(cp&63));return 4;}return 0;}
u32 text_previous(const char *s,u32 n){if(!n)return 0;n--;while(n&&((u8)s[n]&0xc0)==0x80)n--;return n;}
