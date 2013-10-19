#pragma once

#ifdef __cplusplus
#  define EXTERN_C extern "C"
#else
#  define EXTERN_C
#endif

#ifdef _WIN32
    #ifdef BUILD_DLL
        #define DLL_EXPORT EXTERN_C __declspec(dllexport)
    #else
        #define DLL_EXPORT EXTERN_C __declspec(dllimport)
    #endif
#else
    #define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

// Maximal length for output warning of ssb_create_renderer
#define SSB_WARNING_LENGTH 256
// Renderer handle
typedef void* ssb_renderer;

// Create renderer handle
DLL_EXPORT ssb_renderer ssb_create_renderer(int width, int height, int has_alpha, int aligned, const char* script, char* warning);
// Render on image with renderer handle
DLL_EXPORT void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms);
// Destroy renderer handle
DLL_EXPORT void ssb_free_renderer(ssb_renderer renderer);
