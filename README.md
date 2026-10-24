# SINO

**SINO 0.10 / SysBASIC 0.3.0 — development preview**

**A small x86 desktop OS where you can design, run and install BASIC applications.**

[中文说明](README.zh-CN.md) · [Downloads](https://github.com/SkysoftStudio-Felix/SINO/releases) · [Screenshots](#screenshots) · [Contributing](CONTRIBUTING.md)

SINO is a 32-bit x86 operating-system prototype written in C and assembly, with a graphical desktop, Chinese/Pinyin input, native user-mode applications, a transactional software installer, and the SysBASIC visual programming environment. The implementation was developed with AI assistance; [code and third-party provenance](<Source Code/SOURCES.md>) is documented.

![SINO graphical desktop and text editor running in QEMU](<Documentation/Screenshots/Desktop and Editor.png>)

For hobby OS developers, retro-desktop enthusiasts and BASIC experimenters:

- **Explore the system:** a classic desktop, files, terminal, settings and bilingual/Pinyin input.
- **Create an application:** draw controls, edit properties and event code, then run it in SysBASIC.
- **Package what you build:** output a native `.snx` executable or an `.sxp` package for the independent installer.
- **Study the internals:** C/x86 assembly, protected mode, Ring 3 applications, system calls and a loopback network stack.

## Download and run

Download either the **Chinese** or **English** ZIP from [Releases](https://github.com/SkysoftStudio-Felix/SINO/releases), and extract the entire archive. Both contain the same bilingual system build; folder names and documentation differ.

[Chinese ZIP](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO-0.10-SysBASIC-0.3.0-Chinese.zip) · [English ZIP](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO-0.10-SysBASIC-0.3.0-English.zip) · [Standalone ISO](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO.iso)

- English archive: read `Documentation/Getting Started.txt`, then run `Compiled Runtime/Start SINO.cmd`.
- Chinese archive: read `介绍文件/开始使用.txt`, then run `编译后的运行文件/启动SINO.cmd`.
- Install [QEMU for Windows](https://www.qemu.org/download/#windows) separately. If necessary, set `SINO_QEMU_DIR` to the directory containing QEMU.
- The ZIP includes a clean preinstalled virtual disk. A separate `SINO.iso` is also available for BIOS/Legacy installation in a new virtual machine.

Inside SINO, F11 opens SysBASIC, F9 opens the application manager, and F8 opens software setup. Change the interface language in System Settings. The supplied First project is at `/home/user/Projects/First.prj`.

## Screenshots

Actual QEMU captures of the current desktop and applications. Most use the Chinese locale; the language-settings capture shows English. These are runtime screenshots, not mockups.

| Form designer and properties | First project running in the IDE |
| --- | --- |
| ![SysBASIC form designer with toolbox and properties](<Documentation/Screenshots/Form Designer.png>) | ![A BASIC counter project running in the IDE](<Documentation/Screenshots/First Program.png>) |
| **Independent native installer** | **Installed application manager** |
| ![Native software installation wizard](<Documentation/Screenshots/Native Installer.png>) | ![Installed applications with launch, repair and uninstall actions](<Documentation/Screenshots/Application Manager.png>) |

<details>
<summary>English interface and language settings</summary>

![SINO English language settings](<Documentation/Screenshots/Language Settings.png>)

</details>

## Build from source

Requirements: Node.js, GCC/GNU Binutils capable of generating i386 output, and the included NASM 3.02. Set `SINO_TOOLCHAIN` to your GCC/Binutils `bin` directory if the tools are not on PATH.

```powershell
Set-Location '.\Source Code'
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-release.ps1
```

Without a neighboring `Compiled Runtime` folder, the build writes `SINO.iso` to the repository root. Font regeneration additionally needs Python and Pillow; routine builds use the included resources. See [build and regression instructions](<Source Code/docs/testing.md>).

## Scope

This is a runnable learning/development prototype. It supports BIOS/MBR, IDE, standard VGA and PS/2, rather than general physical hardware or UEFI/GPT. SysBASIC implements a BASIC interpreter subset and native SINO packages; it has no VB6/Win32 binary compatibility. Networking is currently validated on loopback. New native applications use Ring 3 isolation; some legacy desktop applications still run in kernel mode.

Read the [release limitations](<Documentation/Release Notes.md>), [roadmap](<Source Code/docs/roadmap.md>), [SysBASIC reference](<Source Code/docs/ide.md>), [installer](<Source Code/docs/installer.md>), and [application guide](<Source Code/docs/app-dev-guide.md>) before extending the system.

## Get involved

Try a release in QEMU, share reproducible bugs in [Issues](https://github.com/SkysoftStudio-Felix/SINO/issues), or improve the documentation and tests. The [contribution guide](CONTRIBUTING.md) lists useful starting points and the [roadmap](<Source Code/docs/roadmap.md>) describes unfinished work.

## Repository and licenses

This repository starts with a clean snapshot of the final distribution's source and documentation. The preinstalled disk, ISO and complete release ZIPs are distributed as Release assets. The verification summary and SHA-256 manifest under `Documentation` describe the original downloadable distribution, not the Git working tree. Private development disks, personal files and old Git history are excluded.

Project code uses [MIT](LICENSE). Font, Pinyin and assembler components retain their own licenses in [Documentation/Licenses](Documentation/Licenses) and the source tree. Keep these notices when redistributing.
