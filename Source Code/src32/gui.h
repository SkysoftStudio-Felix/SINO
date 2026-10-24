#ifndef SINO_GUI_H
#define SINO_GUI_H
#include "kernel.h"
#include "api.h"
extern int gui_width,gui_height;
#define GUI_WIDTH gui_width
#define GUI_HEIGHT gui_height
#define GUI_BACK ((u32 *)0x800000)
extern int gui_active;
void gui_init(void);
void gui_step(void);
void gui_console_changed(void);
void gui_panic(const char *,u32,u32);
void gui_tests(void);
void gui_refresh(void);
void gui_files_changed(void);
void gui_open_installer(void);
int gui_clip_read(u8 *,u32);
int gui_clip_write(const u8 *,u32);
int gui_user_open(u32,const char *,int,int);
void gui_user_close(int);
int gui_user_policy(int,u32);
int gui_user_scene(int,const UiScene *);
#endif
