# SINO 0.10 / SysBASIC 0.3.0 Development Preview

This release fixes programming-key input and form-designer interaction while preserving the BIOS boot path, graphical desktop, Chinese input, native installer, and project format.

## Changes in this release

- PS/2 decoding is isolated in keyboard.c. Left and right Shift/Ctrl/Alt states are tracked independently. Extended keys no longer corrupt Shift state; keypad keys, NumLock, keypad Enter/divide, Pause, and Print Screen sequences are handled.
- The IDE was tested with underscore and common programming symbols: _ + { } : " ~ < > ? | ! @ # $ % ^ & * ( ). A BASIC identifier typed using a separate right-Shift press and release was saved and executed.
- The designer creates a control by dragging a rectangle after selecting a toolbox tool. It supports a live outline, reverse-direction drawing, grid alignment, and double-clicking a tool to add a default-size control.
- Pointer movement and all eight resize handles work with boundary constraints. Esc restores the previous geometry or cancels a new control.
- New projects start with no controls and no source code. The project tree, object/event selectors, property editing, and software-library installation entries pass regression checks.
- The release includes a First project with underscore identifiers, a standalone native program, and an installable native package.
- The release has three folders, a clean preinstalled virtual disk, a separate ISO, the source, and third-party notices.

## Current capabilities

This is a 32-bit x86 BIOS prototype with a Chinese desktop, file operations, system settings, native Ring 3 applications and installer, a BASIC-subset designer/interpreter, two native output formats, and loopback network tests. The executable embeds an interpreter and project resources; BASIC is not compiled into new x86 machine instructions.

## Known limitations and TODOs

- SysBASIC is not Visual Basic 6. It cannot open or run VB6 projects and has no PE/Win32, OCX, COM, or VB6 binary compatibility.
- The language is limited to the documented subset. User-defined Function return values, parameterized Subs, lexical local variables, complete Variant/string arrays, and related features are unfinished. Dim variables currently share module-wide storage.
- One form and one module are supported, with at most 12 controls, 8,191 source bytes, and 256 lines. Undo has one level. Partial text selection, full syntax highlighting, and a full debugger are unavailable.
- Menu is currently a button event. PictureBox image decoding, multi-select controls, alignment tools, and a complete menu designer are unfinished. Complex control combinations remain bounded by the drawing-command limit.
- The IDE needs a logical desktop of at least 1024×768. High scaling or low resolution can clip the layout. The editor uses a fixed US physical keyboard layout; Chinese input is provided by the system Pinyin method.
- Some legacy desktop applications still run in kernel mode. New native applications have isolation, but it is not accurate to claim that every application crash is isolated from the kernel.
- Networking has only been validated on loopback. There is no physical NIC driver, public DNS/DHCP, or general-purpose browser. Native packages do not yet delegate network tokens.
- BIOS/MBR, IDE, and standard VGA are supported. UEFI/GPT/Secure Boot, direct USB keyboard input, general physical hardware, modern filesystems, and large data volumes are unsupported.
- This release is for demonstration and continued development. It has not received a security audit and is not a daily-use replacement for Windows. Do not keep the only copy of important data on the experimental data volume.

## Third-party runtime

The public package does not bundle QEMU, GCC, Node.js, or Python. For Windows, see the [official QEMU download page](https://www.qemu.org/download/#windows). Release testing used the existing QEMU 11.1.0 Windows build; no external software was silently downloaded or installed.

Project provenance, font/input data, NASM, and references are described in Code and Third-Party Sources.md and Source Code/SOURCES.md. Licenses are independent; keep the full Documentation folder when redistributing the release.
