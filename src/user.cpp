#include "user.hpp"
#include "Renderer.hpp"
#include <cstring>

ssb_renderer ssb_create_renderer(int width, int height, int has_alpha, int aligned, const char* script, char* warning){
    try{
        std::string script_string = script;
        return new Renderer(width, height, has_alpha, aligned, script_string, warning != 0);
    }catch(std::string err){
        if(warning)
            strncpy(warning, err.c_str(), SSB_WARNING_LENGTH);
        return 0;
    }
}

void ssb_render(ssb_renderer renderer, unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms){
    reinterpret_cast<Renderer*>(renderer)->render(image, pitch, start_ms, end_ms);
}

void ssb_free_renderer(ssb_renderer renderer){
    if(renderer)
        delete reinterpret_cast<Renderer*>(renderer);
}
