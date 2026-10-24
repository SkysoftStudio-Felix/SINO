# SysBASIC 0.2: Classic Workbench and Selectable Build Modes

This report records version 0.2. Current right-side interaction and blank-project fixes are in the [0.2.1 report](REPORT-IDE-0.2.1.md).

Date: 2026-10-24. The existing SINO 0.10 codebase was modified and deployed. The boot chain, desktop, BASIC interpreter, and user-mode application model were preserved. The toolchain was not changed and Microsoft VB6 assets were not imported.

The screenshots in Documentation/Screenshots show the application with the Chinese system locale.

## How to use

Double-click Start SINO.cmd in Compiled Runtime. Press F11 or open SysBASIC in the desktop Applications view. Version 0.2 opens with the Build Program chooser already available.

Build... on the toolbar provides two outputs:

| Output | Result | Behavior |
| --- | --- | --- |
| Executable | Project-name .snx | A single file embeds the form and code and runs from any folder; it is not registered as installed software. |
| Installer | Project-name .sxp | Opens the separate setup wizard; after confirmation and completion it appears in the software library and can be launched, repaired, or removed. |

Generate and Open can be cleared to create only the file. An installer package never installs silently by default; files are copied and registered only after the user confirms in the wizard. The installer retains welcome, license, location, components, progress, and completion pages.

Design flow: select a toolbox control, place it on the form, edit a property and press Enter, double-click the control to write an event, press F5 to run, press Esc to stop, then build. F8 steps. Ctrl+S saves; Ctrl+O opens a .prj; File includes Save As. Existing Hello.prj loads at startup. New projects and Save As to an existing name require overwrite confirmation; cancel preserves the original file.

The software ID is derived from the 1–18 character English project filename, for example Hello.prj -> basic-hello. Use a different project filename for each app. Version is fixed at 0.2.0. Existing native packages can be repaired/reinstalled under the same ID. Older basic-demo registrations remain and are not deleted automatically.

## Interface and implementation

- Menus/toolbars, toolbox on the left, form/code child windows in the center, project tree/property table on the right, and output/watch/form-layout panes at the bottom; classic gray raised controls and a grid.
- Retains movement, lower-right resizing, property validation, Chinese Pinyin input, and event generation. Adds code select-all/clipboard, single-level undo, horizontal tracking, unsaved-close prompts, and working menu actions.
- F5/F8 are handled inside the IDE; other desktop function keys remain available. Continuing from a breakpoint no longer immediately hits the same breakpoint again.
- The IDE no longer links installation transactions. It builds ordinary SNX1/SXP1 data and launches the separate user-mode setup app. BASIC resources are placed in the initialization data segment; no kernel execution format or permission was added.
- The user-mode canvas supports 256 range-checked drawing commands; older ABI 1 clients with 64 commands remain supported. Closing a window releases the whole canvas. The kernel has no project, compile, or install policy.
- The desktop Applications view merges menu shortcuts registered by the install service and displays friendly names. F9 shows names/versions and supports scrolling. Legacy one-line path shortcuts remain valid.
- The installer handles close messages during its progress phase; the desktop no longer kills the process directly. Tests wait for transaction completion instead of inferring it from temporary registration files.

## Verification

All 15 regression groups passed: ISO, self-test, legacy desktop, recovery, performance, compatibility, fonts, Chinese files, settings, OS installation, user mode, packages, networking, IDE, and native applications. Windows cold start, hidden/minimized-window restore, duplicate-instance prevention, installation entry, and no-ISO hard-disk boot also passed.

The IDE regression runs the app from the software library; moves/resizes controls; edits Chinese properties; previews Click events; tests breakpoints, step, and continue; builds both output types; moves the single-file output to an empty folder and runs it; verifies that an .sxp is unregistered before installation, launches after wizard completion, and remains uninstalled after removal and reboot; and checks save/reopen, unsaved cancel/discard, and overwrite-cancel preservation.

Three independent copies of the populated daily-use disk also passed: release-project-282d448b, release-project-a022cf6f, and release-project-7e212d32. Each upgraded the IDE, created a randomly named QA project, built and ran a standalone Click event, built an installer, completed GUI setup, and checked persistent registration. The three final runs had no SYS_SYNC error or kernel panic.

An earlier copy had a disk-commit failure at InstallBegin; the original file was preserved. ATA idle/completion waiting was improved (both BSY and DRQ must clear), SDK retries S_IO commits at most three times, and kernel error logging was added. Passing retries do not prove the unique cause of the first failure. Longer cold-disk and stress tests remain useful. Another test was blocked by overwrite protection on an existing Untitled project; QA now uses random names and does not bypass the product confirmation.

The settings regression's fixed 17-second host wait was insufficient with parallel VMs. It now waits up to 45 seconds for the actual restored state; the system's 15-second virtual-time limit is unchanged. A resumed test run only reuses passing results when the ISO, test, and shared dependency signatures are identical.

Build and validation:

    powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage
    node build-iso.js
    node verify-system.js
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-launcher.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-setup-entry.ps1

Final ISO: 1,572,864 bytes; SHA-256 c2e286659150c06be99d597d15d35aaecfc876e7f2718d72f4929bbed8144649. Kernel: 131,840 bytes. IDE image: 41,472 bytes; image/static data: 182,944 bytes. Native-app limit: 196,608 bytes.

## Deployment and rollback

The boot partition was updated and checked on a private copy first. The daily-use disk was backed up before boot resources were written. MBR, saved preferences, and data partition remained unchanged during the boot update; all 66 existing records were restored and hash-verified.

SysBASIC was then upgraded through the separate installer. The files/registrations/logs for seven packages changed as expected; the other 59 records, including existing projects and settings, matched their prior content and directory checks. Data/data.img was not modified. The actual system booted from its hard disk with no ISO attached and had no leftover transaction log.

The full pre-upgrade disk backup is retained outside this release. To roll back, close the corresponding QEMU instance, back up the current disk, and restore the matching complete backup to Data/system.img. This returns to the backup's point in time; export any newer files first. Git tags sino-0.9-baseline and sino-0.10 preserve platform baselines; this release stage is tagged sysbasic-0.2.

Evidence includes build32/system-verification.json, build32/host-verification.json, build32/release-project-verification.json, and the release verification records.

## Explicit boundaries

This is not full VB6. It cannot open VB projects or use Win32/COM/ActiveX, and .snx/.sxp are not PE/MSI files for Windows. The direct-run output embeds the BASIC interpreter and project; it does not compile BASIC into new x86 instructions.

The supported subset and unfinished items are listed in ide.md: Function/parameters/local scope, multiple forms, full menus, PictureBox, and a complete debugger are unfinished. Only Hello/counter examples are delivered; five complete examples are not claimed. The tool needs a 1024×768 logical desktop and does not yet adapt its layout. Projects/runtime/packages remain subject to the platform's 64 KiB per-file and 192 KiB image limits. The new installer and IDE run in user mode, while some legacy built-in applications remain in kernel mode. External networking retains the unfinished status in the platform report.

## Changed areas

This stage changed the release guide, README/SOURCES, platform and IDE reports, application-development/package/architecture/installer/roadmap/stage/testing docs, SysBASIC package metadata, system API/disk/GUI/process code, build/release/test tools, native applications, and verification records. The full file-level history is retained in Git.
