#ifndef SINO_BASIC_IMAGE_H
#define SINO_BASIC_IMAGE_H
#include "basic.h"
/* Application data, not a new executable format or loader privilege. */
typedef struct {char magic[8];u32 enabled;char form[3072],code[BASIC_CODE];} BasicImage;
#define BASIC_IMAGE_MAGIC "SBRES01"
#endif
