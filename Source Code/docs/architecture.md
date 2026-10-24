# SINO architecture — native ABI 1

## Decision (2026-10-18)
Keep the BIOS/MBR boot chain, custom SINO kernel, VFS and Platinum compositor. Add Windows-like internal boundaries and an original native ABI. No Win32/PE application compatibility. GCC's PE/COFF intermediate is only a host link format; the guest loads SNX1 flat executables.

## Baseline audit
SINO 0.9 has no process isolation, scheduler, package installer, socket stack or IDE. Eight legacy applications and the OS disk installer execute in cooperative ring 0. `.app` files select built-in enum values. INT 80 previously only supported the trusted F32 demo. E820 allocator manages 52–64 MiB; GUI caches occupy fixed reserved regions below 52 MiB. Boot kernel maximum 159744 bytes; BSS must end below 0x1d0000. FAT12 is preloaded at 2 MiB. Snapshot storage persists `/home/user` using CRC and alternating commits.

## Implemented process boundary
8 process slots, 4 threads each, separate page directories, shared supervisor-only kernel identity mappings, private user image at 0x40000000. Text pages read-only; writable data limited to 192 KiB; independent 16 KiB thread stacks separated by unmapped guards. TSS supplies a dedicated 4 KiB kernel entry stack per process. User selectors 1B/23; IOPL=0, no I/O bitmap. PIT preempts user code every two ticks. Desktop and system calls are deliberately non-preemptible. Context switching saves integer registers; ABI 1 programs must not use FPU/SIMD.

INT 80 exposes bounded file, process, thread, event, handle, window and message primitives. All user ranges, write permissions and strings are checked against the current process page table. Faults from CPL3 kill that process; faults from CPL0 still panic. Generation-tagged handles check object type and access rights. Objects and scenes are reclaimed on process exit.

Window content is a bounded display list copied into kernel-owned storage. Scene clipping and geometry validation keep applications out of compositor memory. Application logic is separately linked user C. Legacy apps remain trusted ring 0 during incremental migration; they do not yet have crash isolation. The old writable F32 execution path is disabled; only the read-only boot demo is allowed.

## Remaining boundaries
No SMP, blocking kernel continuations, demand paging, dynamic linker, per-user accounts or hardened security boundary. Global VFS and desktop remain legacy kernel services. Filesystem permissions currently distinguish writable user home from protected system paths. Do not describe this development OS as production secure.

Architecture references: [Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html), Volume 3, protection, paging and TSS chapters. Original project code; no Windows/FreeDOS/Linux kernel imported.


## 2026-10-24: backward-compatible retained canvas

The original UiScene structure remains 64 commands. SYS_SCENE now validates a 4-byte count, rejects counts above 256, validates exactly count*sizeof(UiCommand)+4 readable bytes, and copies only that range into a 256-command kernel canvas. Old ABI-1 applications remain valid. Panel, grid, selection and clipped-text drawing primitives support the docked IDE; they carry no project/package policy. Each window canvas is now seven 4-KiB pages and is freed on close. User windows may request up to 1000x714, with desktop bounds still applied. Window policy bit 1 requests close messages; bit 2 delivers F5/F8 to the native window rather than the global launcher shortcuts.

IDE generation constructs ordinary SNX1 or SXP1 data in user mode; installation policy remains in the separate setup executable. A direct BASIC executable contains initialized data resources and retains the same native loader and process permissions.
