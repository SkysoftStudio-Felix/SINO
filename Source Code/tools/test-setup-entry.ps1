$ErrorActionPreference='Stop'
$sourceRoot=Split-Path -Parent $PSScriptRoot
$packageRoot=Split-Path -Parent $sourceRoot
$qaRoot=Join-Path $sourceRoot 'build32\setup-entry-qa'
New-Item -ItemType Directory -Force -Path $qaRoot | Out-Null
if(-not(Test-Path -LiteralPath (Join-Path $qaRoot 'QEMU'))){New-Item -ItemType Junction -Path (Join-Path $qaRoot 'QEMU') -Target (Join-Path $packageRoot 'QEMU') | Out-Null}
Copy-Item -LiteralPath (Join-Path $packageRoot 'SINO.iso') -Destination (Join-Path $qaRoot 'SINO.iso')
$launcher=(Get-Content -Raw -LiteralPath (Join-Path $packageRoot 'launch.ps1')).Replace('4555','4570').Replace('4556','4571')
[IO.File]::WriteAllText((Join-Path $qaRoot 'launch.ps1'),$launcher)
$priorDisk=Join-Path $qaRoot 'Data\system.img'
if(Test-Path -LiteralPath $priorDisk){
 $resolved=[IO.Path]::GetFullPath($priorDisk)
 if(-not $resolved.StartsWith([IO.Path]::GetFullPath($qaRoot)+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'QA disk escaped the test directory'}
 $preserved=Join-Path $qaRoot ('previous-system-'+(Get-Date -Format 'yyyyMMddHHmmssfff')+'.img')
 Move-Item -LiteralPath $resolved -Destination $preserved
}
function Launch([bool]$setup){
 $arguments=@('-NoLogo','-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+(Join-Path $qaRoot 'launch.ps1')+'"'))
 if($setup){$arguments+='-Install'}
 $child=Start-Process powershell.exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
 if(-not $child.WaitForExit(30000)){throw 'Setup launcher timed out'}
 if($child.ExitCode-ne 0){throw 'Setup launcher failed'}
}
Launch $true
& node (Join-Path $PSScriptRoot 'test-setup-entry.js') (Join-Path $qaRoot 'Data\Setup') 'setup'
if($LASTEXITCODE-ne 0){throw 'Installer entry validation failed'}
Start-Sleep -Milliseconds 500
$qaDisk=Join-Path $qaRoot 'Data\system.img'
$stream=[IO.File]::Open($qaDisk,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try {
 $header=New-Object byte[] 512; $null=$stream.Read($header,0,512)
 $system=New-Object byte[] 16; $data=New-Object byte[] 16
 [Array]::Copy($header,446,$system,0,16);[Array]::Copy($header,462,$data,0,16)
 [Array]::Clear($header,446,64);[Array]::Copy($data,0,$header,446,16);[Array]::Copy($system,0,$header,478,16)
 $stream.Position=0;$stream.Write($header,0,512);$stream.Flush()
} finally {$stream.Dispose()}
$isoPath=[IO.Path]::GetFullPath((Join-Path $qaRoot 'SINO.iso'))
if(-not $isoPath.StartsWith([IO.Path]::GetFullPath($qaRoot)+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'QA media escaped its test directory'}
Move-Item -LiteralPath $isoPath -Destination (Join-Path $qaRoot ('ejected-install-media-'+(Get-Date -Format 'yyyyMMddHHmmssfff')+'.iso'))
Launch $false
& node (Join-Path $PSScriptRoot 'test-setup-entry.js') (Join-Path $qaRoot 'Data') 'installed'
if($LASTEXITCODE-ne 0){throw 'Installed entry validation failed'}
'PASS: ISO setup launcher, partitioning, copy/verify, GUI Restart, reordered MBR partition slots, normal hard disk boot with ISO file removed.' | Set-Content (Join-Path $qaRoot 'report.txt')
