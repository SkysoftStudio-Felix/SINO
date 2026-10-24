#ifndef SINO_STORAGE_H
#define SINO_STORAGE_H
#include "kernel.h"
extern int storage_ready,storage_dirty;
extern int storage_disk;
extern u32 storage_partition;
extern u32 storage_generation;
extern const char *storage_error;
void storage_init(void);
void storage_changed(void);
void storage_poll(void);
int storage_sync(void);
#endif
