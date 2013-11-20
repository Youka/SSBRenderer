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
#include "utf8.h"

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()), stencil_path_buffer(width, height, CAIRO_FORMAT_A8){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
    this->stencil_path_buffer = CairoImage(width, height, CAIRO_FORMAT_A8);
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
        int src_modulo = src_stride - (src_rect_width << 2);
        int dst_modulo = dst_stride - (src_rect_width * dst_pix_size);
        unsigned char* src_row = src_data + src_rect_y * src_stride + (src_rect_x << 2);
        unsigned char* dst_row = dst_data + dst_offset_y * dst_stride + (dst_offset_x * dst_pix_size);
        unsigned char inv_alpha;
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
                            inv_alpha = 255 - src_row[3];
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
                        if(src_row[3] == 255){
                            dst_row[0] = dst_row[0] * src_row[0] / 255;
                            dst_row[1] = dst_row[1] * src_row[1] / 255;
                            dst_row[2] = dst_row[2] * src_row[2] / 255;
                        }else if(src_row[3] > 0){
                            inv_alpha = 255 - src_row[3];
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
            case SSBBlend::Mode::SCREEN:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] == 255){
                            dst_row[0] = 255 - (255 - dst_row[0]) * (255 - src_row[0]) / 255;
                            dst_row[1] = 255 - (255 - dst_row[1]) * (255 - src_row[1]) / 255;
                            dst_row[2] = 255 - (255 - dst_row[2]) * (255 - src_row[2]) / 255;
                        }else if(src_row[3] > 0){
                            inv_alpha = 255 - src_row[3];
                            dst_row[0] = dst_row[0] * inv_alpha / 255 + (255 - (255 - dst_row[0]) * (255 - src_row[0] * 255 / src_row[3]) / 255) * src_row[3] / 255;
                            dst_row[1] = dst_row[1] * inv_alpha / 255 + (255 - (255 - dst_row[1]) * (255 - src_row[1] * 255 / src_row[3]) / 255) * src_row[3] / 255;
                            dst_row[2] = dst_row[2] * inv_alpha / 255 + (255 - (255 - dst_row[2]) * (255 - src_row[2] * 255 / src_row[3]) / 255) * src_row[3] / 255;
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
                        if(src_row[3] == 255){
                            dst_row[0] = abs(dst_row[0] - src_row[0]);
                            dst_row[1] = abs(dst_row[1] - src_row[1]);
                            dst_row[2] = abs(dst_row[2] - src_row[2]);
                        }else if(src_row[3] > 0){
                            inv_alpha = 255 - src_row[3];
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
    // Applies deform filter on cairo path
    void path_deform(cairo_t* ctx, std::string& deform_x, std::string& deform_y, double progress){
        mu::Parser parser_x, parser_y;
        double x_buf, y_buf;
        parser_x.SetExpr(deform_x);
        parser_y.SetExpr(deform_y);
        parser_x.DefineConst("t", progress);
        parser_y.DefineConst("t", progress);
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
    // Converts SSB points to cairo path
    inline void points_to_cairo(SSBPoints* points, double size, cairo_t* ctx){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(size == 1)
#pragma GCC diagnostic pop
            for(const Point& point : points->points)
                cairo_rectangle(ctx, point.x, point.y, size, size);   // Creates a move + lines + close = closed shape
        else
            for(const Point& point : points->points){
                cairo_new_sub_path(ctx);
                cairo_arc(ctx, point.x, point.y, size / 2, 0, M_PI * 2);
                cairo_close_path(ctx);
            }
    }
    // Converts SSB path to cairo path
    inline void path_to_cairo(SSBPath* path, cairo_t* ctx){
        const std::vector<SSBPath::Segment>& segments = path->segments;
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
}

void Renderer::render(unsigned char* frame, int pitch, unsigned long int start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms){
            // Create render state for rendering behaviour
            RenderState rs;
            // Collect geometry line dimensions (by position group -> by text line -> accumulated dimensions)
            std::vector<std::vector<Point>> pos_line_dim = {{{0, 0}}};
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    if(rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms).position)
                        pos_line_dim.push_back({{0, 0}});
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                    switch(geometry->type){
                        case SSBGeometry::Type::POINTS:
                            {
                                points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                                double x1, y1, x2, y2; cairo_fill_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                                if(x2 > 0 && y2 > 0){
                                    pos_line_dim.back().back().x = std::max(pos_line_dim.back().back().x, rs.off_x + x2);
                                    pos_line_dim.back().back().x = std::max(pos_line_dim.back().back().y, rs.off_y + y2);
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR:
                                        case SSBDirection::Mode::RTL:
                                            rs.off_x += x2;
                                            break;
                                        case SSBDirection::Mode::TTB:
                                            rs.off_y += y2;
                                            break;
                                    }
                                }
                            }
                            break;
                        case SSBGeometry::Type::PATH:
                            {
                                path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                                double x1, y1, x2, y2; cairo_fill_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                                if(x2 > 0 && y2 > 0){
                                    pos_line_dim.back().back().x = std::max(pos_line_dim.back().back().x, rs.off_x + x2);
                                    pos_line_dim.back().back().x = std::max(pos_line_dim.back().back().y, rs.off_y + y2);
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR:
                                        case SSBDirection::Mode::RTL:
                                            rs.off_x += x2;
                                            break;
                                        case SSBDirection::Mode::TTB:
                                            rs.off_y += y2;
                                            break;
                                    }
                                }
                            }
                            break;
                        case SSBGeometry::Type::TEXT:
#pragma message "Implent SSB rendersize precalculations"
                            break;
                    }
                }
            // Reset render state
            rs = {};
            // Draw!
            for(std::shared_ptr<SSBObject>& obj : event.objects)
#pragma message "Implent SSB rendering"
                if(obj->type == SSBObject::Type::TAG){
                    // Apply tag to render state
                    RenderState::StateChange state_change = rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms);
                    if(state_change.position){
                        ;
                    }else if(state_change.stencil && rs.stencil_mode == SSBStencil::Mode::CLEAR){
                        // Clear stencil buffer
                        cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_SOURCE);
                        cairo_set_source_rgba(this->stencil_path_buffer, 0, 0, 0, 0);
                        cairo_paint(this->stencil_path_buffer);
                        cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_OVER);
                    }
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Set transformations

                    // Apply geometry to image path
                    SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                    switch(geometry->type){
                        case SSBGeometry::Type::POINTS:
                            points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                            break;
                        case SSBGeometry::Type::PATH:
                            path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                            break;
                        case SSBGeometry::Type::TEXT:
                            {
                                // Get font informations
                                NativeFont font(rs.font_family, rs.bold, rs.italic, rs.underline, rs.strikeout, rs.font_size, rs.direction == SSBDirection::Mode::RTL);
                                NativeFont::FontMetrics metrics = font.get_metrics();
                                // Iterate through text lines
                                std::stringstream text(dynamic_cast<SSBText*>(geometry)->text);
                                unsigned long int line_i = 0;
                                std::string line;
                                switch(rs.direction){
                                    case SSBDirection::Mode::LTR:
                                    case SSBDirection::Mode::RTL:
                                        while(std::getline(text, line)){
                                            // Update inner-line position on newline
                                            if(++line_i > 1){
                                                rs.off_x = 0;
                                                rs.off_y += metrics.height + metrics.external_lead + rs.font_space_v;
                                            }
                                            // Draw formatted text
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wfloat-equal"
                                            if(rs.font_space_h != 0){
            #pragma GCC diagnostic pop
                                                // Iterate through utf8 characters
                                                std::vector<std::string> chars = utf8_chars(line);
                                                for(size_t i = 0; i < chars.size(); ++i){
                                                    // Set horizontal space
                                                    if(i > 0) rs.off_x += rs.font_space_h;
                                                    // Draw text
                                                    cairo_save(this->stencil_path_buffer);
                                                    cairo_translate(this->stencil_path_buffer, rs.off_x, rs.off_y);
                                                    font.text_path_to_cairo(chars[i], this->stencil_path_buffer);
                                                    cairo_restore(this->stencil_path_buffer);
                                                    // Update horizontal offset
                                                    rs.off_x += font.get_text_width(chars[i]);
                                                }
                                            }else{
                                                // Draw text
                                                cairo_save(this->stencil_path_buffer);
                                                cairo_translate(this->stencil_path_buffer, rs.off_x, rs.off_y);
                                                font.text_path_to_cairo(line, this->stencil_path_buffer);
                                                cairo_restore(this->stencil_path_buffer);
                                                // Update horizontal offset
                                                rs.off_x += font.get_text_width(line);
                                            }
                                        }
                                        break;
                                    case SSBDirection::Mode::TTB:
                                        {
                                            double max_width = 0;
                                            while(std::getline(text, line)){
                                                // Update inner-line position on newline
                                                if(++line_i > 1){
                                                    rs.off_x += max_width + metrics.external_lead + rs.font_space_h;
                                                    rs.off_y = 0;
                                                    max_width = 0;
                                                }
                                                // Draw formatted text
                                                std::vector<std::string> chars = utf8_chars(line);
                                                for(size_t i = 0; i < chars.size(); ++i){
                                                    // Set vertical space
                                                    if(i > 0) rs.off_y += rs.font_space_v;
                                                    // Draw text
                                                    cairo_save(this->stencil_path_buffer);
                                                    cairo_translate(this->stencil_path_buffer, rs.off_x, rs.off_y);
                                                    font.text_path_to_cairo(chars[i], this->stencil_path_buffer);
                                                    cairo_restore(this->stencil_path_buffer);
                                                    // Update vertical offset
                                                    rs.off_y += metrics.height;
                                                    // Update maximal character width for next line offset
                                                    max_width = std::max(max_width, font.get_text_width(chars[i]));
                                                }
                                            }
                                        }
                                        break;
                                }
                            }
                            break;
                    }
                    // Test
                    CairoImage image(this->width, this->height, CAIRO_FORMAT_ARGB32);
                    cairo_transform(image, &rs.matrix);
                    if(!rs.deform_x.empty() || !rs.deform_y.empty())
                        path_deform(this->stencil_path_buffer, rs.deform_x, rs.deform_y, rs.deform_progress);
                    cairo_path_t* path = cairo_copy_path(this->stencil_path_buffer);
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
                    cairo_new_path(this->stencil_path_buffer);
                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
        }
}
