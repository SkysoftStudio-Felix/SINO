#ifndef SINO_BASIC_H
#define SINO_BASIC_H
#include "sdk.h"
#define BASIC_CODE 8192
#define BASIC_LINES 256
#define BASIC_VARS 32
typedef struct {int string;i32 number;char text[128];} BasicValue;
typedef struct {char name[32];int type,array_size;i32 array[16];BasicValue value;} BasicVariable;
typedef struct {char name[32];int first,last;} BasicProcedure;
typedef struct {int variable,line;i32 end,step;} BasicLoop;
typedef struct {int pc,end;} BasicCall;
typedef struct {char code[BASIC_CODE];u16 line[BASIC_LINES];int lines,pc,end,running,error_line,variables,procedures,loops,calls,else_line;char error[80];BasicVariable variable[BASIC_VARS];BasicProcedure procedure[16];BasicLoop loop[8];BasicCall call[8];} Basic;
typedef struct {int (*get)(const char *,BasicValue *);int (*set)(const char *,const BasicValue *);void (*print)(const char *);int (*dialog)(const char *,char *,int);} BasicHost;
int BasicLoad(Basic *,const char *);
int BasicStart(Basic *,const char *);
int BasicStep(Basic *,const BasicHost *);
int BasicEqual(const char *,const char *);
void BasicString(BasicValue *,const char *);
void BasicText(const BasicValue *,char *);
#endif
