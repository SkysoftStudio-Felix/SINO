#ifndef SINO_GRAPHICS_H
#define SINO_GRAPHICS_H
#include "gui.h"
typedef struct {int x,y,w,h;} Rect;
void graphics_init(void);
void g_target(u32 *,int,int);
void g_clip(Rect);
void g_fill(int,int,int,int,u32);
void g_line(int,int,int,int,u32);
void g_round(int,int,int,int,int,u32);
void g_box(int,int,int,int,u32,u32);
void g_bevel(int,int,int,int,int);
void g_text(int,int,int,const char *,u32);
void g_glyph(int,int,int,u32,u32);
int g_width(int,const char *);
int g_advance(int,u32);
u32 utf_next(const char *,int *);
int utf_prev(const char *,int);
int utf_encode(u32,char *);
int g_candidates(const char *,int,u16 *,int);
void g_center(int,int,int,const char *,u32);
void g_cliptext(int,int,int,const char *,u32);
void g_blit(const u32 *,int,int,int,int,int,int,int);
void g_copy(u32 *,const u32 *,u32);
void g_pointer(int,int);
void g_present(Rect);
extern u32 graphics_frames,graphics_pixels;
extern volatile u32 *graphics_video;
extern u32 graphics_pitch;
extern int graphics_direct;
#define DESKTOP_CACHE ((u32 *)0x1000000)
#define WINDOW_CACHE(i) ((u32 *)(0x1c00000+(u32)(i)*0x300000))
extern int display_width,display_height,display_scale,display_contrast;
int graphics_mode(int,int,int,int);
#endif
