// Actual QEMU guest checks. Daily-use data.img is never opened by these tests.
const {execFileSync}=require('node:child_process'),fs=require('node:fs'),path=require('node:path');
for(const test of ['test-iso.js','run-selftest.js','test-desktop.js','test-recovery.js','test-performance.js','test-compatibility.js','test-typography.js','test-chinese-files.js','test-settings.js','test-install.js'])execFileSync(process.execPath,[path.join(__dirname,'tools',test)],{stdio:'inherit',timeout:180000});
const source=path.join(require('./tools/vm-test').runtime,'desktop-qa'),out=path.join(__dirname,'build32');
for(const name of fs.readdirSync(source))if(/^\d\d-.*\.png$/.test(name)||name==='report.json')fs.copyFileSync(path.join(source,name),path.join(out,'platinum-'+name));
for(const name of ['recovery-qa','performance-qa']){for(const file of ['report.json',name==='recovery-qa'?'recovered.png':'drag.png'])fs.copyFileSync(path.join(require('./tools/vm-test').runtime,name,file),path.join(out,'platinum-'+name+'-'+file));}
fs.writeFileSync(path.join(out,'verification.txt'),'PASS: native kernel / exceptions / shell self-tests and real QEMU desktop integration, CRC recovery and cached-drag performance.\nSee platinum-report.json and platinum-*.png.\n');
console.log('PASS: QEMU desktop verification and screenshots saved in build32.');
