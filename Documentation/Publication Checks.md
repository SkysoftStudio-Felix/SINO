# Publication checks — 2026-10-24

The initial public repository contains the final source/documentation snapshot extracted from the English distribution, plus repository readmes and this note. It does not contain previous private Git history or development disks.

Checks performed before the first public commit:

- Rebuilt the exported source with GCC/Binutils and the included NASM using `build-release.ps1`.
- `tools/test-iso.js` passed ISO9660/El Torito validation and an actual QEMU CD-only boot followed by the calculator check.
- `tools/test-keyboard.js` passed programming symbols, independent modifiers, keypad, extended keys and Pause/Print Screen handling.
- The rebuilt `ide.snx` SHA-256 matches the supplied runtime: `2cc717767f2f8d8d25aa80d49e904d94a33f7eb5b90af879c36a1a319233c7c8`.

The source rebuild produced ISO SHA-256 `acbeb987b59a7c9f8ce7ddd1fa4498e9b8e90c83dd24affe402d6b7b478fbcbc`. The English source has translated on-disc documentation, so this differs from the unchanged bilingual runtime ISO distributed with both release ZIPs.

The released runtime ISO SHA-256 is `3b87d73bf185d530efd845d2f49718dd1c4f8f41b7717060318198e36f4ae021`. The original 18-suite verification record is preserved in `Verification Summary.json`; those 18 suites were not all rerun during this publishing step. The distribution manifest describes the English ZIP contents rather than this repository's working tree.

## Distribution integrity

Project-owned copyright notices use `SINO contributors`. The MIT terms and upstream attribution are retained. Both distribution ZIPs have verified per-file manifests and archive checksums. The runtime ISO, preinstalled disk and program binaries match the recorded release hashes. The release tag points to the current source snapshot.
