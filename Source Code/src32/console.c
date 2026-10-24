#include "console.h"
#include "gui.h"
#include "graphics.h"
#define SCROLL_LINES 512u
static u32 cells[SCROLL_LINES][CON_COLS];
static u32 utf_pending,edit_anchor=0xffffffffu,edit_span;static int utf_need;
static u32 line,column,scrollback,cursor_override;
static u8 color=7;
static void blank(u32 n){for(u32 i=0;i<CON_COLS;i++)cells[n%SCROLL_LINES][i]=0x07000020;}
static void cursor(int pos){
    outb(0x3d4,0x0a);u8 v=inb(0x3d5);outb(0x3d5,pos<0?(v|0x20):(v&~0x20));
    if(pos>=0){outb(0x3d4,0x0e);outb(0x3d5,(u8)(pos>>8));outb(0x3d4,0x0f);outb(0x3d5,(u8)pos);}
}
void con_init(void){color=7;con_clear();outb(0x3d4,0x0a);outb(0x3d5,6);outb(0x3d4,0x0b);outb(0x3d5,7);}
void con_clear(void){line=column=scrollback=utf_pending=0;utf_need=0;edit_anchor=cursor_override=0xffffffffu;edit_span=0;for(u32 i=0;i<SCROLL_LINES;i++)blank(i);con_flush();}
void con_color(u8 value){color=value;}
static void put_codepoint(u32 c){
    cursor_override=0xffffffffu;scrollback=0;
    if(c=='\r'){column=0;return;}
    if(c=='\n'){line++;column=0;blank(line);return;}
    if(c=='\t'){do{con_put(' ');}while(column&3);return;}
    if(c=='\b'){if(column)cells[line%SCROLL_LINES][--column]=((u32)color<<24)|' ';return;}
    if(c<32)c='?';int width=c>=128?3:1;if(column+(u32)width>CON_COLS){line++;column=0;blank(line);}cells[line%SCROLL_LINES][column++]=((u32)color<<24)|c;for(int i=1;i<width;i++)cells[line%SCROLL_LINES][column++]=((u32)color<<24)|0x110000;
    if(column==CON_COLS){column=0;line++;blank(line);}
}
void con_put(char c){u8 b=(u8)c;if(utf_need){if((b&0xc0)==0x80){utf_pending=(utf_pending<<6)|(b&63);if(!--utf_need){if(utf_pending<128||utf_pending>0x10ffff||(utf_pending>=0xd800&&utf_pending<=0xdfff))utf_pending='?';put_codepoint(utf_pending);}return;}utf_need=0;put_codepoint('?');}if(b<128){put_codepoint(b);return;}if(b>=0xc2&&b<=0xdf){utf_pending=b&31;utf_need=1;}else if(b>=0xe0&&b<=0xef){utf_pending=b&15;utf_need=2;}else if(b>=0xf0&&b<=0xf4){utf_pending=b&7;utf_need=3;}else put_codepoint('?');}
void con_write(const char *s){debug(s);while(*s)con_put(*s++);}
u32 con_position(void){return line*CON_COLS+column;}
void con_edit(u32 anchor,const char *s,u32 length,u32 pos,u32 old_length){
    (void)old_length;u32 end=anchor,caret=anchor;int i=0;while(i<(int)length){if(i==(int)pos)caret=end;u32 c=utf_next(s,&i),width=c>=128?3:1;if(end%CON_COLS+width>CON_COLS)end+=CON_COLS-end%CON_COLS;end+=width;}if(pos==length)caret=end;
    u32 last=end/CON_COLS;for(u32 row=line+1;row<=last;row++)blank(row);u32 span=end-anchor,count=edit_anchor==anchor&&edit_span>span?edit_span:span;for(u32 j=0;j<count;j++){u32 at=anchor+j;cells[(at/CON_COLS)%SCROLL_LINES][at%CON_COLS]=((u32)color<<24)|' ';}
    u32 at=anchor;i=0;while(i<(int)length){u32 c=utf_next(s,&i),width=c>=128?3:1;if(at%CON_COLS+width>CON_COLS)at+=CON_COLS-at%CON_COLS;cells[(at/CON_COLS)%SCROLL_LINES][at%CON_COLS]=((u32)color<<24)|c;at++;for(u32 j=1;j<width;j++,at++)cells[(at/CON_COLS)%SCROLL_LINES][at%CON_COLS]=((u32)color<<24)|0x110000;}
    edit_anchor=anchor;edit_span=span;line=end/CON_COLS;column=end%CON_COLS;cursor_override=caret;scrollback=0;
}
void con_flush(void){
    if(gui_active){gui_console_changed();return;}
    u32 latest=line>=CON_ROWS?line-CON_ROWS+1:0,earliest=line>=SCROLL_LINES?line-SCROLL_LINES+1:0;
    u32 available=latest>earliest?latest-earliest:0;if(scrollback>available)scrollback=available;
    u32 start=latest-scrollback;volatile u16 *v=(volatile u16 *)0xb8000;
    for(u32 y=0;y<CON_ROWS;y++)for(u32 x=0;x<CON_COLS;x++){u32 c=start+y<=line?cells[(start+y)%SCROLL_LINES][x]:0x07000020;v[y*CON_COLS+x]=(u16)(((c>>24)<<8)|((c&0x1fffff)<128?(c&127):'?'));}
    u32 p=cursor_override==0xffffffffu?con_position():cursor_override;
    cursor(scrollback||p<start*CON_COLS||p>=(start+CON_ROWS)*CON_COLS?-1:(int)(p-start*CON_COLS));
}
void con_snapshot(u32 *dest,u32 *position){
    u32 latest=line>=CON_ROWS?line-CON_ROWS+1:0,earliest=line>=SCROLL_LINES?line-SCROLL_LINES+1:0;
    u32 available=latest>earliest?latest-earliest:0;if(scrollback>available)scrollback=available;
    u32 start=latest-scrollback;
    for(u32 y=0;y<CON_ROWS;y++)for(u32 x=0;x<CON_COLS;x++)dest[y*CON_COLS+x]=start+y<=line?cells[(start+y)%SCROLL_LINES][x]:0x07000020;
    u32 p=cursor_override==0xffffffffu?con_position():cursor_override;
    *position=scrollback||p<start*CON_COLS||p>=(start+CON_ROWS)*CON_COLS?0xffffffffu:p-start*CON_COLS;
}
void con_scroll(int n){u32 latest=line>=CON_ROWS?line-CON_ROWS+1:0,earliest=line>=SCROLL_LINES?line-SCROLL_LINES+1:0;int next=(int)scrollback+n;if(next<0)next=0;if((u32)next>latest-earliest)next=(int)(latest-earliest);scrollback=(u32)next;con_flush();}
u32 con_scrolled(void){return scrollback;}
void console_panic(const char *s,u32 vector,u32 error){
    if(gui_active){gui_panic(s,vector,error);return;}
    con_clear();con_color(0x4f);con_write("SINO kernel panic\n");con_write(s);con_put('\n');char b[11];number(vector,b);con_write("Vector: ");con_write(b);number(error,b);con_write("  Error: ");con_write(b);con_write("\nCPU halted; reset the virtual machine.\n");con_flush();
}
