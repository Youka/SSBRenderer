#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <limits>
#include <muParser.h>
#define M_PI 3.14159265358979323846  // Missing in math header because of strict ANSI C

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

// Helper structures & functions for rendering
namespace{
    // Render state palette
    struct RenderStatePalette{
        // Font
        std::string font_family = "Arial";
        bool bold = false, italic = false, underline = false, strikeout = false;
        unsigned short int font_size = 30;
        SSBCoord font_space_h = 0, font_space_v = 0;
        // Line
        SSBCoord line_width = 2;
        cairo_line_join_t join = CAIRO_LINE_JOIN_ROUND;
        cairo_line_cap_t cap = CAIRO_LINE_CAP_ROUND;
        SSBCoord dash_offset = 0;
        std::vector<SSBCoord> dashes;
        // Geometry
        SSBMode::Mode mode = SSBMode::Mode::FILL;
        std::string deform_x, deform_y;
        // Position
        SSBCoord pos_x = std::numeric_limits<SSBCoord>::max(), pos_y = std::numeric_limits<SSBCoord>::max();
        SSBAlign::Align align = SSBAlign::Align::CENTER_BOTTOM;
        SSBCoord margin_h = 0, margin_v = 0;
        double direction_angle = 0;
        // Transformation
        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
        // Color
#pragma message "Implent render state palette"
    };
    // Updates render state palette by SSB tag
    void tag_to_render_state_palette(SSBTag* tag, RenderStatePalette& rsp){
        switch(tag->type){
#pragma message "Implent updater for render state palette by tag"
        }
    }
    // Converts SSB geometry to cairo path
    void geometry_to_path(SSBGeometry* geometry, RenderStatePalette& rsp, cairo_t* ctx){
        switch(geometry->type){
            case SSBGeometry::Type::POINTS:
                if(rsp.line_width > 1)
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points){
                        cairo_new_sub_path(ctx);
                        cairo_arc(ctx, point.x, point.y, rsp.line_width / 2, 0, M_PI * 2);
                        cairo_close_path(ctx);
                    }
                else
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points)
                        cairo_rectangle(ctx, point.x, point.y, rsp.line_width, rsp.line_width);   // Creates a move + lines + close = closed shape
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
                break;
        }
    }
}

void Renderer::render(unsigned char* image, int pitch, unsigned long long start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms){
            // Create render state palette for rendering behaviour
            RenderStatePalette rsp;
            // Process SSB objects of event
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    // Apply tag to render state palette
                    tag_to_render_state_palette(dynamic_cast<SSBTag*>(obj.get()), rsp);
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Set transformations

                    // Apply geometry to image path
                    geometry_to_path(dynamic_cast<SSBGeometry*>(obj.get()), rsp, this->path_buffer);
                    // Create image with fitting size

                    // Draw on image

                    // Clear image path

                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
#pragma message "Implent SSB rendering"
        }
}
