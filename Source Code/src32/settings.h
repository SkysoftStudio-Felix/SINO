#ifndef SINO_SETTINGS_H
#define SINO_SETTINGS_H
#include "kernel.h"
extern int settings_language,settings_pending;
const char *settings_text(const char *);
void settings_init(void);
void settings_boot(void);
void settings_open(void);
void settings_cancel(void);
void settings_draw(int,int);
int settings_click(int,int,int,int);
int settings_mouse(int,int,int);
int settings_poll(void);
void settings_language_set(int);
#endif
