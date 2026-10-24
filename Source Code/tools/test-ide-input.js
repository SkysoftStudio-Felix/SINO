/* Real press/hold/release input and persisted designer geometry, in a private VM. */
const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const {VM,sleep}=require('./vm-test'),{volume}=require('./test-volume');
(async()=>{const vm=new VM('ide-input'),checks=[];try{
 async function wait(s,at=0){for(let i=0;i<500;i++){if(vm.log().slice(at).includes(s))return;await sleep(20);}throw Error('Missing '+s+'\n'+vm.log());}
 async function raw(code,down){await vm.q.call('input-send-event',{events:[{type:'key',data:{down,key:{type:'qcode',data:code}}}]});await sleep(35);}
 async function tap(code){await raw(code,true);await raw(code,false);}
 await vm.start({fresh:true});await vm.key('f3');
 await raw('shift',true);await tap('minus');await raw('shift_r',true);await raw('shift',false);await tap('minus');await raw('shift_r',false);await tap('minus');
 let w=(await vm.state()).windows.find(w=>w.app===2);assert.equal(w.text,'__-');
 await vm.key('ctrl','a');const symbols='_+{}:"~<>?|!@#$%^&*()';await vm.type(symbols);w=(await vm.state()).windows.find(w=>w.app===2);assert.equal(w.text,symbols);
 await vm.key('ctrl','a');for(const code of ['kp_1','kp_2','kp_3','kp_add','kp_4','kp_subtract','kp_5','kp_multiply','kp_6','kp_divide','kp_7','kp_decimal'])await tap(code);
 w=(await vm.state()).windows.find(w=>w.app===2);assert.equal(w.text,'123+4-5*6/7.');await vm.shot('01-keyboard');
 checks.push('Held left/right Shift, release one while holding the other, all programming symbols, numeric keypad');
 await vm.key('ctrl','w');await vm.key('esc'); // discard unsaved temporary text
 await vm.key('f2');await vm.type('install /quiet:/boot/sysbasic.sxp\n');await wait('QUIET INSTALL PASS');await vm.key('f11');
 for(let i=0;i<120;i++){w=(await vm.state()).windows.find(w=>w.title.includes('SysBASIC'));if(w)break;await sleep(25);}assert(w);
 const click=(x,y)=>vm.click(w.x+x,w.y+y),drag=(x,y,a,b)=>vm.drag(w.x+x,w.y+y,w.x+a,w.y+b);
 async function saved(){let at=vm.log().length;await vm.key('ctrl','s');await wait('SYSBASIC PROJECT SAVED',at);return volume(path.join(vm.run,'data.img')).current.records;}
 async function form(){const records=await saved();return records.get('/home/user/Projects/Untitled.frm').data.toString().trim().split('\n').slice(1).map(l=>l.split('|'));}
 await click(38,71);await click(45,239);await drag(225,212,385,260);let c=(await form())[0];assert.deepEqual(c.slice(3,7).map(Number),[80,64,160,48]);await vm.shot('02-drawn-button');
 // Every visible selection handle must resize the right edges, preserving opposite edges.
 let rect={x:80,y:64,w:160,h:48};const handles=[[0,0,-8,-8],[1,0,0,-8],[2,0,8,-8],[0,1,-8,0],[2,1,8,0],[0,2,-8,8],[1,2,0,8],[2,2,8,8]];
 for(const [col,row,dx,dy] of handles){const hx=145+rect.x+Math.floor(col*(rect.w-5)/2)+2,hy=148+rect.y+Math.floor(row*(rect.h-5)/2)+2;
 await drag(hx,hy,hx+dx,hy+dy);if(col===0){rect.x+=dx;rect.w-=dx;}else if(col===2)rect.w+=dx;if(row===0){rect.y+=dy;rect.h-=dy;}else if(row===2)rect.h+=dy;
 c=(await form())[0];assert.deepEqual(c.slice(3,7).map(Number),[rect.x,rect.y,rect.w,rect.h],'handle '+col+','+row);}
 checks.push('Select a toolbox tool then draw a snapped rectangle; all eight handles resize and anchor the opposite edge');
 await drag(145+rect.x+20,148+rect.y+20,145+rect.x+44,148+rect.y+36);rect.x+=24;rect.y+=16;c=(await form())[0];assert.deepEqual(c.slice(3,7).map(Number),[rect.x,rect.y,rect.w,rect.h]);
 // Escape restores geometry during a held drag; the following release must not reapply it.
 await vm.q.move(w.x+145+rect.x+20,w.y+148+rect.y+20);await vm.q.call('input-send-event',{events:[{type:'btn',data:{button:'left',down:true}}]});await sleep(50);await vm.q.move(w.x+145+rect.x+60,w.y+148+rect.y+52);await sleep(100);await vm.key('esc');await vm.release(w.x+145+rect.x+60,w.y+148+rect.y+52);c=(await form())[0];assert.deepEqual(c.slice(3,7).map(Number),[rect.x,rect.y,rect.w,rect.h]);
 await vm.double(w.x+45,w.y+283);let added=await form();assert.equal(added.length,2);const unchanged=added[1].slice(3,7);const ox=+added[1][3],oy=+added[1][4];await click(145+ox+2,148+oy+2);assert.deepEqual((await form())[1].slice(3,7),unchanged,'Click a handle without dragging preserves exact size');await click(45,194);await drag(560,380,440,348);c=(await form())[2];assert.deepEqual(c.slice(3,7).map(Number),[296,200,120,32]);
 await click(45,283);await vm.q.move(w.x+470,w.y+300);await vm.q.call('input-send-event',{events:[{type:'btn',data:{button:'left',down:true}}]});await vm.q.move(w.x+590,w.y+330);await sleep(100);await vm.key('esc');await vm.release(w.x+590,w.y+330);assert.equal((await form()).length,3);
 checks.push('Pointer moves with grid snapping, Escape restores move/cancels creation, reverse drawing and double-click toolbox default size');
 await click(366,71);await vm.key('ctrl','a');
 const code='Sub Form_Load()\nDim my_value As Long\nmy_value = 12 + 3\nPrint "under_score=" & CStr(my_value)\nEnd Sub\n';
 // Type every underscore with a separately held right Shift, exactly as a physical keyboard.
 for(const part of code.split('_').entries()){if(part[0]){await raw('shift_r',true);await tap('minus');await raw('shift_r',false);}await vm.type(part[1]);}
 const records=await saved();assert.equal(records.get('/home/user/Projects/Untitled.bas').data.toString(),code);await vm.shot('03-programming-symbols');await vm.key('f5');await wait('SYSBASIC: under_score=15');await vm.key('esc');
 checks.push('Native IDE saves manually typed underscores in event names/variables/strings, then actually runs Form_Load');
 assert(!vm.log().includes('SCENE REJECTED'));assert(!vm.log().includes('KERNEL PANIC'));await vm.shot('04-designer');
 fs.writeFileSync(path.join(vm.run,'report.json'),JSON.stringify({checks,isoSha256:crypto.createHash('sha256').update(fs.readFileSync(require('./paths').iso)).digest('hex')},null,2));console.log(checks.map(s=>'PASS '+s).join('\n'));
 }catch(e){try{await vm.shot('failure');}catch{}throw e;}finally{await vm.stop();}})().catch(e=>{console.error(e);process.exitCode=1;});
