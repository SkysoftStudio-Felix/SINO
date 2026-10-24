/* Original SysBASIC: classic docked IDE. No Microsoft code or resources. */
#include "forms.h"
#include "package.h"
#include "basic_image.h"
#include "socket.h"
#define GREY 0xc0c0c0u
#define INK 0x202020u
#define BLUE 0x000080u
#define FX 145
#define FY 148
#define CODE_X 164
#define CODE_Y 162
#define CODE_ROWS 22
#define CODE_COLS 77
enum {NEW,OPEN,SAVE,DESIGN,CODE,RUN,STOP,STEP,BUILD,EXAMPLE};
static FormProject project,preview;
static Basic basic;
static UiCanvas canvas;
#define scene (*(UiScene *)&canvas)
static char source[BASIC_CODE],undo[BASIC_CODE],clipboard[BASIC_CODE];
static char form_text[3072],path[128]="/home/user/Projects/Hello.prj";
static char property[128],status[128],output[4][96],app_id[32];
static u8 package_bytes[MAX_FILE],breakpoints[BASIC_LINES];
static int window,view,selected=-1,tool,field,caret,scroll,hscroll,select_all;
static int dragging,resize_control,drag_x,drag_y,last_control=-1,paused,preview_focus=-1;
static Control drag_before;
static int drag_modified,draw_x,draw_y,last_tool=-1;
static int modified,undo_valid,menu=-1,event_menu,build_type,build_dialog,known_project,open_output=1,skip_breakpoint=-1;
static int object_popup,property_sort=1,property_selected,tree_closed,forms_closed,placing;
static u32 last_click,tool_click;
static const char *default_code="Sub Form_Load()\n  Dim clicks As Long\n  Label1.Caption = \"Hello SINO\"\nEnd Sub\n\nSub Button1_Click()\n  clicks = clicks + 1\n  Label1.Caption = \"Clicked \" & CStr(clicks)\n  Print Label1.Caption\nEnd Sub\n";
static void draw(void);
static void append(char *s,const char *t){copy(s+strlen(s),t);}
static void message(const char *s){copy(status,s);}
static int hit(const UiEvent *e,int x,int y,int w,int h){return e->x>=x&&e->y>=y&&e->x<x+w&&e->y<y+h;}
static void emit(int kind,int x,int y,int w,int h,const char *value,u32 color){
 if(canvas.count==UI_CANVAS_COMMANDS)return;UiCommand *c=&canvas.commands[canvas.count++];memset(c,0,sizeof(*c));c->kind=kind;c->x=x;c->y=y;c->w=w;c->h=h;c->color=color;
 if(value){u32 n=strlen(value);if(n>79){n=79;while(n&&((u8)value[n]&0xc0)==0x80)n--;}memcpy(c->text,value,n);}
}
static void fill(int x,int y,int w,int h,u32 color){if(w<=0||h<=0)return;emit(UI_FILL,x,y,w,h,0,color);}
static void text(int x,int y,const char *s,u32 color){emit(UI_TEXT,x,y,0,2,s,color);}
static void panel(int x,int y,int w,int h,u32 color,int down){emit(UI_PANEL,x,y,w,h,0,color|((u32)down<<24));}
static void button(int x,int y,int w,const char *s,int active){panel(x,y,w,25,active?0xe2e2e2:GREY,active);text(x+7,y+5,s,INK);}
static void title(int x,int y,int w,const char *s){fill(x,y,w,21,0x808080);text(x+5,y+4,s,0xffffff);}
static int lines(void){int n=1;for(u32 i=0;source[i];i++)if(source[i]=='\n')n++;return n;}
static int cursor_line(void){int n=0;for(int i=0;i<caret;i++)if(source[i]=='\n')n++;return n;}
static int line_offset(int line){int i=0;while(line>0&&source[i])if(source[i++]=='\n')line--;return i;}
static int next_char(int at){at++;while(source[at]&&((u8)source[at]&0xc0)==0x80)at++;return at;}
static int column_at(int start,int end){int c=0;while(start<end&&source[start]&&source[start]!='\n'){c+=(u8)source[start]<128?1:2;start=next_char(start);}return c;}
static void reveal(void){int line=cursor_line(),col=column_at(line_offset(line),caret);if(line<scroll)scroll=line;if(line>=scroll+CODE_ROWS)scroll=line-CODE_ROWS+1;if(col<hscroll)hscroll=col;if(col>=hscroll+CODE_COLS)hscroll=col-CODE_COLS+1;}
static void changed(void){modified=1;}
static void save_undo(void){copy(undo,source);undo_valid=1;}
static void insert(const char *value){u32 n=strlen(value),size=select_all?0:strlen(source);if(n+size>=BASIC_CODE){message("代码超过 8191 字节，未插入。");return;}if(select_all){source[0]=0;caret=scroll=hscroll=select_all=0;}for(int i=(int)size;i>=caret;i--)source[i+n]=source[i];memcpy(source+caret,value,n);caret+=(int)n;changed();reveal();}
static void edit_key(int key){
 int n=(int)strlen(source);if(key==1){select_all=1;return;}
 if(key==3||key==24){if(select_all){api(SYS_CLIP_SET,(u32)source,(u32)n,0);if(key==24){save_undo();source[0]=0;caret=0;select_all=0;changed();}}return;}
 if(key==22){int bytes=api(SYS_CLIP_GET,(u32)clipboard,BASIC_CODE-1,0);if(bytes>=0){clipboard[bytes]=0;save_undo();insert(clipboard);}else message("剪贴板内容太大。");return;}
 if(key==26&&undo_valid){copy(clipboard,source);copy(source,undo);copy(undo,clipboard);if(caret>(int)strlen(source))caret=(int)strlen(source);select_all=0;changed();reveal();return;}
 if(key==8||key==0x153){save_undo();if(select_all){source[0]=0;caret=0;changed();}else if(key==8&&caret){int at=(int)text_previous(source,(u32)caret);for(int i=caret;i<=n;i++)source[at+i-caret]=source[i];caret=at;changed();}else if(key==0x153&&caret<n){int end=next_char(caret);for(int i=end;i<=n;i++)source[caret+i-end]=source[i];changed();}}
 else if(key==0x14b&&caret)caret=(int)text_previous(source,(u32)caret);else if(key==0x14d&&caret<n)caret=next_char(caret);
 else if(key==0x148||key==0x150){int line=cursor_line(),col=column_at(line_offset(line),caret),target=line+(key==0x148?-1:1);if(target>=0&&target<lines()){caret=line_offset(target);while(source[caret]&&source[caret]!='\n'&&col>0){col-=(u8)source[caret]<128?1:2;caret=next_char(caret);}}}
 else if(key==0x147)caret=line_offset(cursor_line());else if(key==0x14f){while(source[caret]&&source[caret]!='\n')caret++;}
 else if(key==0x149||key==0x151){int target=cursor_line()+(key==0x149?-CODE_ROWS:CODE_ROWS);if(target<0)target=0;if(target>=lines())target=lines()-1;caret=line_offset(target);}
 else if(key==13){char indent[36]="\n";int at=line_offset(cursor_line()),j=1;while(source[at]==' '&&j<32){indent[j++]=' ';at++;}indent[j]=0;save_undo();insert(indent);}
 else if(key==9){save_undo();insert("  ");}else if(key>=32&&key<127){char s[2]={(char)key,0};save_undo();insert(s);}select_all=0;reveal();
}
static int project_file(char *out,const char *extension){u32 n=strlen(path);if(n<5||n>110||!BasicEqual(path+n-4,".prj"))return 0;const char *base="/home/user/";for(u32 i=0;base[i];i++)if(path[i]!=base[i])return 0;for(u32 i=11;i<n;i++)if(path[i]=='\\'||path[i]==':'||(path[i]=='.'&&path[i+1]=='.'))return 0;copy(out,path);copy(out+n-4,extension);return 1;}
static int save(void){
 char file[128],prj[256],filename[128];if(!known_project&&Read(path,package_bytes,sizeof(package_bytes))>=0){char answer[128];if(!ui_dialog(window,&scene,"该工程路径已有文件，是否覆盖？Esc 取消。",answer,0)){message("已取消覆盖，原文件保留。");return 0;}}if(!project_file(file,".frm")){message("请选择 /home/user 下的 .prj 工程路径。");return 0;}
 char folder[128];copy(folder,path);u32 at=strlen(folder);while(at&&folder[at-1]!='/')at--;folder[at-1]=0;if(Mkdir(folder)<0)goto error;
 int n=FormsSave(&project,form_text,sizeof(form_text));if(n<0){message("窗体序列化失败，请检查 Caption 属性。");return 0;}if(Write(file,form_text,(u32)n)<0)goto error;
 project_file(file,".bas");if(Write(file,source,strlen(source))<0)goto error;
 project_file(filename,".frm");at=strlen(filename);while(at&&filename[at-1]!='/')at--;copy(prj,"SINO-BASIC-1\nForm=");append(prj,filename+at);append(prj,"\nModule=");project_file(filename,".bas");append(prj,filename+at);append(prj,"\nRuntime=1\n");
 if(Write(path,prj,strlen(prj))<0||Sync()<0)goto error;modified=0;known_project=1;message("工程已保存。生成时可选择可执行程序或安装程序。");Log("SYSBASIC PROJECT SAVED\n");return 1;
error:message("保存失败，未标记为已保存，请检查路径和磁盘空间。");return 0;
}
static int confirm(void){if(!modified)return 1;for(;;){ui_begin(&scene);ui_panel(&scene,230,205,520,190,GREY,0);ui_text(&scene,250,230,"工程尚未保存，是否保存更改？");ui_button(&scene,256,335,120,"保存");ui_button(&scene,394,335,120,"不保存");ui_button(&scene,532,335,120,"取消");Scene(window,&scene);UiEvent e;while(Event(&e)!=0)Yield(1);if(e.type==MSG_KEY&&e.key==27)return 0;if(ui_hit(&e,532,335,120,26))return 0;if(ui_hit(&e,394,335,120,26))return 1;if(ui_hit(&e,256,335,120,26)||(e.type==MSG_KEY&&e.key==13))return save();}}
static int load(void){
 char file[128],header[256];if(!project_file(file,".frm")){message("工程路径需要以 .prj 结尾。");return 0;}int n=Read(path,header,sizeof(header)-1);if(n<0){message("没有找到工程文件。");return 0;}header[n]=0;if(n<13||header[0]!='S'||header[1]!='I'||header[2]!='N'||header[3]!='O'){message("不是 SINO BASIC 工程。");return 0;}
 n=Read(file,form_text,sizeof(form_text)-1);if(n<0){message("缺少同名 .frm 窗体。");return 0;}form_text[n]=0;FormProject next;if(FormsLoad(&next,form_text)<0){message("窗体格式或属性错误。");return 0;}
 project_file(file,".bas");n=Read(file,package_bytes,BASIC_CODE-1);if(n<0){message("缺少同名 .bas 模块。");return 0;}package_bytes[n]=0;project=next;copy(source,(const char *)package_bytes);memset(&basic,0,sizeof(basic));memset(breakpoints,0,sizeof(breakpoints));selected=-1;caret=scroll=hscroll=modified=undo_valid=0;view=0;known_project=1;message("工程已打开。");Log("SYSBASIC PROJECT LOADED\n");return 1;
}
static int get(const char *p,BasicValue *v){return FormsGet(&preview,p,v);}
static int set(const char *p,const BasicValue *v){return FormsSet(&preview,p,v);}
static void print(const char *s){for(int i=0;i<3;i++)copy(output[i],output[i+1]);u32 n=strlen(s);if(n>75){n=75;while(n&&((u8)s[n]&0xc0)==0x80)n--;}memcpy(output[3],s,n);output[3][n]=0;Log("SYSBASIC: ");Log(s);Log("\n");}
static int dialog(const char *s,char *out,int input){int r=ui_dialog(window,&scene,s,out,input);if(!r)basic.running=0;return r;}
static BasicHost host={get,set,print,dialog};
static void run(void){if(paused){skip_breakpoint=basic.pc;paused=0;view=2;message("继续运行。");return;}if(BasicLoad(&basic,source)<0){message(basic.error);view=1;caret=line_offset(basic.error_line-1);reveal();return;}preview=project;preview_focus=-1;paused=0;view=2;memset(output,0,sizeof(output));char event[64];copy(event,preview.name);append(event,"_Load");if(BasicStart(&basic,event)<0)BasicStart(&basic,"Form_Load");message("运行中。点击控件触发事件；Esc 或停止返回设计器。");Log("SYSBASIC PREVIEW START\n");}
static void seed_event(void){
 char event[64],needle[80];if(selected<0){copy(event,project.name);append(event,"_Load");}else{copy(event,project.controls[selected].name);append(event,event_menu==1?"_Change":event_menu==2?"_Timer":"_Click");}copy(needle,"Sub ");append(needle,event);int found=-1;
 for(int i=0;source[i];i++){if(i&&source[i-1]!='\n')continue;int j=i;while(source[j]==' ')j++;u32 n=0;while(needle[n]&&source[j+n]&&((source[j+n]|32)==(needle[n]|32)))n++;if(!needle[n]&&(source[j+n]=='('||source[j+n]==' ')){found=i;break;}}
 if(found>=0)caret=found;else{save_undo();caret=(int)strlen(source);char code[160];copy(code,"\nSub ");append(code,event);append(code,"()\n  \nEnd Sub\n");insert(code);caret-=(int)strlen("\nEnd Sub\n");}view=1;field=select_all=0;reveal();message("已定位事件过程，在代码区输入程序。");Log("SYSBASIC EVENT GENERATED\n");
}
static int property_value(int index){Control *c=&project.controls[selected];return index==3?c->x:index==4?c->y:index==5?c->w:index==6?c->h:index==7?c->value:c->interval;}
static void commit_property(void){
 if(field<1||field>8)return;
 if(selected<0){if(field==2){int valid=property[0]!=0&&strlen(property)<31;for(u32 i=0;property[i];i++)if(!((property[i]>='a'&&property[i]<='z')||(property[i]>='A'&&property[i]<='Z')||property[i]=='_'||(i&&property[i]>='0'&&property[i]<='9')))valid=0;for(int i=0;i<project.count;i++)if(BasicEqual(property,project.controls[i].name))valid=0;if(valid){copy(project.name,property);changed();}else message("窗体名称必须为唯一的有效标识符。");}else if(field==1){int valid=strlen(property)<64;for(u32 i=0;property[i];i++)if(property[i]=='|'||property[i]=='\n'||property[i]=='\r')valid=0;if(valid){copy(project.title,property);changed();}else message("Caption 最多 63 字节，不能有竖线或换行。");}
 else if(field==5||field==6){int n=0;for(u32 i=0;property[i];i++){if(property[i]<'0'||property[i]>'9'||n>1000){n=-1;break;}n=n*10+property[i]-'0';}int valid=n>=(field==5?200:100)&&n<=(field==5?560:400);for(int i=0;i<project.count;i++)if((field==5?project.controls[i].x+project.controls[i].w:project.controls[i].y+project.controls[i].h)>n)valid=0;if(valid){if(field==5)project.width=n;else project.height=n;changed();}else message("窗体尺寸超出设计区或会截断控件。");}field=0;return;}
 Control *c=&project.controls[selected];if(field==2){int valid=property[0]!=0&&strlen(property)<31;for(u32 i=0;property[i];i++)if(!((property[i]>='a'&&property[i]<='z')||(property[i]>='A'&&property[i]<='Z')||property[i]=='_'||(i&&property[i]>='0'&&property[i]<='9')))valid=0;if(BasicEqual(project.name,property))valid=0;for(int i=0;i<project.count;i++)if(i!=selected&&BasicEqual(project.controls[i].name,property))valid=0;if(valid){copy(c->name,property);changed();}else message("Name 必须唯一，只用字母、数字和下划线。");}
 else{const char *names[]={"","Caption","","Left","Top","Width","Height","Value","Interval"};char key[64];copy(key,c->name);append(key,".");append(key,names[field]);BasicValue v;memset(&v,0,sizeof(v));if(field==1)BasicString(&v,property);else{if(!property[0]){message("数字属性不能为空。");field=0;return;}for(u32 i=0;property[i];i++){if(property[i]<'0'||property[i]>'9'||v.number>10000){message("请输入有效的非负整数。");field=0;return;}v.number=v.number*10+property[i]-'0';}}if(FormsSet(&project,key,&v)<0)message("属性无效或超出边界，保留原值。");else changed();}field=0;
}
static void begin_property(int index){commit_property();field=index;property_selected=1;if(selected<0){if(index==1)copy(property,project.title);else if(index==2)copy(property,project.name);else if(index==5||index==6)number((u32)(index==5?project.width:project.height),property);else{field=0;message("窗体可修改 Caption、Width、Height。");}}else if(index==1)copy(property,project.controls[selected].text);else if(index==2)copy(property,project.controls[selected].name);else number((u32)property_value(index),property);}
static u32 crc(const void *data,u32 n){const u8 *p=data;u32 c=0xffffffffu;while(n--){c^=*p++;for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320u:0);}return c^0xffffffffu;}
static void build(void){
 if(!save())return;if(BasicLoad(&basic,source)<0){view=1;caret=line_offset(basic.error_line-1);reveal();message(basic.error);return;}
 int frm_bytes=FormsSave(&project,form_text,sizeof(form_text));if(frm_bytes<0)return;memset(package_bytes,0,sizeof(package_bytes));
 u32 offset=build_type?(u32)sizeof(Package):0;int n=Read("/boot/basicrun.snx",package_bytes+offset,sizeof(package_bytes)-offset);if(n<0){message("缺少 BASIC 运行时，无法生成。");return;}
 BasicImage *resource=0;for(u32 i=32;i+sizeof(BasicImage)<=(u32)n;i++){u32 j=0;while(j<8&&package_bytes[offset+i+j]==(u8)BASIC_IMAGE_MAGIC[j])j++;if(j==8){if(resource){message("运行时资源标记重复，拒绝生成。");return;}resource=(BasicImage *)(package_bytes+offset+i);}}
 if(!resource){message("BASIC 运行时版本不兼容。");return;}memset(resource->form,0,sizeof(resource->form));memset(resource->code,0,sizeof(resource->code));copy(resource->form,form_text);copy(resource->code,source);resource->enabled=1;
 char result_path[128];if(!build_type){project_file(result_path,".snx");if(Write(result_path,package_bytes,(u32)n)<0||Sync()<0){message("写入可执行程序失败。");return;}message("可执行程序已生成，内含窗体与代码，无需安装。");Log("SYSBASIC EXECUTABLE BUILT\n");if(open_output&&Spawn(result_path)<0)message("已生成，但当前不能启动，请关闭其他窗口再试。");return;}
 u32 start=strlen(path)-4;while(start&&path[start-1]!='/')start--;u32 length=strlen(path)-4-start;if(!length||length>18){message("生成安装程序需要 1–18 位英文工程文件名。");return;}copy(app_id,"basic-");
 for(u32 i=0;i<length;i++){char c=path[start+i];if(c>='A'&&c<='Z')c+=32;if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_')){message("工程文件名只能包含英文、数字、短横线和下划线。");return;}app_id[6+i]=c;}app_id[6+length]=0;
 Package *p=(Package *)package_bytes;p->magic=PACKAGE_MAGIC;p->format=1;p->arch=386;p->abi=1;p->files=1;p->components=1;p->total_bytes=offset+(u32)n;copy(p->id,app_id);copy(p->name,project.title);copy(p->version,"0.2.0");copy(p->entry,"program.snx");copy(p->shortcut,app_id);copy(p->license,"Original user project. License terms are defined by its author.");copy(p->file[0].path,"program.snx");p->file[0].offset=offset;p->file[0].size=(u32)n;p->file[0].component=1;p->file[0].crc=crc(package_bytes+offset,(u32)n);
 project_file(result_path,".sxp");if(Write(result_path,p,p->total_bytes)<0||Sync()<0){message("写入安装程序失败。");return;}message("安装程序已生成，完成安装向导后会加入应用程序库。");Log("SYSBASIC INSTALLER BUILT\n");
 if(open_output&&SpawnArgs("/boot/setup.snx",result_path)<0)message("安装程序已生成，但无法启动安装器，请稍后打开 .sxp。");
}
static void draw_editor(void){
 panel(126,129,607,431,0xffffff,1);fill(128,132,32,423,0xe6e6e6);int at=line_offset(scroll);
 for(int row=0;row<CODE_ROWS;row++){int line=scroll+row,start=at;if(!source[at]&&row>0)break;int y=CODE_Y+row*18;char label[12],value[80];number((u32)line+1,label);text(132,y,label,0x666666);
 if(line<BASIC_LINES&&breakpoints[line])fill(129,y+3,4,8,0xb00000);
 if((basic.error[0]&&line+1==basic.error_line)||(paused&&line==basic.pc))fill(CODE_X,y-1,556,18,basic.error[0]?0xffd8d8:0xffedb0);else if(select_all)fill(CODE_X,y-1,556,18,0xd3e3ff);
 int skip=0;while(source[at]&&source[at]!='\n'&&skip<hscroll){skip+=(u8)source[at]<128?1:2;at=next_char(at);}int n=0,col=0;
 while(source[at]&&source[at]!='\n'){int end=next_char(at),w=(u8)source[at]<128?1:2;if(col+w>CODE_COLS||n+end-at>78)break;for(int k=at;k<end;k++)value[n++]=source[k];col+=w;at=end;}value[n]=0;
 const char *token=value;while(*token==' ')token++;u32 color=*token=='\''?0x008000:((*token=='S'||*token=='E'||*token=='I'||*token=='D'||*token=='F')?0x0000a0:INK);text(CODE_X,y,value,color);
 int end=at;while(source[end]&&source[end]!='\n')end++;if(caret>=start&&caret<=end){int x=column_at(start,caret)-hscroll;if(x>=0&&x<CODE_COLS)fill(CODE_X+x*7,y,1,15,INK);}at=end;if(source[at]=='\n')at++;else break;
 }
}
static int property_field(int row){static const int grouped[]={2,1,3,4,5,6,7,8},alphabetic[]={2,1,6,8,3,4,7,5};return property_sort?grouped[row]:alphabetic[row];}
static int property_allowed(int f){if(selected<0)return f==1||f==2||f==5||f==6;if(f==7)return project.controls[selected].type==CONTROL_CHECK||project.controls[selected].type==CONTROL_LIST;if(f==8)return project.controls[selected].type==CONTROL_TIMER;return 1;}
static void select_object(int index,int design){commit_property();selected=index;field=object_popup=0;event_menu=selected<0?0:project.controls[selected].type==CONTROL_TEXT?1:project.controls[selected].type==CONTROL_TIMER?2:0;last_control=-2;if(design){view=0;basic.running=paused=0;}message(selected<0?"已选择窗体。":"已选择控件，可编辑右侧属性。");}
static void project_name(char *out){u32 at=strlen(path)-4,start=at;while(start&&path[start-1]!='/')start--;u32 n=at-start;if(n>30)n=30;memcpy(out,path+start,n);out[n]=0;}
static void draw_properties(void){
 panel(741,246,239,315,GREY,0);title(744,249,233,"属性 Properties");panel(748,276,225,23,0xffffff,1);text(753,281,selected<0?project.name:project.controls[selected].name,INK);text(955,281,"v",INK);
 button(748,301,108,"A-Z",!property_sort);button(862,301,110,"分类",property_sort);panel(748,328,225,192,0xffffff,1);
 const char *labels[]={"","Caption","(Name)","Left","Top","Width","Height","Value","Interval"};
 for(int row=0;row<8;row++){int f=property_field(row),y=329+row*23,allowed=property_allowed(f);char value[128];value[0]=0;
 if(field==f){fill(845,y,126,23,property_selected?BLUE:0xe5efff);copy(value,property);}else if(!allowed)copy(value,"--");else if(selected>=0){if(f==1)copy(value,project.controls[selected].text);else if(f==2)copy(value,project.controls[selected].name);else number((u32)property_value(f),value);}else{if(f==2)copy(value,project.name);else if(f==1)copy(value,project.title);else number((u32)(f==5?project.width:project.height),value);}
 text(752,y+4,labels[f],allowed?INK:0x808080);u32 start=0,n=strlen(value);if(field==f&&n>16){start=n-16;while(((u8)value[start]&0xc0)==0x80)start++;}emit(UI_TEXT,850,y+4,120,2,value+start,field==f&&property_selected?0xffffff:allowed?INK:0x808080);if(field==f&&!property_selected)fill(968,y+4,1,15,INK);emit(UI_LINE,750,y+22,220,0,0,0xe6e6e6);
 }text(750,533,field?"Enter 应用 / Esc 取消":"单击值编辑；灰色项不适用",0x404040);
}

static void draw(void){
 ui_begin(&scene);
 if(view!=1)FormsRender(view==2?&preview:&project,&scene,FX,FY,view==0);
 if(view==1)fill(122,93,615,471,0x808080);else{const FormProject *p=view==2?&preview:&project;fill(122,93,615,33,0x808080);fill(122,126,23,438,0x808080);fill(FX+p->width,126,737-FX-p->width,438,0x808080);fill(FX,FY+p->height,p->width,564-FY-p->height,0x808080);fill(FX,126,p->width,22,BLUE);text(FX+5,130,p->title,0xffffff);}
 if(view==0){emit(UI_GRID,FX,FY,project.width,project.height,0,0x939393);UiScene controls;ui_begin(&controls);FormsRender(&project,&controls,FX,FY,1);for(u32 i=1;i<controls.count&&canvas.count<UI_CANVAS_COMMANDS;i++)canvas.commands[canvas.count++]=controls.commands[i];if(selected>=0){Control *c=&project.controls[selected];emit(UI_SELECTION,FX+c->x,FY+c->y,c->w,c->h,0,0x303030);}if(placing){int x=draw_x<drag_x?draw_x:drag_x,y=draw_y<drag_y?draw_y:drag_y,w=draw_x<drag_x?drag_x-draw_x:draw_x-drag_x,h=draw_y<drag_y?drag_y-draw_y:draw_y-drag_y;if(w>=4&&h>=4)emit(UI_SELECTION,FX+x,FY+y,w,h,0,BLUE);}}
 fill(2,30,980,60,GREY);const char *menus[]={"文件(F)","编辑(E)","视图(V)","工程(P)","运行(R)","帮助(H)"};for(int i=0;i<6;i++){if(menu==i)fill(8+i*91,32,88,21,BLUE);text(12+i*91,36,menus[i],menu==i?0xffffff:INK);}
 const char *buttons[]={"新建","打开","保存","窗体","代码","运行 >","停止 []","单步","生成..."};for(int i=0;i<9;i++)button(8+i*82,59,i==8?100:77,buttons[i],(i==DESIGN&&view==0)||(i==CODE&&view==1)||(i==RUN&&view==2));text(858,65,modified?"* 未保存":"已保存",modified?0x9b2020:0x404040);
 panel(4,94,112,469,GREY,0);title(7,97,106,"工具箱");const char *tools[]={"指针","Label","Button","TextBox","CheckBox","ListBox","Timer","Menu"};const char *icons[]={"^","Aa","[OK]","ab|","[x]","= =","(t)","---"};
 for(int i=0;i<8;i++){int y=128+i*45;panel(10,y,99,40,tool==i?0xe2e2e2:GREY,tool==i);text(15,y+4,icons[i],i==tool?BLUE:INK);text(15,y+22,tools[i],INK);}
 panel(741,94,239,145,GREY,0);title(744,97,233,"工程资源管理器");panel(748,126,225,107,0xffffff,1);char project_label[64];copy(project_label,tree_closed?"[+] ":"[-] ");project_name(project_label+4);text(754,133,project_label,INK);if(!tree_closed){text(768,156,forms_closed?"[+] 窗体":"[-] 窗体",INK);if(!forms_closed){text(789,179,project.name,selected<0&&view!=1?BLUE:INK);text(789,205,"代码 / 事件",view==1?BLUE:INK);}}draw_properties();
 if(view==1){draw_editor();panel(128,133,289,23,0xffffff,1);text(134,137,selected<0?project.name:project.controls[selected].name,INK);panel(423,133,306,23,0xffffff,1);text(429,137,selected<0?"Load  v":event_menu==1?"Change  v":event_menu==2?"Timer  v":"Click  v",INK);text(396,137,"v",INK);}else{char label[64];copy(label,project.name);append(label,view==2?" (运行)":" (设计)");text(132,100,label,0xffffff);}
 panel(122,570,615,99,0xffffff,1);title(125,573,609,paused?"监视 / 单步暂停":"输出 / 调试");if(paused){for(int i=0;i<basic.variables&&i<4;i++){char label[128],value[128];copy(label,basic.variable[i].name);append(label," = ");BasicText(&basic.variable[i].value,value);u32 n=strlen(label),m=strlen(value);if(m>58-n)m=58-n;memcpy(label+n,value,m);label[n+m]=0;text(132,599+i*16,label,INK);}}else for(int i=0;i<4;i++)text(132,599+i*16,output[i],INK);
 panel(741,570,239,99,GREY,0);title(744,573,233,"对象操作");button(749,601,108,"查看代码",view==1);button(863,601,108,"查看窗体",view==0);button(749,634,108,selected<0?"删除(不可用)":"删除控件",0);button(863,634,108,"选择窗体",selected<0);
 panel(4,675,976,23,GREY,1);text(10,679,status,INK);char pos[50];copy(pos,"Ln ");number((u32)cursor_line()+1,pos+strlen(pos));append(pos," Col ");number((u32)column_at(line_offset(cursor_line()),caret)+1,pos+strlen(pos));text(818,679,pos,INK);
 if(menu>=0){const char *items[5];int count=0;
 if(menu==0){items[0]="新建工程   Ctrl+N";items[1]="打开工程   Ctrl+O";items[2]="保存工程   Ctrl+S";items[3]="工程另存为...";items[4]="生成程序...";count=5;}
 if(menu==1){items[0]="撤销       Ctrl+Z";items[1]="复制       Ctrl+C";items[2]="剪切       Ctrl+X";items[3]="粘贴       Ctrl+V";items[4]="删除所选控件";count=5;}
 if(menu==2){items[0]="窗体设计器";items[1]="代码编辑器";items[2]="打开应用管理器";count=3;}
 if(menu==3){items[0]="打开所选对象事件";items[1]="选择当前窗体";count=2;}
 if(menu==4){items[0]="启动 / 继续    F5";items[1]="停止          Esc";items[2]="逐语句        F8";count=3;}
 if(menu==5){items[0]="入门与可用功能";items[1]="打开计数器示例";items[2]="关于 SysBASIC";count=3;}
 int x=8+menu*91;panel(x,54,240,count*26+6,GREY,0);for(int i=0;i<count;i++)text(x+8,61+i*26,items[i],INK);
 }
 if(build_dialog){panel(224,195,536,285,GREY,0);fill(227,198,530,24,BLUE);text(239,203,"生成程序",0xffffff);text(246,240,"请选择输出类型：",INK);button(245,270,490,build_type==0?"(*) 可执行程序 (.snx) - 无需安装":"( ) 可执行程序 (.snx) - 无需安装",build_type==0);button(245,309,490,build_type==1?"(*) 安装程序 (.sxp) - 安装到软件库":"( ) 安装程序 (.sxp) - 安装到软件库",build_type==1);text(249,350,build_type?"安装包会打开独立向导，安装后登记软件条目。":"窗体与代码内嵌，生成一个可独立运行的文件。",INK);button(245,382,490,open_output?"[x] 生成后打开程序 / 安装向导":"[ ] 生成后打开程序 / 安装向导",0);button(512,438,100,"生成",0);button(628,438,100,"取消",0);}
 if(object_popup){int x=object_popup==1?748:object_popup==2?128:423,y=object_popup==1?300:156,w=object_popup==1?225:object_popup==2?289:306,rows=object_popup==3?1:project.count+1;panel(x,y,w,rows*20+4,0xffffff,0);for(int row=0;row<rows;row++){if(object_popup!=3&&row-1==selected)fill(x+2,y+2+row*20,w-4,20,0xc4d4f0);text(x+6,y+5+row*20,object_popup==3?(selected<0?"Load":event_menu==1?"Change":event_menu==2?"Timer":"Click"):row==0?project.name:project.controls[row-1].name,INK);}}
 if(Scene(window,&scene)<0)Log("SYSBASIC SCENE REJECTED\n");
}
static void delete_control(void){if(selected<0)return;dragging=placing=0;for(int i=selected;i<project.count-1;i++)project.controls[i]=project.controls[i+1];project.count--;selected=-1;changed();message("已删除控件；已有事件代码保留。");}
static void action(int id){
 commit_property();field=object_popup=dragging=placing=0;menu=-1;
 if(id==NEW||id==EXAMPLE){if(!confirm())return;if(id==EXAMPLE){FormsDefault(&project);copy(source,default_code);}else{FormsBlank(&project);source[0]=0;}copy(path,id==EXAMPLE?"/home/user/Projects/Hello.prj":"/home/user/Projects/Untitled.prj");memset(&basic,0,sizeof(basic));memset(output,0,sizeof(output));memset(breakpoints,0,sizeof(breakpoints));view=0;selected=-1;caret=scroll=hscroll=0;modified=1;known_project=0;tree_closed=forms_closed=object_popup=tool=0;last_control=-2;message(id==EXAMPLE?"已打开计数器示例。":"已新建空白工程：没有预置控件和示例代码。");}
 else if(id==OPEN){if(!confirm())return;char value[128];if(ui_dialog(window,&scene,"打开工程：输入 .prj 完整路径",value,1)&&value[0]){char previous[128];copy(previous,path);copy(path,value);if(!load())copy(path,previous);}}
 else if(id==SAVE)save();else if(id==DESIGN){view=0;basic.running=paused=0;}else if(id==CODE)view=1;else if(id==RUN)run();else if(id==STOP){basic.running=paused=0;view=0;message("已停止运行，返回窗体设计器。");}
 else if(id==STEP){if(!paused){if(BasicLoad(&basic,source)<0){message(basic.error);return;}preview=project;BasicStart(&basic,"Form_Load");}if(basic.running)BasicStep(&basic,&host);paused=1;view=1;caret=line_offset(basic.pc);reveal();message("单步暂停。F8 下一句，F5 继续。");}
 else if(id==BUILD){build_dialog=1;basic.running=paused=0;view=0;}
}
static void menu_action(int m,int row){
 menu=-1;if(m==0){if(row<3)action(row);else if(row==3){char value[128];if(ui_dialog(window,&scene,"另存工程：/home/user/.../Name.prj",value,1)&&value[0]){char previous[128];copy(previous,path);copy(path,value);int previous_known=known_project;known_project=0;if(!save()){copy(path,previous);known_project=previous_known;}}}else if(row==4)action(BUILD);}
 else if(m==1){if(row<4){view=1;edit_key(row==0?26:row==1?3:row==2?24:22);}else if(row==4&&view==0)delete_control();}
 else if(m==2){if(row<2)action(row==0?DESIGN:CODE);else if(row==2)Spawn("/boot/apps.snx");}
 else if(m==3){if(row==0)seed_event();else if(row==1){selected=-1;view=0;}}
 else if(m==4){if(row<3)action(row==0?RUN:row==1?STOP:STEP);}
 else if(m==5){if(row==1){action(EXAMPLE);return;}char out[128];ui_dialog(window,&scene,row==0?"选工具后在窗体拖出矩形；双击工具添加默认控件。八个点缩放，Esc 取消。":"SysBASIC 0.3：SINO 原创 BASIC 子集。非 VB6，不支持直接打开 VB 工程。",out,0);}
}
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static int snap(int n){return (n+4)/8*8;}
static void place_control(int x,int y,int w,int h){
 int i=FormsAdd(&project,tool,x,y);if(i<0){placing=0;message("最多 12 个控件。");return;}Control *c=&project.controls[i];
 if(w>=20&&h>=16){c->w=w;c->h=h;}if(c->w>project.width)c->w=project.width;if(c->h>project.height)c->h=project.height;c->x=clamp(c->x,0,project.width-c->w);c->y=clamp(c->y,0,project.height-c->h);
 selected=i;tool=placing=0;last_tool=-1;last_control=-2;event_menu=c->type==CONTROL_TEXT?1:c->type==CONTROL_TIMER?2:0;changed();message("控件已创建。拖动移动，八个选择点缩放，双击编写事件。");
}
/* Same eight positions as the public UI_SELECTION primitive. Edges remain anchored. */
static int handle_at(const Control *c,int x,int y){for(int i=0;i<8;i++){int col=i<3?i:i<5?(i==3?0:2):i-5,row=i<3?0:i<5?1:2;int hx=c->x+col*(c->w-5)/2,hy=c->y+row*(c->h-5)/2;if(x>=hx-2&&x<hx+7&&y>=hy-2&&y<hy+7)return i+1;}return 0;}
static void design_click(UiEvent *e){
 if(!hit(e,FX,FY,project.width,project.height))return;commit_property();int x=e->x-FX,y=e->y-FY;
 if(tool){drag_x=draw_x=clamp(snap(x),0,project.width);drag_y=draw_y=clamp(snap(y),0,project.height);placing=1;last_control=-2;last_tool=-1;return;}
 resize_control=selected>=0?handle_at(&project.controls[selected],x,y):0;
 if(!resize_control){selected=-1;for(int i=project.count-1;i>=0;i--){Control *c=&project.controls[i];if(x>=c->x&&y>=c->y&&x<c->x+c->w&&y<c->y+c->h){selected=i;break;}}}
 if(selected>=0){Control *c=&project.controls[selected];drag_before=*c;drag_modified=modified;dragging=1;drag_x=x;drag_y=y;u32 now=Tick();event_menu=c->type==CONTROL_TEXT?1:c->type==CONTROL_TIMER?2:0;if(!resize_control&&last_control==selected&&now-last_click<30){dragging=0;seed_event();}last_control=selected;last_click=now;}
 else{u32 now=Tick();if(last_control==-1&&now-last_click<30)seed_event();last_control=-1;last_click=now;}
}
static void motion(UiEvent *e){
 if(view!=0||(e->type!=MSG_MOUSE_MOVE&&e->type!=MSG_MOUSE_UP))return;
 int x=clamp(snap(e->x-FX),0,project.width),y=clamp(snap(e->y-FY),0,project.height);
 if(placing){draw_x=x;draw_y=y;if(e->type==MSG_MOUSE_UP){int left=x<drag_x?x:drag_x,top=y<drag_y?y:drag_y,w=x<drag_x?drag_x-x:x-drag_x,h=y<drag_y?drag_y-y:y-drag_y;place_control(left,top,w,h);}return;}
 if(!dragging||selected<0)return;
 /* Apply the release position too: fast drags can have coalesced motion events. */
 Control *c=&project.controls[selected],next=drag_before;int dx,dy;
 /* C truncates negative division toward zero; round signed deltas symmetrically. */
 dx=e->x-FX-drag_x;dy=e->y-FY-drag_y;dx=dx<0?-snap(-dx):snap(dx);dy=dy<0?-snap(-dy):snap(dy);
 if(resize_control){int i=resize_control-1,col=i<3?i:i<5?(i==3?0:2):i-5,row=i<3?0:i<5?1:2;
 if(col==0&&dx){next.x=clamp(snap(drag_before.x+dx),0,drag_before.x+drag_before.w-20);next.w=drag_before.x+drag_before.w-next.x;}else if(col==2&&dx)next.w=clamp(snap(drag_before.w+dx),20,project.width-next.x);
 if(row==0&&dy){next.y=clamp(snap(drag_before.y+dy),0,drag_before.y+drag_before.h-16);next.h=drag_before.y+drag_before.h-next.y;}else if(row==2&&dy)next.h=clamp(snap(drag_before.h+dy),16,project.height-next.y);
 }else{if(dx)next.x=clamp(snap(drag_before.x+dx),0,project.width-next.w);if(dy)next.y=clamp(snap(drag_before.y+dy),0,project.height-next.h);}
 if(next.x!=c->x||next.y!=c->y||next.w!=c->w||next.h!=c->h){*c=next;changed();last_control=-2;}
 if(e->type==MSG_MOUSE_UP)dragging=0;
}
static int menu_rows(int m){return m==0||m==1?5:m==3?2:3;}
static void click(UiEvent *e){
 if(build_dialog){if(hit(e,245,270,490,25))build_type=0;else if(hit(e,245,309,490,25))build_type=1;else if(hit(e,245,382,490,25))open_output=!open_output;else if(hit(e,628,438,100,25))build_dialog=0;else if(hit(e,512,438,100,25)){build_dialog=0;build();}return;}
 if(object_popup){int popup=object_popup,x=popup==1?748:popup==2?128:423,y=popup==1?300:156,w=popup==1?225:popup==2?289:306,rows=popup==3?1:project.count+1;object_popup=0;if(hit(e,x,y,w,rows*20+4)){int row=(e->y-y-2)/20;if(row<0)row=0;if(row>=rows)row=rows-1;if(popup==3)seed_event();else select_object(row-1,popup==1);return;}}
 if(menu>=0&&hit(e,8+menu*91,54,240,menu_rows(menu)*26+6)){menu_action(menu,(e->y-58)/26);return;}if(hit(e,8,32,546,22)){menu=(e->x-8)/91;return;}menu=-1;
 if(hit(e,8,59,756,25)){int id=(e->x-8)/82;if(id<=BUILD&&e->x<8+id*82+(id==BUILD?100:77))action(id);return;}
 if(hit(e,10,128,99,360)){commit_property();int chosen=(e->y-128)/45;u32 now=Tick();tool=chosen;placing=dragging=0;view=0;basic.running=paused=0;if(chosen&&last_tool==chosen&&now-tool_click<30)place_control(24+(project.count%5)*16,24+(project.count%5)*16,0,0);else message(chosen?"在窗体按下并拖出矩形；双击工具可添加默认大小。":"指针：选择和移动控件；拖动八个选择点缩放。");last_tool=chosen;tool_click=now;return;}
 if(hit(e,748,276,225,23)){commit_property();object_popup=1;return;}
 if(hit(e,748,301,225,25)){commit_property();property_sort=e->x>=862;return;}
 if(hit(e,748,329,225,184)){int f=property_field((e->y-329)/23);if(!property_allowed(f)){commit_property();message("灰色属性对当前对象不适用。");return;}view=0;basic.running=paused=0;begin_property(f);return;}
 commit_property();field=0;
 if(hit(e,749,601,108,25)){view=1;return;}if(hit(e,863,601,108,25)){action(DESIGN);return;}if(hit(e,749,634,108,25)){if(selected>=0){view=0;delete_control();}else message("窗体不能删除，可新建空白工程。");return;}if(hit(e,863,634,108,25)){select_object(-1,1);return;}
 if(hit(e,750,128,220,22)){tree_closed=!tree_closed;return;}if(!tree_closed&&hit(e,765,151,200,22)){forms_closed=!forms_closed;return;}
 if(!tree_closed&&!forms_closed){if(hit(e,780,175,190,26)){select_object(-1,1);return;}if(hit(e,780,202,190,25)){select_object(-1,0);view=1;return;}}
 if(view==0){design_click(e);return;}
 if(view==1){if(hit(e,128,133,289,23)){object_popup=2;return;}if(hit(e,423,133,306,23)){object_popup=3;return;}if(hit(e,128,CODE_Y,598,CODE_ROWS*18)){int line=scroll+(e->y-CODE_Y)/18;if(line>=lines())line=lines()-1;if(e->x<CODE_X){if(line<BASIC_LINES){breakpoints[line]=!breakpoints[line];char number_text[12];number((u32)line+1,number_text);Log("SYSBASIC BREAKPOINT TOGGLED ");Log(number_text);Log("\n");}}else{int col=hscroll+(e->x-CODE_X)/7;caret=line_offset(line);while(col>0&&source[caret]&&source[caret]!='\n'){col-=(u8)source[caret]<128?1:2;caret=next_char(caret);}select_all=0;}reveal();}return;}
 char event[64];if(FormsInput(&preview,e,FX,FY,&preview_focus,event)&&event[0]&&!basic.running)BasicStart(&basic,event);
}

static void key(UiEvent *e){
 int k=e->key;if(build_dialog){if(e->type==MSG_KEY){if(k==27)build_dialog=0;else if(k==0x148||k==0x150||k==9)build_type=!build_type;else if(k==13){build_dialog=0;build();}}return;}
 if(e->type==MSG_KEY){if(k==27&&(placing||dragging||tool)){if(dragging&&selected>=0){project.controls[selected]=drag_before;modified=drag_modified;}placing=dragging=tool=0;last_control=-2;message("已取消控件操作。");return;}if(k==19){commit_property();save();return;}if(k==14){action(NEW);return;}if(k==15){action(OPEN);return;}if(k==0x13f){action(RUN);return;}if(k==0x142){action(STEP);return;}if(k==27){if(field){field=0;message("取消属性编辑。");}else if(object_popup)object_popup=0;else if(menu>=0)menu=-1;else action(STOP);return;}}
 if(field){u32 n=strlen(property);if(e->type==MSG_KEY&&(k==13||k==9)){int previous=field;commit_property();if(k==9){int row=0;while(row<8&&property_field(row)!=previous)row++;for(int i=1;i<=8;i++){int f=property_field((row+i)%8);if(property_allowed(f)){begin_property(f);break;}}}return;}if(e->type==MSG_KEY&&k==1){property_selected=1;return;}if(e->type==MSG_TEXT||(k>=32&&k<127)||k==8){if(property_selected){property[0]=0;n=0;property_selected=0;}}if(e->type==MSG_TEXT){char value[5]={0};text_encode((u32)k,value);if(n+strlen(value)<100)append(property,value);}else if(k==8&&n)property[text_previous(property,n)]=0;else if(k>=32&&k<127&&n<100){property[n]=(char)k;property[n+1]=0;}return;}

 if(view==1){if(e->type==MSG_TEXT){char s[5]={0};text_encode((u32)k,s);save_undo();insert(s);}else edit_key(k);}else if(view==0&&e->type==MSG_KEY&&k==0x153)delete_control();else if(view==2){char event[64];if(FormsInput(&preview,e,FX,FY,&preview_focus,event)&&event[0]&&!basic.running)BasicStart(&basic,event);}
}
int main(void){
 FormsBlank(&project);window=Window("SysBASIC 0.3",984,704);if(window<0)return 1;api(SYS_WINDOW_POLICY,(u32)window,3,0);message("就绪。选工具后拖出控件；双击写事件；F5 运行。");if(Read(path,package_bytes,sizeof(package_bytes))>=0)load();draw();
 for(;;){UiEvent e;int dirty=0;if(Event(&e)==0){dirty=1;if(e.type==MSG_CLOSE){commit_property();if(confirm())return 0;}else if(e.type==MSG_KEY||e.type==MSG_TEXT)key(&e);else if(e.type==MSG_CLICK)click(&e);motion(&e);}
 if(view==2&&!basic.running){char event[64];if(FormsTimer(&preview,event))BasicStart(&basic,event);}
 if(view==2&&basic.running&&!paused){for(int i=0;i<8&&basic.running;i++){if(basic.pc!=skip_breakpoint&&basic.pc>=0&&basic.pc<BASIC_LINES&&breakpoints[basic.pc]){paused=1;view=1;caret=line_offset(basic.pc);reveal();message("命中断点，F8 单步，F5 继续。");Log("SYSBASIC BREAKPOINT HIT\n");break;}skip_breakpoint=-1;BasicStep(&basic,&host);dirty=1;}if(basic.error[0]){view=1;caret=line_offset(basic.error_line-1);reveal();message(basic.error);}}
 if(dirty)draw();Yield(1);}
}
