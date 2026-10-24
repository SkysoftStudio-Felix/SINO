#ifndef SINO_PACKAGE_H
#define SINO_PACKAGE_H
#include "sdk.h"
#define PACKAGE_MAGIC 0x31505853u /* SXP1 */
#define PACKAGE_FILES 8
typedef struct {char path[64];u32 offset,size,crc,component;} PackageFile;
typedef struct {u32 magic,format,arch,abi,files,components,permissions,total_bytes;char id[32],name[64],version[16],entry[64],license[256];char config_key[32],config_value[128],shortcut[32],association[16],service[32];u8 signature[64];PackageFile file[PACKAGE_FILES];} Package;
typedef struct {u32 magic;char root[128],source[128];Package manifest;} AppRecord;
u32 checksum(const void *,u32);
int PackageValidate(const Package *,u32);
int InstallBegin(void);
int InstallWriteFile(const char *,const void *,u32);
int InstallSetConfig(const char *,const char *);
int InstallCreateShortcut(const char *,const char *);
int InstallRegisterUninstall(const AppRecord *);
int InstallCommit(void);
int InstallRollback(void);
int InstallRecover(void);
extern void (*install_progress)(u32,u32);
int InstallPackage(const Package *,const char *,const char *,u32,int);
int Uninstall(const char *);
int ConfigRead(const char *,char *,u32);
void InstallLog(const char *);
int safe_id(const char *);
#define APP_RECORD_MAGIC 0x31525041u
#endif
