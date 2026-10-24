#ifndef SINO_VFS_H
#define SINO_VFS_H
#include "kernel.h"
#define VFS_NODES 128
#define VFS_PATH 128
#define VFS_NAME 32
#define VFS_PAGES 16
typedef struct {char name[VFS_NAME];int used,parent,dir,readonly,pinned,boot;u32 size;u8 *pages[VFS_PAGES];} Node;
extern Node nodes[VFS_NODES];
extern u32 vfs_generation[VFS_NODES];
extern int cwd;
extern const char *vfs_error;
void vfs_init(void);
int vfs_lookup(const char *);
int vfs_canonical(const char *,char *);
void vfs_path(int,char *);
int vfs_create(const char *,int,int);
int vfs_read(int,u8 *,u32);
int vfs_write(int,const u8 *,u32,int);
int vfs_remove(const char *,int);
int vfs_move(const char *,const char *);
int vfs_copy(const char *,const char *,u8 *);
int vfs_child(int,const char *);
#endif
