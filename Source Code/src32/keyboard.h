#ifndef SINO_KEYBOARD_H
#define SINO_KEYBOARD_H
/* PS/2 set 1, US physical layout. Zero means a modifier/release, not text. */
int keyboard_decode(unsigned char scan);
#endif
