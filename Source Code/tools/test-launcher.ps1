$ErrorActionPreference='Stop'
$sourceRoot=Split-Path -Parent $PSScriptRoot
$packageRoot=Split-Path -Parent $sourceRoot
$qaRoot=Join-Path $sourceRoot 'build32\launcher-qa'
New-Item -ItemType Directory -Force -Path $qaRoot | Out-Null
if (-not(Test-Path -LiteralPath (Join-Path $qaRoot 'QEMU'))) {
    New-Item -ItemType Junction -Path (Join-Path $qaRoot 'QEMU') -Target (Join-Path $packageRoot 'QEMU') | Out-Null
}
Copy-Item -LiteralPath (Join-Path $packageRoot 'SINO.iso') -Destination (Join-Path $qaRoot 'SINO.iso')
$launcher=(Get-Content -Raw -LiteralPath (Join-Path $packageRoot 'launch.ps1')).Replace('4555','4564').Replace('SINO Desktop','SINO Launcher QA')
[IO.File]::WriteAllText((Join-Path $qaRoot 'launch.ps1'),$launcher)
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class LauncherQA {
    public delegate bool Callback(IntPtr w,IntPtr v);
    [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback,IntPtr v);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr w,out uint pid);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr w,StringBuilder s,int n);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr w);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr w);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr w,int command);
    public static IntPtr Display(uint pid){IntPtr found=IntPtr.Zero;EnumWindows(delegate(IntPtr w,IntPtr v){uint owner;GetWindowThreadProcessId(w,out owner);if(owner==pid){var s=new StringBuilder(256);GetWindowText(w,s,s.Capacity);if(s.ToString().Contains("SINO"))found=w;}return true;},IntPtr.Zero);return found;}
}
'@
function Run-Launcher {
    $child=Start-Process powershell.exe -ArgumentList @('-NoLogo','-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+(Join-Path $qaRoot 'launch.ps1')+'"')) -WindowStyle Hidden -PassThru
    if(-not $child.WaitForExit(30000)){throw 'Launcher did not return within 30 seconds'}
    if($child.ExitCode-ne 0){throw 'Launcher child failed'}
}
function Check-Visible($expected) {
    $instances=@(Get-CimInstance Win32_Process -Filter "Name = 'qemu-system-i386w.exe'" | Where-Object { $_.CommandLine -match '127\.0\.0\.1:4564' })
    if($instances.Count-ne 1){throw "Expected one QA VM, found $($instances.Count)"}
    $vmId=[uint32]$instances[0].ProcessId
    if($expected-and $expected-ne $vmId){throw 'Relaunch replaced the existing VM'}
    $window=[LauncherQA]::Display($vmId)
    if($window-eq [IntPtr]::Zero-or -not[LauncherQA]::IsWindowVisible($window)-or [LauncherQA]::IsIconic($window)){throw 'VM display is hidden or minimized'}
    return @{PID=$vmId;Window=$window}
}
try {
    Run-Launcher
    $state=Check-Visible 0
    Write-Output "PASS GUI executable cold start: PID=$($state.PID), visible=True"
    & node (Join-Path $PSScriptRoot 'test-launcher-guest.js') (Join-Path $qaRoot 'Data')
    if($LASTEXITCODE-ne 0){throw 'Visible guest input test failed'}
    [LauncherQA]::ShowWindow($state.Window,0) | Out-Null
    if([LauncherQA]::IsWindowVisible($state.Window)){throw 'Hide setup failed'}
    Run-Launcher
    $state=Check-Visible $state.PID
    Write-Output 'PASS Relaunch restores a hidden GTK display and preserves the VM'
    [LauncherQA]::ShowWindow($state.Window,6) | Out-Null
    if(-not[LauncherQA]::IsIconic($state.Window)){throw 'Minimize setup failed'}
    Run-Launcher
    $state=Check-Visible $state.PID
    Run-Launcher
    $state=Check-Visible $state.PID
    Write-Output 'PASS Relaunch restores a minimized window; repeated launches create no duplicates'
    @('PASS: GUI cold start, hidden restore, minimized restore, same process, no duplicate.',(Get-Content -Raw (Join-Path $qaRoot 'Data\launcher.log'))) | Set-Content (Join-Path $qaRoot 'report.txt')
} finally {
    $taskScript='const {QMP}=require(process.argv[2]);(async()=>{const q=await new QMP(4564).connect();try{await q.call("quit");}finally{q.close();}})().catch(e=>{console.error(e);process.exitCode=1;});'
    $quitPath=Join-Path $qaRoot 'quit.js'
    [IO.File]::WriteAllText($quitPath,$taskScript)
    & node $quitPath (Join-Path $PSScriptRoot 'qmp.js')
}
