/*
Project: SSBRenderer
File: Renderer.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "Renderer.hpp"
#include "SSBParser.hpp"
#include "RenderState.hpp"

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

// Helper functions for rendering
namespace{
    // Converts SSB geometry to cairo path
    inline void geometry_to_path(SSBGeometry* geometry, RenderState& rs, cairo_t* ctx){
        switch(geometry->type){
            case SSBGeometry::Type::POINTS:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(rs.line_width == 1)
#pragma GCC diagnostic pop
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points)
                        cairo_rectangle(ctx, point.x, point.y, rs.line_width, rs.line_width);   // Creates a move + lines + close = closed shape
                else
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points){
                        cairo_new_sub_path(ctx);
                        cairo_arc(ctx, point.x, point.y, rs.line_width / 2, 0, M_PI * 2);
                        cairo_close_path(ctx);
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
                                            angle2 = angle1 + DEG_TO_RAD(segments[i+1].angle);
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
                {
#pragma message "Implent SSB text paths correctly"
                    NativeFont font(rs.font_family, rs.bold, rs.italic, rs.underline, rs.strikeout, rs.font_size);
                    NativeFont::FontMetrics metrics = font.get_metrics();
                    std::stringstream text(dynamic_cast<SSBText*>(geometry)->text);
                    std::string line;
                    cairo_save(ctx);
                    while(std::getline(text, line)){
                        font.text_path_to_cairo(line, ctx);
                        cairo_translate(ctx, 0, metrics.height + metrics.external_lead);
                    }
                    cairo_restore(ctx);
                }
                break;
        }
    }
}

void Renderer::render(unsigned char* image, int pitch, unsigned long int start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms){
            // Create render state for rendering behaviour
            RenderState rs;
            // Process SSB objects of event
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    // Apply tag to render state
                    tag_to_render_state(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms, rs);
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Set transformations

                    // Apply geometry to image path
                    geometry_to_path(dynamic_cast<SSBGeometry*>(obj.get()), rs, this->path_buffer);
#pragma message "Implent SSB rendering"
                    // Test
                    if(this->format == Renderer::Colorspace::BGRX || this->format == Renderer::Colorspace::BGRA){
                        cairo_surface_t* surface = cairo_image_surface_create_for_data(image, this->format == Renderer::Colorspace::BGRA ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, this->width, this->height, pitch);
                        cairo_t* context = cairo_create(surface);
                        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
                        cairo_matrix_scale(&matrix, 1, -1);
                        cairo_matrix_translate(&matrix, 0, -this->height);
                        cairo_matrix_multiply(&matrix, &rs.matrix, &matrix);
                        cairo_set_matrix(context, &matrix);
                        if(!rs.deform_x.empty() || !rs.deform_y.empty()){
                            mu::Parser parser_x, parser_y;
                            parser_x.SetExpr(rs.deform_x);
                            parser_x.DefineConst("t", rs.deform_progress);
                            parser_y.SetExpr(rs.deform_y);
                            parser_y.DefineConst("t", rs.deform_progress);
                            cairo_path_filter(this->path_buffer,
                                [&parser_x,&parser_y](double& x, double& y){
                                    parser_x.DefineConst("x", x);
                                    parser_x.DefineConst("y", y);
                                    parser_y.DefineConst("x", x);
                                    parser_y.DefineConst("y", y);
                                    try{
                                        x = parser_x.Eval();
                                        y = parser_y.Eval();
                                    }catch(...){}
                                });
                        }
                        cairo_path_t* path = cairo_copy_path(this->path_buffer);
                        cairo_append_path(context, path);
                        cairo_path_destroy(path);
                        cairo_set_source_rgb(context, rs.colors.front().r, rs.colors.front().g, rs.colors.front().b);
                        cairo_fill(context);
                        cairo_image_surface_blur(surface, rs.blur_h, rs.blur_v);
                        cairo_destroy(context);
                        cairo_surface_destroy(surface);
                    }
                    // Create image with fitting size

                    // Draw on image

                    // Clear image path
                    cairo_new_path(this->path_buffer);
                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
        }
}
