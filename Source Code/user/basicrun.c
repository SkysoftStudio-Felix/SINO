#include "forms.h"
#include "basic_image.h"
/* Explicit initialized .data keeps the editable resource in the disk image. */
static BasicImage embedded={BASIC_IMAGE_MAGIC,0,{0},{0}};
static Basic basic;
static FormProject form;
static UiScene scene;
static int window,focus=-1;
static char output[128],code[BASIC_CODE],serialized[3072];
static int get(const char *p,BasicValue *v){return FormsGet(&form,p,v);}
static int set(const char *p,const BasicValue *v){return FormsSet(&form,p,v);}
static void print(const char *s){copy(output,s);Log("SYSBASIC APP: ");Log(s);Log("\n");}
static int dialog(const char *prompt,char *out,int input){int r=ui_dialog(window,&scene,prompt,out,input);if(!r)basic.running=0;return r;}
static BasicHost host={get,set,print,dialog};
static void draw(void){ui_begin(&scene);FormsRender(&form,&scene,24,54,0);ui_text(&scene,24,form.height+76,output);if(basic.error[0]){char text[128];copy(text,"Line ");number((u32)basic.error_line,text+strlen(text));copy(text+strlen(text),": ");copy(text+strlen(text),basic.error);ui_text(&scene,24,form.height+102,text);}Scene(window,&scene);}
int main(void){__asm__ volatile("" : "+m"(embedded));if(embedded.enabled==1){if(embedded.form[sizeof(embedded.form)-1]||embedded.code[sizeof(embedded.code)-1])return 1;copy(serialized,embedded.form);copy(code,embedded.code);}else{char path[128];copy(path,Executable());u32 n=strlen(path);while(n&&path[n-1]!='/')n--;copy(path+n,"project.frm");int bytes=Read(path,serialized,sizeof(serialized)-1);if(bytes<0)return 1;serialized[bytes]=0;copy(path+n,"project.bas");bytes=Read(path,code,sizeof(code)-1);if(bytes<0)return 1;code[bytes]=0;}if(FormsLoad(&form,serialized)<0||BasicLoad(&basic,code)<0)return 1;window=Window(form.title,form.width+48,form.height+145);if(window<0)return 1;char load_event[64];copy(load_event,form.name);copy(load_event+strlen(load_event),"_Load");if(BasicStart(&basic,load_event)<0)BasicStart(&basic,"Form_Load");draw();for(;;){char event[64];UiEvent e;int changed=0;if(Event(&e)==0){if(FormsInput(&form,&e,24,54,&focus,event)){if(event[0]&&!basic.running)BasicStart(&basic,event);changed=1;}}if(!basic.running&&FormsTimer(&form,event)){BasicStart(&basic,event);changed=1;}for(int i=0;i<8&&basic.running;i++){BasicStep(&basic,&host);changed=1;}if(changed)draw();Yield(1);}}
