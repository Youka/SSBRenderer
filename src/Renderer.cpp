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

void Renderer::blend(cairo_surface_t* src, int dst_x, int dst_y,
                        unsigned char* dst_data, int dst_stride,
                        SSBBlend::Mode blend_mode){
    // Get source data
    int src_width = cairo_image_surface_get_width(src);
    int src_height = cairo_image_surface_get_height(src);
    cairo_format_t src_format = cairo_image_surface_get_format(src);
    int src_stride = cairo_image_surface_get_stride(src);
    cairo_surface_flush(src);   // Flush pending operations on surface
    unsigned char* src_data = cairo_image_surface_get_data(src);
    // Anything to overlay?
    if(dst_x < this->width && dst_y < this->height &&
       dst_x + src_width > 0 && dst_y + src_height > 0 &&
       src_format == CAIRO_FORMAT_ARGB32){
        // Calculate source rectangle to overlay
        int src_rect_x = dst_x < 0 ? -dst_x : 0;
        int src_rect_y = dst_y < 0 ? -dst_y : 0;
        int src_rect_width = dst_x + src_width > this->width ? this->width - dst_x : src_width;
        int src_rect_height = dst_y + src_height > this->height ? this->height - dst_y : src_height;
        // Calculate destination offsets for overlay
        int dst_offset_x = dst_x < 0 ? 0 : dst_x;
        int dst_offset_y = this->height - 1 - (dst_y < 0 ? 0 : dst_y);
        // Processing data
        int dst_pix_size = this->format == Renderer::Colorspace::BGR ? 3 : 4;
        unsigned char* src_row = src_data + src_rect_y * src_stride + (src_rect_x << 2);
        unsigned char* dst_row = dst_data + dst_offset_y * dst_stride + (dst_offset_x * dst_pix_size);
        int src_modulo = src_stride - (src_rect_width << 2);
        int dst_modulo = dst_stride - (src_rect_width * dst_pix_size);
        // Overlay by blending mode (hint: source has premultiplied alpha)
        switch(blend_mode){
            case SSBBlend::Mode::OVER:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] == 255){
                            dst_row[0] = src_row[0];
                            dst_row[1] = src_row[1];
                            dst_row[2] = src_row[2];
                        }else if(src_row[3] > 0){
                            unsigned char inv_alpha = 255 - src_row[3];
                            dst_row[0] = dst_row[0] * inv_alpha / 255 + src_row[0];
                            dst_row[1] = dst_row[1] * inv_alpha / 255 + src_row[1];
                            dst_row[2] = dst_row[2] * inv_alpha / 255 + src_row[2];
                        }
                        dst_row += dst_pix_size;
                        src_row += 4;
                    }
                    src_row += src_modulo;
                    dst_row += -dst_stride + dst_modulo - dst_stride;
                }
                break;
            case SSBBlend::Mode::ADDITION:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] > 0){
                            dst_row[0] = dst_row[0] + src_row[0] > 255 ? 255 : dst_row[0] + src_row[0];
                            dst_row[1] = dst_row[1] + src_row[1] > 255 ? 255 : dst_row[1] + src_row[1];
                            dst_row[2] = dst_row[2] + src_row[2] > 255 ? 255 : dst_row[2] + src_row[2];
                        }
                        dst_row += dst_pix_size;
                        src_row += 4;
                    }
                    src_row += src_modulo;
                    dst_row += -dst_stride + dst_modulo - dst_stride;
                }
                break;
            case SSBBlend::Mode::MULTIPLY:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] > 0){
                            unsigned char inv_alpha = 255 - src_row[3];
                            // Restore original color (invert premultiplied alpha) -> multiply color with destination -> multiply color with alpha -> continue like in OVER
                            dst_row[0] = dst_row[0] * inv_alpha / 255 + dst_row[0] * (src_row[0] * 255 / src_row[3]) / 255 * src_row[3] / 255;
                            dst_row[1] = dst_row[1] * inv_alpha / 255 + dst_row[1] * (src_row[1] * 255 / src_row[3]) / 255 * src_row[3] / 255;
                            dst_row[2] = dst_row[2] * inv_alpha / 255 + dst_row[2] * (src_row[2] * 255 / src_row[3]) / 255 * src_row[3] / 255;
                        }
                        dst_row += dst_pix_size;
                        src_row += 4;
                    }
                    src_row += src_modulo;
                    dst_row += -dst_stride + dst_modulo - dst_stride;
                }
                break;
            case SSBBlend::Mode::DIFFERENT:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] > 0){
                            unsigned char inv_alpha = 255 - src_row[3];
                            dst_row[0] = dst_row[0] * inv_alpha / 255 + abs(dst_row[0] - src_row[0] * 255 / src_row[3]) * src_row[3] / 255;
                            dst_row[1] = dst_row[1] * inv_alpha / 255 + abs(dst_row[1] - src_row[1] * 255 / src_row[3]) * src_row[3] / 255;
                            dst_row[2] = dst_row[2] * inv_alpha / 255 + abs(dst_row[2] - src_row[2] * 255 / src_row[3]) * src_row[3] / 255;
                        }
                        dst_row += dst_pix_size;
                        src_row += 4;
                    }
                    src_row += src_modulo;
                    dst_row += -dst_stride + dst_modulo - dst_stride;
                }
                break;
        }
    }
}

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
    // Applies deform filter on path
    void path_deform(cairo_t* ctx, RenderState& rs){
        mu::Parser parser_x, parser_y;
        double x_buf, y_buf;
        parser_x.SetExpr(rs.deform_x);
        parser_y.SetExpr(rs.deform_y);
        parser_x.DefineConst("t", rs.deform_progress);
        parser_y.DefineConst("t", rs.deform_progress);
        parser_x.DefineVar("x", &x_buf);
        parser_y.DefineVar("x", &x_buf);
        parser_x.DefineVar("y", &y_buf);
        parser_y.DefineVar("y", &y_buf);
        cairo_path_filter(ctx,
            [&parser_x,&parser_y,&x_buf,&y_buf](double& x, double& y){
                x_buf = x;
                y_buf = y;
                try{
                    x = parser_x.Eval();
                    y = parser_y.Eval();
                }catch(...){}
            });
    }
}

void Renderer::render(unsigned char* frame, int pitch, unsigned long int start_ms) noexcept{
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
                    CairoImage image(this->width, this->height, CAIRO_FORMAT_ARGB32);
                    cairo_transform(image, &rs.matrix);
                    if(!rs.deform_x.empty() || !rs.deform_y.empty())
                        path_deform(this->path_buffer, rs);
                    cairo_path_t* path = cairo_copy_path(this->path_buffer);
                    cairo_append_path(image, path);
                    cairo_path_destroy(path);
                    cairo_set_source_rgb(image, rs.colors.front().r, rs.colors.front().g, rs.colors.front().b);
                    cairo_fill(image);
                    cairo_image_surface_blur(image, rs.blur_h, rs.blur_v);
                    if(!rs.texture.empty()){
                        CairoImage texture(rs.texture);
                        if(cairo_surface_status(texture) == CAIRO_STATUS_SUCCESS){
                            cairo_set_source_surface(image, texture, 0, 0);
                            cairo_paint(image);
                        }
                    }
                    this->blend(image, 0, 0, frame, pitch, rs.blend_mode);
                    // Create image with fitting size

                    // Draw on image

                    // Clear image path
                    cairo_new_path(this->path_buffer);
                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
        }
}
