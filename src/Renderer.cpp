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
#include "RendererUtils.hpp"
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
        int src_rect_x = dst_x < 0 ? -dst_x : 0,
            src_rect_y = dst_y < 0 ? -dst_y : 0,
            src_rect_x2 = dst_x + src_width > this->width ? this->width - dst_x : src_width,
            src_rect_y2 = dst_y + src_height > this->height ? this->height - dst_y : src_height,
            src_rect_width = src_rect_x2 - src_rect_x,
            src_rect_height = src_rect_y2 - src_rect_y;
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
                            inv_alpha = src_row[3] ^ 0xFF;
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
                            dst_row[0] = std::min(255, dst_row[0] + src_row[0]);
                            dst_row[1] = std::min(255, dst_row[1] + src_row[1]);
                            dst_row[2] = std::min(255, dst_row[2] + src_row[2]);
                        }
                        dst_row += dst_pix_size;
                        src_row += 4;
                    }
                    src_row += src_modulo;
                    dst_row += -dst_stride + dst_modulo - dst_stride;
                }
                break;
            case SSBBlend::Mode::SUBTRACT:
                for(int src_y = 0; src_y < src_rect_height; ++src_y){
                    for(int src_x = 0; src_x < src_rect_width; ++src_x){
                        if(src_row[3] > 0){
                            dst_row[0] = std::max(0, dst_row[0] - src_row[0]);
                            dst_row[1] = std::max(0, dst_row[1] - src_row[1]);
                            dst_row[2] = std::max(0, dst_row[2] - src_row[2]);
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
                            inv_alpha = src_row[3] ^ 0xFF;
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
                            dst_row[0] = (dst_row[0] ^ 0xFF) * (src_row[0] ^ 0xFF) / 255 ^ 0xFF;
                            dst_row[1] = (dst_row[1] ^ 0xFF) * (src_row[1] ^ 0xFF) / 255 ^ 0xFF;
                            dst_row[2] = (dst_row[2] ^ 0xFF) * (src_row[2] ^ 0xFF) / 255 ^ 0xFF;
                        }else if(src_row[3] > 0){
                            inv_alpha = src_row[3] ^ 0xFF;
                            dst_row[0] = dst_row[0] * inv_alpha / 255 + ((dst_row[0] ^ 0xFF) * (src_row[0] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255;
                            dst_row[1] = dst_row[1] * inv_alpha / 255 + ((dst_row[1] ^ 0xFF) * (src_row[1] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255;
                            dst_row[2] = dst_row[2] * inv_alpha / 255 + ((dst_row[2] ^ 0xFF) * (src_row[2] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255;
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
                            inv_alpha = src_row[3] ^ 0xFF;
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

void Renderer::render(unsigned char* frame, int pitch, unsigned long int start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms){
            // Create render state for rendering behaviour
            RenderState rs;
            // Collect geometry line dimensions (by position group -> by text line -> accumulated dimensions)
            std::vector<std::vector<LineSize>> pos_line_dim = {{{}}};
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    if(rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms).position)
                        pos_line_dim.push_back({{}});
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                    switch(geometry->type){
                        case SSBGeometry::Type::POINTS:
                        case SSBGeometry::Type::PATH:
                            {
                                if(geometry->type == SSBGeometry::Type::POINTS)
                                    points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                                else
                                    path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                                double x1, y1, x2, y2; cairo_path_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                                x2 = std::max(x2, 0.0); y2 = std::max(y2, 0.0);
                                cairo_new_path(this->stencil_path_buffer);
                                pos_line_dim.back().back().width = std::max(pos_line_dim.back().back().width, rs.off_x + x2);
                                pos_line_dim.back().back().height = std::max(pos_line_dim.back().back().height, rs.off_y + y2);
                                switch(rs.direction){
                                    case SSBDirection::Mode::LTR:
                                    case SSBDirection::Mode::RTL: rs.off_x += x2; break;
                                    case SSBDirection::Mode::TTB: rs.off_y += y2; break;
                                }
                            }
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
                                while(std::getline(text, line)){
                                    if(++line_i > 1){
                                        pos_line_dim.back().back().space = (rs.direction == SSBDirection::Mode::TTB) ? rs.font_space_h : metrics.external_lead + rs.font_space_v;
                                        pos_line_dim.back().push_back({});
                                        rs.off_x = 0;
                                        rs.off_y = 0;
                                    }
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR:
                                        case SSBDirection::Mode::RTL:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                                            if(rs.font_space_h != 0){
#pragma GCC diagnostic pop
                                                std::vector<std::string> chars = utf8_chars(line);
                                                for(size_t i = 0; i < chars.size(); ++i)
                                                    rs.off_x += font.get_text_width(chars[i]) + rs.font_space_h;
                                            }else
                                                rs.off_x += font.get_text_width(line);
                                            pos_line_dim.back().back().width = std::max(pos_line_dim.back().back().width, rs.off_x);
                                            pos_line_dim.back().back().height = std::max(pos_line_dim.back().back().height, rs.off_y + metrics.height);
                                            break;
                                        case SSBDirection::Mode::TTB:
                                            {
                                                std::vector<std::string> chars = utf8_chars(line);
                                                double max_width = 0;
                                                for(size_t i = 0; i < chars.size(); ++i){
                                                    rs.off_y += metrics.height + rs.font_space_v;
                                                    max_width = std::max(max_width, font.get_text_width(chars[i]));
                                                }
                                                pos_line_dim.back().back().width = std::max(pos_line_dim.back().back().width, rs.off_x + max_width);
                                                pos_line_dim.back().back().height = std::max(pos_line_dim.back().back().height, rs.off_y);
                                            }
                                            break;
                                    }
                                }
                            }
                            break;
                    }
                }
            // Reset render state
            rs = {};
            // Define geometry path
            size_t pos_i = 0, pos_line_i = 0;
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    // Apply tag to render state
                    if(rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms).position){
                        ++pos_i;
                        pos_line_i = 0;
                    }
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Create geometry
                    Point align_point = calc_align_offset(rs.align, rs.direction, pos_line_dim[pos_i], pos_line_i);
                    SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                    switch(geometry->type){
                        case SSBGeometry::Type::POINTS:
                        case SSBGeometry::Type::PATH:
                            {
                                // Draw points / path
                                if(geometry->type == SSBGeometry::Type::POINTS)
                                    points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                                else
                                    path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                                // Align points / path
                                double x1, y1, x2, y2; cairo_path_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                                x2 = std::max(x2, 0.0); y2 = std::max(y2, 0.0);
                                cairo_matrix_t matrix = {1, 0, 0, 1, align_point.x, align_point.y + rs.off_y};
                                switch(rs.direction){
                                    case SSBDirection::Mode::LTR: matrix.x0 += rs.off_x; break;
                                    case SSBDirection::Mode::RTL: matrix.x0 += pos_line_dim[pos_i][pos_line_i].width - rs.off_x - x2; break;
                                    case SSBDirection::Mode::TTB: matrix.x0 += std::accumulate(pos_line_dim[pos_i].begin(), pos_line_dim[pos_i].end(), 0.0, [](double init, LineSize& line){
                                            return init + line.width + line.space;
                                        }) - rs.off_x - pos_line_dim[pos_i][pos_line_i].width + (pos_line_dim[pos_i][pos_line_i].width - x2) / 2; break;
                                }
                                cairo_apply_matrix(this->stencil_path_buffer, &matrix);
                                // Update inner position offset
                                switch(rs.direction){
                                    case SSBDirection::Mode::LTR:
                                    case SSBDirection::Mode::RTL: rs.off_x += x2; break;
                                    case SSBDirection::Mode::TTB: rs.off_y += y2; break;
                                }
                            }
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
                                cairo_path_t* old_path;
                                while(std::getline(text, line)){
                                    // Save old path to avoid transformations
                                    old_path = cairo_copy_path(this->stencil_path_buffer);
                                    cairo_new_path(this->stencil_path_buffer);
                                    // Recalculate data for new line
                                    if(++line_i > 1){
                                        ++pos_line_i;
                                        align_point = calc_align_offset(rs.align, rs.direction, pos_line_dim[pos_i], pos_line_i);
                                    }
                                    // Draw line
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR:
                                        case SSBDirection::Mode::RTL:
                                            {
                                                if(line_i > 1){
                                                    rs.off_x = 0;
                                                    rs.off_y += pos_line_dim[pos_i][pos_line_i-1].height + metrics.external_lead + rs.font_space_v;
                                                }
                                                double baseline_off_y = pos_line_dim[pos_i][pos_line_i].height - metrics.height;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                                                if(rs.font_space_h != 0){
#pragma GCC diagnostic pop
                                                    // Iterate through utf8 characters
                                                    double text_width = 0;
                                                    std::vector<std::string> chars = utf8_chars(line);
                                                    for(size_t i = 0; i < chars.size(); ++i){
                                                        // Draw text
                                                        cairo_save(this->stencil_path_buffer);
                                                        cairo_translate(this->stencil_path_buffer, text_width, 0);
                                                        font.text_path_to_cairo(chars[i], this->stencil_path_buffer);
                                                        cairo_restore(this->stencil_path_buffer);
                                                        // Update horizontal character offset
                                                        text_width += font.get_text_width(chars[i]) + rs.font_space_h;
                                                    }
                                                    // Align text
                                                    cairo_matrix_t matrix = {1, 0, 0, 1, align_point.x + (rs.direction == SSBDirection::Mode::LTR ? rs.off_x : pos_line_dim[pos_i][pos_line_i].width - rs.off_x - text_width), align_point.y + rs.off_y + baseline_off_y};
                                                    cairo_apply_matrix(this->stencil_path_buffer, &matrix);
                                                    // Update horizontal offset
                                                    rs.off_x += text_width;
                                                }else{
                                                    // Draw text
                                                    double text_width = font.get_text_width(line);
                                                    cairo_save(this->stencil_path_buffer);
                                                    cairo_translate(this->stencil_path_buffer, align_point.x + (rs.direction == SSBDirection::Mode::LTR ? rs.off_x : pos_line_dim[pos_i][pos_line_i].width - rs.off_x - text_width), align_point.y + rs.off_y + baseline_off_y);
                                                    font.text_path_to_cairo(line, this->stencil_path_buffer);
                                                    cairo_restore(this->stencil_path_buffer);
                                                    // Update horizontal offset
                                                    rs.off_x += text_width;
                                                }
                                            }
                                            break;
                                        case SSBDirection::Mode::TTB:
                                            {
                                                if(line_i > 1){
                                                    rs.off_x += pos_line_dim[pos_i][pos_line_i-1].width + rs.font_space_h;
                                                    rs.off_y = 0;
                                                }
                                                // Iterate through utf8 characters
                                                double text_height = 0;
                                                std::vector<std::string> chars = utf8_chars(line);
                                                for(size_t i = 0; i < chars.size(); ++i){
                                                    // Draw text
                                                    cairo_save(this->stencil_path_buffer);
                                                    cairo_translate(this->stencil_path_buffer, (pos_line_dim[pos_i][pos_line_i].width - font.get_text_width(chars[i])) / 2, text_height);
                                                    font.text_path_to_cairo(chars[i], this->stencil_path_buffer);
                                                    cairo_restore(this->stencil_path_buffer);
                                                    // Update vertical character offset
                                                    text_height += metrics.height + rs.font_space_v;
                                                }
                                                // Align text
                                                cairo_matrix_t matrix = {1, 0, 0, 1, align_point.x + std::accumulate(pos_line_dim[pos_i].begin(), pos_line_dim[pos_i].end(), 0.0, [](double init, LineSize& line){
                                                    return init + line.width + line.space;
                                                }) - rs.off_x - pos_line_dim[pos_i][pos_line_i].width, align_point.y + rs.off_y};
                                                cairo_apply_matrix(this->stencil_path_buffer, &matrix);
                                                // Update vertical offset
                                                rs.off_y += text_height;
                                            }
                                            break;
                                    }
                                    // Restore old path
                                    cairo_append_path(this->stencil_path_buffer, old_path);
                                    cairo_path_destroy(old_path);
                                }
                            }
                            break;
                    }
                    // Deform geometry
                    if(!rs.deform_x.empty() || !rs.deform_y.empty())
                        path_deform(this->stencil_path_buffer, rs.deform_x, rs.deform_y, rs.deform_progress);
#pragma message "Rewrite rendering process"
                    // Prepare transformation matrix
                    cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(!(rs.pos_x == std::numeric_limits<decltype(rs.pos_x)>::max() && rs.pos_y == std::numeric_limits<decltype(rs.pos_y)>::max())){
#pragma GCC diagnostic pop
                        if(this->ssb.frame.width > 0 && this->ssb.frame.height > 0)
                            cairo_matrix_scale(&matrix, static_cast<double>(this->width) / this->ssb.frame.width, static_cast<double>(this->height) / this->ssb.frame.height);
                        cairo_matrix_translate(&matrix, rs.pos_x, rs.pos_y);
                    }else{
                        if(this->ssb.frame.width > 0 && this->ssb.frame.height > 0){
                            double scale_x = static_cast<double>(this->width) / this->ssb.frame.width,
                                    scale_y = static_cast<double>(this->height) / this->ssb.frame.height;
                            Point pos = get_auto_pos(this->width, this->height, rs, scale_x, scale_y);
                            cairo_matrix_translate(&matrix, pos.x, pos.y);
                            cairo_matrix_scale(&matrix, scale_x, scale_y);
                        }else{
                            Point pos = get_auto_pos(this->width, this->height, rs);
                            cairo_matrix_translate(&matrix, pos.x, pos.y);
                        }
                    }
                    cairo_matrix_multiply(&matrix, &rs.matrix, &matrix);
                    // Draw by type
                    enum class DrawType{FILL_BLURRED, FILL_WITHOUT_BLUR, BORDER, WIRE};
                    auto draw_func = [&](DrawType draw_type){
                        // Create image
                        int x, y, width, height;
                        int stroke_border_h = 0, stroke_border_v = 0;
                        int border_h = 0, border_v = 0;
                        {
                            double x1, y1, x2, y2; cairo_fill_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                            x = floor(x1), y = floor(y1), width = ceil(x2 - x), height = ceil(y2 - y);
                            switch(draw_type){
                                case DrawType::WIRE:
                                case DrawType::BORDER:
                                    {
                                        set_line_props(this->stencil_path_buffer, rs);
                                        double sx1, sy1, sx2, sy2; cairo_stroke_extents(this->stencil_path_buffer, &sx1, &sy1, &sx2, &sy2);
                                        stroke_border_h = ceil(std::max(x1-sx1, sx2-x2)), stroke_border_v = ceil(std::max(y1-sy1, sy2-y2));
                                        border_h = ceil(rs.blur_h) + stroke_border_h, border_v = ceil(rs.blur_v) + stroke_border_v;

                                    }
                                    break;
                                case DrawType::FILL_BLURRED:
                                    border_h = ceil(rs.blur_h), border_v = ceil(rs.blur_v);
                                    break;
                                case DrawType::FILL_WITHOUT_BLUR:
                                    // Border already with zero initialized
                                    break;
                            }
                        }
                        CairoImage image(width + (border_h << 1), height + (border_v << 1), CAIRO_FORMAT_ARGB32);
                        // Transfer shifted path from buffer to image
                        cairo_save(image);
                        cairo_translate(image, -x + border_h, -y + border_v);
                        cairo_path_t* path = cairo_copy_path(this->stencil_path_buffer);
                        cairo_append_path(image, path);
                        cairo_path_destroy(path);
                        cairo_restore(image);
                        // Set line properties
                        if(draw_type == DrawType::BORDER || draw_type == DrawType::WIRE)
                            set_line_props(image, rs);
                        // Draw colored geometry on image
                        if(draw_type == DrawType::FILL_BLURRED || draw_type == DrawType::FILL_WITHOUT_BLUR){
                            if(rs.colors.size() == 1 && rs.alphas.size() == 1)
                                cairo_set_source_rgba(image, rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[0]);
                            else{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
                                cairo_rectangle_t color_rect = {border_h, border_v, width, height};
#pragma GCC diagnostic pop
                                if(rs.colors.size() == 4 && rs.alphas.size() == 4)
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[0],
                                                                                            rs.colors[1].r, rs.colors[1].g, rs.colors[1].b, rs.alphas[1],
                                                                                            rs.colors[2].r, rs.colors[2].g, rs.colors[2].b, rs.alphas[2],
                                                                                            rs.colors[3].r, rs.colors[3].g, rs.colors[3].b, rs.alphas[3]));
                                else if(rs.colors.size() == 4 && rs.alphas.size() == 1)
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[0],
                                                                                            rs.colors[1].r, rs.colors[1].g, rs.colors[1].b, rs.alphas[0],
                                                                                            rs.colors[2].r, rs.colors[2].g, rs.colors[2].b, rs.alphas[0],
                                                                                            rs.colors[3].r, rs.colors[3].g, rs.colors[3].b, rs.alphas[0]));
                                else    // rs.colors.size() == 1 && rs.alphas.size() == 4
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[0],
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[1],
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[2],
                                                                                            rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[3]));
                            }
                            cairo_fill_preserve(image);
                        }else{  // draw_type == DrawType::BORDER || draw_type == DrawType::WIRE
                            if(rs.line_colors.size() == 1 && rs.line_alphas.size() == 1)
                                cairo_set_source_rgba(image, rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[0]);
                            else{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
                                cairo_rectangle_t color_rect = {border_h - stroke_border_h, border_v - stroke_border_v, width + (stroke_border_h << 1), height + (stroke_border_v << 1)};
#pragma GCC diagnostic pop
                                if(rs.line_colors.size() == 4 && rs.line_alphas.size() == 4)
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[0],
                                                                                            rs.line_colors[1].r, rs.line_colors[1].g, rs.line_colors[1].b, rs.line_alphas[1],
                                                                                            rs.line_colors[2].r, rs.line_colors[2].g, rs.line_colors[2].b, rs.line_alphas[2],
                                                                                            rs.line_colors[3].r, rs.line_colors[3].g, rs.line_colors[3].b, rs.line_alphas[3]));
                                else if(rs.line_colors.size() == 4 && rs.line_alphas.size() == 1)
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[0],
                                                                                            rs.line_colors[1].r, rs.line_colors[1].g, rs.line_colors[1].b, rs.line_alphas[0],
                                                                                            rs.line_colors[2].r, rs.line_colors[2].g, rs.line_colors[2].b, rs.line_alphas[0],
                                                                                            rs.line_colors[3].r, rs.line_colors[3].g, rs.line_colors[3].b, rs.line_alphas[0]));
                                else    // rs.line_colors.size() == 1 && rs.line_alphas.size() == 4
                                    cairo_set_source(image, cairo_pattern_create_rect_color(color_rect,
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[0],
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[1],
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[2],
                                                                                            rs.line_colors[0].r, rs.line_colors[0].g, rs.line_colors[0].b, rs.line_alphas[3]));
                            }
                            cairo_stroke_preserve(image);
                        }
                        // Draw texture over image color
                        if((draw_type == DrawType::FILL_BLURRED || draw_type == DrawType::FILL_WITHOUT_BLUR) && !rs.texture.empty()){
                            CairoImage texture(rs.texture);
                            if(cairo_surface_status(texture) == CAIRO_STATUS_SUCCESS){
                                // Create RGB version of image
                                CairoImage rgb_image(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image), CAIRO_FORMAT_RGB24);
                                cairo_set_source_surface(rgb_image, image, 0, 0);
                                cairo_set_operator(rgb_image, CAIRO_OPERATOR_SOURCE);
                                cairo_paint(rgb_image);
                                // Create texture pattern for color
                                cairo_matrix_t pattern_matrix = {1, 0, 0, 1, border_h + rs.texture_x, border_v + rs.texture_y};
                                cairo_pattern_t* pattern = cairo_pattern_create_for_surface(texture);
                                cairo_pattern_set_matrix(pattern, &pattern_matrix);
                                cairo_pattern_set_extend(pattern, rs.wrap_style);
                                // Multiply image & texture color
                                cairo_set_source(rgb_image, pattern);
                                cairo_set_operator(rgb_image, CAIRO_OPERATOR_MULTIPLY);
                                cairo_paint(rgb_image);
                                // Create texture pattern for alpha
                                pattern = cairo_pattern_create_for_surface(texture);
                                cairo_pattern_set_matrix(pattern, &pattern_matrix);
                                cairo_pattern_set_extend(pattern, rs.wrap_style);
                                // Multiply image & texture alpha
                                cairo_set_source(image, pattern);
                                cairo_set_operator(image, CAIRO_OPERATOR_IN);
                                cairo_paint(image);
                                // Merge color & alpha to image
                                cairo_set_source_surface(image, rgb_image, 0, 0);
                                cairo_paint(image);
                            }
                        }else if((draw_type == DrawType::BORDER || draw_type == DrawType::WIRE) && !rs.line_texture.empty()){
                            CairoImage texture(rs.line_texture);
                            if(cairo_surface_status(texture) == CAIRO_STATUS_SUCCESS){
                                // Create RGB version of image
                                CairoImage rgb_image(cairo_image_surface_get_width(image), cairo_image_surface_get_height(image), CAIRO_FORMAT_RGB24);
                                cairo_set_source_surface(rgb_image, image, 0, 0);
                                cairo_set_operator(rgb_image, CAIRO_OPERATOR_SOURCE);
                                cairo_paint(rgb_image);
                                // Create texture pattern for color
                                cairo_matrix_t pattern_matrix = {1, 0, 0, 1, border_h - stroke_border_h + rs.line_texture_x, border_v - stroke_border_v + rs.line_texture_y};
                                cairo_pattern_t* pattern = cairo_pattern_create_for_surface(texture);
                                cairo_pattern_set_matrix(pattern, &pattern_matrix);
                                cairo_pattern_set_extend(pattern, rs.line_wrap_style);
                                // Multiply image & texture color
                                cairo_set_source(rgb_image, pattern);
                                cairo_set_operator(rgb_image, CAIRO_OPERATOR_MULTIPLY);
                                cairo_paint(rgb_image);
                                // Create texture pattern for alpha
                                pattern = cairo_pattern_create_for_surface(texture);
                                cairo_pattern_set_matrix(pattern, &pattern_matrix);
                                cairo_pattern_set_extend(pattern, rs.wrap_style);
                                // Multiply image & texture alpha
                                cairo_set_source(image, pattern);
                                cairo_set_operator(image, CAIRO_OPERATOR_IN);
                                cairo_paint(image);
                                // Merge color & alpha to image
                                cairo_set_source_surface(image, rgb_image, 0, 0);
                                cairo_paint(image);
                            }
                        }
                        // Draw karaoke over image
                        if(draw_type == DrawType::FILL_BLURRED || draw_type == DrawType::FILL_WITHOUT_BLUR){
                            int elapsed_time = start_ms - event.start_ms;
                            if(rs.karaoke_start >= 0){
                                cairo_set_operator(image, CAIRO_OPERATOR_ATOP);
                                cairo_set_source_rgb(image, rs.karaoke_color.r, rs.karaoke_color.g, rs.karaoke_color.b);
                                if(elapsed_time >= rs.karaoke_start + rs.karaoke_duration)
                                    cairo_fill(image);
                                else if(elapsed_time >= rs.karaoke_start){
                                    double progress = static_cast<double>(elapsed_time - rs.karaoke_start) / rs.karaoke_duration;
                                    cairo_clip(image);
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR: cairo_rectangle(image, border_h, border_v, progress * width, height); break;
                                        case SSBDirection::Mode::RTL: cairo_rectangle(image, border_h + width - progress * width, border_v, progress * width, height); break;
                                        case SSBDirection::Mode::TTB: cairo_rectangle(image, border_h, border_v, width, progress * height); break;
                                    }
                                    cairo_fill_preserve(image);
                                }
                            }
                        }
                        // Blur image
                        if(draw_type != DrawType::FILL_WITHOUT_BLUR)
                            cairo_image_surface_blur(image, rs.blur_h, rs.blur_v);
                        // Erase filling in stroke -> create border
                        if(draw_type == DrawType::BORDER){
                            cairo_set_source_rgba(image, 0, 0, 0, 0);
                            cairo_set_operator(image, CAIRO_OPERATOR_CLEAR);
                            cairo_fill(image);
                        }
                        // Create transformed image
                        double min_x, min_y, max_x, max_y;
                        {
                            double x0 = -border_h + x, y0 = -border_v + y; cairo_matrix_transform_point(&matrix, &x0, &y0);
                            double x1 = x + width + border_h, y1 = -border_v + y; cairo_matrix_transform_point(&matrix, &x1, &y1);
                            double x2 = x + width + border_h, y2 = y + height + border_v; cairo_matrix_transform_point(&matrix, &x2, &y2);
                            double x3 = -border_h + x, y3 = y + height + border_v; cairo_matrix_transform_point(&matrix, &x3, &y3);
                            min_x = floor(std::min(std::min(x0, x1), std::min(x2, x3)));
                            min_y = floor(std::min(std::min(y0, y1), std::min(y2, y3)));
                            max_x = ceil(std::max(std::max(x0, x1), std::max(x2, x3)));
                            max_y = ceil(std::max(std::max(y0, y1), std::max(y2, y3)));
                        }
                        CairoImage timage(max_x - min_x, max_y - min_y, CAIRO_FORMAT_ARGB32);
                        cairo_translate(timage, -min_x, -min_y);
                        cairo_transform(timage, &matrix);
                        cairo_translate(timage, -border_h + x, -border_v + y);
                        cairo_set_source_surface(timage, image, 0, 0);
                        cairo_paint(timage);
                        // Apply stenciling
                        switch(rs.stencil_mode){
                            case SSBStencil::Mode::OFF:
                                this->blend(timage, min_x, min_y, frame, pitch, rs.blend_mode);
                                break;
                            case SSBStencil::Mode::INSIDE:
                                cairo_set_operator(timage, CAIRO_OPERATOR_DEST_IN);
                                cairo_set_source_surface(timage, this->stencil_path_buffer, -min_x, -min_y);
                                cairo_paint(timage);
                                this->blend(timage, min_x, min_y, frame, pitch, rs.blend_mode);
                                break;
                            case SSBStencil::Mode::OUTSIDE:
                                cairo_set_operator(timage, CAIRO_OPERATOR_DEST_OUT);
                                cairo_set_source_surface(timage, this->stencil_path_buffer, -min_x, -min_y);
                                cairo_paint(timage);
                                this->blend(timage, min_x, min_y, frame, pitch, rs.blend_mode);
                                break;
                            case SSBStencil::Mode::SET:
                                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_ADD);
                                cairo_set_source_surface(this->stencil_path_buffer, timage, min_x, min_y);
                                cairo_paint(this->stencil_path_buffer);
                                break;
                            case SSBStencil::Mode::UNSET:
                                // Invert alpha
                                cairo_set_operator(timage, CAIRO_OPERATOR_XOR);
                                cairo_set_source_rgba(timage, 1, 1, 1, 1);
                                cairo_paint(timage);
                                // Multiply alpha
                                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_IN);
                                cairo_set_source_surface(this->stencil_path_buffer, timage, min_x, min_y);
                                cairo_paint(this->stencil_path_buffer);
                                break;
                        }
                    };
                    // Draw!
                    if(rs.mode == SSBMode::Mode::FILL){
                        if(rs.line_width > 0 && geometry->type != SSBGeometry::Type::POINTS){
                            draw_func(DrawType::BORDER);
                            draw_func(DrawType::FILL_WITHOUT_BLUR);
                        }else
                            draw_func(DrawType::FILL_BLURRED);
                    }else   // rs.mode == SSBMode::Mode::WIRE
                        draw_func(DrawType::WIRE);
                    // Clear path
                    cairo_new_path(this->stencil_path_buffer);
                }
                // Clear stencil
                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgba(this->stencil_path_buffer, 0, 0, 0, 0);
                cairo_paint(this->stencil_path_buffer);
        }
}
