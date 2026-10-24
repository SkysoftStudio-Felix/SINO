#ifndef SINO_PROCESS_H
#define SINO_PROCESS_H
#include "api.h"
#include "kernel.h"
extern volatile u32 user_switch_out;
extern u32 process_faults,process_switches,process_exits;
void process_init(void);
int process_launch(const char *,u32);
int process_launch_args(const char *,u32,const char *);
int process_list(ProcessInfo *,u32);
int process_kill(u32,i32);
void process_step(void);
int process_interrupt(Frame *);
void process_message(u32,UiEvent);
void process_tests(void);
#endif
