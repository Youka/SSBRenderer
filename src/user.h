/*
Project: SSBRenderer
File: user.h

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

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
#   define DLL_EXPORT EXTERN_C __attribute__((visibility("default")))
#endif

/// Renderer handle
typedef void* ssb_renderer;

/// Frame colorspaces
enum {SSB_BGR = 0, SSB_BGRX, SSB_BGRA};

/// Maximal length for output warning of ssb_create_renderer
#define SSB_WARNING_LENGTH 256

/**
Create renderer handle.

@param width Frame width
@param height Frame height
@param format Frame colorspace
@param script SSB script to render
@param warning Output warning, pointer can be zero
@return Renderer handle or zero
*/
DLL_EXPORT ssb_renderer ssb_create_renderer(int width, int height, char format, const char* script, char* warning);

/**
Set target frame information.

@param renderer Renderer handle
@param width Frame width
@param height Frame height
@param format Frame colorspace
*/
DLL_EXPORT void ssb_set_target(ssb_renderer renderer, int width, int height, char format);

/**
Render on image.

@param renderer Renderer handle
@param image Frame data
@param pitch Frame row pitch
@param start_ms Start time of frame in milliseconds
*/
DLL_EXPORT void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, unsigned long int start_ms);

/**
Destroy renderer handle.

@param renderer Renderer handle
*/
DLL_EXPORT void ssb_free_renderer(ssb_renderer renderer);
