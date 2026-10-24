# SysBASIC 0.3.0 — Programming Keyboard and Classic Control Designer

2026-10-24. The existing BASIC subset and native Ring 3 model are preserved while the workbench is reimplemented. The interface uses a classic VB-style development-tool layout: gray menus/toolbars, a toolbox on the left, form or code child windows in the center, project tree and properties on the right, and output/watch panes below. All code, text icons, and drawing assets are project-created; Microsoft source code and icons were not copied.

## Launch and first project

Use the installed build documented in the release verification record. Press F11 or open SysBASIC in the desktop Applications view. If it is not installed, F11 opens the /boot/sysbasic.sxp installer. F9 manages software. The tool needs a logical desktop of at least 1024×768; lower resolutions and higher scaling do not yet have a responsive layout.

1. New opens a blank form with zero controls and zero bytes of source code. Open the counter example separately through Help > Open Counter Example. Select a toolbox tool, then press and drag inside the form to draw a rectangle; release to create the control. Double-click a tool to add it at its default size. Pointer selects and moves controls; the eight handles resize them on an 8-pixel grid. Esc cancels creation or restores the prior position. Clicking a handle alone does not resize.
2. Click a value in the property panel to select it, type a replacement, then press Enter to apply, Esc to cancel, or Tab to move to the next applicable item. A-Z and Category change property ordering. Gray properties do not apply to the selected object. The object selector can choose the form or a control. Form properties: Name, Caption, Width, Height. Control properties: unique Name, Caption, Left, Top, Width, Height, Value, Interval. Out-of-range values are rejected.
3. Double-click a control to open or generate its Click, Change, or Timer procedure. The code editor supports UTF-8/Pinyin input, indentation, arrow/Home/End/PageUp/PageDown navigation, horizontal tracking, select-all, clipboard copy/cut/paste, and single-level undo/redo swap.
4. F5 runs or continues; Esc or Stop returns to design mode. Click a line number to toggle a breakpoint; F8 steps; variables appear below while paused. The IDE handles F5/F8 for its own window and preserves other system shortcuts; F5 no longer opens system Paint.
5. Ctrl+S saves .prj/.frm/.bas; File > Save As chooses a new path. Ctrl+O opens a full .prj path. An existing Hello.prj loads at startup and is not overwritten by the example; New or saving to an existing path asks before overwrite. Unsaved changes prompt to save, discard, or cancel before closing or starting a new project.

## Build: executable or installer

Choose Build... from the toolbar or Build Program... from File. Output is placed beside the project.

- Executable (.snx): creates Hello.snx with the form and BASIC source embedded into a native runtime data segment. The single file runs directly and needs no neighboring .frm/.bas; it does not register an application. Generate and Open runs it immediately; it can also be double-clicked in SINO.
- Installer (.sxp): creates Hello.sxp containing the standalone native program. Generate and Open launches the separate setup wizard with welcome, license, location, components, progress, and completion pages. Building alone does not install it; confirm installation in the wizard. Afterward it appears in the desktop Applications library and F9 manager, where it can be launched, repaired, or uninstalled.

The application ID is generated from the English project filename, for example Hello.prj -> basic-hello. Installer project names are limited to 1–18 ASCII letters, digits, underscores, or hyphens; use a distinct filename for each project. Version is currently fixed at 0.2.0. Rebuilding the same ID can repair/reinstall it; there is no separate version editor yet. Executable is the default output and silent installation is never the default. Clear Generate and Open to create only the file.

Both outputs are native SINO formats, not Windows PE .exe or MSI, and cannot run by double-clicking on a Windows host. Build packages an interpreter and source program; it does not compile BASIC into new x86 instructions. This is not a full VB compiler.

## Projects and resources

Projects are plain text: .prj uses SINO-BASIC-1 and matching .frm/.bas files; .frm stores SFORM1 properties; Caption does not accept a vertical bar or newline; .bas is UTF-8. One form and one module are supported. File paths can only be written under /home/user.

The native runtime reserves a BasicImage resource in its initialization data (SBRES01): enabled flag, 3,072 bytes for a form, and 8,192 bytes for code. The builder validates a unique marker before filling it; the SNX1 loader format and permissions remain unchanged. Older runtimes with enabled=0 still read neighboring project.frm/project.bas to preserve existing generated applications.

Installer logic has been removed from the IDE. The IDE creates normal SNX1/SXP1 data and launches the separate user-mode setup app; the installer still owns transactions. During file copy/commit, close requests are handled by user mode and processed after the transaction. The software library merges installer-registered menu shortcuts with built-in entries and shows friendly names without requiring users to find the underlying .snx file.

## Implemented language

Integer (16-bit assignment bounds), Long (32-bit), String (127 UTF-8 bytes), Boolean, and dynamic Variant values; Dim, assignment, and numeric one-dimensional arrays with indices 0–15; arithmetic with checked overflow/division; comparisons; boolean And/Or/Xor/Not; string concatenation; block If/Then/ElseIf/Else/End If, single-statement If/Then, For/To/Step/Next, While/Wend, Do/Loop with While/Until, Select Case/Case Else/End Select; parameterless Sub, Call, and Exit Sub; event Subs. Variables currently share module-wide storage, including Dim inside a Sub.

Built-ins: Len, Left, Right, Mid, Val, Str, CInt, CLng, CStr, MsgBox, InputBox. String offsets/lengths are bytes in this version. Print writes to preview output. MsgBox/InputBox use a user-mode modal component; Esc cancels. ReadFile and WriteFile support small text files; ConfigRead/ConfigWrite use the shared user configuration store. HttpGet, TcpConnect, TcpSend, and TcpRecv bind to real socket APIs and report permission failures unless the app has a network token. Package network-token delegation is not implemented, so ordinary generated applications cannot use these network functions yet.

Limits: 8,191 source bytes, 256 lines, 255 bytes per line, 32 variables, 16 Subs, 8 nested For/Call frames, and 12-level expressions. Unknown syntax, invalid ranges, overflow, and invalid properties report line errors. Programs run in bounded batches between GUI events; Stop remains responsive during ordinary BASIC loops.

## Controls and events

Form, Label, Button, TextBox, CheckBox, ListBox, Timer, and a simple Menu button are implemented. TextBox emits Change; Button/CheckBox/ListBox/Menu emit Click; Timer emits Timer; Form emits Load. List items use semicolon-separated captions. Properties bind as Label1.Caption, TextBox3.Text, and CheckBox4.Value. Text/Caption, Value, geometry, and timer interval are accessible from BASIC. Timers use 100 Hz kernel ticks. The minimal Menu control is a button event, not a full nested drop-down menu.

## Explicitly unfinished

TODO: user-defined Function/return values and Sub parameters; module-level declaration processing, lexical local scope, string/Variant arrays, full Variant coercion, short-circuit expressions, inline Else, colon-separated statements, Now/Date; PictureBox image decoding; full menu hierarchy, multi-form projects/modules, multiple undo/redo levels, partial text selection, token-level highlighting, full debugger call stack, editable application ID/version controls, and comprehensive malformed-source fuzzing. Five complete requested example projects are unfinished; the counter and First tutorials are delivered. HTTP/TCP project templates await package permission delegation. Unsupported Function/parameter syntax is rejected instead of silently accepted.

This is a BASIC-subset development tool, not a VB6-compatible compiler. QA in tools/test-ide.js exercises designer movement/resizing, properties, generated events, edited source, preview execution, breakpoint/step, project persistence, package build/install, and the resulting independent application.

## 0.2.1 interaction fixes

The right-side project tree supports expand/collapse, form selection, and code navigation. The code object's selector chooses real objects; the event selector shows only supported Load/Click/Change/Timer events and navigates to or generates the selected procedure.

The nonfunctional Form Layout mockup was replaced with Object Actions: View Code, View Form, Delete Control, and Select Form all perform their named operations. In 0.2.1, toolbox controls could be dragged directly onto a form; in 0.3 this changed to selecting a tool and dragging a rectangle. Form Name persists as an optional fifth SFORM1 header field, while old four-field forms still load with the default Form1 name. A renamed form generates the new Load event and retains the old Form_Load fallback. New projects contain no sample code and do not rewrite an existing project.

## 0.3 keyboard and release example

PS/2 Set 1 decoding is in src32/keyboard.c. The fixed US physical layout tracks left and right Shift/Ctrl/Alt independently. Shift+minus types underscore; the keypad supports digits, operators, NumLock, and Enter. Pause sequences and Print Screen's fake Shift are ignored; extended arrow keys retain their existing codes. Ctrl+Space switches Pinyin; use EN while writing code.

disk32/FIRST.* provides a runnable example with underscores in event/control/variable names. On the preinstalled release disk it is /home/user/Projects/First.prj, with First.snx, First.sxp, and the installed basic-first entry. After installing a fresh ISO, copy it using the terminal commands in ../Documentation/Getting Started.txt.

tools/test-keyboard.js checks decoding; test-ide-input.js sends real virtual PS/2 press/hold/release events and checks underscore save/run, persisted coordinates for all eight resize handles, cancellation, and reverse-direction drawing. test-ide-panels.js preserves the right-panel regression.
