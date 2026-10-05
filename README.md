# DeskVeil

**English** | [简体中文](README.zh-CN.md)

**Double-click the desktop to hide your icons and enjoy your wallpaper. Double-click again to bring them back.**

A tiny native Windows utility to toggle desktop icons with a double-click.

**143 KiB executable · ~2 MiB private bytes at startup idle · No extra runtime installation**

DeskVeil is built with C++17 and Win32. It stays in the system tray without a taskbar button, ready when you need a clear desktop.

## Small enough to take anywhere

| Distribution | Local Release artifact size |
|---|---:|
| Standalone executable | **143 KiB** (146,432 bytes) |
| Portable ZIP, including the app and documentation | **~79.2 KiB** |
| Installer | **~2.01 MiB** |

The executable is under 150 KiB. No .NET, WebView, Qt, or separate Visual C++ runtime installation is required. Extract the portable edition and run it. The installer adds a setup wizard and uninstall support through Windows Settings; most of its ~2 MiB size comes from Inno Setup itself.

These figures describe the current local v1.0.0 Release artifact snapshot. Future builds and documentation changes may affect package sizes; check the actual download. KiB and MiB use powers of 1024.

## Lightweight, with transparent memory figures

DeskVeil uses Windows messages and mouse events rather than polling desktop state. Mouse movement events do not enter the double-click detection logic. There is no background service, network access, or telemetry.

| Memory metric | Measured at startup idle |
|---|---:|
| Private Bytes | **1.96 MiB** |
| Total Working Set | **11.08 MiB** |

Measured on a local Windows machine using an MSVC x64 Release build with a statically linked CRT: wait 8 seconds after launch, then take 5 samples at one-second intervals. The working set was not deliberately trimmed. No increase in CPU time was recorded during this idle sampling window.

**The ~2 MiB figure is private bytes at startup idle, not total memory or a limit after use.** Toggling icons loads Windows Shell / MSAA COM modules. A later sample after use recorded approximately **35.42 MiB total working set and 10.30 MiB private bytes**. Task Manager figures, working set, and private bytes use different accounting methods and are not interchangeable. Actual usage varies with the system and app state.

Measurement conditions and acceptance results are documented in [VALIDATION.md](docs/VALIDATION.md) (Chinese).

## Features

- **Double-click to toggle:** hide or show all desktop icons by double-clicking empty desktop space.
- **Keep your desktop intact:** only change Windows icon visibility; files and icon positions stay in place.
- **Follow system settings:** use Windows double-click timing, position thresholds, and the primary mouse button.
- **Distinguish clicks from selection drags:** track complete button-down / button-up pairs and exclude icons, labels, normal windows, the taskbar, and drag selection.
- **Tray controls:** toggle icons manually, pause or resume listening, enable startup, or quit.
- **Optional startup:** disabled by default and enabled through the tray menu.
- **Single instance:** launching again does not add another listener.
- **Free and open source:** available under the MIT license.

## Getting started

Requires **Windows 10 / 11 x64**. Runs with standard user permissions.

The project is named DeskVeil. Current executables, packages, and build targets still use the `DeskHide` name; the commands below match those artifacts.

1. Run `DeskHide-1.0.0-x64-Setup.exe`, or extract `DeskHide-1.0.0-x64-portable.zip` and launch `DeskHide.exe`.
2. Find the app in the system tray. Expand the hidden tray icons if necessary.
3. Double-click empty desktop space to hide icons. Double-click again to restore them.

Left-click or right-click the tray icon to open the menu. The app's current UI is in Chinese. If you enable startup for the portable edition, keep the executable in the same location; after moving it, enable startup again from the new location.

Pausing or quitting stops double-click listening and keeps the current icon visibility. To restore icons manually, right-click the desktop → **View → Show desktop icons**. Avoid running another desktop double-click toggle utility at the same time, as one gesture may toggle icons twice.

## Command line

Use these commands in shortcuts or local scripts:

```powershell
.\DeskHide.exe --toggle  # Toggle icon visibility
.\DeskHide.exe --show    # Show icons
.\DeskHide.exe --hide    # Hide icons
.\DeskHide.exe --quit    # Quit the resident instance
```

Commands are forwarded to a resident instance if one exists. Otherwise, the first three perform the action once and exit without starting a resident listener.

## Build from source

Requires Visual Studio 2022 / 2026 with Desktop development with C++, the Windows SDK, CMake 3.21+, and Ninja. Packaging also requires Inno Setup 6. These are development tools; users do not need them to run the app.

```powershell
# Build Release and run targeted tests
powershell -ExecutionPolicy Bypass -File scripts/build.ps1

# Also generate the installer, portable archive, source archive, and checksums
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Package -IsccPath "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
```

Outputs are placed in `artifacts`. The default build runs the double-click state machine and isolated registry tests. If the app is running from the build directory, quit it through the tray first, or use `-BuildDirectory out/release` for a separate build directory.

The desktop integration test briefly toggles real desktop icons and restores their initial visibility. Confirm no other similar utility is listening before running it:

```powershell
.\build\windows_tests.exe --desktop
.\build\windows_tests.exe --status
```

## Implementation boundaries

- A dedicated mouse hook thread forwards input events. Every hook event continues through the hook chain; the hook does not perform COM, file, registry, or synchronous cross-process messaging operations.
- MSAA distinguishes empty desktop space from icons. Windows Shell `IFolderView2` reads and changes the actual visibility, and `SHGetSetSettings` persists it.
- Enabling startup explicitly writes only this app's `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\DeskHide` value.
- The tray icon is registered again when Explorer recreates the tray. Each toggle obtains the Shell View again.

See [PLAN.md](docs/PLAN.md) for the design and [VALIDATION.md](docs/VALIDATION.md) for manual acceptance status, including mixed-DPI monitors, Explorer restarts, and installation / upgrade / uninstall testing. Both documents are in Chinese.

## License and acknowledgments

Licensed under the [MIT License](LICENSE). Project reference: [DesktopIconToggle](https://github.com/hexandcube/DesktopIconToggle). See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for attribution and license details.
