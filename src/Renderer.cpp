#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <cairo.h>
#include <muParser.h>
#define M_PI		3.14159265358979323846  // Missing in math header by strict ANSI C

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
        // Iterate through SSB lines
        for(SSBLine& line : this->ssb.lines)
            // Process active SSB line
            if(start_ms >= line.start_ms && start_ms < line.end_ms){
                // Create reference image + context
                cairo_surface_t* surface = cairo_image_surface_create_for_data(image, this->format == Colorspace::BGRA ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, this->width, this->height, pitch);
                cairo_t* ctx = cairo_create(surface);
                cairo_scale(ctx, 1, -1);
                cairo_translate(ctx, 0, -this->height);
                // Draw geometries
                for(std::shared_ptr<SSBObject>& obj : line.objects)
                    if(obj->type == SSBObject::Type::GEOMETRY){
                        switch(dynamic_cast<SSBGeometry*>(obj.get())->type){
                            case SSBGeometry::Type::POINTS:
                                for(const Point& point : dynamic_cast<SSBPoints*>(obj.get())->points)
                                    cairo_arc(ctx, point.x, point.y, 10, 0, M_PI * 2);
                                break;
                            case SSBGeometry::Type::PATH:
                                {
                                    const SSBPath* ssb_path = dynamic_cast<SSBPath*>(obj.get());
                                    for(size_t i = 0; i < ssb_path->segments.size();)
                                        switch(ssb_path->segments[i].type){
                                            case SSBPath::SegmentType::MOVE_TO:
                                                cairo_move_to(ctx, ssb_path->segments[i].value.point.x, ssb_path->segments[i].value.point.y);
                                                ++i;
                                                break;
                                            case SSBPath::SegmentType::LINE_TO:
                                                cairo_line_to(ctx, ssb_path->segments[i].value.point.x, ssb_path->segments[i].value.point.y);
                                                ++i;
                                                break;
                                            case SSBPath::SegmentType::CURVE_TO:
                                                cairo_curve_to(ctx,
                                                               ssb_path->segments[i].value.point.x, ssb_path->segments[i].value.point.y,
                                                               ssb_path->segments[i+1].value.point.x, ssb_path->segments[i+1].value.point.y,
                                                               ssb_path->segments[i+2].value.point.x, ssb_path->segments[i+2].value.point.y);
                                                i += 3;
                                                break;
                                            case SSBPath::SegmentType::ARC_TO:
                                                if(cairo_has_current_point(ctx)){
                                                    double lx, ly;
                                                    cairo_get_current_point(ctx, &lx, &ly);
                                                    double xc = ssb_path->segments[i].value.point.x;
                                                    double yc = ssb_path->segments[i].value.point.y;
                                                    double r = std::hypot(ly - yc, lx - xc);
                                                    double angle1 = atan2(ly - yc, lx - xc);
                                                    double angle2 = angle1 + ssb_path->segments[i+1].value.angle * M_PI / 180;
                                                    if(ssb_path->segments[i+1].value.angle > 0)
                                                        cairo_arc(ctx,
                                                                  xc, yc,
                                                                  r,
                                                                  angle1, angle2);
                                                    else
                                                        cairo_arc_negative(ctx,
                                                                  xc, yc,
                                                                  r,
                                                                  angle1, angle2);
                                                }
                                                i += 2;
                                                break;
                                            case SSBPath::SegmentType::CLOSE:
                                                cairo_close_path(ctx);
                                                ++i;
                                                break;
                                        }
                                }
                                break;
                            case SSBGeometry::Type::TEXT:
                                cairo_select_font_face(ctx, "Times New Roman", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
                                cairo_set_font_size(ctx, 50);
                                cairo_move_to(ctx, 0, 150);
                                cairo_text_path(ctx, dynamic_cast<SSBText*>(obj.get())->text.c_str());
                                break;
                        }
                        cairo_set_source_rgba(ctx, 1, 1, mu_var/4, 0.75);
                        cairo_fill(ctx);
                    }
                // Free reference image + context
                cairo_destroy(ctx);
                cairo_surface_destroy(surface);
            }
}
