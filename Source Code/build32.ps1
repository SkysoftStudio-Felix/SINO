param([switch]$TestImage)
$ErrorActionPreference = 'Stop'
$taskRoot = $PSScriptRoot
$nasmPath = Join-Path $taskRoot 'tools\nasm-3.02\nasm.exe'
$toolRoot = $env:SINO_TOOLCHAIN
if (-not $toolRoot) {
    $gccCommand = Get-Command gcc -ErrorAction SilentlyContinue
    if ($gccCommand) { $toolRoot = Split-Path -Parent $gccCommand.Source }
    elseif (Test-Path -LiteralPath 'Z:\LunaMakerDebug\w64devkit\bin\gcc.exe') { $toolRoot = 'Z:\LunaMakerDebug\w64devkit\bin' }
    else { throw 'Set SINO_TOOLCHAIN to the bin folder of w64devkit / MinGW GCC with 32-bit code generation.' }
}
$gcc = Join-Path $toolRoot 'gcc.exe'
$ld = Join-Path $toolRoot 'ld.exe'
$objcopy = Join-Path $toolRoot 'objcopy.exe'
$env:SINO_TOOLCHAIN = $toolRoot
$buildPath = Join-Path $taskRoot 'build32'
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
$sourcePath = Join-Path $taskRoot 'src32'
foreach ($source in @('boot','loader','hello','mbr')) {
    & $nasmPath -f bin -Wall -Werror -w-reloc-abs-word -w-reloc-abs-dword (Join-Path $sourcePath "$source.asm") -o (Join-Path $buildPath "$source.bin") -l (Join-Path $buildPath "$source.lst")
    if ($LASTEXITCODE -ne 0) { throw "Assembly failed: $source" }
}
# COFF relocations are resolved by the linker below.
& $nasmPath -f win32 -Wall -Werror -w-reloc-abs-dword -w-reloc-rel-dword (Join-Path $sourcePath 'entry.asm') -o (Join-Path $buildPath 'entry.o')
if ($LASTEXITCODE -ne 0) { throw '32-bit entry assembly failed' }
& node (Join-Path $taskRoot 'tools/build-user.js')
if ($LASTEXITCODE -ne 0) { throw 'User application build failed' }
$variants = @('normal')
if ($TestImage) { $variants += 'test' }
foreach ($variant in $variants) {
    # The kernel owns its mapped stack; Windows stack-guard probing is inapplicable.
    $cflags = @('-m32','-march=i386','-std=c11','-Os','-ffreestanding','-fno-builtin','-fno-pic','-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables','-mno-stack-arg-probe','-mno-sse','-mno-mmx','-msoft-float','-mpreferred-stack-boundary=2','-mincoming-stack-boundary=2','-Wall','-Wextra','-Werror','-Wno-misleading-indentation')
    if ($variant -eq 'test') { $cflags += '-DSELFTEST=1' }
    foreach ($source in @('kernel','keyboard','object','process','net','console','vfs','shell','storage','disk','settings','install','graphics','gui')) {
        & $gcc @cflags -c (Join-Path $sourcePath "$source.c") -o (Join-Path $buildPath "$source-$variant.o")
        if ($LASTEXITCODE -ne 0) { throw "C compilation failed: $source ($variant)" }
    }
    & $ld -m i386pe --image-base 0x10000 --file-alignment 0x200 --section-alignment 0x1000 --disable-reloc-section --disable-dynamicbase --no-insert-timestamp -T (Join-Path $sourcePath 'linker.ld') (Join-Path $buildPath 'entry.o') (Join-Path $buildPath "kernel-$variant.o") (Join-Path $buildPath "keyboard-$variant.o") (Join-Path $buildPath "object-$variant.o") (Join-Path $buildPath "process-$variant.o") (Join-Path $buildPath "net-$variant.o") (Join-Path $buildPath "console-$variant.o") (Join-Path $buildPath "vfs-$variant.o") (Join-Path $buildPath "shell-$variant.o") (Join-Path $buildPath "storage-$variant.o") (Join-Path $buildPath "disk-$variant.o") (Join-Path $buildPath "settings-$variant.o") (Join-Path $buildPath "install-$variant.o") (Join-Path $buildPath "graphics-$variant.o") (Join-Path $buildPath "gui-$variant.o") -Map (Join-Path $buildPath "kernel-$variant.map") -o (Join-Path $buildPath "kernel-$variant.exe")
    if ($LASTEXITCODE -ne 0) { throw '32-bit kernel link failed' }
    & $objcopy -O binary (Join-Path $buildPath "kernel-$variant.exe") (Join-Path $buildPath "kernel-$variant.bin")
    if ($LASTEXITCODE -ne 0) { throw 'Flat kernel extraction failed' }
    & node (Join-Path $taskRoot 'build-image32.js') $variant
    if ($LASTEXITCODE -ne 0) { throw '32-bit floppy image creation failed' }
}
