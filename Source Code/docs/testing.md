# Build, execution, and regression testing

Host: existing w64devkit GCC/Binutils (override with SINO_TOOLCHAIN), bundled NASM 3.02, and Node.js. No new toolchain or large library is required. Python/Pillow are used only for resource generation and screenshots. Guest: 32-bit x86, 64 MiB RAM, standard VGA/VBE, PS/2, BIOS/MBR, and ATA IDE. Current QEMU runs with networking disabled; the guest loopback stack is independent.

    powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage
    node build-iso.js
    node verify-system.js
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-launcher.ps1
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/test-setup-entry.ps1

The first command must succeed before building an ISO, so a stale image is not tested. Build checks enforce a flat kernel of at most 159,744 bytes, BSS below reserved page tables, user files no larger than 64 KiB, and image/data limits of 192 KiB. PE/COFF is only an intermediate host link format. build-release.ps1 builds the normal release and ISO without a test image.

verify-system.js runs the original 10-suite desktop gate plus user-mode isolation, installer, loopback network, IDE, and native application tests. QEMU instances use separate disk images and dynamically allocated local QMP ports. Core regressions do not open Data/system.img or Data/data.img. GUI input uses actual keyboard/mouse events; assertions inspect guest memory and CRC-valid persisted snapshots. Screenshots are real QEMU framebuffers, not mockups. A passing test exits zero; failed assertions are not counted as success.

Coverage includes original GUI boot/apps, pointer, cached dragging, no-SSE2 fallback, file/clipboard/undo, snapshot-corruption recovery, Chinese/Pinyin and drag/drop, settings/mode timeout/1920 display, OS partition planning and exact system-sector readback, CD-free installed boot; CPL3 memory/I/O/opcode faults, spinning-process preemption, thread events, handle rights/references/stale reuse, private-resource reclamation; install/upgrade/downgrade/repair/uninstall/rollback/reboot recovery; packet checksum/loss/duplicate/window/bounds and UDP/TCP/ICMP/DNS/HTTP local tests; visual designer, breakpoints/step, packaged independent BASIC app, reboot project load; and native editor/file/task operations.

Artifacts: build32/manifest-normal.json, manifest-test.json, system-verification.json, and vm-tests/*/report.json plus PNGs. Host launcher/setup results are under build32/launcher-qa and setup-entry-qa. Current release records and full-disk backup information are under the neighboring Documentation/Licenses folder and external SINO archive. Git stage reports document checks performed at each commit.

Limitations: no hardware-in-the-loop, external NIC/interoperability tests, security certification, or comprehensive malformed-package/BASIC fuzz campaign. The UI is a hybrid migration; crashes in remaining Ring 0 legacy applications can still affect the kernel. Snapshot storage has bounded catalogs/files and no power-loss hardware simulator beyond interrupted-transaction/reboot tests. Package CRCs detect corruption, not signatures.

## SysBASIC 0.2 / 2026-10-24

node tools/test-ide.js launches the IDE from the desktop software library and validates control movement/resizing, Chinese properties, event generation, F5 preview, F8 breakpoint/step/continue, and a single-file .snx. It moves the output to an empty directory and runs it. For .sxp it asserts that the application is not registered before installation, installs through the GUI, launches it from the software entry, uninstalls it, then reboots and verifies it remains absent with no recovery log. It also tests project save/reopen and unsaved-close protection. Screenshots are in build32/vm-tests/ide-qa.

Ring 3 probes submit both the original 64-command scene and a 256-command scene. They check rejection of oversized counts, invalid user pointers, and panel bounds while retaining reclamation/preemption regressions. SYS_SCENE validates the count before the full command range and does not require older clients to know the newer canvas size.

SysBASIC 0.2 release addendum: copies of the daily-use disk use random QA project names so an existing Untitled.prj is not blocked by overwrite protection. One InstallBegin disk-commit failure was observed; the original file remained intact and the log was recoverable. PIO idle waits now check both BSY=0 and DRQ=0. SDK Sync retries S_IO at most three times and yields; it returns success only after the kernel confirms the commit. SYS_SYNC records failure reasons in the kernel log. Passing retries do not establish the unique cause of the original failure; longer cold-disk/stress stability testing remains necessary.

## SysBASIC 0.3 release validation (2026-10-24)

Build with powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage, then node build-iso.js. The full node verify-system.js run now contains 18 suites: the original 15 plus keyboard decoding, real key input/designer, and right-panel tests. Every failing suite exits nonzero. --resume reuses a passing result only when ISO, test scripts, and shared test-dependency signatures match.

SINO_TOOLCHAIN selects GCC/Binutils bin; SINO_QEMU_DIR selects the QEMU directory; SINO_PYTHON selects Python with Pillow; SINO_ISO overrides the test ISO path. The three-folder release layout automatically finds ../Compiled Runtime/SINO.iso; the development layout remains supported.

node tools/prepare-release-disk.js starts with a zero-filled private 128 MiB virtual disk and uses the real OS installer and native package service to make a clean preinstalled disk. It validates the First project, preview, standalone executable, GUI installation, and cold boot without an ISO. It never reads or writes the daily Data/system.img.

Run node tools/package-release.js <new absolute output directory> only after all 18 regression suites and the clean release disk pass. The exporter checks ISO/IDE/disk hashes and writes exactly three folders; it refuses to overwrite an existing directory. The delivery excludes personal files, Git, build32, old test disks, and third-party QEMU binaries. The SHA256 manifest's disk hash refers to the pristine pre-first-boot release state.

The release launcher is also tested through Windows CMD → PowerShell → visible GTK window, repeated launch/restore, F11 IDE, First execution, and actual keyboard input. Tests operate on private copies; the pristine delivery disk stays sealed.

## Stage 07: SysBASIC 0.2 workbench and two output modes

Completed: classic feature layout, directly runnable output, separate GUI-installer output, friendly software-library names, project-overwrite/close protection, and installer close protection. The BASIC interpreter and existing platform were extended incrementally; this does not claim VB6 compatibility.

All 15 regression groups, Windows startup/install entry, three populated disk copies, and actual deployment passed. Investigation notes, resource limits, rollback backups, build commands, and the full file list are in REPORT-IDE-0.2.md.

## Stage 08: interaction fixes

New projects are blank. The right-side tree, properties, object/event selectors, and operation panel have real behavior. Dedicated click/persistence checks, all 15 regression groups, populated disk copies, and the actual release passed. See REPORT-IDE-0.2.1.md.
