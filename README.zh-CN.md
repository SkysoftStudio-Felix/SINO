# SINO

**SINO 0.10 / SysBASIC 0.3.0 开发预览版**

**一个可以在桌面上设计、运行和安装 BASIC 应用的小型 x86 操作系统。**

[English](README.md) · [下载最终版](https://github.com/SkysoftStudio-Felix/SINO/releases) · [截图](#截图) · [参与开发](CONTRIBUTING.md)

SINO 是用 C 和汇编编写的 32 位 x86 操作系统原型，包含图形桌面、中文与拼音输入、原生用户态应用、事务式软件安装器及 SysBASIC 可视化编程环境。项目实现由 AI 辅助开发，具体代码来源及第三方组件见 [来源说明](<Source Code/SOURCES.md>)。

![SINO 在 QEMU 中运行的桌面与文本编辑器](<Documentation/Screenshots/Desktop and Editor.png>)

适合对自制操作系统、复古桌面和 BASIC 编程感兴趣的人体验：

- **探索系统：**图形桌面、文件操作、终端、系统设置和中文拼音输入。
- **制作应用：**在 SysBASIC 中绘制控件、编辑属性与事件代码，并直接运行。
- **打包安装：**生成原生 `.snx` 程序，或交给独立安装器处理的 `.sxp` 软件包。
- **研究实现：**查看 C/x86 汇编、保护模式、Ring 3 应用、系统调用与回环网络实现。

## SINO 有什么值得体验？

SINO 把桌面环境和一套小型应用开发流程放进同一个实验性操作系统：浏览文件、打开可视化设计器、编写 BASIC 事件代码、预览程序，再选择生成可执行文件或安装包。希望通过可阅读、可修改的 C 与汇编代码，让操作系统原理和应用开发连接起来。

- **复古桌面与双语交互：**经典窗口式桌面，支持中英文界面和拼音输入。
- **可视化应用开发：**SysBASIC 集成窗体设计、控件属性和事件代码；工程采用纯文本 `.prj`、`.frm`、`.bas` 文件。
- **原生软件分发：**以 `.snx` 可执行文件与 `.sxp` 安装包串联 IDE、独立安装向导和应用管理器。
- **可研究的系统实现：**源码与文档覆盖启动流程、保护模式内核、用户态 API、安装事务以及实验性网络功能。

### 第一次体验可以这样开始

1. 下载并完整解压发行 ZIP，安装 QEMU，运行随包启动器。
2. 浏览桌面，在系统设置里切换界面语言。
3. 按 **F11** 打开 SysBASIC，加载 `/home/user/Projects/First.prj`。
4. 运行示例，查看它的控件、属性和事件代码。
5. 尝试生成程序或安装包；按 **F8** 打开安装器，按 **F9** 管理应用。

想研究实现，可以从 [系统架构](<Source Code/docs/architecture.md>)、[应用开发指南](<Source Code/docs/app-dev-guide.md>) 和 [SysBASIC 参考](<Source Code/docs/ide.md>) 开始。已实现范围和仍有限制的部分见下方的“当前边界”。

## 下载和运行

进入 [Releases 下载页](https://github.com/SkysoftStudio-Felix/SINO/releases)，下载 Chinese 或 English ZIP，并完整解压。两份 ZIP 使用相同的双语系统镜像，文件夹名称和说明文档语言不同。

[直接下载中文版](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO-0.10-SysBASIC-0.3.0-Chinese.zip) · [英文版](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO-0.10-SysBASIC-0.3.0-English.zip) · [独立 ISO](https://github.com/SkysoftStudio-Felix/SINO/releases/download/v0.10-sysbasic-0.3.0/SINO.iso)

- 中文包：阅读 `介绍文件/开始使用.txt`，双击 `编译后的运行文件/启动SINO.cmd`。
- 英文包：阅读 `Documentation/Getting Started.txt`，双击 `Compiled Runtime/Start SINO.cmd`。
- 另行安装 [Windows 版 QEMU](https://www.qemu.org/download/#windows)。启动器找不到 QEMU 时，将 `SINO_QEMU_DIR` 设置为 QEMU 所在目录。
- ZIP 已包含干净的预装虚拟硬盘。下载区另有独立 `SINO.iso`，可在新虚拟机中通过 BIOS/Legacy 模式安装。

进入系统后，F11 打开 SysBASIC，F9 打开应用管理器，F8 打开软件安装器。系统设置中可切换中英文。First 示例工程位于 `/home/user/Projects/First.prj`。

## 截图

以下均为 QEMU 中的实际运行截图。多数使用中文界面，语言设置图展示英文界面。

| 窗体设计器与属性面板 | IDE 中运行 First 示例 |
| --- | --- |
| ![SysBASIC 工具箱、窗体设计与属性编辑](<Documentation/Screenshots/Form Designer.png>) | ![BASIC 计数器示例正在 IDE 中运行](<Documentation/Screenshots/First Program.png>) |
| **独立原生软件安装器** | **已安装应用管理器** |
| ![原生软件安装向导](<Documentation/Screenshots/Native Installer.png>) | ![应用启动、修复与卸载](<Documentation/Screenshots/Application Manager.png>) |

<details>
<summary>查看英文界面与语言设置</summary>

![SINO 英文界面与语言设置](<Documentation/Screenshots/Language Settings.png>)

</details>

## 源码构建

需要 Node.js、能够生成 i386 目标的 GCC/GNU Binutils，以及源码中保留的 NASM 3.02。工具链不在 PATH 时，先将 `SINO_TOOLCHAIN` 设置为 GCC/Binutils 的 `bin` 目录。

```powershell
Set-Location '.\Source Code'
powershell -NoProfile -ExecutionPolicy Bypass -File .\build-release.ps1
```

没有相邻 `Compiled Runtime` 目录时，ISO 输出到仓库根目录。修改字体资源另需 Python 与 Pillow；普通构建使用现有资源。更多命令见 [构建与测试](<Source Code/docs/testing.md>)。

## 当前边界

这是供学习、演示和继续开发的系统原型。硬件范围为 BIOS/MBR、IDE、标准 VGA 和 PS/2，尚不支持 UEFI/GPT 与通用实体硬件。SysBASIC 是 BASIC 子集解释器，输出 SINO 原生程序或安装包，没有 VB6/Win32 二进制兼容。网络目前只验证了回环路径。新原生应用已有 Ring 3 隔离，部分旧桌面应用仍运行在内核态。

完整限制见 [发行说明](<Documentation/Release Notes.md>)，后续规划见 [路线图](<Source Code/docs/roadmap.md>)。其他文档包括 [语言与 IDE](<Source Code/docs/ide.md>)、[安装器](<Source Code/docs/installer.md>) 和 [应用开发](<Source Code/docs/app-dev-guide.md>)。

## 参与开发

欢迎在 QEMU 中试用，通过 [Issues](https://github.com/SkysoftStudio-Felix/SINO/issues) 反馈可复现的问题，或帮助完善文档与测试。[参与指南](CONTRIBUTING.md) 提供了合适的起点；未完成功能以路线图为准。

## 仓库与许可

仓库只包含最终发行版的源码与公开文档快照。完整 ZIP、预装虚拟盘和 ISO 通过 Releases 分发。`Documentation` 中的验证摘要和 SHA-256 清单对应原始发行包，不是当前 Git 工作树。个人文件、日常开发磁盘及旧 Git 历史没有上传。

项目代码采用 [MIT](LICENSE)。字体、拼音数据与汇编工具保留各自许可，见 [第三方许可目录](Documentation/Licenses) 及源码中的来源清单。再发布时请保留这些文件。
