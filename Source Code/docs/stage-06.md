# SINO 0.10 release and installed-disk stage

Normal and SELFTEST images build with warnings treated as errors. Exact final ISO SHA-256: 174576c8c35cdf4c0077f0f7c022e0cec0e74ee6727b34699de3655a01797f9f. All 15 current suites pass on this image. Native GUI cold start/restore and OS installer launcher/restart/no-CD boot pass. Reports and limits are in REPORT.md, testing.md, and roadmap.md.

Settings confirmation is now published only after durable persistence. Mouse regression waits for the guest to process button release and the final pointer position instead of guessing with a short delay. Package tests explicitly validate version upgrade and downgrade refusal. Boot documentation describes the real memory layout and hybrid user/legacy-application model.

Before changing the daily-use virtual disk, a clone was made. Only its boot volume was updated; MBR/preferences/data bytes remained unchanged. The clone booted without ISO, all six original directory/file records were compared, and the real Ring 3 probe ran. The production update then saved and hash-checked a full 128 MiB backup, updated/read back the boot image, and again verified the complete data partition was unchanged. New native packages were installed through the user-mode installer with original file hashes preserved. The installed SysBASIC was launched on the actual visible virtual machine. Data/data.img remains untouched.

Additional full-desktop copies validate that an IDE-generated program fits the populated catalog, commits, and executes its Click event. One initial additional run did not produce the save-completion marker even though project files were present; two later runs passed. The transient cause is unknown. This is recorded as a persistence/stress-testing risk, not a fixed bug.

Rollback source snapshots are in Git; the full disk backup and pre-release metadata are under an external archive. No physical disk or unrelated workspace project was modified. Full acceptance of the entire requested OS/IDE/network scope remains unfinished; unsupported features are not claimed as complete.
