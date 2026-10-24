// ISO 9660 + El Torito BIOS CD, using the unchanged 1.44 MB SINO boot image.
// A real boot catalog requests floppy emulation; this is not a renamed IMG.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const input=path.resolve(process.argv[2]||path.join(__dirname,'sino.img'));
const output=path.resolve(process.argv[3]||require('./tools/paths').iso);
const boot=fs.readFileSync(input),block=2048;
if(boot.length!==1474560||boot.readUInt16LE(510)!==0xaa55)throw Error('Expected a bootable 1.44 MB SINO image');
const layout={lpath:19,mpath:20,root:21,bootdir:22,catalog:23,image:24,readme:744,sectors:768};
const iso=Buffer.alloc(layout.sectors*block);
const both16=(b,o,n)=>{b.writeUInt16LE(n,o);b.writeUInt16BE(n,o+2);};
const both32=(b,o,n)=>{b.writeUInt32LE(n,o);b.writeUInt32BE(n,o+4);};
const padded=(b,o,n,s)=>{b.fill(32,o,o+n);b.write(s,o,Math.min(n,Buffer.byteLength(s)),'ascii');};
const sector=n=>iso.subarray(n*block,(n+1)*block);
const stamp=Buffer.from([126,10,17,0,0,0,0]);
function record(lba,size,name,dir=false){if(typeof name==='number')name=Buffer.from([name]);else name=Buffer.from(name,'ascii');const b=Buffer.alloc(33+name.length+(name.length%2===0?1:0));b[0]=b.length;both32(b,2,lba);both32(b,10,size);stamp.copy(b,18);b[25]=dir?2:0;both16(b,28,1);b[32]=name.length;name.copy(b,33);return b;}
function directory(lba,records){let at=lba*block;for(const r of records){r.copy(iso,at);at+=r.length;}if(at>(lba+1)*block)throw Error('Directory exceeds one sector');}
function pathtable(big){const b=Buffer.alloc(22);b[0]=1;(big?b.writeUInt32BE.bind(b):b.writeUInt32LE.bind(b))(layout.root,2);(big?b.writeUInt16BE.bind(b):b.writeUInt16LE.bind(b))(1,6);b[10]=4;(big?b.writeUInt32BE.bind(b):b.writeUInt32LE.bind(b))(layout.bootdir,12);(big?b.writeUInt16BE.bind(b):b.writeUInt16LE.bind(b))(1,16);b.write('BOOT',18,'ascii');return b;}
const pvd=sector(16);pvd[0]=1;pvd.write('CD001',1);pvd[6]=1;padded(pvd,8,32,'SINO');padded(pvd,40,32,'SINO_DESKTOP');both32(pvd,80,layout.sectors);both16(pvd,120,1);both16(pvd,124,1);both16(pvd,128,block);both32(pvd,132,22);pvd.writeUInt32LE(layout.lpath,140);pvd.writeUInt32BE(layout.mpath,148);record(layout.root,block,0,true).copy(pvd,156);padded(pvd,190,128,'SINO_DESKTOP');padded(pvd,318,128,'SINO PROJECT');padded(pvd,446,128,'SINO ISO BUILDER');padded(pvd,574,128,'SINO 0.10 PLATINUM DESKTOP');for(const at of [813,830,864]){pvd.write('2026101800000000',at,'ascii');pvd[at+16]=0;}pvd.write('0000000000000000',847,'ascii');pvd[881]=1;
const br=sector(17);br[0]=0;br.write('CD001',1);br[6]=1;br.write('EL TORITO SPECIFICATION',7,'ascii');br.writeUInt32LE(layout.catalog,71);const end=sector(18);end[0]=255;end.write('CD001',1);end[6]=1;
pathtable(false).copy(iso,layout.lpath*block);pathtable(true).copy(iso,layout.mpath*block);
const readme=Buffer.from('SINO 0.10 Platinum Desktop\r\n\r\nBoot this ISO from a BIOS/Legacy x86 virtual CD-ROM drive.\r\n64 MB RAM, standard VGA/VBE, PS/2 keyboard and mouse.\r\nThe El Torito catalog emulates a 1.44 MB boot floppy.\r\nThe operating system and its apps run from this optical image.\r\nAn optional dedicated blank ATA disk (16 MB) persists home files.\r\nThe CD itself is read-only. This is not an EFI/UEFI image.\r\n\r\nFor the supplied Windows package, double-click the SINO CMD launcher.\r\nMounting this ISO in Windows only shows its files; it does not run the OS.\r\n','ascii');
directory(layout.root,[record(layout.root,block,0,true),record(layout.root,block,1,true),record(layout.bootdir,block,'BOOT',true),record(layout.readme,readme.length,'README.TXT;1')]);
directory(layout.bootdir,[record(layout.bootdir,block,0,true),record(layout.root,block,1,true),record(layout.catalog,block,'BOOT.CAT;1'),record(layout.image,boot.length,'SINO.IMG;1')]);
const cat=sector(layout.catalog);cat[0]=1;cat[1]=0;cat.write('SINO BIOS BOOT CATALOG',4,'ascii');cat[30]=0x55;cat[31]=0xaa;let sum=0;for(let i=0;i<32;i+=2)sum=(sum+cat.readUInt16LE(i))&65535;cat.writeUInt16LE((-sum)&65535,28);cat[32]=0x88;cat[33]=2;cat.writeUInt16LE(0,34);cat.writeUInt16LE(1,38);cat.writeUInt32LE(layout.image,40);
boot.copy(iso,layout.image*block);readme.copy(iso,layout.readme*block);fs.mkdirSync(path.dirname(output),{recursive:true});fs.writeFileSync(output,iso);
const sha256=crypto.createHash('sha256').update(iso).digest('hex');console.log(`Built ${output}: ${iso.length} bytes, ISO9660 / El Torito BIOS, SHA256 ${sha256}`);
