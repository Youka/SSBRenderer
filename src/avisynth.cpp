#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <windows.h>
#include <avisynth.h>
#include "file_info.hpp"

// Avisynth plugin interface
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env){
    // Valid Avisynth interface version?
    env->CheckVersion();
    // Register functin to Avisynth scripting environment
    env->AddFunction(SSB_NAME, "cs[warnings]b", [](AVSValue args, void*, IScriptEnvironment* env) -> AVSValue {
        return 1;
    }, 0);
    // Return plugin description
    return SSB_DESCRIPTION;
}
