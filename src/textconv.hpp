/*
Project: SSBRenderer
File: textconv.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

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
