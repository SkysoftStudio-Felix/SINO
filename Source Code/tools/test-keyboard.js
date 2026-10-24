const {execFileSync}=require('node:child_process'),path=require('node:path'),fs=require('node:fs'),{tool}=require('./host-tools');
const root=path.resolve(__dirname,'..'),out=path.join(root,'build32/test-keyboard.exe');fs.mkdirSync(path.dirname(out),{recursive:true});
execFileSync(tool('gcc'),['-Wall','-Wextra','-Werror','-Wno-misleading-indentation',path.join(root,'tools/test-keyboard.c'),path.join(root,'src32/keyboard.c'),'-o',out],{stdio:'inherit'});
execFileSync(out,[],{stdio:'inherit'});
