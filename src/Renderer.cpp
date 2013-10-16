#include "Renderer.hpp"

Renderer::Renderer(int width, int height, bool has_alpha, double fps, int frames, std::string script, bool warnings)
: width(width), height(height), frames(frames), has_alpha(has_alpha), fps(fps), script(script), warnings(warnings){
    #pragma message "Implent SSB parser"
}

void Renderer::Render(unsigned char* image, int pitch, int frame_index){
    #pragma message "Implent SSB rendering"
}
