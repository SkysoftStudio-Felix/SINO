#ifndef SINO_API_H
#define SINO_API_H
/* Native ABI 1. Values are shared with separately linked ring-3 programs. */
#include "types.h"
#define MAX_FILE 65536u
enum {S_OK=0,S_INVALID=-1,S_NOMEM=-2,S_DENIED=-3,S_NOTFOUND=-4,S_BUSY=-5,S_AGAIN=-6,S_IO=-7,S_LIMIT=-8,S_BADHANDLE=-9,S_EXISTS=-10};
enum {SYS_EXIT=0x100,SYS_YIELD,SYS_LOG,SYS_TICKS,SYS_WINDOW,SYS_SCENE,SYS_EVENT,SYS_CLOSE,SYS_READ,SYS_WRITE,SYS_MKDIR,SYS_REMOVE,SYS_LIST,SYS_SPAWN,SYS_KILL,SYS_PROCESSES,SYS_SYNC,SYS_THREAD,SYS_THREAD_EXIT,SYS_EVENT_CREATE,SYS_EVENT_SET,SYS_EVENT_WAIT,SYS_LEASE,SYS_SOCKET,SYS_BIND,SYS_LISTEN,SYS_CONNECT,SYS_ACCEPT,SYS_SEND,SYS_RECV,SYS_NET_READY,SYS_NET_INFO,SYS_PING,SYS_SENDTO,SYS_RECVFROM,SYS_MOVE,SYS_CLIP_GET,SYS_CLIP_SET,SYS_WINDOW_POLICY,SYS_DUP};
enum {CAP_HOME=1,CAP_NETWORK=2,CAP_MANAGE=4};
enum {UI_FILL=1,UI_TEXT,UI_BUTTON,UI_LINE,UI_PANEL,UI_GRID,UI_SELECTION};
enum {MSG_CLICK=1,MSG_KEY,MSG_CLOSE,MSG_MOUSE_MOVE,MSG_MOUSE_UP,MSG_TEXT,MSG_CONTEXT};
#define UI_COMMANDS 64
#define UI_CANVAS_COMMANDS 256
typedef struct {i32 kind,x,y,w,h;u32 color;char text[80];} UiCommand;
typedef struct {u32 count;UiCommand commands[UI_COMMANDS];} UiScene;
/* Variable-length SYS_SCENE keeps original 64-command ABI 1 clients valid. */
typedef struct {u32 count;UiCommand commands[UI_CANVAS_COMMANDS];} UiCanvas;
typedef struct {u32 type,window;i32 x,y,key;} UiEvent;
typedef struct {u32 pid,state,threads,pages;char name[32];i32 exit_code;} ProcessInfo;
typedef struct {char name[32];u32 size,dir;} FileInfo;
#define USER_BASE 0x40000000u
#define USER_BYTES 0x30000u
#define USER_ARGUMENTS (USER_BASE+0x30000u)
#define USER_STACK(t) (USER_BASE+0x40000u+(u32)(t)*0x8000u)
#define NATIVE_MAGIC 0x31584e53u /* SNX1; deliberately not PE/Win32 */
typedef struct {u32 magic,abi,image_bytes,entry,data_offset,memory_bytes,reserved[2];} NativeHeader;
#endif
