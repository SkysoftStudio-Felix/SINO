# Native packages: SXP1

Original little-endian format, independent of MSI/PE/Win32. Host tool: node tools/package.js packages/hello.json build32/user/hello.sxp. user/package.h is the authoritative ABI layout; all integer fields are 32 bits.

The header/manifest is 1,408 bytes: magic SXP1, format 1, architecture 386, runtime ABI 1, file count, component bits, permissions, and total size. Fixed UTF-8 strings contain ID/name/version/entry/license, configuration key/value, shortcut, file association, and optional service name. A 64-byte signature field is reserved; this release rejects nonzero signatures and unsupported permissions/services instead of implying verification. Eight 80-byte file entries hold a 64-byte relative path plus payload offset, size, CRC32, and component mask. Payloads follow the manifest. Bit 1 is required and bit 2 is optional.

Validation rejects absolute paths, empty/dot/parent components, backslashes, drive syntax, overlong names, duplicate targets, malformed bounds, CRC mismatches, a missing required entry, incompatible ABI/architecture, and unsupported capability requests. No arbitrary install hooks execute. The guest independently enforces canonical home paths. CRC detects corruption; it is not cryptographic authentication.

Installed AppRecord contains the manifest, installation root, and source, with magic APR1. Records are stored as .sino/apps/<id>.rec; cached SXP supports repair and rollback. IDs and shared metadata cannot be changed by upgrades in this release.

## Friendly application entries (2026-10-24)

The first line of a newly installed .lnk contains the native target path; an optional second line contains the UTF-8 display name (up to 63 bytes). Older shortcuts with one path line remain valid. The desktop and Applications view use the display name; launching executes only the first-line path. The Applications desktop file window merges shortcuts from .sino/menu and does not use the underlying executable extension as the application name. A normal VFS List(/Applications) still lists the actual directory; it is not a new mounted filesystem.

SysBASIC 0.2 packages contain a single native entry with the project embedded and do not execute arbitrary installer scripts. Building an .sxp opens the separate installer; the app is registered only after the user confirms installation. Building an .snx does not register an application.
