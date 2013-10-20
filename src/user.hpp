#pragma once

#ifdef __cplusplus
#  define EXTERN_C extern "C"
#else
#  define EXTERN_C
#endif

#ifdef _WIN32
#   ifdef BUILD_DLL
#       define DLL_EXPORT EXTERN_C __declspec(dllexport)
#   else
#       define DLL_EXPORT EXTERN_C __declspec(dllimport)
#   endif
#else
#   define DLL_EXPORT EXTERN_C __attribute__ ((visibility("default")))
#endif

/// Maximal length for output warning of ssb_create_renderer
#define SSB_WARNING_LENGTH 256

/// Renderer handle
typedef void* ssb_renderer;

/**
Create renderer handle

@param width Frame width
@param height Frame height
@param has_alpha Frame has alpha channel?
@param aligned Frame pixels are 4 byte aligned?
@param script SSB script to render
@param warning Output warning, pointer can be zero
@return Renderer handle or zero
*/
DLL_EXPORT ssb_renderer ssb_create_renderer(int width, int height, int has_alpha, int aligned, const char* script, char* warning);

/**
Set target frame information

@param renderer Renderer handle
@param width Frame width
@param height Frame height
@param has_alpha Frame has alpha channel?
@param aligned Frame pixels are 4 byte aligned?
*/
DLL_EXPORT void ssb_set_target(ssb_renderer renderer, int width, int height, int has_alpha, int aligned);

/**
Render on image

@param renderer Renderer handle
@param image Frame data
@param pitch Frame row pitch
@param start_ms Start time of frame in milliseconds
@param end_ms End time of frame in milliseconds
*/
DLL_EXPORT void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms);

/**
Destroy renderer handle

@param renderer Renderer handle
*/
DLL_EXPORT void ssb_free_renderer(ssb_renderer renderer);
