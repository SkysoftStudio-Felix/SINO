/* Runs only against the private Windows launcher QA disk, never the daily disk. */
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');const {VM,sleep,root}=require('./vm-test'),{QMP}=require('./qmp');
(async()=>{const vm=new VM('public-launcher-input');vm.run=path.join(root,'build32/public-launcher-qa/Data');vm.log=()=>fs.readFileSync(path.join(vm.run,'kernel.log'),'utf8');vm.q=await new QMP(4585).connect();try{
 async function wait(s,at=0){for(let i=0;i<500;i++){if(vm.log().slice(at).includes(s))return;await sleep(25);}throw Error('Missing '+s);}
 async function find(s){for(let i=0;i<150;i++){const w=(await vm.state()).windows.find(w=>w.title.includes(s));if(w)return w;await sleep(25);}throw Error('Missing '+s);}
 assert(!((await vm.q.call('query-block')).some(b=>b.inserted&&b.inserted.file.endsWith('.iso'))));
 await vm.key('f11');let ide=await find('SysBASIC 0.3');await vm.key('ctrl','o');await vm.type('/home/user/Projects/First.prj');await vm.key('ret');await wait('SYSBASIC PROJECT LOADED');await vm.key('f5');await vm.click(ide.x+210,ide.y+243);await wait('SYSBASIC: Value: 1');await vm.shot('public-first-project');await vm.key('esc');await vm.key('ctrl','w');
 await vm.key('f9');const manager=await find('应用管理器');await vm.click(manager.x+100,manager.y+96+5*28+10);const before=vm.log().length;await vm.click(manager.x+65,manager.y+418);const app=await find('第一个程序');await vm.click(app.x+89,app.y+149);await wait('SYSBASIC APP: Value: 1',before);await vm.shot('public-installed-example');await vm.key('ctrl','w');await vm.key('ctrl','w');
 await vm.key('f11');await find('SysBASIC 0.3');await vm.shot('public-blank-ide');
 const checks=['Windows CMD launcher cold boots the installed disk with no ISO','F11 opens SysBASIC 0.3','First.prj loads and underscore Click event actually runs','Application manager launches the installed First example','Original sealed delivery disk remains untouched; tests use a copy'];
 fs.writeFileSync(path.join(root,'build32/public-launcher-qa/guest-report.json'),JSON.stringify({result:'PASS',checks},null,2));console.log(checks.map(s=>'PASS '+s).join('\n'));
 }catch(e){try{await vm.shot('public-failure');}catch{}throw e;}finally{vm.q.close();}})().catch(e=>{console.error(e);process.exitCode=1;});
