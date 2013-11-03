#include "user.h"
#include "Renderer.hpp"

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

void ssb_set_target(ssb_renderer renderer, int width, int height, char format){
    if(renderer)
        reinterpret_cast<Renderer*>(renderer)->set_target(width, height, format == SSB_BGR ? Renderer::Colorspace::BGR : (format == SSB_BGRX ? Renderer::Colorspace::BGRX : Renderer::Colorspace::BGRA));
}

void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, unsigned long long start_ms){
    if(renderer)
        reinterpret_cast<Renderer*>(renderer)->render(image, pitch, start_ms);
}

void ssb_free_renderer(ssb_renderer renderer){
    if(renderer)
        delete reinterpret_cast<Renderer*>(renderer);
}
