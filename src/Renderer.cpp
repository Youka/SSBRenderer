#include "Renderer.hpp"
#include "SSBParser.hpp"

Renderer::Renderer(int width, int height, bool has_alpha, bool aligned, std::string& script, bool warnings)
: width(width), height(height), has_alpha(has_alpha), aligned(aligned), ssb(SSBParser(script, warnings).data()){}

void Renderer::render(unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms){
    #pragma message "Implent SSB rendering"
}
