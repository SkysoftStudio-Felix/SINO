# SINO 0.10 Platform Delivery Report (2026-10-18)

Later work upgraded SysBASIC to 0.3 on 2026-10-24. For the current IDE UI, build modes, and verification, see [REPORT-IDE-0.3.md](REPORT-IDE-0.3.md). This report preserves the platform's initial delivery record.

The existing C/assembly system was extended while retaining its BIOS/MBR boot chain, Chinese graphical desktop, storage, and settings. The result is a runnable development build. The full Windows-style system and a full VB6-scale IDE have not met all requested acceptance criteria.

## Implemented and tested

- Ring 3: private page tables, read-only code, protected kernel, TSS, guard page for user stacks, PIT preemption, process termination, and fault cleanup. The limits are eight processes and four threads per process. Attempts by user programs to write kernel memory, perform privileged port I/O, execute UD2, and spin forever were tested. Older built-in applications remain in kernel mode.
- Shared API: unified negative error codes; typed, permissioned, reference-counted handles; per-handle cookies to prevent stale-handle reuse; reduced-rights duplication; user-pointer validation; and file/directory/message/window/thread/event/clipboard/socket primitives.
- Independent software installer: user/setup.c is a separate user-mode GUI with welcome, license, path, components, progress, and completion pages. install_service.c provides InstallBegin/WriteFile/SetConfig/CreateShortcut/RegisterUninstall/Commit/Rollback. Native SXP1 includes ID, version, architecture, entry, file CRC, configuration, shortcut, association, uninstall data, ABI dependency, and reserved signature field.
- Install transactions: file copy and configuration registration, desktop and menu shortcuts, upgrade, downgrade rejection, repair, uninstall, quiet install, logging, failed-install rollback, and recovery of interrupted journals at reboot. Path bounds, traversal, and takeover of existing files are checked. Unsupported service, permission-grant, or signature-verification requests are explicitly rejected.
- Native applications: independent user-mode application manager, Files, Editor, task/process viewer, and Hello. Native Files supports context-menu creation, directories, rename/delete, in-directory drag, and opening Editor. Editor supports Chinese Pinyin, UTF-8, clipboard, undo, save, and unsaved-close prompts. The trusted user-mode F10 task manager can terminate a spinning process; ordinary installed viewers have no implicit management rights.
- Loopback networking: IPv4 packet queue, ICMP, UDP, simplified TCP, socket handles, and nonblocking/timeout wrappers; user-mode DNS A and HTTP/1.0 client/server. Tests cover dropped SYN/ACK, duplicate data, checksums, receive-window exhaustion/recovery, EOF, retransmission, and resource cleanup.
- SysBASIC 0.1: the IDE itself installs as an application package. It includes a toolbox, control move/resize/grid, properties, event generation, UTF-8 code editor, partial syntax coloring, run/stop, breakpoints/step/watch, plain-text .prj/.frm/.bas projects, and project reload after reboot. It can package the runtime and project, install transactionally, and launch an independent application from the manager.
- Deployment: the daily Data/system.img was upgraded and six original directory/file records remained hash-consistent. Five native packages were installed, and the installed SysBASIC was launched in the real VM. The original Data/data.img was preserved.

## Incomplete areas and limits

These are follow-up engineering work, not placeholder features reported as complete:

1. Older Files/TextEdit, settings, terminal, calculator, Paint, Clock, and the OS disk installer remain trusted kernel-mode built-ins. Not all application crashes are isolated from the kernel. New native applications do have isolation.
2. There is no unified device-object/IRP layer, full mount-based VFS, per-user ACL, general blocking synchronization, service/startup execution, notification service, or update service. The new configuration store is a persistent key/value file in the user directory; older settings files remain.
3. External networking is not implemented: no Ethernet/ARP, virtual or physical NIC driver, static external routing, DHCP, public DNS, or public HTTP. QEMU is launched with -nic none. TCP is a bounded loopback stop-and-wait subset, not an Internet-grade stack.
4. Packages have permission/service/signature fields, but the system does not grant network capabilities, run services, or verify signatures. Nonzero signatures and unsupported permissions are rejected. The installer is currently a shared user-mode library serialized by a lease, not a resident IPC daemon; concurrent transaction visibility is not fully isolated.
5. SysBASIC has unfinished user Functions/return values, Sub parameters, local scope, string/Variant arrays, full menus, PictureBox, Now/Date, multiple forms, a complete debugger, and five complete example projects. Hello and counter examples are delivered; this is not full VB6. Generated packages currently use a fixed basic-demo ID. Network bindings exist, but generated apps have no network token.
6. Native UI is primarily Chinese and not fully connected to the global language switch. Some layouts are fixed; the full IDE recommends at least 1024×768 logical space. Native Files lacks cross-window drag, Trash, and full copy workflows; the older desktop retains the validated versions.
7. Resources are bounded: 64 MiB RAM, 128 VFS nodes, 64 KiB per file/package, about 4 MiB usable persistent snapshot space, 192 KiB native image/static data, 15 queued input messages, and eight desktop windows. The IDE is near the image/data limits; controlled user heap or smaller buffers should precede expansion.

No credentials or external hardware decision blocked this delivery. Remaining work is part of continued development; see [roadmap.md](roadmap.md).

## Build, tests, and results

    powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage
    node build-iso.js
    node verify-system.js
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-launcher.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-setup-entry.ps1

The project uses the existing GCC/Binutils, NASM, Node.js, and Python/Pillow for screenshots. No toolchain replacement or large third-party kernel/GUI/network library was introduced. Set SINO_TOOLCHAIN to the GCC bin directory if needed.

All 15 regression suites passed on the final ISO: legacy desktop/self-test/recovery/performance/compatibility/fonts/Chinese/settings/OS installation, plus user mode/packages/network/IDE/native applications. Windows GTK cold start, hidden/minimized restore, duplicate-instance prevention, installer entry, GUI restart, and hard-disk boot without an ISO also passed. Before upgrading the daily disk, a private copy was booted, its file hashes checked, and the Ring 3 probe run.

A timing issue where a settings-confirmation flag preceded durable persistence was fixed. Mouse regression now waits for the actual release event instead of guessing with a short delay. Tests use private images; production update, backup, and software installation have separate validation records.

- Normal kernel: 130,464 bytes; BSS end address: 0x1cb980.
- ISO: 1,572,864 bytes, ISO9660 / El Torito BIOS.
- ISO SHA-256: 174576c8c35cdf4c0077f0f7c022e0cec0e74ee6727b34699de3655a01797f9f.
- Full verification records: build32/system-verification.json and vm-tests/*/report.json plus PNG files.
- Release/production-disk evidence: release verification records under Documentation/Licenses.
- Screenshots are actual QEMU framebuffers; mouse-performance numbers do not represent a physical display refresh rate.
- A separate copy of the populated daily-use disk validated default-project build, installation, and an independent Click event. The first extra run did not show a save-completion marker; two retries passed. The first run created project files, but its cause is unknown. The failed log was retained. Cold-cache and disk-stress persistence tests are still needed; this was not marked as fixed.

## Run and install

Double-click Start SINO.cmd in Compiled Runtime. The preinstalled virtual disk boots directly and no longer depends on an ISO. Shortcuts: F7 legacy system settings, F8 separate software install, F9 application manager, F10 native task manager, F11 SysBASIC.

Five packages are installed: sysbasic, files, editor, tasks, hello. On a fresh system, run these commands in Terminal:

    install /boot/hello.sxp
    install /quiet:/boot/editor.sxp
    install /quiet:/boot/files.sxp
    install /quiet:/boot/tasks.sxp
    install /quiet:/boot/sysbasic.sxp
    nettest

You can also double-click .sxp files in /boot using the legacy F1 desktop file manager. F9 can launch, repair, and uninstall applications. See installer.md and app-package.md for quiet-install arguments and the package format. OS installation uses Install SINO.cmd and supports BIOS/MBR/IDE, not UEFI/GPT.

## Build a first program in the IDE

Press F11 to open the installed IDE. The default form has Label1 and Button1; clicking the button counts. Stop, drag controls from the toolbox, edit Caption/Name, and double-click a control to create its event. For example:

    Sub Button1_Click()
      Label1.Caption = "Hello SINO"
      Print Label1.Caption
    End Sub

Save Hello.prj/Hello.frm/Hello.bas. Build Installer creates Hello.sxp and installs it as basic-demo. Launch basic-demo.lnk from the desktop or use F9 to run the independent user-mode application. Set a breakpoint by clicking a line number; Step executes a statement and displays variables. See ide.md for language, controls, events, APIs, and unsupported syntax; see app-dev-guide.md for C applications.

## Stages and rollback

| Stage | Git commit |
| --- | --- |
| Original 0.9 baseline | 1e15950 |
| User mode/handles/preemption/API/message GUI | 852eb59 |
| Independent transactional installer/manager | 39d2164 |
| Loopback stack/socket/DNS/HTTP | f526640 |
| SysBASIC prototype and native build/install chain | cff8849 |
| Native file/editor/task apps and handle hardening | e917188 |
| 0.10 release regression, fixes, upgrade, and docs | Release commit containing this report |

A full disk backup was made before upgrade. During the boot-resource update, MBR, preference sectors, and the data partition remained byte-identical; later package installs extended the data volume through normal transactions. Each source stage can be rebuilt from its corresponding commit. To roll back, use a matching full-disk backup. Before restoring an older system, export files created afterward because older kernel limits may make them incompatible.

## Code provenance

The incremental C/assembly kernel, installer, user runtime, native applications, protocol stack, BASIC runtime, and IDE were written for the existing project. No Windows, VB6, FreeDOS, or Linux source code, binaries, or artwork were copied. CPU/protocol implementation follows Intel SDM and public RFCs. QEMU, firmware, NASM/GCC tools, fonts, and Pinyin data are third-party components with their original licenses retained. See [SOURCES.md](../SOURCES.md). SINO is not DOS/Win32 binary compatible.
