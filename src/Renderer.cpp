#include "Renderer.hpp"

Renderer::Renderer(int width, int height, bool has_alpha, double fps, std::string& script, bool warnings)
: width(width), height(height), has_alpha(has_alpha), fps(fps), script(script), warnings(warnings){
    #pragma message "Implent SSB parser"
}

void Renderer::Render(unsigned char* image, int pitch, int frame_index){
    #pragma message "Implent SSB rendering"
}

void Renderer::Render(unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms){
    #pragma message "Implent SSB rendering"
}
