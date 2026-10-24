#include "basic.h"
static Basic program;
static char output[128];
static void print(const char *s){copy(output,s);}
static BasicHost host={0,0,print,0};
static int test(const char *code,const char *expected,int error){if(BasicLoad(&program,code)<0||BasicStart(&program,"Form_Load")<0)return 0;u32 count=0;while(program.running&&count++<2000)BasicStep(&program,&host);if(program.running)return 0;if(error)return program.error[0]!=0;if(program.error[0]){Log(program.error);Log("\n");return 0;}return streq(output,expected);}
int main(void){const char *code="Sub Form_Load()\nDim total As Long\nDim a(3) As Integer\nFor i = 0 To 3\na(i) = i * 2\ntotal = total + a(i)\nNext i\nIf total = 12 Then\ntotal = total + 1\nElseIf total = 0 Then\ntotal = 100\nElse\ntotal = 200\nEnd If\nWhile total < 15\ntotal = total + 1\nWend\nDo\ntotal = total + 1\nLoop Until total = 17\nSelect Case total\nCase 17\nCall Helper()\nCase Else\ntotal = 300\nEnd Select\nPrint CStr(total) & Left(\"hello\", 2) & Mid(\"abcd\", 2, 2)\nEnd Sub\nSub Helper()\ntotal = total + 1\nEnd Sub\n";
 if(!test(code,"18hebc",0)||!test("Sub Form_Load()\nDim x As Long\nx = 2147483647 + 1\nEnd Sub\n","",1)||!test("Sub Form_Load()\nPrint 4 / 0\nEnd Sub\n","",1)||!test("Sub Form_Load()\nDim a(2) As Integer\na(3)=1\nEnd Sub\n","",1)){Log("BASIC SELFTEST FAIL\n");return 1;}Log("BASIC SELFTEST PASS: expressions arrays types loops branches calls strings errors\n");return 0;}
