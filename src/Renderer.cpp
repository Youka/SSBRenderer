#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <cairo.h>
#include <muParser.h>

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

void Renderer::render(unsigned char* image, int pitch, unsigned long long start_ms, unsigned long long) noexcept{
#pragma message "Implent SSB rendering"
    // Test muParser
    mu::Parser parser;
    parser.DefineConst("x", start_ms);
    parser.SetExpr("(sin(x/100)+1)*2");
    double mu_var = 2;
    try{
        mu_var = parser.Eval();
    }catch(mu::Parser::exception_type& e){}
    // Test cairo
    if(this->format == Colorspace::BGRA || this->format == Colorspace::BGRX)
        for(auto it = this->ssb.lines.begin(); it != this->ssb.lines.end(); it++)
            if(start_ms >= (*it).start_ms && start_ms < (*it).end_ms){
                cairo_surface_t* surface = cairo_image_surface_create_for_data(image, this->format == Colorspace::BGRA ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, this->width, this->height, pitch);
                cairo_t* ctx = cairo_create(surface);
                cairo_scale(ctx, 1, -1);
                cairo_translate(ctx, 0, -height);
                cairo_move_to(ctx, 0, 0);
                cairo_line_to(ctx, 200, 200);
                cairo_set_source_rgba(ctx, 1, 1, 0, 0.75);
                const double dashes[3] = {1,5,3};
                cairo_set_dash(ctx, dashes, sizeof(dashes) / sizeof(*dashes), 0.0L);
                cairo_set_line_width(ctx, mu_var);
                cairo_stroke(ctx);
                cairo_destroy(ctx);
                cairo_surface_destroy(surface);
            }
}
