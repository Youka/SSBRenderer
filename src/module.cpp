#include <windows.h>
#include "module.hpp"

// See "module.hpp"
void* module;
// DLL entry point
BOOL APIENTRY DllMain(HANDLE dll_module, DWORD reason, LPVOID){
    // Save module handle for global access
    if(reason == DLL_PROCESS_ATTACH)
        module = dll_module;
    // No errors
    return TRUE;
}
