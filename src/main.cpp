#include "click_detector.h"
#include "desktop.h"
#include "startup.h"
#include <shellapi.h>
#include <atomic>
#include <algorithm>
#include <new>

namespace {
constexpr wchar_t WindowClass[] = L"DeskHide.Tray.1";
constexpr wchar_t MutexName[] = L"Local\\DeskHide.7D56E20F-7281-4FB2-82A4-5D9C0712B036";
constexpr UINT TrayMessage = WM_APP + 1, MouseMessage = WM_APP + 2, CommandMessage = WM_APP + 3;
constexpr UINT Toggle = 100, Show = 101, Hide = 102, Pause = 103, Startup = 104, About = 105, Quit = 106;
constexpr UINT LanguageChinese = 107, LanguageEnglish = 108;
constexpr UINT_PTR TrayRetry = 1;
struct MouseEvent { UINT message; POINT point; DWORD time; HWND window; bool modified; };
HWND appWindow = nullptr;
HHOOK mouseHook = nullptr;
HANDLE hookThread = nullptr, hookReady = nullptr;
DWORD hookThreadId = 0;
std::atomic<bool> paused{false};
std::atomic<unsigned> pendingEvents{0};
std::atomic<bool> lostEvent{false};
deskh::ClickDetector clicks;
deskh::Desktop desktop;
deskh::Limits limits;
bool swapped = false, trayPresent = false;
bool english = false;
UINT taskbarCreated = 0;
HICON activeIcon = nullptr;
NOTIFYICONDATAW tray{};

void updateTray(bool add);

const wchar_t* text(const wchar_t* chinese, const wchar_t* englishText) {
    return english ? englishText : chinese;
}

void setLanguage(bool useEnglish) {
    english = useEnglish;
    SetWindowTextW(appWindow, text(L"桌隐", L"DeskHide"));
    updateTray(false);
}

void notifyError(const wchar_t* message) {
    tray.uFlags = NIF_INFO;
    lstrcpynW(tray.szInfoTitle, text(L"桌隐", L"DeskHide"), ARRAYSIZE(tray.szInfoTitle));
    lstrcpynW(tray.szInfo, message, ARRAYSIZE(tray.szInfo));
    tray.dwInfoFlags = NIIF_WARNING;
    Shell_NotifyIconW(NIM_MODIFY, &tray);
}

void updateTray(bool add) {
    tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    tray.hIcon = paused ? LoadIconW(nullptr, IDI_WARNING) : activeIcon;
    lstrcpynW(tray.szTip,
        paused ? text(L"桌隐 · 双击监听已暂停", L"DeskHide · Double-click listening is paused")
               : text(L"桌隐 · 双击桌面空白处隐藏/显示图标", L"DeskHide · Double-click the desktop to toggle icons"),
        ARRAYSIZE(tray.szTip));
    if (Shell_NotifyIconW(add ? NIM_ADD : NIM_MODIFY, &tray)) {
        trayPresent = true;
        if (add) {
            tray.uVersion = NOTIFYICON_VERSION_4;
            Shell_NotifyIconW(NIM_SETVERSION, &tray);
        }
        KillTimer(appWindow, TrayRetry);
    } else {
        trayPresent = false;
        SetTimer(appWindow, TrayRetry, 2000, nullptr);
    }
}

void readSettings() {
    limits.milliseconds = GetDoubleClickTime();
    limits.doubleX = std::max(1, GetSystemMetrics(SM_CXDOUBLECLK) / 2);
    limits.doubleY = std::max(1, GetSystemMetrics(SM_CYDOUBLECLK) / 2);
    limits.dragX = std::max(1, GetSystemMetrics(SM_CXDRAG) / 2);
    limits.dragY = std::max(1, GetSystemMetrics(SM_CYDRAG) / 2);
    swapped = GetSystemMetrics(SM_SWAPBUTTON) != 0;
    clicks.reset();
}

LRESULT CALLBACK mouseProc(int code, WPARAM message, LPARAM data) {
    if (code == HC_ACTION && message != WM_MOUSEMOVE && !paused.load(std::memory_order_relaxed)) {
        // Bounded work only. No COM, registry, disk or cross-process SendMessage.
        if (pendingEvents.load(std::memory_order_relaxed) >= 64) {
            lostEvent.store(true);
        } else {
            const auto* input = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
            auto* event = new (std::nothrow) MouseEvent{
                static_cast<UINT>(message), input->pt, input->time, WindowFromPoint(input->pt),
                ((GetAsyncKeyState(VK_CONTROL) | GetAsyncKeyState(VK_SHIFT) | GetAsyncKeyState(VK_MENU)) & 0x8000) != 0
            };
            if (event) {
                pendingEvents.fetch_add(1);
                if (!PostMessageW(appWindow, MouseMessage, 0, reinterpret_cast<LPARAM>(event))) {
                    pendingEvents.fetch_sub(1);
                    delete event;
                    lostEvent.store(true);
                }
            } else lostEvent.store(true);
        }
    }
    return CallNextHookEx(nullptr, code, message, data);
}

DWORD WINAPI hookMain(void*) {
    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    mouseHook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, GetModuleHandleW(nullptr), 0);
    SetEvent(hookReady);
    if (!mouseHook) return 1;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    UnhookWindowsHookEx(mouseHook);
    return 0;
}

void handleMouse(const MouseEvent& event) {
    if (paused || lostEvent.exchange(false) || event.modified || GetTickCount() - event.time > 1000) {
        clicks.reset();
        return;
    }
    const UINT down = swapped ? WM_RBUTTONDOWN : WM_LBUTTONDOWN;
    const UINT up = swapped ? WM_RBUTTONUP : WM_LBUTTONUP;
    if (event.message == down || event.message == up) {
        const bool blank = desktop.isBlank(event.point, event.window);
        const deskh::Point point{event.point.x, event.point.y};
        if (event.message == down) clicks.press(point, event.time, blank);
        else if (clicks.release(point, blank, limits) && FAILED(desktop.toggle()))
            notifyError(text(L"暂时无法切换桌面图标，请等待资源管理器就绪后重试。",
                             L"Could not toggle desktop icons. Wait for File Explorer and try again."));
    } else clicks.reset();
}

void runCommand(UINT command) {
    clicks.reset();
    HRESULT result = S_OK;
    switch (command) {
    case Toggle: result = desktop.toggle(); break;
    case Show: result = desktop.setHidden(false); break;
    case Hide: result = desktop.setHidden(true); break;
    case Pause:
        paused = !paused.load();
        updateTray(false);
        break;
    case Startup: {
        const auto path = deskh::executablePath();
        if (deskh::setStartup(!deskh::startupEnabled(path), path) != ERROR_SUCCESS)
            notifyError(text(L"无法更改当前用户的开机启动项。",
                             L"Could not change the startup setting for this user."));
        break;
    }
    case About:
        MessageBoxW(appWindow,
            text(L"桌隐 DeskHide 1.0.0\n\n双击桌面空白处隐藏或显示图标。\n右键托盘可暂停监听、设置开机启动或退出。\n\n原生 C++ / Win32 · 无运行库安装 · 无联网\n项目参考：DesktopIconToggle。\n项目地址：https://github.com/we-used-to-be/DeskHide\n许可信息见随附 LICENSE 与 THIRD_PARTY_NOTICES.md。",
                 L"DeskHide 1.0.0\n\nDouble-click an empty area of the desktop to hide or show icons.\nUse the tray menu to pause listening, start with Windows, or quit.\n\nNative C++ / Win32 · No runtime installation · No network access\nProject reference: DesktopIconToggle.\nProject: https://github.com/we-used-to-be/DeskHide\nSee LICENSE and THIRD_PARTY_NOTICES.md for license information."),
            text(L"关于桌隐", L"About DeskHide"), MB_OK | MB_ICONINFORMATION);
        break;
    case Quit: DestroyWindow(appWindow); break;
    case LanguageChinese: setLanguage(false); break;
    case LanguageEnglish: setLanguage(true); break;
    default: break;
    }
    if (FAILED(result)) notifyError(text(L"桌面尚未就绪，或 Windows Shell 拒绝了请求。请稍后重试。",
                                         L"The desktop is not ready, or Windows Shell rejected the request. Try again later."));
}

void showMenu() {
    clicks.reset();
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    bool hidden = false;
    const bool known = SUCCEEDED(desktop.hidden(hidden));
    AppendMenuW(menu, MF_STRING, Toggle, known
        ? (hidden ? text(L"显示桌面图标", L"Show desktop icons") : text(L"隐藏桌面图标", L"Hide desktop icons"))
        : text(L"切换桌面图标", L"Toggle desktop icons"));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (paused ? MF_CHECKED : 0), Pause, text(L"暂停双击监听", L"Pause double-click listening"));
    AppendMenuW(menu, MF_STRING | (deskh::startupEnabled(deskh::executablePath()) ? MF_CHECKED : 0), Startup, text(L"开机自动启动", L"Start with Windows"));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    HMENU languageMenu = CreatePopupMenu();
    if (languageMenu) {
        AppendMenuW(languageMenu, MF_STRING | (!english ? MF_CHECKED : 0), LanguageChinese, L"简体中文");
        AppendMenuW(languageMenu, MF_STRING | (english ? MF_CHECKED : 0), LanguageEnglish, L"English");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(languageMenu), text(L"语言 / Language", L"Language / 语言"));
    }
    AppendMenuW(menu, MF_STRING, About, text(L"关于桌隐", L"About DeskHide"));
    AppendMenuW(menu, MF_STRING, Quit, text(L"退出", L"Quit"));
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(appWindow);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, point.x, point.y, 0, appWindow, nullptr);
    PostMessageW(appWindow, WM_NULL, 0, 0);
    DestroyMenu(menu);
    if (command) runCommand(command);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (taskbarCreated && message == taskbarCreated) { clicks.reset(); updateTray(true); return 0; }
    switch (message) {
    case MouseMessage: {
        auto* event = reinterpret_cast<MouseEvent*>(lparam);
        pendingEvents.fetch_sub(1);
        handleMouse(*event);
        delete event;
        return 0;
    }
    case CommandMessage: runCommand(static_cast<UINT>(wparam)); return 0;
    case TrayMessage:
        if (LOWORD(lparam) == WM_CONTEXTMENU || LOWORD(lparam) == NIN_SELECT || LOWORD(lparam) == NIN_KEYSELECT) showMenu();
        return 0;
    case WM_TIMER: if (wparam == TrayRetry) updateTray(!trayPresent); return 0;
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE: readSettings(); return 0;
    case WM_POWERBROADCAST: clicks.reset(); return TRUE;
    case WM_QUERYENDSESSION: return TRUE;
    case WM_ENDSESSION: if (wparam) DestroyWindow(window); return 0;
    case WM_CLOSE: DestroyWindow(window); return 0;
    case WM_DESTROY:
        paused = true;
        KillTimer(window, TrayRetry);
        Shell_NotifyIconW(NIM_DELETE, &tray);
        PostQuitMessage(0);
        return 0;
    default: return DefWindowProcW(window, message, wparam, lparam);
    }
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int) {
    UINT command = 0;
    if (*arguments) {
        if (lstrcmpW(arguments, L"--toggle") == 0) command = Toggle;
        else if (lstrcmpW(arguments, L"--show") == 0) command = Show;
        else if (lstrcmpW(arguments, L"--hide") == 0) command = Hide;
        else if (lstrcmpW(arguments, L"--quit") == 0) command = Quit;
        else return 2;
    }
    // CLI does not launch a resident instance; handy for shortcuts and recovery.
    if (command) {
        if (const HWND running = FindWindowW(WindowClass, nullptr))
            return PostMessageW(running, CommandMessage, command, 0) ? 0 : 1;
        if (command == Quit) return 0;
        if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;
        const HRESULT hr = command == Toggle ? desktop.toggle() : desktop.setHidden(command == Hide);
        CoUninitialize();
        return FAILED(hr) ? 1 : 0;
    }
    HANDLE mutex = CreateMutexW(nullptr, FALSE, MutexName);
    if (!mutex) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) { CloseHandle(mutex); return 1; }
    WNDCLASSW wc{};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = WindowClass;
    RegisterClassW(&wc);
    // A hidden top-level window receives TaskbarCreated broadcasts; HWND_MESSAGE does not.
    appWindow = CreateWindowExW(WS_EX_TOOLWINDOW, WindowClass, text(L"桌隐", L"DeskHide"), WS_POPUP,
        0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!appWindow) { CoUninitialize(); CloseHandle(mutex); return 1; }
    readSettings();
    taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    activeIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR));
    tray.cbSize = sizeof(tray);
    tray.hWnd = appWindow;
    tray.uID = 1;
    tray.uCallbackMessage = TrayMessage;
    updateTray(true);
    hookReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (hookReady) hookThread = CreateThread(nullptr, 0, hookMain, nullptr, 0, &hookThreadId);
    if (hookThread) WaitForSingleObject(hookReady, INFINITE);
    const bool ready = hookThread && mouseHook;
    if (!ready) {
        MessageBoxW(nullptr, text(L"无法启动鼠标监听，桌隐将退出。", L"Could not start mouse monitoring. DeskHide will exit."),
                    text(L"桌隐", L"DeskHide"), MB_OK | MB_ICONERROR);
        DestroyWindow(appWindow);
    }
    MSG message{};
    if (ready) while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (hookThread) {
        PostThreadMessageW(hookThreadId, WM_QUIT, 0, 0);
        WaitForSingleObject(hookThread, INFINITE);
        CloseHandle(hookThread);
    }
    while (PeekMessageW(&message, nullptr, MouseMessage, MouseMessage, PM_REMOVE))
        delete reinterpret_cast<MouseEvent*>(message.lParam);
    if (hookReady) CloseHandle(hookReady);
    if (activeIcon) DestroyIcon(activeIcon);
    CoUninitialize();
    CloseHandle(mutex);
    return ready ? 0 : 1;
}
