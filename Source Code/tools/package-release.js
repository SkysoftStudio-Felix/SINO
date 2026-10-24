/* Export three clean folders. Refuses to replace an existing delivery. */
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),assert=require('node:assert/strict'),{execFileSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),repo=path.dirname(root),destination=process.argv[2];
if(!destination||!path.isAbsolute(destination))throw Error('Usage: node tools/package-release.js <new absolute output folder>');
if(fs.existsSync(destination))throw Error('Destination already exists; preserve it and choose a new output directory.');
if(execFileSync('git',['status','--porcelain'],{cwd:repo,encoding:'utf8'}).trim())throw Error('Commit the reviewed source before exporting a release.');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex'),seed=path.join(root,'build32/public-release'),gate=JSON.parse(fs.readFileSync(path.join(root,'build32/system-verification.json'))),prepared=JSON.parse(fs.readFileSync(path.join(seed,'report.json')));
const iso=fs.readFileSync(require('./paths').iso),isoHash=sha(iso);assert.equal(gate.isoSha256,isoHash);assert.equal(gate.results.length,18);assert(gate.results.every(t=>t.result==='PASS'));assert.equal(prepared.isoSha256,isoHash);
const windows=JSON.parse(fs.readFileSync(path.join(root,'build32/public-launcher-qa/report.json'),'utf8').replace(/^\uFEFF/,''));assert.equal(windows.result,'PASS');assert.equal(windows.isoSha256,isoHash);assert.equal(windows.launcherSha256,sha(fs.readFileSync(path.join(root,'release/launch.ps1'))));
assert.equal(sha(fs.readFileSync(path.join(seed,'system.img'))),prepared.diskSha256);assert.equal(sha(fs.readFileSync(path.join(root,'build32/user/ide.snx'))),prepared.ideSha256);
const source=path.join(destination,'源码'),runtime=path.join(destination,'编译后的运行文件'),intro=path.join(destination,'介绍文件');for(const dir of [source,runtime,intro])fs.mkdirSync(dir,{recursive:true});
function copy(from,to){fs.mkdirSync(path.dirname(to),{recursive:true});fs.copyFileSync(from,to);}
const files=execFileSync('git',['ls-files','--cached','--others','--exclude-standard','-z'],{cwd:repo,encoding:'utf8'}).split('\0').filter(Boolean);
for(const name of files)if(name.startsWith('源码/'))copy(path.join(repo,name),path.join(source,name.slice(3)));
fs.writeFileSync(path.join(source,'.gitignore'),'build32/\nsino.img\nsino32-test.img\n');
for(const name of ['launch.ps1','启动SINO.cmd','安装SINO.cmd'])copy(path.join(root,'release',name),path.join(runtime,name));
copy(require('./paths').iso,path.join(runtime,'SINO.iso'));copy(path.join(seed,'system.img'),path.join(runtime,'Data/system.img'));
for(const app of ['hello','sysbasic','files','editor','tasks'])copy(path.join(root,'build32/user',app+'.sxp'),path.join(runtime,'软件包',app+'.sxp'));
for(const app of ['ide','basicrun','setup','apps','hello','files','editor','tasks'])copy(path.join(root,'build32/user',app+'.snx'),path.join(runtime,'原生程序',app+'.snx'));
for(const ext of ['prj','frm','bas','snx','sxp'])copy(path.join(seed,'First.'+ext),path.join(runtime,'示例','First.'+ext));
for(const file of ['开始使用.txt','发布说明.md'])copy(path.join(root,'release',file),path.join(intro,file));
// Do not include prior private-disk verification metadata or personal data inventories.
for(const file of ['SINO-MIT.txt','字体-OFL.txt','中文字体-Unifont.txt','Unifont-README.txt','拼音数据-MIT.txt'])copy(path.join(repo,'许可',file),path.join(intro,'许可',file));
copy(path.join(root,'SOURCES.md'),path.join(intro,'许可/来源说明.md'));copy(path.join(root,'tools/nasm-3.02/LICENSE'),path.join(intro,'许可/NASM-LICENSE.txt'));
for(const [from,to] of [['build32/vm-tests/ide-input/03-programming-symbols.png','截图/编程输入.png'],['build32/vm-tests/ide-input/04-designer.png','截图/窗体设计器.png'],['build32/vm-tests/public-release-seed/software-library.png','截图/应用管理.png'],['build32/vm-tests/public-release-seed/first-project-running.png','截图/第一个程序.png']])copy(path.join(root,from),path.join(intro,to));
const version={version:prepared.version,date:new Date().toISOString(),sourceCommit:execFileSync('git',['rev-parse','HEAD'],{cwd:repo,encoding:'utf8'}).trim(),isoSha256:isoHash,diskSha256:prepared.diskSha256,ideSha256:prepared.ideSha256,sourceFiles:files.filter(n=>n.startsWith('源码/')).length,regression:gate.results.map(t=>({test:path.basename(t.test),result:t.result})),windowsLauncher:windows,cleanDisk:prepared,thirdPartyVM:'QEMU not redistributed; install separately using the official download channel',personalDataIncluded:false};
fs.writeFileSync(path.join(intro,'验证摘要.json'),JSON.stringify(version,null,2)+'\n');
const manifest=[];function walk(dir){for(const entry of fs.readdirSync(dir,{withFileTypes:true})){const name=path.join(dir,entry.name);if(entry.isDirectory())walk(name);else{const bytes=fs.readFileSync(name);manifest.push({path:path.relative(destination,name).replaceAll('\\','/'),bytes:bytes.length,sha256:sha(bytes)});}}}walk(destination);
fs.writeFileSync(path.join(intro,'SHA256清单.json'),JSON.stringify({algorithm:'SHA-256',scope:'All distribution files except this manifest; runtime disk hash is for the pristine delivery',files:manifest},null,2)+'\n');
assert.deepEqual(fs.readdirSync(destination).sort(),['源码','编译后的运行文件','介绍文件'].sort());console.log(JSON.stringify({destination,files:manifest.length,bytes:manifest.reduce((n,f)=>n+f.bytes,0),isoSha256:isoHash},null,2));
