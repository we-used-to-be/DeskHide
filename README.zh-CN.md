# DeskVeil · 桌隐

[English](README.md) | **简体中文**

**双击桌面空白处，让图标隐去，让壁纸露出来。再双击，图标即刻回来。**

A tiny native Windows utility to toggle desktop icons with a double-click.

**143 KiB 单文件 · 启动空闲私有提交约 2 MiB · 无需安装额外运行库**

DeskVeil 是一个原生 C++17 / Win32 桌面小工具。启动后安静地驻留系统托盘，不占用任务栏，只在你需要时切换桌面图标。

## 小到可以随手带走

| 发布文件 | 本地 Release 产物体积 |
|---|---:|
| 单个可执行文件 | **143 KiB**（146,432 字节） |
| 便携 ZIP，含程序与文档 | **约 79.2 KiB** |
| 安装包 | **约 2.01 MiB** |

程序本身不到 150 KiB。无需 .NET、WebView 或 Qt，也无需另行安装 Visual C++ 运行库；便携版解压即可运行。安装版提供安装向导和系统应用列表卸载，约 2 MiB 的体积主要来自 Inno Setup 安装器。

以上为当前本地 v1.0.0 Release 产物快照；后续构建或随附文档变化会影响包体积，以实际下载文件为准。KiB / MiB 按 1024 进制计算。

## 轻量常驻，内存数据透明

采用 Windows 消息和鼠标事件驱动，不轮询桌面状态，鼠标移动事件不进入双击检测逻辑。没有后台服务、网络访问或遥测。

| 内存指标 | 启动后空闲实测 |
|---|---:|
| 私有提交（Private Bytes） | **1.96 MiB** |
| 总工作集（Working Set） | **11.08 MiB** |

测量环境为 Windows 本机、MSVC x64 Release、静态 CRT：启动后等待 8 秒，再每秒采样一次，共 5 次；未主动修剪工作集。该轮空闲采样中 CPU 时间增量为 0 秒。

**约 2 MiB 指启动空闲时的私有提交，不是全部内存，也不是使用后的上限。** 实际切换图标后会加载 Windows Shell / MSAA COM 模块；已有后续样本约为 **35.42 MiB 总工作集、10.30 MiB 私有提交**。任务管理器显示值、总工作集和私有提交口径不同，不能直接互换，实际占用也会随系统环境和使用状态变化。

测量条件与验收记录见 [VALIDATION.md](docs/VALIDATION.md)。

## 功能

- **双击切换**：双击桌面空白处隐藏或显示所有桌面图标。
- **保留桌面内容**：不移动文件，不改变图标位置，只切换 Windows 的图标显示状态。
- **遵循系统习惯**：使用 Windows 双击速度、位置阈值和主鼠标键设置。
- **区分点击与框选**：检测完整按下 / 松开过程，排除图标、文字、普通窗口、任务栏和拖动框选。
- **托盘控制**：手动切换图标、暂停 / 恢复双击监听、设置开机启动、退出。
- **按需自启动**：默认关闭，由用户通过托盘菜单开启。
- **单实例运行**：重复启动不会叠加多个监听实例。
- **免费开源**：源码采用 MIT 许可。

## 开始使用

支持 **Windows 10 / 11 x64**，使用普通用户权限运行。

项目名称为 DeskVeil；当前版本的程序、安装包和构建目标仍使用 `DeskHide` 名称，以下命令与现有产物一致。

1. 选择安装版 `DeskHide-1.0.0-x64-Setup.exe`，或解压便携版 `DeskHide-1.0.0-x64-portable.zip` 后运行 `DeskHide.exe`。
2. 程序启动后驻留右下角系统托盘；如果没有看到图标，展开隐藏的托盘图标。
3. 双击桌面空白处隐藏图标，再双击恢复。

左键或右键点击托盘图标均可打开中文菜单。便携版启用开机启动后，请保留 EXE 的位置；移动文件后，在新位置重新勾选自启动。

暂停或退出会停止双击监听，并保留当前图标显隐状态。需要手动恢复时，右键桌面 → **查看 → 显示桌面图标**。请避免与其他同类双击工具同时运行，以免一次双击切换两次。

## 命令行

可用于快捷方式或本地脚本：

```powershell
.\DeskHide.exe --toggle  # 切换图标显隐
.\DeskHide.exe --show    # 显示图标
.\DeskHide.exe --hide    # 隐藏图标
.\DeskHide.exe --quit    # 退出驻留实例
```

已有驻留实例时，命令转发给该实例；没有实例时，前三项执行一次后退出，不启动常驻监听。

## 从源码构建

需要 Visual Studio 2022 / 2026 的 C++ 桌面开发工具、Windows SDK、CMake 3.21+ 和 Ninja；生成安装包另需 Inno Setup 6。以上均为开发工具，用户运行程序无需安装它们。

```powershell
# 构建 Release 并运行定向测试
powershell -ExecutionPolicy Bypass -File scripts/build.ps1

# 同时生成安装包、便携包、源码包与校验和
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Package -IsccPath "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
```

产物位于 `artifacts`。默认构建自动运行双击输入状态机和隔离注册表测试。对应构建目录中有程序运行时，请先通过托盘退出，或使用 `-BuildDirectory out/release` 指定独立目录。

桌面集成测试会短暂切换真实桌面图标并恢复，运行前确认没有其他同类工具监听：

```powershell
.\build\windows_tests.exe --desktop
.\build\windows_tests.exe --status
```

## 实现边界

- 独立鼠标钩子线程转发输入事件，所有钩子事件继续传递；钩子内不执行 COM、文件、注册表或同步跨进程消息。
- 使用 MSAA 区分桌面空白与图标，通过 Windows Shell `IFolderView2` 读取和切换真实状态，并使用 `SHGetSetSettings` 保存显隐设置。
- 仅显式开启自启动时写入本应用的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\DeskHide` 值。
- Explorer 重建托盘时重新注册图标，每次切换重新获取 Shell View。

设计方案见 [PLAN.md](docs/PLAN.md)。多屏混合 DPI、Explorer 重启、实际安装 / 升级 / 卸载等人工验收状态见 [VALIDATION.md](docs/VALIDATION.md)。

## 许可与致谢

采用 [MIT License](LICENSE)。项目参考 [DesktopIconToggle](https://github.com/hexandcube/DesktopIconToggle)。来源和完整许可说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
