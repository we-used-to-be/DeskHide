# 桌隐 / DeskHide

- 原生 C++17 / Win32，Windows 10/11 x64。保持无第三方运行时、无网络、无轮询。
- `src/click_detector.h` 是纯输入状态机；`desktop.cpp` 是 Shell/命中边界；`main.cpp` 管理钩子线程和托盘；`startup.cpp` 只操作本应用 HKCU Run 值。
- 鼠标钩子不得执行 COM、文件、注册表或同步跨进程消息；所有事件必须继续 CallNextHookEx。
- 保留参考分支的 Down→Up 配对、系统半宽阈值、松键位移及框选后清空候选语义。
- `powershell -ExecutionPolicy Bypass -File scripts/build.ps1` 构建并运行定向测试；加 `-Package -IsccPath <ISCC.exe>` 生成安装包。
- 桌面集成测试会短暂切换图标并恢复；测试前确认没有旧版 DesktopIconToggle 同时监听。
- 不提交、推送、打标签或发布，除非用户当前任务明确授权。不得覆盖参考仓库。
