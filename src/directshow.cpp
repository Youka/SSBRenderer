#include <windows.h>
#include "module.hpp"

#pragma message "Implent directshow complete!"

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
