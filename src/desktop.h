#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <wrl/client.h>

namespace deskh {
class Desktop {
public:
    bool isBlank(POINT point, HWND observedWindow);
    HRESULT hidden(bool& value);
    HRESULT setHidden(bool value);
    HRESULT toggle();
private:
    HRESULT folderView(Microsoft::WRL::ComPtr<IFolderView2>& view);
};
}
