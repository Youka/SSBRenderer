/*
Project: SSBRenderer
File: module.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include <windows.h>
#include "module.hpp"

// See "module.hpp"
void* module;
// DLL entry point
extern "C" BOOL APIENTRY DllMain(HANDLE dll_module, DWORD reason, LPVOID){
    // Save module handle for global access
    if(reason == DLL_PROCESS_ATTACH)
        module = dll_module;
    // No errors
    return TRUE;
}
