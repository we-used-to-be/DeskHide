#pragma once
#include <windows.h>
#include <string>
namespace deskh {
inline constexpr wchar_t RunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr wchar_t RunValue[] = L"DeskHide";
std::wstring executablePath();
bool startupEnabled(const std::wstring& path, const wchar_t* key = RunKey);
LSTATUS setStartup(bool enabled, const std::wstring& path, const wchar_t* key = RunKey);
}
