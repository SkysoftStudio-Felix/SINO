const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=__dirname,build=path.join(root,'build32'),variant=process.argv[2]||'normal';
const image=Buffer.alloc(1474560),reserved=321,rootSector=339,dataSector=353;
const boot=fs.readFileSync(path.join(build,'boot.bin')),loader=fs.readFileSync(path.join(build,'loader.bin')),kernel=fs.readFileSync(path.join(build,`kernel-${variant}.bin`));
if(boot.length!==512||boot.readUInt16LE(510)!==0xaa55||loader.length!==4096||kernel.length>159744)throw Error('Boot / loader / kernel size validation failed');
const map=fs.readFileSync(path.join(build,`kernel-${variant}.map`),'utf8');
const bss=map.match(/0x([0-9a-f]+)\s+___bss_end/);if(!bss||parseInt(bss[1],16)<0x100000||parseInt(bss[1],16)>=0x1d0000)throw Error('Kernel BSS must stay below the page-table reserve at 0x1e0000');
boot.copy(image);loader.copy(image,512);kernel.copy(image,512+4096);
const fat=Buffer.alloc(4608);fat.set([0xf0,0xff,0xff]);
function setFat(c,v){const n=c+(c>>1);if(c&1){fat[n]=(fat[n]&15)|((v&15)<<4);fat[n+1]=v>>4;}else{fat[n]=v&255;fat[n+1]=(fat[n+1]&240)|(v>>8);}}
const entries=['README.TXT','ABOUT.TXT','KERNEL.TXT','SOURCES.TXT','DEMO.SH','UIASSET.BIN','FONT_OFL.TXT','CJK_OFL.TXT','PINYIN.TXT'].map(name=>[name,fs.readFileSync(path.join(root,'disk32',name))]);entries.push(['MBR.BIN',fs.readFileSync(path.join(build,'mbr.bin'))]);entries.push(['HELLO.F32',fs.readFileSync(path.join(build,'hello.bin'))]);
for(const name of ['FIRST.PRJ','FIRST.FRM','FIRST.BAS'])entries.push([name,fs.readFileSync(path.join(root,'disk32',name))]);
for(const name of ['probe','fault','iofault','spin','udfault','setup','apps','instest','nettest','bstest','basicrun','tasks'])entries.push([name.toUpperCase()+'.SNX',fs.readFileSync(path.join(build,'user',name+'.snx'))]);
entries.push(['SYSBASIC.SXP',fs.readFileSync(path.join(build,'user/sysbasic.sxp'))]);
entries.push(['HELLO.SXP',fs.readFileSync(path.join(build,'user/hello.sxp'))]);
for(const name of ['files','editor','tasks'])entries.push([name.toUpperCase()+'.SXP',fs.readFileSync(path.join(build,'user',name+'.sxp'))]);
const directory=Buffer.alloc(7168);directory.write('SINO       ',0,'ascii');directory[11]=8;
let next=2;const manifest=[];
for(let i=0;i<entries.length;i++){
    let [name,data]=entries[i];if(name.endsWith('.TXT'))data=Buffer.from(data.toString().replace(/\r?\n/g,'\r\n'),'utf8');
    const count=Math.ceil(data.length/512),clusters=Array.from({length:count},(_,j)=>next+j*(i===0?2:1));next=clusters.at(-1)+1;
    const e=(i+1)*32,[base,ext]=name.split('.');directory.write(base.padEnd(8)+ext.padEnd(3),e,'ascii');directory[e+11]=32;
    directory.writeUInt16LE(clusters[0]||0,e+26);directory.writeUInt32LE(data.length,e+28);directory.writeUInt16LE(((2026-1980)<<9)|(10<<5)|18,e+24);
    clusters.forEach((c,j)=>{if(dataSector+c-2>=2880)throw Error('Floppy full');setFat(c,clusters[j+1]||0xfff);data.copy(image,(dataSector+c-2)*512,j*512,(j+1)*512);});
    manifest.push({name,bytes:data.length,clusters});
}
fat.copy(image,reserved*512);fat.copy(image,(reserved+9)*512);directory.copy(image,rootSector*512);
const filename=variant==='test'?'sino32-test.img':'sino.img';fs.writeFileSync(path.join(root,filename),image);
fs.writeFileSync(path.join(build,`manifest-${variant}.json`),JSON.stringify({name:'SINO',version:'0.10',bits:32,interface:'SINO Platinum Desktop / VBE 1024x768x32',filename,kernelBytes:kernel.length,bssEnd:parseInt(bss[1],16),sha256:crypto.createHash('sha256').update(image).digest('hex'),files:manifest},null,2)+'\n');
console.log(`Built ${filename}: native 32-bit kernel ${kernel.length} bytes, FAT12 + full RAM disk`);
