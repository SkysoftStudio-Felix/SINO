const fs=require('node:fs'),path=require('node:path'),{execFileSync:run}=require('node:child_process');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build32','user'),tc=path.dirname(require('./host-tools').tool('gcc'));fs.mkdirSync(out,{recursive:true});
const flags=['-m32','-march=i386','-std=c11','-Os','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables','-mno-stack-arg-probe','-mno-sse','-mno-mmx','-msoft-float','-mpreferred-stack-boundary=2','-mincoming-stack-boundary=2','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation'];
function command(exe,args){run(exe,args,{cwd:root,stdio:'inherit'});}
command(path.join(root,'tools/nasm-3.02/nasm.exe'),['-f','win32','user/entry.asm','-o',path.join(out,'entry.o')]);
command(path.join(tc,'gcc.exe'),[...flags,'-c','user/runtime.c','-o',path.join(out,'runtime.o')]);
const apps=[['probe','probe',0],['fault','probe',1],['iofault','probe',2],['spin','probe',3],['udfault','probe',4],['hello','hello',0],['setup','setup',0],['apps','apps',0],['instest','installer_tests',0],['nettest','nettest',0],['bstest','basic_tests',0],['basicrun','basicrun',0],['ide','ide',0],['files','files',0],['editor','editor',0],['tasks','tasks',0]];
for(const [name,source,mode] of apps){
 command(path.join(tc,'gcc.exe'),[...flags,`-DMODE=${mode}`,'-c',`user/${source}.c`,'-o',path.join(out,`${name}.o`)]);
 const extra=[];if(name==='setup'||name==='instest'){const obj=path.join(out,`${name}-service.o`);command(path.join(tc,'gcc.exe'),[...flags,'-c','user/install_service.c','-o',obj]);extra.push(obj);}
 if(name==='bstest'||name==='basicrun'||name==='ide'){for(const module of (name==='bstest'?['basic','netlib']:['basic','netlib','forms'])){const obj=path.join(out,`${name}-${module}.o`);command(path.join(tc,'gcc.exe'),[...flags,'-c',`user/${module}.c`,'-o',obj]);extra.push(obj);}}
 if(name==='nettest'){const obj=path.join(out,`${name}-netlib.o`);command(path.join(tc,'gcc.exe'),[...flags,'-c','user/netlib.c','-o',obj]);extra.push(obj);}
 const exe=path.join(out,`${name}.exe`),bin=path.join(out,`${name}.snx`);
 command(path.join(tc,'ld.exe'),['-m','i386pe','--image-base','0x3fff0000','--disable-reloc-section','--disable-dynamicbase','--no-insert-timestamp','-T','user/linker.ld',path.join(out,'entry.o'),path.join(out,'runtime.o'),path.join(out,`${name}.o`),...extra,'-o',exe]);
 command(path.join(tc,'objcopy.exe'),['-O','binary',exe,bin]);
 const symbols={};for(const line of run(path.join(tc,'nm.exe'),['-n',exe],{encoding:'utf8'}).split('\n')){const m=line.match(/^([0-9a-f]+) [A-Za-z] (\w+)\s*$/);if(m)symbols[m[2]]=parseInt(m[1],16)-0x40000000;}
 for(const key of ['_user_start','___user_data','___user_end'])if(!Number.isInteger(symbols[key])||symbols[key]<0)throw Error('Missing/invalid native link symbol '+key);
 const bytes=fs.readFileSync(bin);bytes.writeUInt32LE(bytes.length,8);bytes.writeUInt32LE(symbols._user_start,12);bytes.writeUInt32LE(symbols.___user_data,16);bytes.writeUInt32LE(symbols.___user_end,20);
 if(bytes.length>65536||symbols.___user_end>0x30000)throw Error('Native image exceeds resource limits');fs.writeFileSync(bin,bytes);console.log(`Native ${name}.snx: ${bytes.length} bytes, memory ${symbols.___user_end}`);
}


run(process.execPath,[path.join(root,"tools/package.js"),path.join(root,"packages/hello.json"),path.join(out,"hello.sxp")],{stdio:"inherit"});

run(process.execPath,[path.join(root,"tools/package.js"),path.join(root,"packages/sysbasic.json"),path.join(out,"sysbasic.sxp")],{stdio:"inherit"});

for(const name of ["files","editor","tasks"])run(process.execPath,[path.join(root,"tools/package.js"),path.join(root,`packages/${name}.json`),path.join(out,`${name}.sxp`)],{stdio:"inherit"});
