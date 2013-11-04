#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <muParser.h>
#define M_PI 3.14159265358979323846  // Missing in math header because of strict ANSI C

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()), cache({nullptr, CairoImage()}){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

// Helper functions for rendering
namespace{
    // Converts SSB geometry to cairo path
    void geometry_to_path(SSBGeometry* geometry, cairo_t* ctx){
        switch(geometry->type){
            case SSBGeometry::Type::POINTS:
                {
                    double width = cairo_get_line_width(ctx);
                    if(width > 1)
                        for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points){
                            cairo_new_sub_path(ctx);
                            cairo_arc(ctx, point.x, point.y, width / 2, 0, M_PI * 2);
                            cairo_close_path(ctx);
                        }
                    else
                        for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points)
                            cairo_rectangle(ctx, point.x, point.y, width, width);   // Creates a move + lines + close = closed shape
                }
                break;
            case SSBGeometry::Type::PATH:
                {
                    const std::vector<SSBPath::Segment>& segments = dynamic_cast<SSBPath*>(geometry)->segments;
                    for(size_t i = 0; i < segments.size();)
                        switch(segments[i].type){
                            case SSBPath::SegmentType::MOVE_TO:
                                cairo_move_to(ctx, segments[i].point.x, segments[i].point.y);
                                ++i;
                                break;
                            case SSBPath::SegmentType::LINE_TO:
                                cairo_line_to(ctx, segments[i].point.x, segments[i].point.y);
                                ++i;
                                break;
                            case SSBPath::SegmentType::CURVE_TO:
                                cairo_curve_to(ctx,
                                                segments[i].point.x, segments[i].point.y,
                                                segments[i+1].point.x, segments[i+1].point.y,
                                                segments[i+2].point.x, segments[i+2].point.y);
                                i += 3;
                                break;
                            case SSBPath::SegmentType::ARC_TO:
                                if(cairo_has_current_point(ctx)){
                                    double lx, ly; cairo_get_current_point(ctx, &lx, &ly);
                                    double xc = segments[i].point.x,
                                            yc = segments[i].point.y,
                                            r = hypot(ly - yc, lx - xc),
                                            angle1 = atan2(ly - yc, lx - xc),
                                            angle2 = angle1 + segments[i+1].angle * M_PI / 180.0L;
                                    if(angle2 > angle1)
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
    #pragma message "Implent SSB text paths"
                cairo_select_font_face(ctx, "Times New Roman", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(ctx, 50);
                cairo_move_to(ctx, 0, 150);
                cairo_text_path(ctx, dynamic_cast<SSBText*>(geometry)->text.c_str());
                break;
        }
    }
}

void Renderer::render(unsigned char* image, int pitch, unsigned long long start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms)
            // Process SSB objects
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){

                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Create path

                    // Create image with fitting size

                    // Draw on image

                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
#pragma message "Implent SSB rendering"
}
