# Independent native software installer

user/setup.c is a separate SNX1 Ring 3 application. It is unrelated to src32/install.c, the existing BIOS/MBR operating-system installer. F8 launches software setup; F9 launches the separate apps.snx manager. Double-click .sxp files to install. The wizard has welcome, license, path, components, progress, and completion pages. Click the path field, press Ctrl+A, and type an absolute path under /home/user/Programs/<directory>.

user/install_service.c provides InstallBegin, InstallWriteFile, InstallSetConfig, InstallCreateShortcut, InstallRegisterUninstall, InstallCommit, InstallRollback, and InstallRecover. They run in user mode; the kernel has no package parser or install policy. A kernel named lease serializes installers and is released when a process exits. The library saves per-file backups and a CRC-protected undo journal, commits each preimage before changing its target, then commits the completed snapshot before marking the journal complete. Startup detects an interrupted journal and launches user-mode recovery. A committed journal only needs cleanup. Corrupt journals stop recovery without guessing.

Registry-like key/value files, application records, cached repair packages, associations, menu shortcuts, and logs reside under /home/user/.sino. A desktop shortcut is a .lnk containing the native executable path. Installation and uninstall use the same transaction mechanism. Upgrades reject downgrades and changes to root/ownership metadata; reinstalling the same version repairs files and changes optional components. Uninstall preserves user-created files and installation directories. Existing unowned files, configuration, or shortcuts cannot be silently taken over.

Commands in the SINO terminal:

    install /boot/hello.sxp
    install /quiet:/boot/hello.sxp
    install /repair:hello
    install /remove:hello

Removal requires confirmation in the independent setup window. Repair uses the locally cached package. The application manager provides launch, repair, uninstall, and refresh. Its first version displays ten installed entries; paging a larger catalog is TODO.

Limits: eight package files, 64 KiB per package/file, 32 undo records, 128 total VFS nodes, and one installer lease. Persistence requires a writable data volume; a failed disk sync prevents a successful commit. Network permissions, running services/startup execution, signature verification, shared runtime dependency resolution, cross-user administration, and package migration are not implemented; unsupported package requests are rejected. Signature space is reserved and must be zero. The service API is a user library serialized by a lease, not yet a resident IPC daemon. Older applications remain trusted kernel built-ins pending migration.

QA: node tools/test-packages.js runs guest-side validation of rollback, install, component repair, uninstall, configuration restore, GUI navigation, application launch, persistence, and reboot recovery. Screenshots and reports are written to build32/vm-tests/packages-qa.

2026-10-24 update: The independent GUI installer now handles close messages in user mode. The desktop cannot kill it while file-copy or transaction-commit work is running; the close request is handled after the operation returns. The GUI writes its completion log only after InstallCommit or Uninstall returns. Regression tests wait for this confirmation instead of inferring completion from the temporary appearance or disappearance of a registration file. New shortcuts support an optional friendly display name, while old single-line path shortcuts remain compatible.
