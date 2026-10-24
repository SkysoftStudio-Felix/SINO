param([switch]$TestImage)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build32.ps1') -TestImage:$TestImage
