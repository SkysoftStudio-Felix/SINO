const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path');
const {VM,sleep}=require('./vm-test');
(async()=>{const vm=new VM('usermode-qa');const results=[];try{
 await vm.start({fresh:true});await vm.key('f2');
 async function run(name){await vm.type(`run /boot/${name}.snx\n`);await sleep(300);if(vm.log().includes('KERNEL PANIC'))throw Error(vm.log());}
 const before=await vm.value('free_pages');await run('probe');
 for(let i=0;i<40&&!vm.log().includes('RING3 PROBE PASS');i++)await sleep(50);
 assert(vm.log().includes('RING3 PROBE PASS'),vm.log());let state=await vm.state(),w=state.windows.find(w=>w.app===9);assert(w,'user GUI window');await vm.shot('ring3-probe');await vm.click(w.x+70,w.y+173);await sleep(200);assert(!(await vm.state()).windows.some(w=>w.app===9));results.push('CPL3 threads, events, stale handles, pointer validation, VFS and message GUI');
 for(const name of ['fault','iofault','udfault'])await run(name);
 assert.equal(await vm.value('process_faults'),3);assert(!vm.log().includes('KERNEL PANIC'));results.push('Kernel write, I/O instruction and invalid opcode faults contained');
 const baseTicks=await vm.value('ticks');await run('spin');await run('probe');for(let i=0;i<40&&(await vm.state()).windows.every(w=>w.app!==9);i++)await sleep(50);state=await vm.state();w=state.windows.find(w=>w.app===9);assert(w,'GUI works alongside spinning user process');assert(await vm.value('ticks')>baseTicks);await vm.click(w.x+70,w.y+173);await vm.key('f2');await vm.type('kill 5\n');await sleep(300);assert(!vm.log().includes('KERNEL PANIC'));assert(await vm.value('process_switches')>10);results.push('PIT preemption keeps GUI alive during infinite user loop; process termination');
 const after=await vm.value('free_pages');assert.equal(before-after,1,'only persistent probe file retains one page');results.push('Address spaces, kernel stacks, thread stacks, handles and window scenes reclaimed');
 fs.writeFileSync(path.join(vm.run,'report.json'),JSON.stringify({results,before,after,log:vm.log()},null,2));console.log(results.map(r=>'PASS '+r).join('\n'));
 }finally{await vm.stop();}})().catch(e=>{console.error(e);process.exitCode=1;});
