/* ATA PIO user volume. Two checksummed snapshots; commit header is written last. */
#include "storage.h"
#include "vfs.h"
#include "disk.h"
#define BANK_SECTORS 8192u
#define BANK_START(b) (volume_base+8u+(u32)(b)*BANK_SECTORS)
#define STORE_MAGIC 0x31534653u
int storage_ready,storage_dirty;
u32 storage_generation;
const char *storage_error="No data disk attached";
static int current_bank=-1,loading;
static u32 changed_at,sectors,crc_table[256];
static u8 io_sector[512],file_buffer[MAX_FILE];
int storage_disk=-1;u32 storage_partition;
static u32 volume_base;
static int ata_read(u32 lba,u8 *dest){return disk_read(storage_disk,lba,dest);}
static int ata_write(u32 lba,const u8 *source){return disk_write(storage_disk,lba,source);}
static int ata_flush(void){return disk_flush(storage_disk);}
static u32 crc_step(u32 crc,const u8 *p,u32 length){while(length--)crc=(crc>>8)^crc_table[(crc^*p++)&255];return crc;}
static int identify(void){u8 mbr[512];int first=*(volatile u8 *)0x4f0c>=0x80?(int)*(volatile u8 *)0x4f0c-0x80:0;if(*(volatile u8 *)0x4f0c>=0x80){u32 signature=*(volatile u32 *)0x4f10;for(int d=0;d<4;d++)if(disks[d].present&&disk_read(d,0,mbr)&&mbr[510]==0x55&&mbr[511]==0xaa&&*(u32 *)(mbr+440)==signature){first=d;break;}}for(int n=0;n<4;n++){int d=(first+n)%4;if(!disks[d].present||disks[d].sectors<8+2*BANK_SECTORS||!disk_read(d,0,mbr))continue;int blank=1;for(int i=0;i<512;i++)if(mbr[i])blank=0;if(mbr[0]=='S'&&mbr[1]=='I'&&mbr[2]=='N'&&mbr[3]=='O'){storage_disk=d;volume_base=0;sectors=disks[d].sectors;return 1;}if(mbr[510]==0x55&&mbr[511]==0xaa){for(int i=0;i<4;i++){u8 *p=mbr+446+i*16;u32 start=*(u32 *)(p+8),length=*(u32 *)(p+12);if(p[4]==0x7f&&start>=2048&&length>=8+2*BANK_SECTORS&&start<disks[d].sectors&&length<=disks[d].sectors-start){storage_disk=d;volume_base=storage_partition=start;sectors=disks[d].sectors;return 1;}}}else if(blank&&disks[d].sectors<=65536){storage_disk=d;volume_base=0;sectors=disks[d].sectors;return 1;}}return 0;}
typedef struct {u32 generation,bytes,count,crc;int bank,valid;} Snapshot;
static Snapshot inspect(int bank){
    Snapshot s={0,0,0,0,bank,0};if(!ata_read(BANK_START(bank),io_sector))return s;u32 *header=(u32 *)io_sector;
    if(header[0]!=STORE_MAGIC||header[1]!=1||header[6]!=0x434f4d54||header[3]>(BANK_SECTORS-1)*512||header[4]>VFS_NODES)return s;
    s.generation=header[2];s.bytes=header[3];s.count=header[4];s.crc=header[5];u32 left=s.bytes,crc=0xffffffff,lba=BANK_START(bank)+1;
    while(left){if(!ata_read(lba++,io_sector))return s;u32 n=left>512?512:left;crc=crc_step(crc,io_sector,n);left-=n;}s.valid=(crc^0xffffffff)==s.crc;return s;
}
typedef struct {u32 lba,offset,left;} Reader;
static int read_bytes(Reader *r,u8 *dest,u32 length){
    if(length>r->left)return 0;while(length){if(r->offset==512){r->offset=0;if(!ata_read(r->lba++,io_sector))return 0;}u32 n=512-r->offset;if(n>length)n=length;memcpy(dest,io_sector+r->offset,n);dest+=n;r->offset+=n;r->left-=n;length-=n;}return 1;
}
static int user_node(int index){int user=vfs_lookup("/home/user");if(index==user)return 0;for(int n=0;index>0&&n<VFS_NODES;n++,index=nodes[index].parent)if(nodes[index].parent==user)return 1;return 0;}
static int restore(Snapshot s){
    Reader r={BANK_START(s.bank)+1,512,s.bytes};u8 record[160];
    /* Validate the stream before changing any live nodes. */
    for(u32 i=0;i<s.count;i++){if(!read_bytes(&r,record,160)||!record[0]||record[127]!=0)return 0;const char *path=(const char *)record;char canonical[VFS_PATH];if(strlen(path)<11||vfs_canonical(path,canonical)<0||!streq(path,canonical))return 0;const char *prefix="/home/user/";for(u32 j=0;j<11;j++)if(path[j]!=prefix[j])return 0;u32 size=*(u32 *)(record+132);if(size>MAX_FILE||*(u32 *)(record+128)>1||!read_bytes(&r,file_buffer,size))return 0;}if(r.left)return 0;
    for(int i=1;i<VFS_NODES;i++)if(nodes[i].used&&user_node(i)){for(u32 p=0;p<VFS_PAGES;p++)if(nodes[i].pages[p])page_free(nodes[i].pages[p]);nodes[i].used=0;}
    r=(Reader){BANK_START(s.bank)+1,512,s.bytes};
    for(u32 i=0;i<s.count;i++){
        if(!read_bytes(&r,record,160))return 0;char *path=(char *)record;u32 size=*(u32 *)(record+132);int dir=*(u32 *)(record+128)!=0;
        char parent[VFS_PATH];memcpy(parent,path,VFS_PATH);u32 n=strlen(parent);while(n&&parent[n-1]!='/')n--;if(n>1)parent[n-1]=0;int folder=vfs_create(parent,1,1);if(folder<0)return 0;
        int index=vfs_create(path,dir,dir);if(index<0||!read_bytes(&r,file_buffer,size)||(!dir&&vfs_write(index,file_buffer,size,0)<0))return 0;
    }
    return 1;
}
typedef struct {u32 lba,used,bytes,crc;int ok;u8 data[512];} Writer;
static void write_bytes(Writer *w,const u8 *source,u32 length){
    w->crc=crc_step(w->crc,source,length);w->bytes+=length;
    while(length&&w->ok){u32 n=512-w->used;if(n>length)n=length;memcpy(w->data+w->used,source,n);w->used+=n;source+=n;length-=n;if(w->used==512){w->ok=ata_write(w->lba++,w->data);w->used=0;}}
}
void storage_changed(void){if(storage_ready&&!loading){storage_dirty=1;changed_at=ticks;}}
int storage_sync(void){
    if(!storage_ready)return 0;if(!storage_dirty)return 1;int bank=current_bank==0?1:0;u32 count=0,total=0;
    for(int i=1;i<VFS_NODES;i++)if(nodes[i].used&&user_node(i)){count++;total+=160+(nodes[i].dir?0:nodes[i].size);}if(total>(BANK_SECTORS-1)*512){storage_error="Data volume is full";return 0;}
    memset(io_sector,0,512);if(!ata_write(BANK_START(bank),io_sector)){storage_error="Data disk write failed";return 0;}
    Writer w={BANK_START(bank)+1,0,0,0xffffffff,1,{0}};
    for(int i=1;i<VFS_NODES&&w.ok;i++)if(nodes[i].used&&user_node(i)){
        u8 record[160];memset(record,0,sizeof(record));vfs_path(i,(char *)record);*(u32 *)(record+128)=(u32)nodes[i].dir;*(u32 *)(record+132)=nodes[i].dir?0:nodes[i].size;write_bytes(&w,record,160);
        if(!nodes[i].dir){int n=vfs_read(i,file_buffer,MAX_FILE);if(n<0){w.ok=0;break;}write_bytes(&w,file_buffer,(u32)n);}
    }
    if(w.used&&w.ok){memset(w.data+w.used,0,512-w.used);w.ok=ata_write(w.lba,w.data);}if(!w.ok||!ata_flush()){storage_error="Data disk payload failed";return 0;}
    memset(io_sector,0,512);u32 *header=(u32 *)io_sector;header[0]=STORE_MAGIC;header[1]=1;header[2]=storage_generation+1;header[3]=w.bytes;header[4]=count;header[5]=w.crc^0xffffffff;header[6]=0x434f4d54;
    if(!ata_write(BANK_START(bank),io_sector)||!ata_flush()){storage_error="Data disk commit failed";return 0;}storage_generation++;current_bank=bank;storage_dirty=0;storage_error="";debug("STORAGE committed generation ");char b[11];number(storage_generation,b);debug(b);debug("\n");return 1;
}
void storage_poll(void){if(storage_ready&&storage_dirty&&ticks-changed_at>=20){if(!storage_sync())changed_at=ticks;}}
void storage_init(void){
    for(u32 i=0;i<256;i++){u32 c=i;for(int j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320:0);crc_table[i]=c;}
    if(!identify()){debug("STORAGE no writable ATA data volume; RAM mode\n");return;}
    if(!ata_read(volume_base,io_sector))return;const char magic[8]={'S','I','N','O','D','S','K','1'};int known=1,blank=1;for(int i=0;i<8;i++)if(io_sector[i]!=(u8)magic[i])known=0;for(int i=0;i<512;i++)if(io_sector[i])blank=0;
    if(!known&&!blank){storage_error="Unrecognized data disk; writes disabled";return;}
    Snapshot a=inspect(0),b=inspect(1);if(a.valid||b.valid){Snapshot first=a.valid&&(!b.valid||a.generation>=b.generation)?a:b,second=first.bank==0?b:a;loading=1;int ok=restore(first);if(!ok&&second.valid){first=second;ok=restore(first);}loading=0;if(!ok){storage_error="Data snapshot restore failed";return;}storage_generation=first.generation;current_bank=first.bank;storage_ready=1;storage_dirty=0;storage_error="";debug("STORAGE restored generation ");char number_text[11];number(storage_generation,number_text);debug(number_text);debug("\n");}
    else if(blank){memset(io_sector,0,512);memcpy(io_sector,magic,8);if(!ata_write(volume_base,io_sector)||!ata_flush())return;storage_ready=1;storage_error="";storage_dirty=1;(void)storage_sync();debug("STORAGE initialized SINO data volume\n");}
    else{storage_error="No valid data snapshot; writes disabled";debug("STORAGE recovery required\n");}
}
