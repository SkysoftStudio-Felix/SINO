/* Build the public demo disk exclusively from a fresh private virtual disk. */
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict'),crypto=require('node:crypto');
const {VM,sleep,root}=require('./vm-test'),{volume}=require('./test-volume');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
(async()=>{const vm=new VM('public-release-seed');try{
 async function wait(s,at=0){for(let i=0;i<800;i++){if(vm.log().slice(at).includes(s))return;await sleep(25);}throw Error('Missing '+s+'\n'+vm.log());}
 async function find(title){for(let i=0;i<200;i++){const w=(await vm.state()).windows.find(w=>w.title.includes(title));if(w)return w;await sleep(25);}throw Error('Missing window '+title);}
 await vm.start({fresh:true,diskBytes:128*1024*1024});let setup=(await vm.state()).windows.find(w=>w.app===8);assert(setup);
 const next=()=>vm.click(setup.x+setup.w-92,setup.y+setup.h-39);
 await next();await next();await next();assert.equal(await vm.value('install_step'),3);
 await vm.click(setup.x+609,setup.y+setup.h-96);assert.equal(await vm.value('region_count'),2);await next();await next();
 for(let i=0;i<500&&!await vm.value('install_complete');i++)await sleep(25);assert.equal(await vm.value('install_complete'),1);await vm.stop();
 await vm.start({hardBoot:true});assert(!(await vm.q.call('query-block')).some(b=>b.inserted&&b.inserted.file.endsWith('.iso')));
 await vm.key('f2');for(const name of ['sysbasic','files','editor','tasks','hello']){let at=vm.log().length;await vm.type(`install /quiet:/boot/${name}.sxp\n`);await wait('QUIET INSTALL PASS',at);}
 await vm.type('mkdir /home/user/Projects\n');for(const ext of ['prj','frm','bas'])await vm.type(`cp /boot/first.${ext} /home/user/Projects/First.${ext}\n`);await vm.type('sync\n');await vm.key('ctrl','w');await vm.key('f11');
 let ide=await find('SysBASIC 0.3'),click=(x,y)=>vm.click(ide.x+x,ide.y+y);await vm.key('ctrl','o');await vm.type('/home/user/Projects/First.prj');await vm.key('ret');await wait('SYSBASIC PROJECT LOADED');
 await vm.key('f5');await click(145+65,148+95);await wait('SYSBASIC: Value: 1');await vm.shot('first-project-running');await vm.key('esc');
 await click(700,71);await click(560,450);await wait('SYSBASIC EXECUTABLE BUILT');let app=await find('第一个程序');await vm.click(app.x+24+65,app.y+54+95);await wait('SYSBASIC APP: Value: 1');await vm.key('ctrl','w');
 await click(700,71);await click(360,320);await click(560,450);await wait('SYSBASIC INSTALLER BUILT');setup=await find('原生软件安装程序');for(let i=0;i<4;i++)await vm.click(setup.x+500,setup.y+405);await wait('SOFTWARE INSTALL PASS');await vm.click(setup.x+500,setup.y+405);await vm.key('ctrl','w');
 await vm.key('f2');await vm.type('sync\n');for(let i=0;i<100&&await vm.value('storage_dirty');i++)await sleep(25);assert.equal(await vm.value('storage_dirty'),0);await vm.key('ctrl','w');await vm.stop();
 // Cold boot with no optical image, and launch the actual installed example via its registry entry.
 await vm.start({hardBoot:true});await vm.key('f9');let manager=await find('应用管理器');await vm.shot('software-library');await vm.key('ctrl','w');await vm.key('f2');await vm.type('run /home/user/Programs/basic-first/program.snx\n');app=await find('第一个程序');await vm.click(app.x+89,app.y+149);await wait('SYSBASIC APP: Value: 1');await vm.key('ctrl','w');await vm.key('ctrl','w');await vm.key('f11');ide=await find('SysBASIC 0.3');await vm.shot('blank-ide');await vm.key('ctrl','w');await vm.stop();
 const bytes=fs.readFileSync(path.join(vm.run,'data.img')),dataStart=bytes.readUInt32LE(462+8),records=volume(path.join(vm.run,'data.img'),dataStart*512).current.records;
 for(const name of ['sysbasic','files','editor','tasks','hello','basic-first'])assert(records.has(`/home/user/.sino/apps/${name}.rec`),name);
 assert(!records.has('/home/user/.sino/rollback/journal'));const tutorial={};for(const ext of ['prj','frm','bas','snx','sxp']){const key='/home/user/Projects/First.'+ext;assert(records.has(key));tutorial[ext]=records.get(key).data;}
 const output=path.join(root,'build32/public-release');fs.mkdirSync(output,{recursive:true});fs.copyFileSync(path.join(vm.run,'data.img'),path.join(output,'system.img'));
 for(const [ext,bytes] of Object.entries(tutorial))fs.writeFileSync(path.join(output,'First.'+ext),bytes);
 assert(!vm.log().includes('KERNEL PANIC'));assert(!vm.log().includes('SCENE REJECTED'));
 const report={version:'SINO 0.10 / SysBASIC 0.3.0',createdFrom:'new zero-filled 128 MiB private virtual disk; OS and packages installed through their actual installers',isoSha256:hash(fs.readFileSync(require('./paths').iso)),diskSha256:hash(bytes),ideSha256:hash(fs.readFileSync(path.join(root,'build32/user/ide.snx'))),dataStart,records:records.size,installed:['sysbasic','files','editor','tasks','hello','basic-first'],checks:['BIOS ISO installation and MBR partitioning','cold hard-disk boot with no ISO','five native packages installed independently','First project opens and handles underscore event names','preview, standalone executable and GUI-installed app actually run','installed example survives cold reboot','blank IDE opens from F11','no interrupted install journal','no daily-use disk or personal files used']};
 fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));
 }catch(e){try{await vm.shot('failure');}catch{}throw e;}finally{await vm.stop();}})().catch(e=>{console.error(e);process.exitCode=1;});
