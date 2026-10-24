$ErrorActionPreference='Stop'
$sourceRoot=Split-Path -Parent $PSScriptRoot
$runtimeRoot=Split-Path -Parent $sourceRoot
$publicRuntime=Join-Path $runtimeRoot 'Compiled Runtime'
if(Test-Path -LiteralPath $publicRuntime){$runtimeRoot=$publicRuntime}
$qaRoot=Join-Path $sourceRoot 'build32\public-launcher-qa'
if(Get-NetTCPConnection -State Listen -LocalPort 4585 -ErrorAction SilentlyContinue){throw 'Port 4585 is occupied; close the public preview before testing.'}
$resolved=[IO.Path]::GetFullPath($qaRoot)
if(-not $resolved.StartsWith([IO.Path]::GetFullPath((Join-Path $sourceRoot 'build32'))+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'QA target escaped build32'}
New-Item -ItemType Directory -Force -Path (Join-Path $qaRoot 'Data') | Out-Null
foreach($name in @('launch.ps1','Start SINO.cmd')){Copy-Item -LiteralPath (Join-Path $sourceRoot ('release\'+$name)) -Destination (Join-Path $qaRoot $name)}
Copy-Item -LiteralPath (Join-Path $runtimeRoot 'SINO.iso') -Destination (Join-Path $qaRoot 'SINO.iso')
Copy-Item -LiteralPath (Join-Path $sourceRoot 'build32\public-release\system.img') -Destination (Join-Path $qaRoot 'Data\system.img') -Force
if(-not $env:SINO_QEMU_DIR){$env:SINO_QEMU_DIR=Join-Path $runtimeRoot 'QEMU'}
Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class PublicLauncherQA {
 public delegate bool Callback(IntPtr w,IntPtr v);
 [DllImport("user32.dll")] static extern bool EnumWindows(Callback c,IntPtr v);
 [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr w,out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr w,StringBuilder s,int n);
 [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr w,int cmd);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr w);
 [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr w);
 public static IntPtr Display(uint pid){IntPtr found=IntPtr.Zero;EnumWindows(delegate(IntPtr w,IntPtr v){uint owner;GetWindowThreadProcessId(w,out owner);if(owner==pid){var s=new StringBuilder(512);GetWindowText(w,s,s.Capacity);if(s.ToString().Contains("SINO"))found=w;}return true;},IntPtr.Zero);return found;}
}
'@
function Launch {
 $argsText='/d /c call "'+(Join-Path $qaRoot 'Start SINO.cmd')+'"'
 $child=Start-Process cmd.exe -ArgumentList $argsText -WindowStyle Hidden -RedirectStandardOutput (Join-Path $qaRoot 'command.log') -RedirectStandardError (Join-Path $qaRoot 'command-error.log') -PassThru
 $null=$child.Handle
 if(-not $child.WaitForExit(30000)){throw 'CMD launcher timed out'}
 $child.WaitForExit()
 if($child.ExitCode-ne 0){throw 'CMD launcher failed'}
}
function Visible([uint32]$expected){
 $instances=@(Get-CimInstance Win32_Process -Filter "Name='qemu-system-i386w.exe'" | Where-Object {$_.CommandLine-match '127\.0\.0\.1:4585'})
 if($instances.Count-ne 1){throw 'Expected exactly one public QA VM'}
 $vmPid=[uint32]$instances[0].ProcessId
 if($expected-and $vmPid-ne $expected){throw 'Relaunch replaced the VM'}
 $handle=[PublicLauncherQA]::Display($vmPid)
 if($handle-eq [IntPtr]::Zero-or -not [PublicLauncherQA]::IsWindowVisible($handle)-or [PublicLauncherQA]::IsIconic($handle)){throw 'VM window is not visible'}
 return @{PID=$vmPid;Handle=$handle}
}
try {
 Launch;$display=Visible 0
 & node (Join-Path $PSScriptRoot 'test-public-guest.js')
 if($LASTEXITCODE-ne 0){throw 'Public guest interaction failed'}
 [PublicLauncherQA]::ShowWindow($display.Handle,0) | Out-Null
 Launch;$display=Visible $display.PID
 [PublicLauncherQA]::ShowWindow($display.Handle,6) | Out-Null
 Launch;$display=Visible $display.PID
 Launch;$display=Visible $display.PID
 $checks=@('actual Windows CMD cold start','GTK window visible','no-ISO installed disk boots','native IDE and installed First app run','hidden/minimized window restored','repeated CMD launch keeps the same process without duplicates')
 $report=@{result='PASS';isoSha256=(Get-FileHash -LiteralPath (Join-Path $qaRoot 'SINO.iso') -Algorithm SHA256).Hash.ToLower();launcherSha256=(Get-FileHash -LiteralPath (Join-Path $qaRoot 'launch.ps1') -Algorithm SHA256).Hash.ToLower();checks=$checks}
 $report | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $qaRoot 'report.json')
 $checks | ForEach-Object {Write-Output ('PASS '+$_)}
} finally {
 $quit=Join-Path $qaRoot 'quit.js'
 [IO.File]::WriteAllText($quit,'const {QMP}=require(process.argv[2]);(async()=>{const q=await new QMP(4585).connect();try{await q.call("quit");}finally{q.close();}})().catch(e=>{console.error(e);process.exitCode=1;});')
 & node $quit (Join-Path $PSScriptRoot 'qmp.js')
}
