SINO 0.10 Desktop and Hard-Disk Installation

File operations
- Right-click an empty desktop area to create a folder or text file.
- Right-click a file to open, rename, copy, cut, paste, move it to Trash, or view its information.
- Deleting an item again from Trash removes it permanently. Empty non-empty folders before deleting them.
- Drag a file to a folder, file window, desktop, or Trash and release to move it.
- Desktop icons can be dragged to reposition them. Their positions persist only for the current session.
- Double-click a folder to navigate inside the current file window. Double-click a desktop disk or folder to open a new window.
- Ctrl+N creates a window or document; Ctrl+O opens by path; Ctrl+S saves; Ctrl+W closes.
- Ctrl+C / X / V copy, cut, and paste. The text editor also supports Ctrl+A and Ctrl+Z.

Chinese and Pinyin
- Press Ctrl+Space to switch between English and Pinyin, or click the language toggle at the upper right.
- Pinyin input is available in filename dialogs, the text editor, and the terminal.
- Type a syllable; Space selects the first character, keys 1–7 choose candidates, [ ] change pages, or click a candidate.
- Esc cancels composition; Enter inserts the typed Pinyin. Switch back to EN before typing English commands or file extensions.
- Type v for ü, for example lv. Tone marks are not required.
- Common words include nihao, zhongwen, wenjian, wenjianjia, xinjian, and baocun.
- This is a basic Pinyin method with single-character candidates and a small common-word list; it has no sentence prediction.
- Chinese uses a 16-pixel bitmap font; Latin text retains the classic desktop's original-size font.
- Filenames and document contents use UTF-8. Cursor movement, Backspace, character selection, and copy handle complete Chinese characters.

Applications and windows
- The SINO menu launches File Manager, Text Editor, Calculator, Paint, Clock, and Terminal.
- Double-click .app entries in the Applications folder to run them.
- Drag a title bar to move a window. File and text windows can be resized from the lower-right corner.
- Alt+Tab switches windows; the Window menu restores minimized windows. Up to eight windows can be open at once.
- F1 File Manager / F2 Terminal / F3 Text Editor / F4 Calculator / F5 Paint
- F6 Clock / F7 About SINO

Saving and limits
- Files under /home/user are stored on a dedicated SINO data disk and persist across reboot.
- Use Tools > Save Personal Files to Disk or run sync in Terminal to save explicitly.
- /boot is read-only optical media; /tmp is temporary. Without a data disk, files exist only in memory.
- The filesystem supports 64 file nodes, up to 64 KB per file, and documents up to 16 KB.
- Filenames are limited to 31 bytes (up to 10 Chinese characters); paths are limited to 127 bytes.
- Moving is supported within the current folder. Clipboard copy supports files but does not copy folders recursively.

Kernel, desktop, drivers, and applications are C/assembly written for this project.
Third parties include QEMU, the classic bitmap fonts, GNU Unifont, and pinyin-data; their licenses are retained.
This is a custom 32-bit operating-system prototype, not compatible with DOS, Windows, Linux, or macOS applications.
See About / Credits in Terminal or cat /boot/sources.txt for detailed provenance.

System settings and installation
- Press F7 or choose SINO > System Settings to view the version, switch English/Chinese, and adjust resolution, scaling, and contrast.
- After applying display settings, confirm within 15 seconds or the previous mode is restored. Settings are saved on the personal data disk.
- Choose SINO > Install SINO to open the graphical installer, which supports disk selection and MBR primary-partition planning.
- When booted from CD with an IDE disk of at least 64 MB attached, the installer opens automatically.
- New, delete, and format operations first create a plan; the disk changes only after you choose Begin Installation.
- After installation, boot from the hard disk and remove the ISO. GPT / UEFI are not supported.
- Install SINO.cmd on the host uses the dedicated Data/system.img and does not modify a Windows physical disk.
- The data partition has about 4 MB of usable snapshot capacity and retains file-count and size limits.
