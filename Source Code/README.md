# SINO 0.10 Development Preview / SysBASIC 0.3.0

This release incrementally extends the existing 32-bit C/assembly system. It preserves the BIOS/MBR boot path, Chinese desktop, and data volume while adding Ring 3 processes and threads, controlled system calls, native application packages and a transactional installer, a loopback network stack, and a packable SysBASIC prototype.

For the three-folder release, first read ../Documentation/Getting Started.txt. Normal use requires only the neighboring Compiled Runtime folder and a separately installed Windows QEMU. The clean preinstalled disk already includes the IDE and First example. build-release.ps1 writes SINO.iso to the adjacent Compiled Runtime folder. BASIC remains a documented subset, not a VB6-compatible compiler.

Technical references:
- IDE and build modes: docs/REPORT-IDE-0.3.md
- Platform report: docs/REPORT.md
- Architecture: docs/architecture.md
- Roadmap: docs/roadmap.md
- Build and tests: docs/testing.md
- Application development: docs/app-dev-guide.md
- Installer: docs/installer.md
- Network: docs/network.md
- SysBASIC language: docs/ide.md

Build and verify:

    powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage
    node build-iso.js
    node verify-system.js

Requirements include an existing GCC/Binutils toolchain, Node.js, and the included NASM. Set SINO_TOOLCHAIN to the GCC bin directory if needed. Outputs are written to the neighboring Compiled Runtime folder. Tests use private virtual disks; the normal development disk is not used. F8 opens software setup, F9 the application manager, F10 task management, and F11 SysBASIC.

## Historical build and usage notes

In this directory, run:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\build-release.ps1

This builds the current 32-bit system and updates SINO.iso in the neighboring Compiled Runtime folder. Node.js and a GCC/Binutils toolchain capable of producing 32-bit output are required. NASM and the current font resources are included with the source. Set SINO_TOOLCHAIN on other computers; the former development-machine path is not portable.

Regenerate bitmap fonts and pointer resources with Python and Pillow:

    python tools\build-ui-assets.py

Full validation:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -TestImage
    node .\build-iso.js
    node .\verify-desktop.js

For normal desktop validation, boot from ISO without attaching a physical floppy disk; kernel self-tests use a separate test image. Temporary test images and reports are under build32 and do not modify the normal Data\data.img.

Source layout:
- src32: final kernel, desktop, drivers, and system application source.
- disk32: boot-disk contents and current font assets.
- tools: build, font provenance, and automated validation tools.
- build-iso.js: ISO 9660 / El Torito BIOS optical-disc builder.

The ISO uses a standard El Torito 1.44 MB floppy-emulation boot entry while retaining a real ISO 9660 filesystem. The existing bootloader can read boot data from a virtual optical drive without modification.
Reference: Phoenix/IBM El Torito 1.0 and ECMA-119.
https://pdos.csail.mit.edu/6.828/2017/readings/boot-cdrom.pdf

See LICENSE, SOURCES.md, and the neighboring Documentation/Licenses folder for license and font provenance.

Version 0.8 added the Chinese interface, UTF-8 text and terminal, basic Pinyin input, context menus, and file drag-and-drop. Chinese resources can be regenerated from the original Unifont and pinyin-data sources included with the source. node tools/test-chinese-files.js checks Chinese input, candidate selection, drag-and-drop, context menus, and reboot persistence. The Windows launcher test checks that the GTK window actually appears and can be restored. It uses a private directory, disk, and port.

Version 0.9 added system settings and a real BIOS/MBR hard-disk installer. settings.c handles display mode, scaling, contrast, bilingual strings, persistence, and 15-second rollback. disk.c provides LBA28 sector access for four IDE ATA devices. install.c and mbr.asm implement the graphical partition/install wizard and hard-disk boot. boot.asm and loader.asm share the floppy-emulation and partition EDD paths. build-locale.py contains this project's Chinese/English UI strings; user file contents are not translated. Regression tools cover settings, installation entry, and reboot persistence.

Version 0.10 added src32/process.c, object.c, api.h, and net.c for Ring 3, threads/preemption, handles/system calls, and the loopback protocol stack. user/ contains the independent installer, common runtime, file/editor/task applications, and SysBASIC. These additions were written for this project; Windows, VB6, Linux, and FreeDOS implementations were not imported. See SOURCES.md for details.

The English archive translates human-authored documentation and distribution names. Chinese UI strings and the Pinyin dataset remain in source because they are the system's supported Chinese-language resources. Third-party license texts and data are preserved as supplied.
