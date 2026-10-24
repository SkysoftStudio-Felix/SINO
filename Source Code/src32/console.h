#ifndef SINO_CONSOLE_H
#define SINO_CONSOLE_H
#include "kernel.h"
#define CON_COLS 80
#define CON_ROWS 26
void con_init(void);
void con_clear(void);
void con_color(u8);
void con_put(char);
void con_write(const char *);
void con_flush(void);
void con_snapshot(u32 *,u32 *);
u32 con_position(void);
void con_edit(u32,const char *,u32,u32,u32);
void con_scroll(int);
u32 con_scrolled(void);
#endif
