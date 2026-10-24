# SysBASIC 0.2.1: Blank New Projects and Right-Panel Fixes

Date: 2026-10-24. The existing IDE was fixed in response to reports that the right panel could not be used, new projects still contained an example, and many controls did not respond. Both 0.2 build modes were retained.

The screenshots in Documentation/Screenshots show the application with the Chinese system locale.

## Fixed

- New now creates a blank project with zero controls and zero source bytes. Old output and selection state are cleared; no Hello/button/counter procedure is inserted. Form1 remains the default form name, not a sample. Rename it in the right-side (Name) property. The counter sample is now opened separately through Help > Open Counter Example.
- The project tree can expand/collapse the project and form folders, select the current form, and switch to code. The project name comes from the actual file, and the form displays its real name.
- The property object selector chooses a form or any control. A-Z/Category buttons really change sort order. Clicking a value selects it; typing replaces it; Enter applies, Esc cancels, and Tab moves to the next applicable property. Invalid values remain rejected. Gray properties are inapplicable, such as Timer Interval on a normal Button.
- Form Name/Caption/Width/Height can be edited and saved; control names remain unique. Form Name is an optional fifth SFORM1 header field; older four-field forms remain readable.
- Code object/event selectors are real lists. Supported Load/Click/Change/Timer events vary by object; choosing one navigates to or creates its procedure. Load executes after a form rename; the old Form_Load remains as a fallback.
- The old illustrative Form Layout area was replaced by working Object Actions: View Code, View Form, Delete Control, and Select Form.
- The toolbox supports direct drag-and-drop while retaining click-tool-then-place behavior. Adding a control after deleting one no longer risks a duplicate name. Menu hit regions now match the actual menu items.

## Use

Double-click Start SINO.cmd in Compiled Runtime and press F11 to open SysBASIC 0.2.1. Click New for a blank form. Change its name in (Name) on the right and its title in Caption. Unsaved changes still offer Save, Discard, or Cancel. New does not overwrite an existing project file; saving to an existing path still asks before replacement.

After adding a control, select it from the object list or on the canvas. Double-click an empty form to generate Load; double-click a control to generate its event. Build... still offers a single-file .snx or an .sxp with a separate installation wizard. Installed packages appear in the software library and can be removed. See ide.md for operations and language boundaries.

## Verification

All 15 regression groups passed. node tools/test-ide-panels.js additionally checks: blank files; form Name/Caption; toolbox drag/drop; property object selection and replacement editing; both sort modes and Width; tree expand/collapse, selection, and code navigation; code object/Change selection; actual execution of renamed-form Load and TextBox Change; switching/deleting/selecting with the lower-right actions panel; and saved .frm/.bas contents.

Screenshots: Documentation/Screenshots contains the designer and application views; these captures use the Chinese system locale. The focused test record includes the exact ISO hash. A populated daily-use disk copy, release-project-e7c60b64, also passed IDE upgrade, standalone example execution, setup wizard, and persistent registration. Windows launcher and install entry were revalidated.

Build and repeat validation:

    powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1 -TestImage
    node build-iso.js
    node verify-system.js
    node tools/test-ide-panels.js

Final ISO SHA-256: 694d3e77d8828cd6a08aa5e42649645fd190cbd7d93162aa5c3c7d908f520027; size 1,572,864 bytes. IDE image: 45,536 bytes; image/static data: 187,104 bytes, within the 196,608-byte limit.

## Deployment and rollback

The 67 original records were booted and hash-checked in a copy before the daily-use disk was updated. A complete backup was saved first. During the boot-resource update, MBR, preferences, and data partition remained unchanged. SysBASIC was upgraded to 0.2.1 through the independent installer. The expected seven package-owned files/records/logs changed; the other 60 records and directories matched their prior checks, and old projects remained.

The actual daily-use disk boots without an ISO and shows the blank new form; this blank project had not been saved over the existing Untitled file. Verification evidence is in the SysBASIC 0.2.1 and release reports. To roll back, close QEMU, back up the current disk, and restore the matching complete backup. Source 0.2 is preserved by commit fd6fbb4/tag sysbasic-0.2; this patch is tagged sysbasic-0.2.1.

## Boundaries

This fixes interactions in the current BASIC-subset IDE; it is not full VB6 compatibility. One form, 12 controls, 8,191 code bytes, a 192 KiB image limit, and a 1024×768 logical-desktop requirement remain. Form1 is a default name and can be changed. Inapplicable properties stay disabled instead of pretending to work. The 0.2 platform, language, network, and unfinished-feature limits remain in REPORT-IDE-0.2.md.

## Changed areas

This stage changed the Getting Started guide, README and IDE reports, SysBASIC package metadata, release/test tools, BASIC runtime, form controls, IDE UI, and verification records. The complete file-level history is in Git.
