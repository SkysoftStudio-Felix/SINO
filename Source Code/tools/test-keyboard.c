#include <assert.h>
#include <stdio.h>
#include "../src32/keyboard.h"
static int key(unsigned char scan){int n=keyboard_decode(scan);keyboard_decode(scan|128);return n;}
static int ext(unsigned char scan){keyboard_decode(0xe0);int n=keyboard_decode(scan);keyboard_decode(0xe0);keyboard_decode(scan|128);return n;}
int main(void){
 const unsigned char codes[]={2,3,4,5,6,7,8,9,10,11,12,13,26,27,39,40,41,51,52,53,43};
 const char *plain="1234567890-=[];'`,./\\",*upper="!@#$%^&*()_+{}:\"~<>?|";
 for(unsigned i=0;i<sizeof(codes);i++)assert(key(codes[i])==plain[i]);
 keyboard_decode(42);for(unsigned i=0;i<sizeof(codes);i++)assert(key(codes[i])==upper[i]);
 keyboard_decode(54);keyboard_decode(170);assert(key(12)=='_');keyboard_decode(182);assert(key(12)=='-');
 keyboard_decode(42);keyboard_decode(0xe0);keyboard_decode(42);keyboard_decode(0xe0);keyboard_decode(170);assert(key(12)=='_');keyboard_decode(170);
 assert(key(58)==0);assert(key(30)=='A');keyboard_decode(42);assert(key(30)=='a');assert(key(12)=='_');keyboard_decode(170);key(58);assert(key(30)=='a');
 keyboard_decode(29);keyboard_decode(0xe0);keyboard_decode(29);keyboard_decode(157);assert(key(31)==19);keyboard_decode(0xe0);keyboard_decode(157);assert(key(31)=='s');
 assert(ext(28)==13);assert(ext(53)=='/');assert(ext(75)==0x14b);assert(ext(83)==0x153);
 assert(key(71)=='7');assert(key(82)=='0');assert(key(83)=='.');assert(key(74)=='-');assert(key(78)=='+');assert(key(55)=='*');
 keyboard_decode(54);assert(key(74)=='-');assert(key(78)=='+');assert(key(55)=='*');assert(key(71)==0x147);keyboard_decode(182);
 key(69);assert(key(71)==0x147);key(69);assert(key(71)=='7');
 unsigned char pause[]={0xe1,0x1d,0x45,0xe1,0x9d,0xc5};for(unsigned i=0;i<sizeof(pause);i++)assert(keyboard_decode(pause[i])==0);assert(key(30)=='a');
 puts("PASS keyboard: US symbols, held dual modifiers, fake shifts, locks, keypad, extended Enter/navigation, Pause isolation");return 0;
}
