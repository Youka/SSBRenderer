#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <cairo.h>

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

void Renderer::render(unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms){
#pragma message "Implent SSB rendering"
    if(this->format == Colorspace::BGRA){
        cairo_surface_t* surface = cairo_image_surface_create_for_data(image, CAIRO_FORMAT_ARGB32, this->width, this->height, this->width << 2);
        cairo_t* ctx = cairo_create(surface);
        cairo_set_source_rgba(ctx, 1, 1, 0, 0.75);
        cairo_move_to(ctx, 0, 0);
        cairo_line_to(ctx, 200, 200);
        const double dashes[3] = {1,5,3};
        cairo_set_dash(ctx, dashes, sizeof(dashes) / sizeof(*dashes), 0.0L);
        cairo_set_line_width(ctx, 5);
        cairo_stroke(ctx);
        cairo_destroy(ctx);
        cairo_surface_destroy(surface);
    }
}
