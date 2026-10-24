# Staged roadmap and acceptance status

Route: retain SINO boot/desktop, add protected native API/app ecosystem; Win32/PE compatibility deferred. Each committed stage has a report and actual guest checks. Existing trusted applications remain usable while new user-mode applications are introduced.

| Requested area | Current implementation | Still required |
|---|---|---|
| Audit/boot | Baseline reproducible; ISO and hard-disk installer retained | Broader real hardware matrix |
| Objects/handles | Typed objects, reference counts, reduced-rights duplicate handles, independent stale-handle cookies | Device/process/file objects and cross-process handle transfer |
| Processes/threads | 8 isolated processes × 4 threads; PIT preemption; event objects; terminate/fault cleanup | Priorities, blocking waits, richer synchronization/services |
| VM/user ABI | Private page directories, supervisor kernel, RO text, guarded stacks, CPL3/TSS, bounded INT80 | Demand paging, larger heaps, relocatable/dynamic loading, FPU context |
| HAL/drivers | Existing bounded ATA and PS/2/VGA; new NetDevice interface | Unified device objects/IRP, PCI, real/virtual Ethernet drivers |
| Files/config | Common bounded user APIs, writable custom snapshot VFS, persistent user config keys | General mount VFS, larger files/catalogs, ACLs, full registry hierarchy |
| GUI | Existing compositor retained; independent user scenes and messages, input, clipboard, close guards | All legacy apps migrated, shared controls/dialogs expanded, scalable desktop service boundary |
| Software install | Independent GUI + native package format, transaction recovery, upgrade/repair/uninstall, manager | Resident IPC broker, signed packages, dependencies, service/startup execution, strict concurrent transaction isolation |
| Network | Real IPv4/ICMP/UDP/TCP loopback; socket handles; DNS/HTTP local clients and servers | Ethernet/ARP, NIC, routing/static config/DHCP, public DNS/HTTP, manifest token delegation, hardened TCP |
| SysBASIC | Working packaged designer/editor/interpreter/debug/build/install chain | Full Function/parameter/scope semantics, PictureBox, full menus, multiple forms, full debug UI and five complete examples |
| Apps | Packaged native Files/Editor/Process Viewer/Hello/SysBASIC; bootstrap installer/manager/task manager | Native settings/terminal/calculator/paint/log viewer/image viewer/browser; migrate legacy apps |
| Desktop/services | Shortcuts, persisted associations, task view, basic tokens, clipboard, install logs, repair | Default-app dispatch, comprehensive drag/drop, notifications, services/startup, update service |

The user's complete acceptance matrix is **not yet fully satisfied**. There is no missing credential/hardware decision blocking the implemented virtual-machine work; remaining items are substantial engineering stages, not successful placeholders. Before expanding the IDE, first move bulk installer/package buffers to a larger user heap: the current IDE is close to the 192 KiB image/data limit. Before enabling non-loopback networking, implement one virtual NIC and capture/test malformed and lost frames. Before claiming general application crash isolation, migrate all remaining kernel built-ins.

Rollback: commits preserve each source stage; rebuild the chosen stage. User data/snapshot formats and package catalogs may exceed older kernels' capacities, so restoring an older production release also requires its full virtual-disk backup. Keep newer files separately before such a rollback.


2026-10-24 update: SysBASIC 0.2 now has the classic docked layout and separate executable/installer output. Installer buffers were removed from the IDE by launching the independent installer, but embedded-output and canvas buffers still leave only about 13 KiB below the 192 KiB process image/data cap. Full BASIC semantics, a user heap, adaptive layout and multiple forms remain subsequent work.
