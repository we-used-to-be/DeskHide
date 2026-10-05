#include "desktop.h"
#include "startup.h"
#include <cstdio>
#include <string>

static BOOL CALLBACK scanMonitor(HMONITOR, HDC, LPRECT rect, LPARAM data) {
    auto* desktop = reinterpret_cast<deskh::Desktop*>(data);
    unsigned blanks = 0, listPoints = 0, rejected = 0;
    POINT firstBlank{-1, -1}, firstList{-1, -1};
    for (LONG y = rect->top + 10; y < rect->bottom; y += 25) {
        for (LONG x = rect->left + 10; x < rect->right; x += 25) {
            const POINT p{x, y};
            const HWND window = WindowFromPoint(p);
            wchar_t name[64]{}; GetClassNameW(window, name, 64);
            const bool blank = desktop->isBlank(p, window);
            if (blank) { ++blanks; if (firstBlank.x == -1) firstBlank = p; }
            else if (wcscmp(name, L"SysListView32") == 0) { ++listPoints; if (firstList.x == -1) firstList = p; }
            else ++rejected;
        }
    }
    std::printf("monitor=(%ld,%ld,%ld,%ld) blanks=%u listNonblank=%u rejected=%u firstBlank=(%ld,%ld) firstListNonblank=(%ld,%ld)\n",
        rect->left, rect->top, rect->right, rect->bottom, blanks, listPoints, rejected,
        firstBlank.x, firstBlank.y, firstList.x, firstList.y);
    return TRUE;
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    if (wcscmp(argv[1], L"--registry") == 0) {
        const auto key = L"Software\\DeskHide.Tests." + std::to_wstring(GetCurrentProcessId());
        const std::wstring path = L"D:\\中文 路径\\DeskHide.exe";
        bool pass = !deskh::startupEnabled(path, key.c_str());
        pass &= deskh::setStartup(true, path, key.c_str()) == ERROR_SUCCESS;
        pass &= deskh::startupEnabled(path, key.c_str());
        pass &= !deskh::startupEnabled(L"D:\\Elsewhere\\DeskHide.exe", key.c_str());
        pass &= deskh::setStartup(false, path, key.c_str()) == ERROR_SUCCESS;
        pass &= !deskh::startupEnabled(path, key.c_str());
        pass &= deskh::setStartup(false, path, key.c_str()) == ERROR_SUCCESS;
        // Remove only this test's empty registry key; never any filesystem data.
        RegDeleteKeyW(HKEY_CURRENT_USER, key.c_str());
        std::printf("%s: startup opt-in, quoted Unicode path, disable and idempotence\n", pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;
    deskh::Desktop desktop;
    bool initial = false;
    HRESULT hr = desktop.hidden(initial);
    if (FAILED(hr)) { std::printf("FAIL: desktop unavailable 0x%08lx\n", hr); CoUninitialize(); return 1; }
    if (wcscmp(argv[1], L"--status") == 0) {
        POINT p{}; GetCursorPos(&p);
        const HWND window = WindowFromPoint(p);
        wchar_t name[128]{}; GetClassNameW(window, name, 128);
        std::wprintf(L"hidden=%d cursor=(%ld,%ld) window=%s blank=%d\n", initial, p.x, p.y, name, desktop.isBlank(p, window));
        CoUninitialize(); return 0;
    }
    if (wcscmp(argv[1], L"--scan") == 0) {
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        EnumDisplayMonitors(nullptr, nullptr, scanMonitor, reinterpret_cast<LPARAM>(&desktop));
        CoUninitialize(); return 0;
    }
    if (wcscmp(argv[1], L"--desktop") != 0) { CoUninitialize(); return 2; }
    bool pass = SUCCEEDED(desktop.setHidden(!initial));
    bool changed = initial;
    pass &= SUCCEEDED(desktop.hidden(changed)) && changed != initial;
    const HRESULT restored = desktop.setHidden(initial);
    bool after = !initial;
    pass &= SUCCEEDED(restored) && SUCCEEDED(desktop.hidden(after)) && after == initial;
    std::printf("%s: real Explorer toggle + readback + original-state restoration (initial=%d)\n", pass ? "PASS" : "FAIL", initial);
    CoUninitialize();
    return pass ? 0 : 1;
}
