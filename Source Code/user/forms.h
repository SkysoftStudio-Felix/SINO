#ifndef SINO_FORMS_H
#define SINO_FORMS_H
#include "basic.h"
enum {CONTROL_LABEL=1,CONTROL_BUTTON,CONTROL_TEXT,CONTROL_CHECK,CONTROL_LIST,CONTROL_TIMER,CONTROL_MENU};
#define FORM_CONTROLS 12
typedef struct {int type,x,y,w,h,value,interval;u32 last_tick;char name[32],text[64];} Control;
typedef struct {char title[64],name[32];int width,height,count;Control controls[FORM_CONTROLS];} FormProject;
void FormsBlank(FormProject *);
void FormsDefault(FormProject *);
int FormsLoad(FormProject *,const char *);
int FormsSave(const FormProject *,char *,u32);
void FormsRender(const FormProject *,UiScene *,int,int,int);
int FormsGet(const FormProject *,const char *,BasicValue *);
int FormsSet(FormProject *,const char *,const BasicValue *);
int FormsInput(FormProject *,UiEvent *,int,int,int *,char *);
int FormsTimer(FormProject *,char *);
int FormsAdd(FormProject *,int,int,int);
#endif
