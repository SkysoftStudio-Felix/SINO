param([string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
if(-not $OutputDirectory){
    $parent=Split-Path -Parent $PSScriptRoot
    $publicRuntime=Join-Path $parent 'Compiled Runtime'
    $OutputDirectory=if(Test-Path -LiteralPath $publicRuntime){$publicRuntime}else{$parent}
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
& (Join-Path $PSScriptRoot 'build.ps1')
if ($LASTEXITCODE -ne 0) { throw 'System build failed.' }
& node (Join-Path $PSScriptRoot 'build-iso.js') (Join-Path $PSScriptRoot 'sino.img') (Join-Path $OutputDirectory 'SINO.iso')
if ($LASTEXITCODE -ne 0) { throw 'ISO build failed.' }
