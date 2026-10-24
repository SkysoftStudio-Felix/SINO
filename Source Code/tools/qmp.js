const net=require('node:net');
class QMP {
  constructor(port){this.port=port;this.serial=0;this.pending=new Map();this.buffer='';}
  async connect(){
    this.socket=net.createConnection({host:'127.0.0.1',port:this.port});
    await new Promise((resolve,reject)=>{this.socket.once('error',e=>{this.socket.destroy();reject(e);});this.socket.on('data',b=>{this.buffer+=b.toString();let end;while((end=this.buffer.indexOf('\n'))>=0){const line=this.buffer.slice(0,end);this.buffer=this.buffer.slice(end+1);if(!line.trim())continue;let value;try{value=JSON.parse(line);}catch{continue;}if(value.QMP)resolve();else if(value.id&&this.pending.has(value.id)){const item=this.pending.get(value.id);this.pending.delete(value.id);clearTimeout(item.timer);value.error?item.reject(Error(JSON.stringify(value.error))):item.resolve(value.return);}}});});
    await this.call('qmp_capabilities');return this;
  }
  call(execute,args={}){const id=++this.serial;return new Promise((resolve,reject)=>{const timer=setTimeout(()=>{this.pending.delete(id);reject(Error('QMP timeout: '+execute));},15000);this.pending.set(id,{resolve,reject,timer});this.socket.write(JSON.stringify({execute,arguments:args,id})+'\n');});}
  hmp(command){return this.call('human-monitor-command',{'command-line':command});}
  async move(x,y){await this.call('input-send-event',{events:[{type:'abs',data:{axis:'x',value:Math.round(x*32767/1023)}},{type:'abs',data:{axis:'y',value:Math.round(y*32767/767)}}]});}
  async click(x,y,button='left'){await this.move(x,y);await this.call('input-send-event',{events:[{type:'btn',data:{button,down:true}}]});await this.call('input-send-event',{events:[{type:'btn',data:{button,down:false}}]});}
  async key(qcode){await this.call('send-key',{keys:[{type:'qcode',data:qcode}],'hold-time':25});}
  close(){if(this.socket)this.socket.destroy();for(const item of this.pending.values()){clearTimeout(item.timer);item.reject(Error('QMP closed'));}this.pending.clear();}
}
module.exports={QMP};
