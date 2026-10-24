/* SINO SH: project-specific shell, separate from the SINO32 kernel. */
#include "console.h"
#include "graphics.h"
#include "vfs.h"
#include "process.h"
#include "storage.h"
#include "banner.h"
#define LINE_SIZE 256
#define TOKENS 32
#define PIPE_SIZE 8192u
#define SCRIPT_DEPTH 4
typedef struct {u8 *data;u32 length,capacity;int overflow;} Sink;
typedef struct {const u8 *input;u32 input_length;int input_present;Sink *out;int depth;} Context;
typedef struct {char text[LINE_SIZE];int type;} Token;
static char line[LINE_SIZE],history[32][LINE_SIZE],draft[LINE_SIZE],hostname[32]="sino";
static u32 length,position,painted,anchor,history_count;
static int history_position=-1,last_status,previous_cwd;
static u8 work[MAX_FILE+1],pipes[SCRIPT_DEPTH][2][PIPE_SIZE+1];
static char scripts[SCRIPT_DEPTH][4096];
u32 native_base,native_size;int native_running;
static Sink *native_sink;
static const char *commands[]={"help","ls","cd","pwd","cat","echo","clear","uname","ver","mem","free","uptime","date","time","history","whoami","hostname","env","which","touch","mkdir","rmdir","rm","cp","mv","write","append","head","tail","wc","grep","hexdump","stat","tree","find","df","run","source","sleep","ps","about","credits","reboot","halt","sync","install","nettest","kill",0};
static void strcopy(char *d,const char *s){while((*d++=*s++)){} }
static int starts(const char *s,const char *prefix){while(*prefix)if(*s++!=*prefix++)return 0;return 1;}
static int contains(const char *s,u32 length,const char *pattern){u32 n=strlen(pattern);if(n>length)return 0;for(u32 i=0;i+n<=length;i++){u32 j=0;while(j<n&&s[i+j]==pattern[j])j++;if(j==n)return 1;}return 0;}
static void emit(Sink *sink,const void *bytes,u32 n){const u8 *p=bytes;if(sink&&sink->data){if(n>sink->capacity-sink->length){sink->overflow=1;return;}memcpy(sink->data+sink->length,p,n);sink->length+=n;sink->data[sink->length]=0;}else{for(u32 i=0;i<n;i++){outb(0xe9,p[i]);con_put((char)p[i]);}}}
static void put(Sink *s,const char *text){emit(s,text,strlen(text));}
static void num(Sink *s,u32 n){char b[11];number(n,b);put(s,b);}
static int error(Sink *s,const char *command,const char *reason){put(s,command);put(s,": ");put(s,reason);put(s,"\n");return 1;}
static int verror(Sink *s,const char *command){return error(s,command,vfs_error);}
void native_write(const char *s,u32 n){emit(native_sink,s,n);}
static void prompt(void){char path[VFS_PATH];vfs_path(cwd,path);con_color(10);con_write("user@");con_write(hostname);con_color(7);con_write(":");con_color(11);if(starts(path,"/home/user")&&(path[10]==0||path[10]=='/')){con_write("~");con_write(path+10);}else con_write(path);con_color(7);con_write("$ ");anchor=con_position();painted=0;}
static void repaint(void){con_edit(anchor,line,length,position,painted);painted=length;con_flush();}
static const char *variable(const char *name,char *buffer){if(streq(name,"PWD")){vfs_path(cwd,buffer);return buffer;}if(streq(name,"HOME"))return "/home/user";if(streq(name,"USER"))return "user";if(streq(name,"HOSTNAME"))return hostname;if(streq(name,"SHELL"))return "sinosh";if(streq(name,"PATH"))return "/boot";if(streq(name,"VERSION"))return "0.10";if(streq(name,"?")){number((u32)last_status,buffer);return buffer;}return "";}
static int token_char(Token *t,u32 *n,char c){if(*n>=LINE_SIZE-1)return 0;t->text[(*n)++]=c;return 1;}
static int tokenize(const char *s,Token *t){int count=0;u32 i=0;
    while(s[i]){while(s[i]==' '||s[i]=='\t')i++;if(!s[i]||s[i]=='#')break;if(count>=TOKENS)return -1;Token *v=&t[count];v->type=0;u32 n=0;
        if(s[i]=='|'||s[i]==';'||s[i]=='<'||s[i]=='>'){v->type=(u8)s[i++];if(v->type=='>'&&s[i]=='>'){v->type='+';i++;}v->text[0]=0;count++;continue;}
        char quote=0;int began=0;
        while(s[i]){char c=s[i];if(!quote&&(c==' '||c=='\t'||c=='|'||c==';'||c=='<'||c=='>'))break;
            began=1;if((c=='\''||c=='"')&&(!quote||quote==c)){quote=quote?0:c;i++;continue;}
            if(c=='\\'&&quote!='\''){if(quote=='"'&&s[i+1]!='"'&&s[i+1]!='\\'&&s[i+1]!='$'){if(!token_char(v,&n,'\\'))return -1;i++;continue;}i++;if(!s[i])return -1;if(!token_char(v,&n,s[i++]))return -1;continue;}
            if(c=='$'&&quote!='\''){i++;char name[32],buffer[VFS_PATH];u32 p=0;if(s[i]=='?')name[p++]=s[i++];else if(s[i]=='{'){i++;while(s[i]&&s[i]!='}'&&p<31)name[p++]=s[i++];if(s[i]!='}')return -1;i++;}else{while(((s[i]>='A'&&s[i]<='Z')||(s[i]>='a'&&s[i]<='z')||s[i]=='_'||(s[i]>='0'&&s[i]<='9'))&&p<31)name[p++]=s[i++];}
                name[p]=0;if(!p){if(!token_char(v,&n,'$'))return -1;}else for(const char *value=variable(name,buffer);*value;value++)if(!token_char(v,&n,*value))return -1;continue;
            }
            if(!token_char(v,&n,c))return -1;i++;
        }if(quote)return -1;v->text[n]=0;if(began)count++;
    }return count;
}
static int execute(const char *,Sink *,int);
static int read_input(Context *c,const char *path,const u8 **data,u32 *size){
    if(!path){if(!c->input_present)return error(c->out,"input","provide a file or pipe");*data=c->input;*size=c->input_length;return 0;}
    int n=vfs_lookup(path);if(n<0)return verror(c->out,path);int bytes=vfs_read(n,work,MAX_FILE);if(bytes<0)return verror(c->out,path);work[bytes]=0;*data=work;*size=(u32)bytes;return 0;
}
static int parse_number(const char *s,u32 *value){if(!*s)return 0;u32 n=0;while(*s){if(*s<'0'||*s>'9'||n>100000)return 0;n=n*10+(u32)(*s++-'0');}*value=n;return 1;}
static int builtin(const char *name){for(int i=0;commands[i];i++)if(streq(name,commands[i]))return 1;return 0;}
static void list_node(Sink *out,int n,int detail){Node *p=&nodes[n];if(detail){put(out,p->dir?"dir ":p->readonly?"ro  ":"ram ");num(out,p->size);put(out,"\t");}put(out,p->name);if(p->dir)put(out,"/");put(out,"\n");}
static void list_tree(Sink *out,int parent,u32 depth){if(depth>16)return;for(int i=1;i<VFS_NODES;i++)if(nodes[i].used&&nodes[i].parent==parent){for(u32 j=0;j<depth;j++)put(out,"  ");list_node(out,i,0);if(nodes[i].dir)list_tree(out,i,depth+1);}}
static int run_program(Context *c,const char *path,const char *args){int n=vfs_lookup(path);if(n<0)return verror(c->out,path);if(nodes[n].size>=32){int size=vfs_read(n,work,MAX_FILE);if(size>=32&&*(u32 *)work==NATIVE_MAGIC){int pid=process_launch_args(path,CAP_HOME,args);if(pid<0)return error(c->out,path,"native launch failed");put(c->out,"Started user process ");num(c->out,(u32)pid);put(c->out,"\n");return 0;}}if(nodes[n].boot<0||!streq(nodes[n].name,"hello.f32"))return error(c->out,path,"legacy ring-0 binaries are restricted to trusted boot demo");int bytes=vfs_read(n,work,4096);if(bytes<0)return verror(c->out,path);if(bytes<16||work[0]!='F'||work[1]!='X'||work[2]!='3'||work[3]!='2')return error(c->out,path,"not a native F32 executable");u32 entry=*(u32 *)(work+4);if(entry<16||entry>=(u32)bytes)return error(c->out,path,"invalid F32 entry");
    native_base=(u32)page_alloc();if(!native_base)return error(c->out,path,"out of memory");native_size=(u32)bytes;memcpy((void *)native_base,work,native_size);native_sink=c->out;native_running=1;debug("NATIVE32 shell program entered\n");run_native(native_base+entry);native_running=0;native_sink=0;if(!page_free((void *)native_base))panic("Native page release",native_base,0);native_base=native_size=0;debug("NATIVE32 shell program returned\n");return 0;
}
static u8 cmos(u8 r){outb(0x70,r);return inb(0x71);}
static u32 bcd(u8 v,int binary){return binary?v:(v>>4)*10+(v&15);}
static void two(Sink *out,u32 n){if(n<10)put(out,"0");num(out,n);}
static void clock_output(Sink *out,int date){u8 a[7],b[7];const u8 reg[]={0,2,4,7,8,9,0x32};u8 format=cmos(0x0b);for(u32 tries=0;tries<8;tries++){u32 guard=100000;while((cmos(10)&128)&&guard--){}for(u32 i=0;i<7;i++)a[i]=cmos(reg[i]);guard=100000;while((cmos(10)&128)&&guard--){}int equal=1;for(u32 i=0;i<7;i++){b[i]=cmos(reg[i]);if(a[i]!=b[i])equal=0;}if(equal)break;}
    u32 v[7];for(u32 i=0;i<7;i++)v[i]=bcd(a[i],format&4);v[2]=bcd(a[2]&127,format&4);if(!(format&2))v[2]=v[2]%12+((a[2]&128)?12:0);
    if(date){num(out,(v[6]?v[6]:20)*100+v[5]);put(out,"-");two(out,v[4]);put(out,"-");two(out,v[3]);}else{two(out,v[2]);put(out,":");two(out,v[1]);put(out,":");two(out,v[0]);}put(out,"\n");
}
static int command(Context *c,int argc,char **argv){Sink *out=c->out;char *cmd=argv[0];for(char *p=cmd;*p;p++)if(*p>='A'&&*p<='Z')*p+=32;
    if(streq(cmd,"dir"))cmd="ls";if(streq(cmd,"type"))cmd="cat";if(streq(cmd,"cls"))cmd="clear";if(streq(cmd,"md"))cmd="mkdir";if(streq(cmd,"del"))cmd="rm";if(streq(cmd,"copy"))cmd="cp";if(streq(cmd,"ren"))cmd="mv";if(streq(cmd,"."))cmd="source";
    if(streq(cmd,"help")){put(out,
        "SINO SH commands\n"
        "  ls [-l] [path]   cd [path]   pwd   tree [path]   find [path] [text]\n"
        "  cat [file]       head/tail [-n N] [file]       stat file\n"
        "  grep [-n] text [file]   wc [-l|-w|-c] [file]   hexdump file\n"
        "  mkdir [-p] path  rmdir path  touch file  rm file  cp/mv source dest\n"
        "  write file text   append file text   echo [-e|-n] text\n"
        "  run file.f32    source file.sh    sleep seconds\n"
        "  uname [-a]  ver  mem/free  df  uptime  date  time  ps\n"
        "  history  whoami  hostname [name]  env  which command\n"
        "  clear  about/credits  reboot  halt\n\n"
        "Syntax: quotes, $PWD/$HOME/$USER/$?, ;, pipes, <, > and >>\n"
        "Keys: Tab complete; Up/Down history; Left/Right/Home/End edit;\n"
        "      PgUp/PgDn scroll; Ctrl+L clear; Ctrl+C cancel input.\n"
        "DOS aliases: DIR TYPE CLS MD DEL COPY REN VER\n"
        "Home files are saved to the data disk when one is attached. /tmp is temporary.\n");return 0;}
    if(streq(cmd,"pwd")){char p[VFS_PATH];vfs_path(cwd,p);put(out,p);put(out,"\n");return 0;}
    if(streq(cmd,"cd")){int back=argc>1&&streq(argv[1],"-"),n=back?previous_cwd:vfs_lookup(argc>1?argv[1]:"/home/user");if(n<0)return verror(out,"cd");if(!nodes[n].used||!nodes[n].dir)return error(out,"cd","not a directory");previous_cwd=cwd;cwd=n;if(back){char p[VFS_PATH];vfs_path(cwd,p);put(out,p);put(out,"\n");}return 0;}
    if(streq(cmd,"ls")){int detail=0,all=0;const char *path=".";for(int i=1;i<argc;i++){if(argv[i][0]=='-'&&argv[i][1]){for(u32 j=1;argv[i][j];j++){if(argv[i][j]=='l')detail=1;else if(argv[i][j]=='a')all=1;else return error(out,"ls","supported options: -l, -a, -la");}}else path=argv[i];}int n=vfs_lookup(path);if(n<0)return verror(out,"ls");if(!nodes[n].dir)list_node(out,n,detail);else for(int i=1;i<VFS_NODES;i++)if(nodes[i].used&&nodes[i].parent==n&&(all||nodes[i].name[0]!='.'))list_node(out,i,detail);return 0;}
    if(streq(cmd,"tree")){int n=vfs_lookup(argc>1?argv[1]:".");if(n<0)return verror(out,"tree");put(out,argc>1?argv[1]:".");put(out,"\n");if(nodes[n].dir)list_tree(out,n,1);return 0;}
    if(streq(cmd,"find")){char base[VFS_PATH],p[VFS_PATH];if(vfs_canonical(argc>1?argv[1]:".",base)<0)return verror(out,"find");int n=vfs_lookup(base);if(n<0)return verror(out,"find");u32 len=strlen(base);for(int i=0;i<VFS_NODES;i++)if(nodes[i].used){vfs_path(i,p);if(starts(p,base)&&(len==1||p[len]==0||p[len]=='/')&&(argc<3||contains(p,strlen(p),argv[2]))){put(out,p);put(out,"\n");}}return 0;}
    if(streq(cmd,"cat")){if(argc==1){const u8 *data;u32 size;if(read_input(c,0,&data,&size))return 1;emit(out,data,size);}else for(int i=1;i<argc;i++){const u8 *data;u32 size;if(read_input(c,argv[i],&data,&size))return 1;emit(out,data,size);}return 0;}
    if(streq(cmd,"echo")){int escape=0,newline=1,start=1;while(start<argc&&argv[start][0]=='-'){if(streq(argv[start],"-e"))escape=1;else if(streq(argv[start],"-n"))newline=0;else break;start++;}for(int i=start;i<argc;i++){if(i>start)put(out," ");for(u32 j=0;argv[i][j];j++){char ch=argv[i][j];if(escape&&ch=='\\'&&argv[i][j+1]){ch=argv[i][++j];if(ch=='n')ch='\n';else if(ch=='t')ch='\t';else if(ch=='r')ch='\r';}emit(out,&ch,1);}}if(newline)put(out,"\n");return 0;}
    if(streq(cmd,"clear")){con_clear();return 0;}
    if(streq(cmd,"touch")||streq(cmd,"mkdir")){int dir=streq(cmd,"mkdir"),parents=argc>1&&streq(argv[1],"-p"),start=parents?2:1;if(argc<=start)return error(out,cmd,"missing path");for(int i=start;i<argc;i++){int n=vfs_create(argv[i],dir,parents);if(n<0)return verror(out,cmd);if(nodes[n].readonly)return error(out,cmd,"read-only file system (/boot)");}return 0;}
    if(streq(cmd,"rm")||streq(cmd,"rmdir")){if(argc<2)return error(out,cmd,"missing path");for(int i=1;i<argc;i++)if(vfs_remove(argv[i],streq(cmd,"rmdir"))<0)return verror(out,cmd);return 0;}
    if(streq(cmd,"cp")||streq(cmd,"mv")){if(argc!=3)return error(out,cmd,"usage: source destination");int status=streq(cmd,"cp")?vfs_copy(argv[1],argv[2],work):vfs_move(argv[1],argv[2]);return status<0?verror(out,cmd):0;}
    if(streq(cmd,"write")||streq(cmd,"append")){if(argc<3)return error(out,cmd,"usage: file text");u32 n=0;for(int i=2;i<argc;i++){if(i>2)work[n++]=' ';u32 bytes=strlen(argv[i]);if(n+bytes+1>MAX_FILE)return error(out,cmd,"text too long");memcpy(work+n,argv[i],bytes);n+=bytes;}work[n++]='\n';int index=vfs_create(argv[1],0,0);if(index<0||vfs_write(index,work,n,streq(cmd,"append"))<0)return verror(out,cmd);return 0;}
    if(streq(cmd,"stat")){if(argc!=2)return error(out,cmd,"provide one path");int n=vfs_lookup(argv[1]);if(n<0)return verror(out,cmd);char p[VFS_PATH];vfs_path(n,p);put(out,"Path: ");put(out,p);put(out,"\nType: ");put(out,nodes[n].dir?"directory":"file");put(out,"\nSize: ");num(out,nodes[n].size);put(out," bytes\nStorage: ");put(out,nodes[n].readonly?"FAT12 boot image (read-only)\n":"RAM (temporary)\n");return 0;}
    if(streq(cmd,"head")||streq(cmd,"tail")){u32 lines=10;int a=1;if(argc>2&&streq(argv[1],"-n")){if(!parse_number(argv[2],&lines))return error(out,cmd,"invalid line count");a=3;}const u8 *data;u32 size;if(read_input(c,a<argc?argv[a]:0,&data,&size))return 1;
        if(streq(cmd,"head")){u32 end=0,n=0;while(end<size&&n<lines)if(data[end++]=='\n')n++;emit(out,data,end);}else{u32 n=0,start=size;if(start&&data[start-1]=='\n')start--;while(start&&n<lines){start--;if(data[start]=='\n')n++;}if(n==lines)start++;if(lines)emit(out,data+start,size-start);}return 0;}
    if(streq(cmd,"grep")){int numbered=argc>1&&streq(argv[1],"-n"),a=numbered?2:1;if(a>=argc)return error(out,cmd,"missing literal search text");const char *pattern=argv[a++];const u8 *data;u32 size;if(read_input(c,a<argc?argv[a]:0,&data,&size))return 1;u32 begin=0,row=1;int found=0;for(u32 i=0;i<=size;i++)if(i==size||data[i]=='\n'){if(i>begin&&contains((const char *)data+begin,i-begin,pattern)){if(numbered){num(out,row);put(out,":");}emit(out,data+begin,i-begin);put(out,"\n");found=1;}begin=i+1;row++;}return found?0:1;}
    if(streq(cmd,"wc")){int a=1,which=0;if(argc>1&&argv[1][0]=='-'){which=streq(argv[1],"-l")?1:streq(argv[1],"-w")?2:streq(argv[1],"-c")?3:0;if(!which)return error(out,cmd,"use -l, -w or -c");a++;}const u8 *data;u32 size;if(read_input(c,a<argc?argv[a]:0,&data,&size))return 1;u32 rows=0,words=0;int inside=0;for(u32 i=0;i<size;i++){if(data[i]=='\n')rows++;int space=data[i]==' '||data[i]=='\t'||data[i]=='\n'||data[i]=='\r';if(!space&&!inside)words++;inside=!space;}if(!which||which==1)num(out,rows);if(!which)put(out," ");if(!which||which==2)num(out,words);if(!which)put(out," ");if(!which||which==3)num(out,size);put(out,"\n");return 0;}
    if(streq(cmd,"hexdump")){const u8 *data;u32 size;if(read_input(c,argc>1?argv[1]:0,&data,&size))return 1;const char *hex="0123456789abcdef";for(u32 off=0;off<size;off+=16){char address[9];for(u32 j=0;j<8;j++)address[j]=hex[(off>>(28-j*4))&15];address[8]=0;put(out,address);put(out,"  ");for(u32 j=0;j<16;j++){if(off+j<size){char byte[3]={hex[data[off+j]>>4],hex[data[off+j]&15],0};put(out,byte);put(out," ");}else put(out,"   ");}put(out," |");for(u32 j=0;j<16&&off+j<size;j++){char ch=data[off+j]>=32&&data[off+j]<=126?(char)data[off+j]:'.';emit(out,&ch,1);}put(out,"|\n");}return 0;}
    if(streq(cmd,"uname")||streq(cmd,"ver")){put(out,"SINO");if(streq(cmd,"ver")||(argc>1&&streq(argv[1],"-a"))){put(out," ");put(out,hostname);put(out," 0.10 SINO32 i386 (ring-3 native processes; legacy desktop)");}put(out,"\n");return 0;}
    if(streq(cmd,"mem")||streq(cmd,"free")){put(out,"Mapped usable range: ");num(out,usable_memory/1024);put(out," KiB\nFree memory: ");num(out,free_pages*4);put(out," KiB / ");num(out,free_pages);put(out," pages (4 KiB each)\n");return 0;}
    if(streq(cmd,"df")){u32 pages=0;for(int i=1;i<VFS_NODES;i++)if(nodes[i].used&&!nodes[i].readonly)pages+=(nodes[i].size+4095)/4096;put(out,"Mount   Type    Mode  Usage\n/boot   FAT12   ro    1440 KiB boot image\n/       RAMFS   rw    ");num(out,pages*4);put(out," KiB allocated; home is mirrored to the data disk\n");return 0;}
    if(streq(cmd,"uptime")){put(out,"up ");num(out,ticks/100);put(out," seconds (100 Hz kernel clock)\n");return 0;}
    if(streq(cmd,"date")||streq(cmd,"time")){clock_output(out,streq(cmd,"date"));return 0;}
    if(streq(cmd,"whoami")){put(out,"user\n");return 0;}
    if(streq(cmd,"hostname")){if(argc>1){u32 n=strlen(argv[1]);if(!n||n>=sizeof(hostname))return error(out,cmd,"use 1-31 characters");for(u32 i=0;i<n;i++)if(!(argv[1][i]>='a'&&argv[1][i]<='z')&&!(argv[1][i]>='A'&&argv[1][i]<='Z')&&!(argv[1][i]>='0'&&argv[1][i]<='9')&&argv[1][i]!='-')return error(out,cmd,"use letters, digits or hyphens");strcopy(hostname,argv[1]);}put(out,hostname);put(out,"\n");return 0;}
    if(streq(cmd,"env")){const char *names[]={"USER","HOME","PWD","HOSTNAME","SHELL","PATH","VERSION",0};for(int i=0;names[i];i++){char b[VFS_PATH];put(out,names[i]);put(out,"=");put(out,variable(names[i],b));put(out,"\n");}return 0;}
    if(streq(cmd,"which")){if(argc<2)return error(out,cmd,"missing command");if(builtin(argv[1])){put(out,argv[1]);put(out," (shell builtin)\n");return 0;}return error(out,cmd,"not found; use run /boot/hello.f32 for native programs");}
    if(streq(cmd,"history")){u32 first=history_count>32?history_count-32:0;for(u32 i=first;i<history_count;i++){num(out,i+1);put(out,"  ");put(out,history[i%32]);put(out,"\n");}return 0;}
    if(streq(cmd,"ps")){ProcessInfo list[8];int count=process_list(list,8);put(out,"PID  THREADS  PAGES  NATIVE APP\n");for(int i=0;i<count;i++){num(out,list[i].pid);put(out,"    ");num(out,list[i].threads);put(out,"        ");num(out,list[i].pages);put(out,"    ");put(out,list[i].name);put(out,"\n");}return 0;}
    if(streq(cmd,"about")||streq(cmd,"credits")){put(out,"SINO 0.10 + SINO SH\nProject-specific C/assembly generated and written for this project.\nNot a FreeDOS or Linux kernel fork; SINO is this project's name.\nThird-party tools: NASM, GCC/Binutils, Node.js and QEMU.\nFirmware: SeaBIOS / VGA BIOS (QEMU).\nUI fonts: Jarrah, Kurrajong, Mallee and GNU Unifont (SIL OFL 1.1).\nPinyin character readings: mozillazg/pinyin-data (MIT).\nReferences include Intel architecture manuals and QEMU documentation.\nSource details: cat /boot/sources.txt\n");return 0;}
    if(streq(cmd,"kill")){u32 pid;if(argc!=2||!parse_number(argv[1],&pid))return error(out,cmd,"expected PID");return process_kill(pid,0)<0?error(out,cmd,"no such process"):0;}
    if(streq(cmd,"nettest")){return process_launch("/boot/nettest.snx",CAP_HOME|CAP_NETWORK)<0?error(out,cmd,"network test cannot start"):0;}
    if(streq(cmd,"install")){if(argc!=2)return error(out,cmd,"expected package path or /quiet:path or /remove:id");int pid=process_launch_args("/boot/setup.snx",CAP_HOME,argv[1]);return pid<0?error(out,cmd,"cannot start installer"):0;}
    if(streq(cmd,"run")){if(argc<2||argc>3)return error(out,cmd,"run native.snx [argument]");return run_program(c,argv[1],argc==3?argv[2]:"");}
    if(streq(cmd,"source")){if(argc!=2)return error(out,cmd,"provide one script file");if(c->depth>=SCRIPT_DEPTH-1)return error(out,cmd,"script nesting limit is 4");int index=vfs_lookup(argv[1]);if(index<0)return verror(out,cmd);int size=vfs_read(index,(u8 *)scripts[c->depth],4095);if(size<0)return verror(out,cmd);scripts[c->depth][size]=0;u32 p=0;int status=0;while(p<(u32)size){char script_line[LINE_SIZE];u32 n=0;while(p<(u32)size&&scripts[c->depth][p]!='\n'){char ch=scripts[c->depth][p++];if(ch!='\r'){if(n>=LINE_SIZE-1)return error(out,cmd,"script line exceeds 255 characters");script_line[n++]=ch;}}if(p<(u32)size)p++;script_line[n]=0;status=execute(script_line,out,c->depth+1);}return status;}
    if(streq(cmd,"sleep")){u32 seconds;if(argc!=2||!parse_number(argv[1],&seconds)||seconds>3600)return error(out,cmd,"use 0-3600 seconds");u32 start=ticks;while(ticks-start<seconds*100){if(key_read()==3)return 130;halt();}return 0;}
    if(streq(cmd,"sync")){if(!storage_sync())return error(out,cmd,storage_error);put(out,"Home data saved.\n");return 0;}
    if(streq(cmd,"reboot")){if(storage_ready&&!storage_sync())return error(out,cmd,storage_error);put(out,"Restarting SINO...\n");con_flush();for(u32 n=0;n<100000&&(inb(0x64)&2);n++){}outb(0x64,0xfe);for(;;)halt();}
    if(streq(cmd,"halt")){if(storage_ready&&!storage_sync())return error(out,cmd,storage_error);put(out,"SINO halted. Close or reset the virtual machine.\n");con_flush();cli();for(;;)halt();}
    return error(out,cmd,"command not found; type help");
}
static int execute_segment(Token *tokens,int begin,int end,Sink *outer,int depth){
    u32 input_length=0;const u8 *input=0;int input_present=0;int status=0,stage=0;
    while(begin<end){int finish=begin;while(finish<end&&tokens[finish].type!='|')finish++;if(finish==begin)return error(outer,"syntax","empty pipeline stage");
        char *argv[TOKENS];int argc=0,append=0;char redirect[VFS_PATH];redirect[0]=0;
        Context context={input,input_length,input_present,outer,depth};
        for(int i=begin;i<finish;i++){int type=tokens[i].type;if(type=='>'||type=='+'||type=='<'){if(i+1>=finish||tokens[i+1].type)return error(outer,"syntax","redirection requires a filename");const char *name=tokens[++i].text;if(strlen(name)>=VFS_PATH)return error(outer,"syntax","path too long");
                if(type=='<'){if(input_present)return error(outer,"syntax","input already provided");const u8 *bytes;u32 size;if(read_input(&context,name,&bytes,&size))return 1;if(size>PIPE_SIZE)return error(outer,"input","redirection exceeds 8192 bytes");u8 *buffer=pipes[depth][(stage+1)&1];memcpy(buffer,bytes,size);input=buffer;input_length=size;input_present=1;context.input=input;context.input_length=size;context.input_present=1;}
                else{if(finish<end)return error(outer,"syntax","output redirection is allowed on the final stage");if(redirect[0])return error(outer,"syntax","multiple output targets");strcopy(redirect,name);append=type=='+';}
            }else if(type)return error(outer,"syntax","unexpected operator");else argv[argc++]=tokens[i].text;
        }
        if(!argc)return error(outer,"syntax","missing command");Sink capture={pipes[depth][stage&1],0,PIPE_SIZE,0};int capturing=finish<end||redirect[0];if(capturing){capture.data[0]=0;context.out=&capture;}
        status=command(&context,argc,argv);if(capture.overflow)return error(outer,"output","exceeds 8192-byte pipe/redirection limit");
        if(redirect[0]&&!status){int index=vfs_create(redirect,0,0);if(index<0||vfs_write(index,capture.data,capture.length,append)<0)return verror(outer,"redirect");}
        input=capture.data;input_length=capture.length;input_present=1;stage++;begin=finish+1;
    }return status;
}
static int execute(const char *s,Sink *out,int depth){
    if(depth>=SCRIPT_DEPTH)return error(out,"source","nesting limit exceeded");Token tokens[TOKENS];char segment[LINE_SIZE];u32 used=0;char quote=0;int escaped=0,status=0,comment=0;
    for(u32 i=0;;i++){char ch=s[i];if(!quote&&!escaped&&ch=='#'&&(used==0||segment[used-1]==' '||segment[used-1]=='\t'))comment=1;
        if(ch==0||comment||(!quote&&!escaped&&ch==';')){segment[used]=0;int count=tokenize(segment,tokens);if(count<0){error(out,"syntax","unclosed quote, too many tokens or argument too long");last_status=2;return 2;}if(count){status=execute_segment(tokens,0,count,out,depth);last_status=status;}else if(ch==';'){error(out,"syntax","empty command");last_status=2;return 2;}used=0;if(!ch||comment)break;continue;}
        if(used>=LINE_SIZE-1){last_status=2;error(out,"syntax","command exceeds 255 characters");return 2;}segment[used++]=ch;
        if(escaped){escaped=0;continue;}if(ch=='\\'&&quote!='\''){escaped=1;continue;}if((ch=='\''||ch=='"')&&(!quote||quote==ch))quote=quote?0:ch;
    }return status;
}
static void add_history(void){if(!length)return;if(history_count&&streq(history[(history_count-1)%32],line))return;strcopy(history[history_count%32],line);history_count++;}
static void history_move(int direction){u32 count=history_count>32?32:history_count;if(!count)return;if(history_position<0){if(direction<0)return;strcopy(draft,line);history_position=0;}else{history_position+=direction;if(history_position<0){strcopy(line,draft);history_position=-1;length=position=strlen(line);repaint();return;}if((u32)history_position>=count)history_position=(int)count-1;}strcopy(line,history[(history_count-1-(u32)history_position)%32]);length=position=strlen(line);repaint();}
static void insert(const char *s){u32 n=strlen(s);if(length+n>=LINE_SIZE)return;for(u32 i=length+1;i>position;i--)line[i+n-1]=line[i-1];memcpy(line+position,s,n);position+=n;length+=n;}
static void complete(void){
    u32 start=position;while(start&&line[start-1]!=' ')start--;char prefix[VFS_PATH];u32 n=position-start;if(n>=sizeof(prefix))return;memcpy(prefix,line+start,n);prefix[n]=0;
    char candidates[VFS_NODES][VFS_NAME+1];u32 count=0;
    if(start==0){for(int i=0;commands[i]&&count<VFS_NODES;i++)if(starts(commands[i],prefix))strcopy(candidates[count++],commands[i]);}
    else{u32 slash=n;while(slash&&prefix[slash-1]!='/')slash--;char parent[VFS_PATH];if(slash){memcpy(parent,prefix,slash);parent[slash]=0;}else strcopy(parent,".");int node=vfs_lookup(parent);if(node>=0&&nodes[node].dir)for(int i=1;i<VFS_NODES&&count<VFS_NODES;i++)if(nodes[i].used&&nodes[i].parent==node&&starts(nodes[i].name,prefix+slash)){strcopy(candidates[count],nodes[i].name);if(nodes[i].dir){u32 len=strlen(candidates[count]);candidates[count][len]='/';candidates[count][len+1]=0;}count++;}start+=slash;n-=slash;}
    if(!count)return;u32 common=strlen(candidates[0]);for(u32 i=1;i<count;i++){u32 j=0;while(j<common&&candidates[i][j]==candidates[0][j])j++;common=j;}
    while(common&&((u8)candidates[0][common]&0xc0)==0x80)common--;if(common>n){char suffix[VFS_NAME+2];u32 j=0;for(u32 i=n;i<common;i++)suffix[j++]=candidates[0][i];suffix[j]=0;insert(suffix);if(count==1&&common&&candidates[0][common-1]!='/')insert(" ");repaint();}
    else if(count>1){con_put('\n');for(u32 i=0;i<count;i++){con_write(candidates[i]);con_write("  ");}con_put('\n');prompt();repaint();}
}
static void key_event(int key){
    if(key==13){line[length]=0;con_edit(anchor,line,length,length,painted);debug(line);debug("\n");con_put('\n');add_history();execute(line,0,0);length=position=painted=0;line[0]=0;history_position=-1;prompt();repaint();return;}
    if(key==3){con_edit(anchor,line,length,length,painted);con_write("^C\n");length=position=painted=0;line[0]=0;history_position=-1;last_status=130;prompt();repaint();return;}
    if(key==12){con_clear();prompt();repaint();return;}if(key==9){complete();return;}
    if(key==0x149){con_scroll(25);return;}if(key==0x151){con_scroll(-25);return;}
    if(key==0x148){history_move(1);return;}if(key==0x150){history_move(-1);return;}
    if(key==0x14b){if(position)position=(u32)utf_prev(line,(int)position);}else if(key==0x14d){if(position<length){int next=(int)position;utf_next(line,&next);position=(u32)next;}}else if(key==0x147)position=0;else if(key==0x14f)position=length;
    else if(key==8&&position){u32 previous=(u32)utf_prev(line,(int)position),size=position-previous;for(u32 i=position;i<=length;i++)line[i-size]=line[i];position=previous;length-=size;}
    else if(key==0x153&&position<length){int next=(int)position;utf_next(line,&next);u32 size=(u32)next-position;for(u32 i=position;i<=length-size;i++)line[i]=line[i+size];length-=size;}
    else if(key==27){length=position=0;line[0]=0;}
    else if(key>=32&&key<=126){char s[2]={(char)key,0};insert(s);}else return;repaint();
}
static void welcome(void){
    con_put('\n');
    for(u32 row=0;row<BANNER_ROWS;row++){
        con_write("             ");
        con_color(11);con_write(banner[row][0]);
        con_write("    ");
        con_color(13);con_write(banner[row][1]);con_put('\n');
    }
    con_color(15);con_write("\n               SINO 0.10  |  SINO SH  |  80 x 26 graphical terminal\n");
    con_color(8);con_write("        ----------------------------------------------------------------\n\n");
    con_color(7);con_write("32-bit SINO kernel with a Linux-style command line.\n");
    con_write("Type ");con_color(10);con_write("help");con_color(7);con_write(" for commands, ");con_color(10);con_write("about");con_color(7);con_write(" for source information.\n");
    con_color(14);con_write("/boot is read-only. Home files persist on the data disk; /tmp is temporary.\n\n");
    con_color(7);
}
void shell_init(void){con_init();vfs_init();previous_cwd=cwd;welcome();prompt();con_flush();debug("SINO SH initialized in the graphical terminal\n");}
void shell_step(void){mouse_dirty=0;int key;while((key=key_read())!=0)key_event(key);}
void shell_key(int key){key_event(key);}
void shell_text(const char *text){insert(text);repaint();}
#ifndef SELFTEST
void shell_tests(void){}
#else
static u8 test_output[PIPE_SIZE+1];
static void check(int ok,const char *label){if(!ok)panic(label,(u32)last_status,free_pages);}
static void checkpoint(void){con_flush();}
static void test_command(const char *command_line,const char *expected,int status){Sink out={test_output,0,PIPE_SIZE,0};test_output[0]=0;debug("SHELL TEST: ");debug(command_line);debug("\n");int actual=execute(command_line,&out,0);if(actual!=status||out.overflow||(expected&&!contains((const char *)test_output,out.length,expected))){debug("TEST OUTPUT: ");debug((const char *)test_output);debug("\n");panic("Shell command self-check",(u32)actual,(u32)status);}}
static void file_is(const char *path,const char *expected){int n=vfs_lookup(path);check(n>=0,"Test file exists");int bytes=vfs_read(n,work,MAX_FILE);check(bytes>=0,"Test file read");work[bytes]=0;check(streq((const char *)work,expected),"Test file contents");}
static void key_send(u8 scan){u32 before=keyboard_irqs;ps2_byte(0,scan);for(u32 n=0;keyboard_irqs==before&&n<100000;n++){}check(keyboard_irqs>before,"Keyboard IRQ1 delivery");shell_step();}
static void type_text(const char *s){static const u8 codes[26]={30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};while(*s){char ch=*s++;if(ch>='a'&&ch<='z')key_send(codes[ch-'a']);else if(ch==' ')key_send(57);else panic("Unsupported test key",(u8)ch,0);}}
static void extended_key(u8 scan){key_send(0xe0);key_send(scan);}
static void test_demo(const char *s){prompt();con_write(s);con_put('\n');execute(s,0,0);}
void shell_tests(void){
    check(*(volatile u16 *)0x4f04==0x144&&*(volatile u8 *)0x4f06==16,"VBE graphics and 8x16 font");checkpoint();
    test_command("pwd","/home/user",0);test_command("mkdir -p projects/demo",0,0);test_command("cd projects/demo; echo $PWD","/home/user/projects/demo",0);
    test_command("cd ~; cd -; pwd","/home/user/projects/demo",0);test_command("touch .hidden; ls -la",".hidden",0);
    test_command("echo \"alpha beta\" > notes.txt",0,0);test_command("echo gamma >> notes.txt",0,0);file_is("notes.txt","alpha beta\ngamma\n");
    test_command("cp notes.txt copy.txt; mv copy.txt finished.txt",0,0);file_is("finished.txt","alpha beta\ngamma\n");
    test_command("cat notes.txt | grep beta > matches.txt",0,0);file_is("matches.txt","alpha beta\n");
    test_command("head -n 1 notes.txt","alpha beta\n",0);test_command("tail -n 1 notes.txt","gamma\n",0);test_command("cat < notes.txt | wc -l","2\n",0);
    test_command("echo one two | wc","1 2 8\n",0);test_command("echo \"hello world\" > \"two words.txt\"",0,0);file_is("two words.txt","hello world\n");
    test_command("echo -e \"one\\ntwo\" > escaped.txt",0,0);file_is("escaped.txt","one\ntwo\n");
    test_command("echo '$HOME'","$HOME",0);test_command("echo $HOME","/home/user",0);test_command("cat missing.txt","no such file",1);test_command("echo $?","1\n",0);
    test_command("echo hacked > /boot/readme.txt","read-only",1);test_command("mkdir tree; rmdir tree",0,0);test_command("mkdir -p tree/child; rmdir tree","not empty",1);test_command("mv tree tree/child","itself",1);
    test_command("echo 'echo script-ok > scripted.txt' > job.sh; source job.sh",0,0);file_is("scripted.txt","script-ok\n");
    test_command("echo 'source loop.sh' > loop.sh; source loop.sh","nesting limit",1);
    test_command("echo \"unterminated", "unclosed quote",2);test_command("echo literal # comment; touch should-not-exist", "literal",0);check(vfs_lookup("should-not-exist")<0,"Comment suppresses commands");
    int large=vfs_create("large.bin",0,0);check(large>=0,"Large RAM file create");for(u32 i=0;i<MAX_FILE;i++)work[i]=(u8)i;u32 before=free_pages;check(vfs_write(large,work,MAX_FILE,0)==0&&free_pages+16==before,"RAM file page allocation");
    check(vfs_write(large,work,1,1)<0&&nodes[large].size==MAX_FILE,"RAM file size limit");test_command("cat large.bin | wc","8192-byte",1);check(vfs_remove("large.bin",0)==0&&free_pages==before,"RAM file page reclamation");
    test_command("stat notes.txt","RAM (temporary)",0);test_command("grep -n gamma notes.txt","2:gamma",0);test_command("hexdump matches.txt","61 6c 70 68 61",0);
    before=free_pages;test_command("run /boot/hello.f32","Returned safely to SINO shell.",0);check(free_pages==before,"Native app page reclamation");
    test_command("source /boot/demo.sh","hello from SINO SH",0);test_command("credits","Not a FreeDOS or Linux kernel fork",0);test_command("date","2026-",0);
    debug("SINO SH FILES PARSER PIPELINES SCRIPTS TEST PASS\n");
    execute("cd /home/user",0,0);con_clear();prompt();length=position=painted=0;line[0]=0;
    type_text("echo axc");extended_key(0x4b);key_send(14);type_text("b");extended_key(0x4f);key_send(28);check(history_count&&streq(history[(history_count-1)%32],"echo abc"),"Inline keyboard editing");
    extended_key(0x48);check(streq(line,"echo abc"),"History Up");extended_key(0x50);check(length==0,"History Down draft");
    type_text("unam");key_send(15);check(streq(line,"uname "),"Tab command completion");key_send(28);
    type_text("echo cancelled");key_send(29);key_send(46);key_send(157);check(length==0&&last_status==130,"Ctrl C cancels input");
    type_text("pwd");key_send(29);key_send(38);key_send(157);check(streq(line,"pwd"),"Ctrl L retains input");key_send(28);
    for(u32 i=0;i<75;i++){con_write("scrollback test line ");char n[11];number(i,n);con_write(n);con_put('\n');}extended_key(0x49);check(con_scrolled()>0,"Page Up scrollback");checkpoint();extended_key(0x51);check(con_scrolled()==0,"Page Down restores view");
    debug("SINO SH KEYBOARD HISTORY COMPLETION SCROLL TEST PASS\n");
    con_clear();welcome();
    test_demo("uname -a");test_demo("pwd");test_demo("ls -l /boot");test_demo("echo hello > /tmp/hello.txt; cat /tmp/hello.txt");test_demo("cat /tmp/hello.txt | wc");test_demo("mem");test_demo("about");
    con_write("\nType help for commands. Home files persist on the data disk. Use sync before shutdown.\n");length=position=painted=0;line[0]=0;prompt();con_flush();
    debug("SINO 0.10 SHELL HARDWARE TEST PASS\n");checkpoint();
}
#endif
