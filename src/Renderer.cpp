#include "Renderer.hpp"
#include "SSBParser.hpp"

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

void Renderer::render(unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms){
#pragma message "Implent SSB rendering"
}
