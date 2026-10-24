const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'..'),parent=path.dirname(root),publicRuntime=path.join(parent,'Compiled Runtime');
const runtime=fs.existsSync(publicRuntime)?publicRuntime:parent;
module.exports={runtime,iso:process.env.SINO_ISO||path.join(runtime,'SINO.iso')};
