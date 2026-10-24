# Stage 0–4 / 7 foundation report

Completed: baseline audit and reproducible original hash; Git baseline; error codes; typed generation handles; private page tables; Ring 3/TSS; preemption at user boundaries; native loader and host SDK; four threads/process; shared event objects; bounds-checked system-call dispatch; user window display lists and click/key messages; application fault containment; resource reclamation.

Validation: build.ps1 -TestImage; build-iso.js; run-selftest.js; test-usermode.js. QEMU tests execute real CPL3 code, kernel-write/#GP/UD2 faults, infinite-loop preemption, multithread event signaling, stale handles, invalid pointers, and file permissions. The free-page count returns to baseline minus the intentionally persisted one-page test file. Original GUI self-tests still pass.

Repairs: QMP cold-start retry and failed-socket cleanup; MinGW entry hook; linker image-base offset; nm parser now matches complete symbols rather than a .stop suffix. Final tests pass. No user data disks were opened. Rollback: the Git baseline commit and preserved original SINO 0.9 image remain available outside this release.

Not completed: legacy-app migration, comprehensive blocking synchronization, driver framework, network, software installation, and IDE. Subsequent stages track these items explicitly.
