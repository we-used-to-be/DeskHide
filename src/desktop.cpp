#include "desktop.h"
#include <exdisp.h>
#include <servprov.h>
#include <shlguid.h>
#include <shlobj.h>
#include <oleacc.h>

namespace deskh {
using Microsoft::WRL::ComPtr;

HRESULT Desktop::folderView(ComPtr<IFolderView2>& view) {
    // Reacquire per operation: no stale Explorer COM proxy after a shell restart.
    ComPtr<IShellWindows> windows;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER,
                                  IID_PPV_ARGS(&windows));
    if (FAILED(hr)) return hr;
    VARIANT location{}, empty{};
    long handle = 0;
    ComPtr<IDispatch> dispatch;
    hr = windows->FindWindowSW(&location, &empty, SWC_DESKTOP, &handle,
                              SWFO_NEEDDISPATCH, &dispatch);
    if (hr != S_OK || !dispatch) return FAILED(hr) ? hr : E_FAIL;
    ComPtr<IServiceProvider> provider;
    hr = dispatch.As(&provider);
    if (FAILED(hr)) return hr;
    ComPtr<IShellBrowser> browser;
    hr = provider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser));
    if (FAILED(hr)) return hr;
    ComPtr<IShellView> shellView;
    hr = browser->QueryActiveShellView(&shellView);
    return FAILED(hr) ? hr : shellView.As(&view);
}

HRESULT Desktop::hidden(bool& value) {
    ComPtr<IFolderView2> view;
    HRESULT hr = folderView(view);
    DWORD flags = 0;
    if (SUCCEEDED(hr)) hr = view->GetCurrentFolderFlags(&flags);
    if (SUCCEEDED(hr)) value = (flags & FWF_NOICONS) != 0;
    return hr;
}

HRESULT Desktop::setHidden(bool value) {
    ComPtr<IFolderView2> view;
    HRESULT hr = folderView(view);
    if (FAILED(hr)) return hr;
    hr = view->SetCurrentFolderFlags(FWF_NOICONS, value ? FWF_NOICONS : 0);
    if (FAILED(hr)) return hr;
    DWORD flags = 0;
    hr = view->GetCurrentFolderFlags(&flags);
    if (FAILED(hr) || ((flags & FWF_NOICONS) != 0) != value) return E_FAIL;
    SHELLSTATE state{};
    state.fHideIcons = value;
    SHGetSetSettings(&state, SSF_HIDEICONS, TRUE);
    return S_OK;
}

HRESULT Desktop::toggle() {
    bool value = false;
    const HRESULT hr = hidden(value);
    return FAILED(hr) ? hr : setHidden(!value);
}

static bool classIs(HWND window, const wchar_t* name) {
    wchar_t actual[64]{};
    return GetClassNameW(window, actual, 64) && lstrcmpW(actual, name) == 0;
}

bool Desktop::isBlank(POINT point, HWND observedWindow) {
    // Reject overlays, normal folders, taskbar, menus, icon edit boxes, and
    // windows that moved between capture and handling the event.
    if (!observedWindow || WindowFromPoint(point) != observedWindow) return false;
    DWORD shellPid = 0, pointPid = 0;
    GetWindowThreadProcessId(GetShellWindow(), &shellPid);
    GetWindowThreadProcessId(observedWindow, &pointPid);
    if (!shellPid || shellPid != pointPid) return false;
    const HWND root = GetAncestor(observedWindow, GA_ROOT);
    if (!classIs(root, L"Progman") && !classIs(root, L"WorkerW")) return false;
    if (classIs(observedWindow, L"SysListView32")) {
        if (!classIs(GetParent(observedWindow), L"SHELLDLL_DefView")) return false;
        ComPtr<IAccessible> accessible;
        HRESULT hr = AccessibleObjectFromWindow(observedWindow, static_cast<DWORD>(OBJID_CLIENT),
                                                 IID_PPV_ARGS(&accessible));
        if (FAILED(hr)) return false;
        VARIANT child{};
        hr = accessible->accHitTest(point.x, point.y, &child);
        const bool blank = hr == S_OK && child.vt == VT_I4 && child.lVal == CHILDID_SELF;
        VariantClear(&child);
        return blank;
    }
    // With icons hidden the list window is absent from hit testing. Only the
    // actual desktop root/DefView is allowed, never an arbitrary child window.
    return observedWindow == root || classIs(observedWindow, L"SHELLDLL_DefView");
}
}
