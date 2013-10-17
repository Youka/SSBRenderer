#pragma once

#include <windows.h>
#include <string>

// Convert UTF-16 text to UTF-8 text
inline std::string utf16_to_utf8(std::wstring& ws){
    std::string s(WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.length(), 0, 0, 0, 0), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.data(), ws.length(), const_cast<char*>(s.data()), s.length(), 0, 0);
    return s;
}

// Convert UTF-8 text to UTF-16 text
inline std::wstring utf8_to_utf16(std::string& s){
    std::wstring ws(MultiByteToWideChar(CP_UTF8, 0, s.data(), s.length(), 0, 0), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), s.length(), const_cast<wchar_t*>(ws.data()), ws.length());
    return ws;
}
