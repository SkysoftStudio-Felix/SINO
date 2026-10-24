const fs=require('node:fs'),path=require('node:path'),{execFileSync}=require('node:child_process');
function fromPath(name){try{return execFileSync('where.exe',[name],{encoding:'utf8',windowsHide:true,stdio:['ignore','pipe','ignore']}).trim().split(/\r?\n/)[0];}catch{return null;}}
const gcc=process.env.SINO_TOOLCHAIN||(fromPath('gcc.exe')&&path.dirname(fromPath('gcc.exe')));
const local='Z:/LunaMakerDebug/w64devkit/bin';
function tool(name){const folder=gcc||(fs.existsSync(path.join(local,name+'.exe'))?local:null);if(!folder)throw Error('Set SINO_TOOLCHAIN to the GCC/Binutils bin directory.');return path.join(folder,name+'.exe');}
function python(){if(process.env.SINO_PYTHON)return process.env.SINO_PYTHON;const found=fromPath('python.exe');if(found&&!found.includes('WindowsApps'))return found;const local=path.join(process.env.USERPROFILE||'','.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe');if(fs.existsSync(local))return local;throw Error('Install Python/Pillow or set SINO_PYTHON.');}
module.exports={tool,python};
