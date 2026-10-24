/* Host packager for original SXP1 ABI; no scripts, executable installer hooks or downloads. */
const fs=require('node:fs'),path=require('node:path');
const {crc32}=require('./test-volume');
function build(manifest,base){const m=manifest;if(!/^[a-z0-9_-]{1,24}$/.test(m.id)||!/^\d{1,4}\.\d{1,4}\.\d{1,4}$/.test(m.version)||!m.files?.length||m.files.length>8)throw Error('Invalid ID, version or file count');
 const h=Buffer.alloc(1408);[0x31505853,1,386,1,m.files.length,m.files.some(f=>f.optional)?3:1,m.permissions||0,0].forEach((v,i)=>h.writeUInt32LE(v,i*4));
 function string(value,offset,cap){const b=Buffer.from(value||'');if(b.length>=cap||b.includes(0))throw Error('Manifest string overflow at '+offset);b.copy(h,offset);}
 string(m.id,32,32);string(m.name,64,64);string(m.version,128,16);string(m.entry,144,64);string(m.license,208,256);string(m.config?.key,464,32);string(m.config?.value,496,128);string(m.shortcut,624,32);string(m.association,656,16);string(m.service,672,32);
 const chunks=[h],names=new Set();let offset=h.length;
 for(let i=0;i<m.files.length;i++){const f=m.files[i];if(!/^[a-zA-Z0-9_.\/-]+$/.test(f.path)||f.path.startsWith('/')||f.path.split('/').some(x=>!x||x==='.'||x==='..'||x.length>31)||names.has(f.path))throw Error('Unsafe package path '+f.path);names.add(f.path);const bytes=f.text!==undefined?Buffer.from(f.text):fs.readFileSync(path.resolve(base,f.source));const at=768+i*80;string(f.path,at,64);h.writeUInt32LE(offset,at+64);h.writeUInt32LE(bytes.length,at+68);h.writeUInt32LE(crc32(bytes),at+72);h.writeUInt32LE(f.optional?2:1,at+76);offset+=bytes.length;chunks.push(bytes);}
 if(!names.has(m.entry)||offset>65536)throw Error('Missing entry or package exceeds 64 KiB');h.writeUInt32LE(offset,28);return Buffer.concat(chunks);}
if(require.main===module){const file=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]);const bytes=build(JSON.parse(fs.readFileSync(file,'utf8')),path.dirname(file));fs.writeFileSync(out,bytes);console.log(`Packaged ${path.basename(out)}: ${bytes.length} bytes`);}
module.exports={build};
