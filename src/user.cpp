/*
Project: SSBRenderer
File: user.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "user.h"
#include "Renderer.hpp"
#include <sstream>

ssb_renderer ssb_create_renderer(int width, int height, char format, const char* script, char* warning){
    try{
        std::string script_string = script;
        return new Renderer(width, height, format == SSB_BGR ? Renderer::Colorspace::BGR : (format == SSB_BGRX ? Renderer::Colorspace::BGRX : Renderer::Colorspace::BGRA), script_string, warning != 0);
    }catch(std::string err){
        if(warning)
            warning[err.copy(warning, SSB_WARNING_LENGTH - 1)] = '\0';
        return 0;
    }
}

ssb_renderer ssb_create_renderer_from_memory(int width, int height, char format, const char* data, char* warning){
    try{
        std::istringstream data_stream(data);
        return new Renderer(width, height, format == SSB_BGR ? Renderer::Colorspace::BGR : (format == SSB_BGRX ? Renderer::Colorspace::BGRX : Renderer::Colorspace::BGRA), data_stream, warning != 0);
    }catch(std::string err){
        if(warning)
            warning[err.copy(warning, SSB_WARNING_LENGTH - 1)] = '\0';
        return 0;
    }
}

void ssb_set_target(ssb_renderer renderer, int width, int height, char format){
    if(renderer)
        reinterpret_cast<Renderer*>(renderer)->set_target(width, height, format == SSB_BGR ? Renderer::Colorspace::BGR : (format == SSB_BGRX ? Renderer::Colorspace::BGRX : Renderer::Colorspace::BGRA));
}

void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, unsigned long int start_ms){
    if(renderer)
        reinterpret_cast<Renderer*>(renderer)->render(image, pitch, start_ms);
}

void ssb_free_renderer(ssb_renderer renderer){
    if(renderer)
        delete reinterpret_cast<Renderer*>(renderer);
}
