#include "keyboard.h"
static unsigned shift,control,alt,locks_down;
static int extended,pause_bytes,caps,num=1;
static const char plain[128]={
 [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',
 [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',
 [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]='\'',[41]='`',[43]='\\',
 [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[55]='*',[57]=' ',[74]='-',[78]='+',[86]='\\'
};
int keyboard_decode(unsigned char scan){
 if(pause_bytes){pause_bytes--;return 0;}if(scan==0xe1){pause_bytes=5;extended=0;return 0;}if(scan==0xe0){extended=1;return 0;}
 int ext=extended,up=scan&128;unsigned code=scan&127,bit=ext?2u:1u;extended=0;
 /* Print Screen's E0 2A/E0 AA are fake shifts. They must never release a held Shift. */
 if((code==42||code==54)&&!ext){bit=code==42?1u:2u;if(up)shift&=~bit;else shift|=bit;return 0;}
 if(code==29){if(up)control&=~bit;else control|=bit;return 0;}if(code==56){if(up)alt&=~bit;else alt|=bit;return 0;}
 if(ext){if(up)return 0;if(code==28)return 13;if(code==53)return '/';if(code>=71&&code<=83)return 0x100|(int)code;return 0;}
 if(code==58||code==69){bit=code==58?1u:2u;if(up)locks_down&=~bit;else if(!(locks_down&bit)){locks_down|=bit;if(code==58)caps=!caps;else num=!num;}return 0;}
 if(up)return 0;
 if(code==1)return 27;if(code==28)return 13;if(code==14)return 8;if(code==15)return alt?0x20f:9;
 if((code>=59&&code<=68)||code==87||code==88)return 0x100|(int)code;
 if(code>=71&&code<=83&&code!=74&&code!=78){static const char keypad[]="789-456+1230.";if(num^!!shift)return keypad[code-71];return 0x100|(int)code;}
 char c=plain[code];if(control&&c==' ')return 0x220;if(control&&c>='a'&&c<='z')return c-'a'+1;
 if(c>='a'&&c<='z'&&(!!shift^caps))c-=32;
 if(shift&&code!=55&&code!=74&&code!=78){static const char from[]="1234567890-=[];'`,./\\",to[]="!@#$%^&*()_+{}:\"~<>?|";for(unsigned i=0;from[i];i++)if(c==from[i]){c=to[i];break;}}
 return (unsigned char)c;
}
