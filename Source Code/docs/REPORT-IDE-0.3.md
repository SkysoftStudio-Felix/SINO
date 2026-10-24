# SysBASIC 0.3: Keyboard, Form Designer, and Release Example

2026-10-24. Incremental fixes were made on the existing C/assembly system. BIOS boot, desktop, and native APIs were retained; the toolchain was not replaced.

## Implemented

Keyboard: extracted a pure PS/2 Set 1 decoder from kernel.c into keyboard.c/.h. Fixed shared left/right Shift state and interference from extended key sequences; completed keypad and lock-key handling. A fixed US physical layout is used while system Pinyin remains available. Underscore and common programming punctuation were entered with real key events, saved in .bas, and executed with output verified.

Designer: after selecting a tool, press/drag/release in the form to create a rectangle. Supports live outline, reverse-direction drawing, default-size shortcut, and an 8-pixel grid. All eight visible handles resize their controls with minimum-size and form-boundary constraints. Release coordinates are processed so a fast drag cannot miss its final position. Esc restores geometry and the original modified-state flag. Clicking a handle without dragging does not change the size.

Example: disk32/FIRST.PRJ/.FRM/.BAS demonstrates Result_Label, Run_Button, and my_value with underscores. The preinstalled release disk was built from a zero-filled new disk through the actual OS installer and software installer, and contains five native packages plus First installed through the GUI. First.snx and First.sxp are also included separately.

Distribution: three folders for source, compiled runtime, and documentation; launcher finds external QEMU; source has configurable tool paths, build, and regression scripts. Licenses and limitations are retained; a SHA-256 manifest is generated. The development repository, daily-use disk, and old running VMs were preserved and are not part of the public archive.

## Verification and rollback

build.ps1 -TestImage plus build-iso.js, all 15 existing regression suites, and test-keyboard, test-ide-input, and test-ide-panels were run (18 suites total). prepare-release-disk.js separately tested a new-disk install, native software packages, First in both output modes, and cold boot with no ISO. The Windows launcher was exercised on a copy of the release disk: visible window, restore after re-entry, and IDE startup.

Every test passed. The release summary records ISO, IDE, clean-disk hashes, and source commit; the complete file manifest is in Documentation. The prior source state is recoverable from Git tag sysbasic-0.2.1. The release directory is a new copy and did not migrate or clean user files.

## Unfinished

See Release Notes.md, ide.md, and network.md. BASIC is still an interpreter subset, not a VB6 compiler. Multiple forms, parameterized procedures/user Functions, local scope, PictureBox, full menus, multi-select/alignment, a complete debugger, and network permission delegation are unfinished. There is no general physical-hardware or public-network support. This release is a development preview.
