# 实施方案

## 依据与边界

参考 `D:\GitHub\DesktopIconToggle` 的 `fix/desktop-double-click-drag`，HEAD `04a4261`；用户优化提交 `74da0c7`、`e79dc6d`。不参考原始 main 的旧判断逻辑，不修改参考仓库。

保留：第二次 Up 后触发、两次 Down 的时间/位置、系统双击/拖动矩形半宽、最终松键位移、拖动后清空候选、抖动后回原位仍算点击。

## 选型

C++17 + Win32，MSVC 静态 CRT，Release `/O1 /GL /OPT:REF /OPT:ICF`。不引入 .NET、WebView、Qt、后台服务。用户安装无需运行库、管理员权限。Inno Setup 提供每用户安装与卸载；另交付便携 EXE/ZIP。

1. 独立消息线程注册低级鼠标钩子，只采集按键，不处理每次移动；主线程执行命中判断和 Shell 调用。忽略积压超过一秒的输入，防止延迟误触发。
2. 验证真实 Shell 桌面窗口及进程，使用 Windows MSAA 命中检测区分桌面空白、图标和文字。失败时不切换。
3. 使用 `IFolderView2` 的 `FWF_NOICONS` 读取并更新实际状态，再保存对应 Shell 设置，不维护容易失步的隐藏布尔变量。
4. 只有已完成的两次空白点击配对；额外鼠标键、滚轮、修饰键、暂停、系统设置改变清空候选。
5. 中文托盘：显示/隐藏、暂停双击、自启动、关于、退出；Explorer 重建托盘后重注册。默认托盘常驻，无主窗口。
6. 自启动只操作当前用户 Run 下 `DeskHide` 值，路径加引号；默认关闭。安装、便携使用同一值，保留其他应用配置。

## 验收

- 纯状态机测试：正常双击/单击、超时、距离、拖动后单击、抖动返回、第三次点击、负坐标、计时回绕、非桌面和修饰键。
- Windows 集成：Shell 显示/隐藏/恢复、真实桌面命中、自启动往返（独立测试键）、单实例及正常退出。
- 托盘和真实鼠标路径尽量实测；没有执行的多屏混合 DPI、真实重启/登录、Explorer 重启须明确列为人工验收项。
- 同机比较 Release 工作集、私有提交与 CPU，记录启动后与使用后；不得用强制修剪工作集伪造数据。
- 安装包目标 < 3 MiB；EXE < 500 KiB；内存低于参考版本同机样本。目标是否达成以最终测量为准。

## 官方接口资料

- [桌面 Shell View 获取](https://devblogs.microsoft.com/oldnewthing/20130318-00/?p=4933)
- [IFolderView2](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifolderview2)
- [MSAA 命中](https://learn.microsoft.com/en-us/windows/win32/api/oleacc/nf-oleacc-iaccessible-acchittest)
- [钩子超时约束](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelmouseproc)
