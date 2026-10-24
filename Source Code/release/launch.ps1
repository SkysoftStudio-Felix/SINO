param([switch]$Install,[switch]$Live)
$ErrorActionPreference = 'Stop'
try {
    $packageRoot = $PSScriptRoot
    # The public bundle contains SINO binaries, not a redistributed QEMU build.
    $candidates=@()
    if($env:SINO_QEMU_DIR){$candidates+=$env:SINO_QEMU_DIR}
    $candidates+=Join-Path $packageRoot 'QEMU'
    $candidates+=Join-Path $env:ProgramFiles 'qemu'
    # Reuse the user's existing development installation when kept beside this release.
    $candidates+=Join-Path (Split-Path -Parent (Split-Path -Parent $packageRoot)) 'sino\QEMU'
    foreach($name in @('qemu-system-i386w.exe','qemu-system-i386.exe')){
        $found=Get-Command $name -ErrorAction SilentlyContinue
        if($found){$candidates+=Split-Path -Parent $found.Source}
    }
    $qemuExe=$null
    foreach($folder in $candidates){
        foreach($name in @('qemu-system-i386w.exe','qemu-system-i386.exe')){
            $candidate=Join-Path $folder $name
            if(Test-Path -LiteralPath $candidate){$qemuExe=[IO.Path]::GetFullPath($candidate);break}
        }
        if($qemuExe){break}
    }
    if(-not $qemuExe){throw 'Please install QEMU for Windows: https://www.qemu.org/download/#windows . Or set SINO_QEMU_DIR to its directory. See the Documentation folder.'}
    $qemuRoot=Split-Path -Parent $qemuExe
    $oldQemuExe=Join-Path $qemuRoot 'qemu-system-i386.exe'
    $qemuShare=Join-Path $qemuRoot 'share'
    if(Test-Path -LiteralPath (Join-Path $qemuShare 'qemu\bios-256k.bin')){$qemuShare=Join-Path $qemuShare 'qemu'}
    $iso = Join-Path $packageRoot 'SINO.iso'
    $baseData = Join-Path $packageRoot 'Data'
    $systemDisk = Join-Path $baseData 'system.img'
    $installed = $false
    if (-not $Live -and -not $Install -and (Test-Path -LiteralPath $systemDisk)) {
        $header = New-Object byte[] 512
        $stream = [IO.File]::Open($systemDisk,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
        try { $null=$stream.Read($header,0,512) } finally { $stream.Dispose() }
        $hasSystem=$false; $hasData=$false
        for($slot=0;$slot-lt 4;$slot++) {
            $offset=446+$slot*16
            if($header[$offset]-eq 128 -and $header[$offset+4]-eq 1 -and [BitConverter]::ToUInt32($header,$offset+12)-ge 2880){$hasSystem=$true}
            if($header[$offset+4]-eq 127 -and [BitConverter]::ToUInt32($header,$offset+12)-ge 16392){$hasData=$true}
        }
        $installed=$header[510]-eq 85 -and $header[511]-eq 170 -and $hasSystem -and $hasData
    }
    $port=if($Install){4586}else{4585}
    $vmName=if($Install){'SINO Setup'}else{'SINO Desktop'}
    $dataRoot=if($Install){Join-Path $baseData 'Setup'}else{$baseData}
    $runtimePrefix=if($Install){'../../'}else{'../'}
    if (-not (Test-Path -LiteralPath $qemuExe)) { throw 'QEMU is missing. Keep the QEMU folder beside this launcher.' }
    if (-not $installed -and -not (Test-Path -LiteralPath $iso)) { throw 'SINO.iso is missing. Keep the ISO beside this launcher.' }
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class SinoWindow {
    public delegate bool EnumProc(IntPtr window, IntPtr value);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc callback, IntPtr value);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr window, StringBuilder name, int count);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern int GetWindowText(IntPtr window, StringBuilder text, int count);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr window);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr window, int command);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr window);
    public static IntPtr FindDisplay(uint pid) {
        IntPtr primary=IntPtr.Zero, visible=IntPtr.Zero, fallback=IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr value) {
            uint owner; GetWindowThreadProcessId(window,out owner);
            if(owner!=pid) return true;
            var name=new StringBuilder(256); GetClassName(window,name,name.Capacity);
            string kind=name.ToString();
            if(kind!="SDL_app"&&kind!="gdkWindowToplevel") return true;
            var title=new StringBuilder(512); GetWindowText(window,title,title.Capacity);
            if(title.ToString().Contains("-0)")||(kind=="gdkWindowToplevel"&&title.ToString().Contains("SINO"))) primary=window;
            if(IsWindowVisible(window)) visible=window;
            if(fallback==IntPtr.Zero) fallback=window;
            return true;
        },IntPtr.Zero);
        return primary!=IntPtr.Zero?primary:visible!=IntPtr.Zero?visible:fallback;
    }
    public static bool Reveal(IntPtr window) {
        if(window==IntPtr.Zero) return false;
        if(IsIconic(window)) ShowWindow(window,9);
        ShowWindow(window,5);
        SetForegroundWindow(window);
        return IsWindowVisible(window)&&!IsIconic(window);
    }
}
'@
    function Show-SinoDisplay($process) {
        $deadline = [DateTime]::UtcNow.AddSeconds(20)
        do {
            $process.Refresh()
            if ($process.HasExited) { throw 'QEMU exited before its display opened. See Data\startup.log.' }
            $window = [SinoWindow]::FindDisplay([uint32]$process.Id)
            $bootLog = Join-Path $dataRoot 'kernel.log'
            $ready = (Test-Path -LiteralPath $bootLog) -and (Select-String -LiteralPath $bootLog -Pattern 'SINO desktop ready' -Quiet)
            if ($ready -and $window -ne [IntPtr]::Zero -and [SinoWindow]::Reveal($window)) {
                # Recheck after the display has settled at the desktop's mode.
                Start-Sleep -Milliseconds 250
                $window = [SinoWindow]::FindDisplay([uint32]$process.Id)
                if (-not [SinoWindow]::Reveal($window)) { continue }
                $line = '{0:u} PID={1} HWND={2} visible=True minimized=False' -f [DateTime]::UtcNow,$process.Id,$window.ToInt64()
                Add-Content -LiteralPath (Join-Path $dataRoot 'launcher.log') -Value $line
                Write-Output 'The SINO Desktop window is open.'
                return
            }
            Start-Sleep -Milliseconds 100
        } while ([DateTime]::UtcNow -lt $deadline)
        throw 'QEMU is running, but its desktop window could not be shown. See Data\startup.log.'
    }
    $active = Get-CimInstance Win32_Process -Filter "Name = 'qemu-system-i386.exe'" |
        Where-Object { $_.ExecutablePath -eq $oldQemuExe -and $_.CommandLine -match ('127\.0\.0\.1:'+$port) } | Select-Object -First 1
    if (-not $active) {
        $active = Get-CimInstance Win32_Process -Filter "Name = 'qemu-system-i386w.exe'" |
            Where-Object { $_.ExecutablePath -eq $qemuExe -and $_.CommandLine -match ('127\.0\.0\.1:'+$port) } | Select-Object -First 1
    }
    if ($active) {
        $existing = Get-Process -Id $active.ProcessId
        # MainWindowHandle excludes hidden windows. Enumerate owned GUI windows.
        Show-SinoDisplay $existing
        exit 0
    }
    $otherPort=if($Install){4585}else{4586}
    $other=Get-CimInstance Win32_Process -Filter "Name = 'qemu-system-i386w.exe'" | Where-Object { $_.ExecutablePath -eq $qemuExe -and $_.CommandLine -match ('127\.0\.0\.1:'+$otherPort) -and $_.CommandLine -match 'system\.img' } | Select-Object -First 1
    if($other -and ($Install -or $installed)) {
        if($Install){throw 'The installed system is running. Close its VM window before starting setup.'}
        $dataRoot=Join-Path $baseData 'Setup'
        Show-SinoDisplay (Get-Process -Id $other.ProcessId)
        exit 0
    }
    New-Item -ItemType Directory -Path $dataRoot -Force | Out-Null
    $dataDisk = if($Install -or $installed){$systemDisk}else{Join-Path $dataRoot 'data.img'}
    if (-not (Test-Path -LiteralPath $dataDisk)) {
        $stream = [IO.File]::Open($dataDisk,[IO.FileMode]::CreateNew)
        try { $stream.SetLength($(if($Install){134217728}else{16777216})) } finally { $stream.Dispose() }
    }
    # QEMU receives ASCII relative paths, so spaces in the release path remain safe.
    $diskFile=if($Install){'../system.img'}elseif($installed){'system.img'}else{'data.img'}
    $dataUri=New-Object Uri ([IO.Path]::GetFullPath($dataRoot)+'\')
    $shareUri=New-Object Uri ([IO.Path]::GetFullPath($qemuShare)+'\')
    $sharePath=if($dataUri.Scheme-eq $shareUri.Scheme-and $dataUri.Host-eq $shareUri.Host-and $dataUri.Segments[1]-eq $shareUri.Segments[1]){[Uri]::UnescapeDataString($dataUri.MakeRelativeUri($shareUri).ToString())}else{$qemuShare}
    $arguments = @('-L',('"'+$sharePath+'"'),'-name',('"'+$vmName+'"'),
        '-machine','pc,vmport=on','-accel','tcg','-m','64M','-vga','std','-nic','none',
        '-drive',('file='+$diskFile+',format=raw,if=ide,index=0'))
    if(-not $installed){$arguments+=@('-drive',('file='+$runtimePrefix+'SINO.iso,format=raw,media=cdrom,if=ide,index=2,readonly=on'))}
    $bootOrder=if($Install){'order=c,once=d'}elseif($installed){'c'}else{'d'}
    $arguments+=@('-boot',$bootOrder,'-rtc','base=localtime','-display','gtk,show-menubar=off,show-cursor=off,zoom-to-fit=off',
        '-debugcon','file:kernel.log','-global','isa-debugcon.iobase=0xe9',
        '-qmp',('tcp:127.0.0.1:'+$port+',server=on,wait=off'))
    # The official GUI executable needs no console. Hidden also hides its display.
    $process = Start-Process -FilePath $qemuExe -ArgumentList $arguments -WorkingDirectory $dataRoot `
        -WindowStyle Normal -RedirectStandardError (Join-Path $dataRoot 'startup.log') -PassThru
    Show-SinoDisplay $process
} catch {
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'SINO - Startup failed') | Out-Null
    exit 1
}
