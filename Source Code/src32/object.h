#ifndef SINO_OBJECT_H
#define SINO_OBJECT_H
#include "api.h"
#define OBJECTS 64
#define HANDLES 24
enum {OBJ_EVENT=1,OBJ_WINDOW,OBJ_SOCKET};
typedef struct {u32 object,generation,rights,cookie;} Handle;
typedef struct {u32 generation,refs,type;int value;} Object;
extern Object objects[OBJECTS];
int object_new(u32,int);
int handle_open(Handle *,int,u32);
Object *handle_get(Handle *,u32,u32,u32);
int handle_duplicate(Handle *,u32,u32);
int handle_close(Handle *,u32);
void handles_close(Handle *);
void object_tests(void);
#endif
