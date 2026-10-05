#include "startup.h"
#include <vector>
namespace deskh {
std::wstring executablePath() {
    std::vector<wchar_t> buffer(32768);
    const DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!count || count >= buffer.size()) return {};
    return {buffer.data(), count};
}
static std::wstring command(const std::wstring& path) { return L"\"" + path + L"\""; }
bool startupEnabled(const std::wstring& path, const wchar_t* key) {
    DWORD bytes = 0;
    if (RegGetValueW(HKEY_CURRENT_USER, key, RunValue, RRF_RT_REG_SZ, nullptr, nullptr, &bytes)
        != ERROR_SUCCESS || bytes > 65536) return false;
    std::vector<wchar_t> value(bytes / sizeof(wchar_t) + 1);
    return RegGetValueW(HKEY_CURRENT_USER, key, RunValue, RRF_RT_REG_SZ, nullptr,
                        value.data(), &bytes) == ERROR_SUCCESS
        && _wcsicmp(value.data(), command(path).c_str()) == 0;
}
LSTATUS setStartup(bool enabled, const std::wstring& path, const wchar_t* key) {
    if (enabled && (path.empty() || path.find(L'"') != std::wstring::npos)) return ERROR_INVALID_PARAMETER;
    HKEY handle = nullptr;
    LSTATUS status = enabled
        ? RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &handle, nullptr)
        : RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_SET_VALUE, &handle);
    if (!enabled && status == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (status != ERROR_SUCCESS) return status;
    const auto value = command(path);
    status = enabled
        ? RegSetValueExW(handle, RunValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()),
                         static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)))
        : RegDeleteValueW(handle, RunValue);
    RegCloseKey(handle);
    return !enabled && status == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : status;
}
}
