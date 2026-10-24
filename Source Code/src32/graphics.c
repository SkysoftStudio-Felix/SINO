/* Opaque cached surfaces, grayscale text and a clipped alpha pointer. */
#include "graphics.h"
#include "settings.h"
#define FRONT ((u32 *)0x1600000)
#define ASSETS ((u8 *)0x700000)
typedef struct {u32 w,h;const u8 *advance,*pixels;} Font;
static int pointer_visible,front_valid;
static Font fonts[4];
int gui_width=1024,gui_height=768,display_width=1024,display_height=768,display_scale=100,display_contrast=100;
#define XMAP ((u16 *)0xe00000)
#define YMAP ((u16 *)0xe01000)
static u8 tone[256];
static u32 output_color(u32 c){return((u32)tone[(c>>16)&255]<<16)|((u32)tone[(c>>8)&255]<<8)|tone[c&255];}
static void dispi(int index,int value){outw(0x1ce,(u16)index);outw(0x1cf,(u16)value);}
int graphics_mode(int w,int h,int scale,int contrast){
 if(w<800||w>1920||h<540||h>1200||scale<100||scale>200||contrast<60||contrast>140)return 0;
 int logical_w=w*100/scale,logical_h=h*100/scale;if(logical_w<800||logical_h<540||logical_w>1280||logical_h>960)return 0;
 outw(0x1ce,0);u16 id=inw(0x1cf);if(id<0xb0c0||id>0xb0c5)return 0;
 dispi(4,0);dispi(1,w);dispi(2,h);dispi(3,32);dispi(6,w);dispi(7,h);dispi(8,0);dispi(9,0);dispi(4,0x41);
 display_width=w;display_height=h;display_scale=scale;display_contrast=contrast;gui_width=logical_w;gui_height=logical_h;graphics_pitch=(u32)w;
 for(int i=0;i<256;i++){int v=128+(i-128)*contrast/100;tone[i]=(u8)(v<0?0:v>255?255:v);}
 for(int i=0;i<w;i++)XMAP[i]=(u16)(i*logical_w/w);for(int i=0;i<h;i++)YMAP[i]=(u16)(i*logical_h/h);
 pointer_visible=front_valid=0;return 1;
}

static u32 *surface;
static int sw,sh;
static Rect clip;
static const u8 *pointer_pixels;
static const u8 *chinese_pixels,*pinyin_data;
static u32 chinese_count,pinyin_size;
static int pointer_x=512,pointer_y=384,drawn_x,drawn_y;
volatile u32 *graphics_video;
u32 graphics_pitch,graphics_frames,graphics_pixels;
int graphics_sse2,graphics_direct;
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
static u32 mix(u32 back,u32 front,u32 a){return((((back&0xff00ff)*(256-a)+(front&0xff00ff)*a)>>8)&0xff00ff)|((((back&0xff00)*(256-a)+(front&0xff00)*a)>>8)&0xff00);}
/* Only these leaf rendering routines use XMM. C compilation disables SSE and
   IRQ handlers use no XMM, so asynchronous PS/2/PIT interrupts preserve them. */
void g_copy(u32 *to,const u32 *from,u32 count){
 if(graphics_sse2&&count>=16){u32 blocks=count/16;__asm__ volatile(
  "1: movdqu (%1),%%xmm0; movdqu 16(%1),%%xmm1; movdqu 32(%1),%%xmm2; movdqu 48(%1),%%xmm3;"
  "movdqu %%xmm0,(%0); movdqu %%xmm1,16(%0); movdqu %%xmm2,32(%0); movdqu %%xmm3,48(%0);"
  "addl $64,%0; addl $64,%1; decl %2; jnz 1b":"+D"(to),"+S"(from),"+c"(blocks)::"memory","cc");count&=15;}
 __asm__ volatile("rep movsl":"+D"(to),"+S"(from),"+c"(count)::"memory");}
static void solid(u32 *p,u32 n,u32 color){if(graphics_sse2&&n>=16){u32 blocks=n/16;__asm__ volatile(
 "movd %2,%%xmm0; pshufd $0,%%xmm0,%%xmm0; 1: movdqu %%xmm0,(%0); movdqu %%xmm0,16(%0); movdqu %%xmm0,32(%0); movdqu %%xmm0,48(%0); addl $64,%0; decl %1; jnz 1b"
 :"+D"(p),"+c"(blocks):"a"(color):"memory","cc");n&=15;}__asm__ volatile("rep stosl":"+D"(p),"+c"(n):"a"(color):"memory");}
void g_target(u32 *pixels,int w,int h){surface=pixels;sw=w;sh=h;clip=(Rect){0,0,w,h};}
void g_clip(Rect r){int right=min(sw,r.x+r.w),bottom=min(sh,r.y+r.h);r.x=max(0,r.x);r.y=max(0,r.y);r.w=max(0,right-r.x);r.h=max(0,bottom-r.y);clip=r;}
void g_fill(int x,int y,int w,int h,u32 color){int right=min(x+w,clip.x+clip.w),bottom=min(y+h,clip.y+clip.h);x=max(x,clip.x);y=max(y,clip.y);if(right<=x)return;for(int j=y;j<bottom;j++)solid(surface+j*sw+x,(u32)(right-x),color);}
void g_line(int x,int y,int x2,int y2,u32 color){int dx=x2>x?x2-x:x-x2,sx=x<x2?1:-1,dy=y2>y?y-y2:y2-y,sy=y<y2?1:-1,e=dx+dy;for(;;){if(x>=clip.x&&x<clip.x+clip.w&&y>=clip.y&&y<clip.y+clip.h)surface[y*sw+x]=color;if(x==x2&&y==y2)break;int p=2*e;if(p>=dy){e+=dy;x+=sx;}if(p<=dx){e+=dx;y+=sy;}}}
void g_round(int x,int y,int w,int h,int r,u32 color){g_fill(x+r,y,w-2*r,h,color);g_fill(x,y+r,r,h-2*r,color);g_fill(x+w-r,y+r,r,h-2*r,color);for(int j=0;j<r;j++){int d=r-1;while(d>0&&d*d+(r-j-1)*(r-j-1)>r*r)d--;g_fill(x+r-d,y+j,2*d+w-2*r,1,color);g_fill(x+r-d,y+h-j-1,2*d+w-2*r,1,color);}}
void g_box(int x,int y,int w,int h,u32 border,u32 background){g_fill(x,y,w,h,border);g_fill(x+1,y+1,w-2,h-2,background);}
void g_bevel(int x,int y,int w,int h,int down){g_fill(x,y,w,h,down?0xc9cbd0:0xdfe0e4);g_line(x,y,x+w-1,y,down?0x62666d:0xffffff);g_line(x,y,x,y+h-1,down?0x62666d:0xffffff);g_line(x,y+h-1,x+w-1,y+h-1,down?0xffffff:0x72767e);g_line(x+w-1,y,x+w-1,y+h-1,down?0xffffff:0x72767e);}
u32 utf_next(const char *s,int *at){int i=*at;u8 c=(u8)s[i++];u32 cp=c;int extra=0;if(c>=0xc2&&c<=0xdf){cp=c&31;extra=1;}else if(c>=0xe0&&c<=0xef){cp=c&15;extra=2;}else if(c>=0xf0&&c<=0xf4){cp=c&7;extra=3;}else if(c>=128){*at=i;return '?';}for(int n=0;n<extra;n++){u8 b=(u8)s[i];if((b&0xc0)!=0x80){*at=i;return '?';}cp=(cp<<6)|(b&63);i++;}*at=i;if((extra==1&&cp<128)||(extra==2&&cp<2048)||(extra==3&&cp<65536)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return '?';return cp;}
int utf_prev(const char *s,int i){if(i>0){i--;while(i>0&&((u8)s[i]&0xc0)==0x80)i--;}return i;}
int utf_encode(u32 cp,char *s){if(cp<128){s[0]=(char)cp;return 1;}if(cp<2048){s[0]=(char)(0xc0|(cp>>6));s[1]=(char)(0x80|(cp&63));return 2;}s[0]=(char)(0xe0|(cp>>12));s[1]=(char)(0x80|((cp>>6)&63));s[2]=(char)(0x80|(cp&63));return 3;}
int g_candidates(const char *key,int start,u16 *out,int cap){u32 n=*(const u32 *)pinyin_data;for(u32 i=0;i<n;i++){const u8 *p=pinyin_data+4+i*16;if(!streq((const char *)p,key))continue;u32 offset=*(const u32 *)(p+8),count=*(const u16 *)(p+12);if(offset+count*2>pinyin_size)return 0;int written=0;while(start<(int)count&&written<cap)out[written++]=*(const u16 *)(pinyin_data+offset+(u32)start++*2);return written;}return 0;}
int g_width(int font,const char *s){s=settings_text(s);int width=0,i=0;while(s[i])width+=g_advance(font,utf_next(s,&i));return width;}
int g_advance(int font,u32 c){if(c>=128)return 17;if(c<32)c='?';return fonts[font].advance[c-32];}
void g_glyph(int font,int x,int y,u32 c,u32 color){if(c>=128){u32 lo=0,hi=chinese_count;while(lo<hi){u32 mid=(lo+hi)/2,key=*(const u16 *)(chinese_pixels+mid*34);if(key<c)lo=mid+1;else hi=mid;}if(lo<chinese_count&&*(const u16 *)(chinese_pixels+lo*34)==c){const u8 *p=chinese_pixels+lo*34+2;y+=font==1?5:1;for(int j=0;j<16;j++)for(int i=0;i<16;i++)if(p[j*2+i/8]&(128>>(i%8)))g_fill(x+i+1,y+j,1,1,color);return;}c='?';}if(c<32)c='?';if(c==' ')return;Font *f=&fonts[font];u32 start=(c-32)*f->w*f->h;for(u32 j=0;j<f->h;j++)if(y+(int)j>=clip.y&&y+(int)j<clip.y+clip.h)for(u32 i=0;i<f->w;i++)if(x+(int)i>=clip.x&&x+(int)i<clip.x+clip.w){u32 at=start+j*f->w+i;u8 b=f->pixels[at/2],a=at&1?b&15:b>>4;if(a){u32 p=(u32)((y+(int)j)*sw+x+(int)i);surface[p]=mix(surface[p],color,(u32)a*17+1);}}}
void g_text(int font,int x,int y,const char *s,u32 color){s=settings_text(s);int i=0;while(s[i]){u32 c=utf_next(s,&i);g_glyph(font,x,y,c,color);x+=g_advance(font,c);}}
void g_center(int x,int y,int w,const char *s,u32 color){g_text(0,x+(w-g_width(0,s))/2,y,s,color);}
void g_cliptext(int x,int y,int width,const char *s,u32 color){s=settings_text(s);int right=x+width,i=0;while(s[i]){u32 c=utf_next(s,&i);int advance=g_advance(0,c);if(x+advance>right-(s[i]?10:0)){g_text(0,x,y,"...",color);return;}g_glyph(0,x,y,c,color);x+=advance;}}
void g_blit(const u32 *source,int stride,int x,int y,int width,int height,int sx,int sy){int left=max(x,clip.x),top=max(y,clip.y),right=min(x+width,clip.x+clip.w),bottom=min(y+height,clip.y+clip.h);if(right<=left)return;for(int row=top;row<bottom;row++)g_copy(surface+row*sw+left,source+(sy+row-y)*stride+sx+left-x,(u32)(right-left));}
static void pointer_restore(void){if(!pointer_visible)return;int left=max(0,drawn_x*display_width/GUI_WIDTH),right=min(display_width,(drawn_x+28)*display_width/GUI_WIDTH+1),top=max(0,drawn_y*display_height/GUI_HEIGHT),bottom=min(display_height,(drawn_y+34)*display_height/GUI_HEIGHT+1);for(int y=top;y<bottom;y++)for(int x=left;x<right;x++)graphics_video[y*graphics_pitch+x]=output_color(FRONT[YMAP[y]*GUI_WIDTH+XMAP[x]]);}
static void pointer_draw(void){drawn_x=pointer_x-1;drawn_y=pointer_y-1;int left=max(0,drawn_x*display_width/GUI_WIDTH),right=min(display_width,(drawn_x+28)*display_width/GUI_WIDTH+1),top=max(0,drawn_y*display_height/GUI_HEIGHT),bottom=min(display_height,(drawn_y+34)*display_height/GUI_HEIGHT+1);for(int y=top;y<bottom;y++)for(int x=left;x<right;x++){int sx=XMAP[x]-drawn_x,sy=YMAP[y]-drawn_y;if(sx<0||sx>=28||sy<0||sy>=34)continue;const u8 *p=pointer_pixels+(sy*28+sx)*4;if(p[3])graphics_video[y*graphics_pitch+x]=output_color(mix(FRONT[YMAP[y]*GUI_WIDTH+XMAP[x]],((u32)p[0]<<16)|((u32)p[1]<<8)|p[2],(u32)p[3]+(p[3]==255)));}pointer_visible=1;}
void g_pointer(int x,int y){pointer_restore();pointer_x=x;pointer_y=y;if(front_valid)pointer_draw();}
void g_present(Rect r){
 pointer_restore();int left=max(0,r.x),right=min(GUI_WIDTH,r.x+r.w),top=max(0,r.y),bottom=min(GUI_HEIGHT,r.y+r.h);u32 pixels=0;if(right<=left||bottom<=top){if(front_valid)pointer_draw();return;}
 for(int y=top;y<bottom;y++){u32 *row=GUI_BACK+y*GUI_WIDTH,*old=FRONT+y*GUI_WIDTH;int first=left,last=right;if(front_valid&&!graphics_direct){while(first<right&&row[first]==old[first])first++;if(first==right)continue;while(last>first&&row[last-1]==old[last-1])last--;}g_copy(old+first,row+first,(u32)(last-first));if(display_width==GUI_WIDTH&&display_height==GUI_HEIGHT&&display_contrast==100)g_copy((u32 *)(graphics_video+y*graphics_pitch+first),row+first,(u32)(last-first));else{int x1=first*display_width/GUI_WIDTH,x2=(last*display_width+GUI_WIDTH-1)/GUI_WIDTH,y1=y*display_height/GUI_HEIGHT,y2=((y+1)*display_height+GUI_HEIGHT-1)/GUI_HEIGHT;for(int py=y1;py<y2&&py<display_height;py++)if(YMAP[py]==y)for(int px=x1;px<x2&&px<display_width;px++)if(XMAP[px]>=first&&XMAP[px]<last)graphics_video[py*graphics_pitch+px]=output_color(row[XMAP[px]]);}pixels+=(u32)(last-first);}
 graphics_pixels=pixels;graphics_frames++;front_valid=1;pointer_visible=0;pointer_draw();
}
void graphics_init(void){
    u32 before,after;__asm__ volatile("pushfl; popl %0":"=r"(before));after=before^0x200000;__asm__ volatile("pushl %0; popfl"::"r"(after):"cc");__asm__ volatile("pushfl; popl %0":"=r"(after));__asm__ volatile("pushl %0; popfl"::"r"(before):"cc");
    if((before^after)&0x200000){u32 a=1,b,c,d;__asm__ volatile("cpuid":"+a"(a),"=b"(b),"=c"(c),"=d"(d));if(d&(1u<<26)){u32 cr0,cr4;__asm__ volatile("mov %%cr0,%0":"=r"(cr0));__asm__ volatile("mov %%cr4,%0":"=r"(cr4));cr0=(cr0|2)&~12u;cr4|=0x600;__asm__ volatile("mov %0,%%cr0; mov %1,%%cr4"::"r"(cr0),"r"(cr4):"memory");graphics_sse2=1;debug("GRAPHICS SSE2 surface copies enabled\n");}}
    const u8 *vbe=(const u8 *)0x5800;graphics_video=(volatile u32 *)*(const u32 *)(vbe+40);graphics_pitch=*(const u16 *)(vbe+16)/4;if(graphics_pitch<(u32)GUI_WIDTH)panic("VBE stride",graphics_pitch,0);
    int index=-1;for(u32 i=0;i<file_count;i++)if(streq(files[i].name,"UIASSET.BIN"))index=(int)i;if(index<0||files[index].size>0x100000||!fs_read((u32)index,ASSETS,0x100000))panic("UI resources",0,0);u32 size=files[index].size;
    if(size<64||ASSETS[0]!='S'||ASSETS[1]!='N'||ASSETS[2]!='U'||ASSETS[3]!='I'||*(u32 *)(ASSETS+4)!=2)panic("UI resource header",0,0);
    for(int i=0;i<4;i++){u32 offset=*(u32 *)(ASSETS+8+i*4);if(offset>size-112)panic("Font offset",offset,0);u8 *p=ASSETS+offset;u32 w=*(u16 *)(p+4),h=*(u16 *)(p+6);if(!w||!h||w>32||h>40||offset+112+96*w*h/2>size)panic("Font range",w,h);fonts[i]=(Font){w,h,p+16,p+112};}
    u32 cursor=*(u32 *)(ASSETS+24);if(cursor>size-3808)panic("Pointer resource",cursor,0);pointer_pixels=ASSETS+cursor;
    u32 ch=*(u32 *)(ASSETS+28),py=*(u32 *)(ASSETS+44);if(ch>size-4||py>size-4)panic("Chinese resources",ch,py);chinese_count=*(u32 *)(ASSETS+ch);if(chinese_count>(size-ch-4)/34)panic("Chinese glyph range",ch,0);chinese_pixels=ASSETS+ch+4;pinyin_data=ASSETS+py;pinyin_size=size-py;if(*(const u32 *)pinyin_data>(pinyin_size-4)/16)panic("Pinyin index range",py,0);
    for(int i=0;i<256;i++)tone[i]=(u8)i;for(int i=0;i<1024;i++)XMAP[i]=(u16)i;for(int i=0;i<768;i++)YMAP[i]=(u16)i;g_target(GUI_BACK,GUI_WIDTH,GUI_HEIGHT);debug("GRAPHICS Platinum bitmap fonts ready\n");
}
