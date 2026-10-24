#ifndef SINO_INSTALL_H
#define SINO_INSTALL_H
#include "kernel.h"
void install_open(void);
void install_draw(int,int);
int install_click(int,int,int,int);
int install_key(int);
int install_poll(void);
int install_offer(void);
int install_can_close(void);
extern int install_step,install_complete;
#endif
