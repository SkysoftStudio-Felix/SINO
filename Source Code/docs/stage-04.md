# SysBASIC prototype stage

Completed: separately packaged ring-3 IDE; original form/control model and text formats; toolbox placement, select/move/resize with grid, validated properties, automatic event procedure generation; UTF-8 multiline editor and user-mode Pinyin messages; interpreter with checked arithmetic, numeric arrays, supported types, block branches/loops/select, parameterless Subs, strings and bounded system bindings; preview run/stop; source breakpoints, single-step and variable display; native runtime packaging and transactional installation; application-manager launch of generated app; disk save/load after reboot.

Kernel changes are generic primitives only: contiguous-page allocation, 64-command bounded scenes, text font choice, pointer move/release events, immutable executable path and Unicode text messages. No BASIC parser, designer or package builder is linked into the kernel.

Validation: build normal/test; real guest language test and failures; test-ide.js designer/control/property/code/debug/build/install/launch/reboot checks (including Chinese Caption via Pinyin); isolation and network regressions; installer regression; Chinese desktop/file drag/drop regression; no-SSE2 fallback. Screenshots in build32/vm-tests/ide-qa.

This is not the full requested IDE. ide.md explicitly lists missing Function/parameters/scoped locals, PictureBox, complete menus, full language/debugger, multiple forms and five complete examples. No empty APIs claim these features. Native application/runtime memory is checked against the 192 KiB image/data cap; current IDE is near that cap, so further expansion must first reduce its resident buffers or evolve the loader.
