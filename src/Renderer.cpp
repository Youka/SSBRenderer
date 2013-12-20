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

// Needed for POSIX path functions
#undef __STRICT_ANSI__
#ifdef _WIN32
#define __MSVCRT__ 1
#endif
#include <cstdlib>
#include "Renderer.hpp"
#include "SSBParser.hpp"
#include "RendererUtils.hpp"
#include "utf8.h"

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()), stencil_path_buffer(width, height, CAIRO_FORMAT_A8){
    // Save initialization directory for later file loading
#ifdef _WIN32
    wchar_t file_path[_MAX_PATH];
    if(_wfullpath(file_path, utf8_to_utf16(script).c_str(), _MAX_PATH)){
        wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR];
        _wsplitpath(file_path, drive, dir, NULL, NULL); // Path, drive, directory, name, extension
        std::wstring full_dir(drive); full_dir += dir;
        FileReader::set_additional_directory(utf16_to_utf8(full_dir));
    }
#else
    char file_path[_MAX_PATH];
    if(_fullpath(file_path, script.c_str(), _MAX_PATH)){
        char drive[_MAX_DRIVE], dir[_MAX_DIR];
        _splitpath(file_path, drive, dir, NULL, NULL); // Path, drive, directory, name, extension
        FileReader::set_additional_directory(std::string(drive) + dir);
    }
#endif
}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
    this->stencil_path_buffer = CairoImage(width, height, CAIRO_FORMAT_A8);
    this->cache.clear();
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
        // Overlay by blending mode (hint: source & destination have premultiplied alpha)
        switch(blend_mode){
            case SSBBlend::Mode::OVER:
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = src_row[0];
                                dst_row[1] = src_row[1];
                                dst_row[2] = src_row[2];
                                dst_row[3] = src_row[3];
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = src_row[0] + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = src_row[1] + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = src_row[2] + dst_row[2] * inv_alpha / 255;
                                dst_row[3] = src_row[3] + dst_row[3] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = src_row[0];
                                dst_row[1] = src_row[1];
                                dst_row[2] = src_row[2];
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = src_row[0] + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = src_row[1] + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = src_row[2] + dst_row[2] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                break;
            case SSBBlend::Mode::ADDITION:
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] > 0){
                                dst_row[0] = std::min(255, dst_row[0] + src_row[0]);
                                dst_row[1] = std::min(255, dst_row[1] + src_row[1]);
                                dst_row[2] = std::min(255, dst_row[2] + src_row[2]);
                                dst_row[3] = std::min(255, dst_row[3] + src_row[3]);
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
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
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] > 0){
                                dst_row[0] = std::max(0, dst_row[0] - src_row[0]);
                                dst_row[1] = std::max(0, dst_row[1] - src_row[1]);
                                dst_row[2] = std::max(0, dst_row[2] - src_row[2]);
                                dst_row[3] = std::max(0, dst_row[3] - src_row[3]);
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
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
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = (dst_row[0] * 255 / dst_row[3]) * src_row[0] / 255;
                                dst_row[1] = (dst_row[1] * 255 / dst_row[3]) * src_row[1] / 255;
                                dst_row[2] = (dst_row[2] * 255 / dst_row[3]) * src_row[2] / 255;
                                dst_row[3] = src_row[3];
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = (dst_row[0] * 255 / dst_row[3]) * (src_row[0] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = (dst_row[1] * 255 / dst_row[3]) * (src_row[1] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = (dst_row[2] * 255 / dst_row[3]) * (src_row[2] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[2] * inv_alpha / 255;
                                dst_row[3] = src_row[3] + dst_row[3] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = dst_row[0] * src_row[0] / 255;
                                dst_row[1] = dst_row[1] * src_row[1] / 255;
                                dst_row[2] = dst_row[2] * src_row[2] / 255;
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = dst_row[0] * (src_row[0] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = dst_row[1] * (src_row[1] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = dst_row[2] * (src_row[2] * 255 / src_row[3]) * src_row[3] / 65025 + dst_row[2] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                break;
            case SSBBlend::Mode::SCREEN:
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = (dst_row[0] * 255 / dst_row[3] ^ 0xFF) * (src_row[0] ^ 0xFF) / 255 ^ 0xFF;
                                dst_row[1] = (dst_row[1] * 255 / dst_row[3] ^ 0xFF) * (src_row[1] ^ 0xFF) / 255 ^ 0xFF;
                                dst_row[2] = (dst_row[2] * 255 / dst_row[3] ^ 0xFF) * (src_row[2] ^ 0xFF) / 255 ^ 0xFF;
                                dst_row[3] = src_row[3];
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = ((dst_row[0] * 255 / dst_row[3] ^ 0xFF) * (src_row[0] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = ((dst_row[1] * 255 / dst_row[3] ^ 0xFF) * (src_row[1] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = ((dst_row[2] * 255 / dst_row[3] ^ 0xFF) * (src_row[2] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[2] * inv_alpha / 255;
                                dst_row[3] = src_row[3] + dst_row[3] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = (dst_row[0] ^ 0xFF) * (src_row[0] ^ 0xFF) / 255 ^ 0xFF;
                                dst_row[1] = (dst_row[1] ^ 0xFF) * (src_row[1] ^ 0xFF) / 255 ^ 0xFF;
                                dst_row[2] = (dst_row[2] ^ 0xFF) * (src_row[2] ^ 0xFF) / 255 ^ 0xFF;
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = ((dst_row[0] ^ 0xFF) * (src_row[0] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = ((dst_row[1] ^ 0xFF) * (src_row[1] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = ((dst_row[2] ^ 0xFF) * (src_row[2] * 255 / src_row[3] ^ 0xFF) / 255 ^ 0xFF) * src_row[3] / 255 + dst_row[2] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                break;
            case SSBBlend::Mode::DIFFERENCES:
                if(this->format == Renderer::Colorspace::BGRA)
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = abs((dst_row[0] * 255 / dst_row[3]) - src_row[0]);
                                dst_row[1] = abs((dst_row[1] * 255 / dst_row[3]) - src_row[1]);
                                dst_row[2] = abs((dst_row[2] * 255 / dst_row[3]) - src_row[2]);
                                dst_row[3] = src_row[3];
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = abs((dst_row[0] * 255 / dst_row[3]) - (src_row[0] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = abs((dst_row[1] * 255 / dst_row[3]) - (src_row[1] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = abs((dst_row[2] * 255 / dst_row[3]) - (src_row[2] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[2] * inv_alpha / 255;
                                dst_row[3] = src_row[3] + dst_row[3] * inv_alpha / 255;
                            }
                            dst_row += dst_pix_size;
                            src_row += 4;
                        }
                        src_row += src_modulo;
                        dst_row += -dst_stride + dst_modulo - dst_stride;
                    }
                else
                    for(int src_y = 0; src_y < src_rect_height; ++src_y){
                        for(int src_x = 0; src_x < src_rect_width; ++src_x){
                            if(src_row[3] == 255){
                                dst_row[0] = abs(dst_row[0] - src_row[0]);
                                dst_row[1] = abs(dst_row[1] - src_row[1]);
                                dst_row[2] = abs(dst_row[2] - src_row[2]);
                            }else if(src_row[3] > 0){
                                inv_alpha = src_row[3] ^ 0xFF;
                                dst_row[0] = abs(dst_row[0] - (src_row[0] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[0] * inv_alpha / 255;
                                dst_row[1] = abs(dst_row[1] - (src_row[1] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[1] * inv_alpha / 255;
                                dst_row[2] = abs(dst_row[2] - (src_row[2] * 255 / src_row[3])) * src_row[3] / 255 + dst_row[2] * inv_alpha / 255;
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
            // Draw from cache
            if(this->cache.contains(&event))
                for(Renderer::ImageData& idata : this->cache.get(&event))
                    this->blend(create_faded_image(idata.image, idata.fade_in, idata.fade_out, start_ms, event.start_ms, event.end_ms),
                                idata.x, idata.y, frame, pitch, idata.blend_mode);
            // Draw new
            else{
                // Buffer for cache entry
                std::vector<Renderer::ImageData> event_images;
                // Calculate image-to-video scale
                double frame_scale_x, frame_scale_y;
                if(this->ssb.frame.width > 0 && this->ssb.frame.height > 0)
                    frame_scale_x = static_cast<double>(this->width) / this->ssb.frame.width, frame_scale_y = static_cast<double>(this->height) / this->ssb.frame.height;
                else
                    frame_scale_x = frame_scale_y = 0;
                // Create render state for rendering behaviour
                RenderState rs;
                // Collect render sizes (position groups -> lines -> geometry positions)
                std::vector<PosSize> render_sizes = {{}};
                for(std::shared_ptr<SSBObject>& obj : event.objects)
                    if(obj->type == SSBObject::Type::TAG){
                        if(rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms).position)
                            render_sizes.push_back({});
                    }else{  // obj->type == SSBObject::Type::GEOMETRY
                        // Calculate wrap limits
                        double wrap_width, wrap_height;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        if(rs.pos_x == std::numeric_limits<decltype(rs.pos_x)>::max() && rs.pos_y == std::numeric_limits<decltype(rs.pos_y)>::max()){
#pragma GCC diagnostic pop
                            if(frame_scale_x > 0 && frame_scale_y > 0)
                                wrap_width = (this->width - 2 * rs.margin_h) / frame_scale_x, wrap_height = (this->height - 2 * rs.margin_v) / frame_scale_y;
                            else
                                wrap_width = this->width - 2 * rs.margin_h, wrap_height = this->height - 2 * rs.margin_v;
                        }else
                            wrap_width = wrap_height = 0;
                        // Work with geometry
                        SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                        switch(geometry->type){
                            case SSBGeometry::Type::POINTS:
                            case SSBGeometry::Type::PATH:
                                {
                                    // Get points / path dimensions
                                    if(geometry->type == SSBGeometry::Type::POINTS)
                                        points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                                    else
                                        path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                                    double x1, y1, x2, y2; cairo_path_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                                    cairo_new_path(this->stencil_path_buffer);
                                    x2 = std::max(x2, 0.0); y2 = std::max(y2, 0.0);
                                    // Save render information
                                    switch(rs.direction){
                                        case SSBDirection::Mode::LTR:
                                        case SSBDirection::Mode::RTL:
                                            // Line wrap?
                                            if(render_sizes.back().lines.back().geometries.size() > 0 && wrap_width > 0 && render_sizes.back().lines.back().width + x2 > wrap_width){
                                                render_sizes.back().lines.back().space = rs.font_space_v;
                                                render_sizes.back().lines.push_back({});
                                            }
                                            // Save
                                            render_sizes.back().lines.back().geometries.push_back({render_sizes.back().lines.back().width, std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end()-1, 0.0, [](double init, LineSize& lsize) -> double{
                                                return init + lsize.height + lsize.space;
                                            }), x2, y2});
                                            render_sizes.back().lines.back().width += x2;
                                            render_sizes.back().lines.back().height = std::max(render_sizes.back().lines.back().height, y2);
                                            render_sizes.back().width = std::max(render_sizes.back().width, render_sizes.back().lines.back().width);
                                            render_sizes.back().height = std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end(), 0.0, [](double init, LineSize& lsize){
                                                return init + lsize.height + lsize.space;
                                            });
                                            break;
                                        case SSBDirection::Mode::TTB:
                                            // Line wrap?
                                            if(render_sizes.back().lines.back().geometries.size() > 0 && wrap_height > 0 && render_sizes.back().lines.back().height + y2 > wrap_height){
                                                render_sizes.back().lines.back().space = rs.font_space_h;
                                                render_sizes.back().lines.push_back({});
                                            }
                                            // Save
                                            render_sizes.back().lines.back().geometries.push_back({std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end()-1, 0.0, [](double init, LineSize& lsize){
                                                return init + lsize.width + lsize.space;
                                            }), render_sizes.back().lines.back().height, x2, y2});
                                            render_sizes.back().lines.back().width = std::max(render_sizes.back().lines.back().width, x2);
                                            render_sizes.back().lines.back().height += y2;
                                            render_sizes.back().width = std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end(), 0.0, [](double init, LineSize& lsize){
                                                return init + lsize.width + lsize.space;
                                            });
                                            render_sizes.back().height = std::max(render_sizes.back().height, render_sizes.back().lines.back().height);
                                            break;
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
                                    while(getlineex(text, line)){
                                        if(++line_i > 1){
                                            render_sizes.back().lines.back().space = (rs.direction == SSBDirection::Mode::TTB) ? rs.font_space_h : metrics.external_lead + rs.font_space_v;
                                            render_sizes.back().lines.push_back({});
                                        }
                                        switch(rs.direction){
                                            case SSBDirection::Mode::LTR:
                                            case SSBDirection::Mode::RTL:
                                                {
                                                    // Width calculation
                                                    auto get_text_width = [&font,&rs](std::string& text) -> double{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                                                        if(rs.font_space_h != 0){
#pragma GCC diagnostic pop
                                                            double width = 0;
                                                            std::vector<std::string> chars = utf8_chars(text);
                                                            for(std::string& c : chars)
                                                                width += font.get_text_width(c) + rs.font_space_h;
                                                            return width;
                                                        }else
                                                            return font.get_text_width(text);
                                                    };
                                                    // Words iteration
                                                    std::vector<Word> words = getwords(line);
                                                    std::string merged_word;
                                                    double width;
                                                    for(Word& word : words){
                                                        merged_word = word.prespace + word.text;
                                                        width = get_text_width(merged_word);
                                                        if(render_sizes.back().lines.back().geometries.size() > 0 && wrap_width > 0 && render_sizes.back().lines.back().width + width > wrap_width){
                                                            render_sizes.back().lines.back().space = metrics.external_lead + rs.font_space_v;
                                                            render_sizes.back().lines.push_back({});
                                                            width = get_text_width(word.text);
                                                        }
                                                        render_sizes.back().lines.back().geometries.push_back({render_sizes.back().lines.back().width, std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end()-1, 0.0, [](double init, LineSize& lsize){
                                                            return init + lsize.height + lsize.space;
                                                        }), width, metrics.height});
                                                        render_sizes.back().lines.back().width += width;
                                                        render_sizes.back().lines.back().height = std::max(render_sizes.back().lines.back().height, metrics.height);
                                                        render_sizes.back().width = std::max(render_sizes.back().width, render_sizes.back().lines.back().width);
                                                    }
                                                    // Update position render height
                                                    render_sizes.back().height = std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end(), 0.0, [](double init, LineSize& lsize){
                                                        return init + lsize.height + lsize.space;
                                                    });
                                                }
                                                break;
                                            case SSBDirection::Mode::TTB:
                                                {
                                                    // Extents calculation
                                                    auto get_text_extents = [&font,&metrics,&rs](std::string& text, double& width, double& height){
                                                        width = height = 0;
                                                        std::vector<std::string> chars = utf8_chars(text);
                                                        for(std::string& c : chars){
                                                            width = std::max(width, font.get_text_width(c));
                                                            height += metrics.height + rs.font_space_v;
                                                        }
                                                    };
                                                    // Words iteration
                                                    std::vector<Word> words = getwords(line);
                                                    std::string merged_word;
                                                    double width, height;
                                                    for(Word& word : words){
                                                        merged_word = word.prespace + word.text;
                                                        get_text_extents(merged_word, width, height);
                                                        if(render_sizes.back().lines.back().geometries.size() > 0 && wrap_height > 0 && render_sizes.back().lines.back().height + height > wrap_height){
                                                            render_sizes.back().lines.back().space = rs.font_space_h;
                                                            render_sizes.back().lines.push_back({});
                                                            get_text_extents(word.text, width, height);
                                                        }
                                                        render_sizes.back().lines.back().geometries.push_back({std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end()-1, 0.0, [](double init, LineSize& lsize){
                                                            return init + lsize.width + lsize.space;
                                                        }), render_sizes.back().lines.back().height, width, height});
                                                        render_sizes.back().lines.back().width = std::max(render_sizes.back().lines.back().width, width);
                                                        render_sizes.back().lines.back().height += height;
                                                        render_sizes.back().height = std::max(render_sizes.back().height, render_sizes.back().lines.back().height);
                                                    }
                                                    // Update position render width
                                                    render_sizes.back().width = std::accumulate(render_sizes.back().lines.begin(), render_sizes.back().lines.end(), 0.0, [](double init, LineSize& lsize){
                                                        return init + lsize.width + lsize.space;
                                                    });
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
                struct{
                    size_t pos = 0, line = 0, geometry = 0;
                }size_index;
                for(std::shared_ptr<SSBObject>& obj : event.objects)
                    if(obj->type == SSBObject::Type::TAG){
                        // Apply tag to render state
                        if(rs.eval_tag(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms).position){
                            ++size_index.pos;
                            size_index.line = size_index.geometry = 0;
                        }
                    }else{  // obj->type == SSBObject::Type::GEOMETRY
                        // Create geometry
                        SSBGeometry* geometry = dynamic_cast<SSBGeometry*>(obj.get());
                        Point align_point = calc_align_offset(rs.align, rs.direction, render_sizes[size_index.pos], size_index.line);
                        switch(geometry->type){
                            case SSBGeometry::Type::POINTS:
                            case SSBGeometry::Type::PATH:
                                // Update geometry index by newline
                                if(size_index.geometry >= render_sizes[size_index.pos].lines[size_index.line].geometries.size()){
                                    align_point = calc_align_offset(rs.align, rs.direction, render_sizes[size_index.pos], ++size_index.line);
                                    size_index.geometry = 0;
                                }
                                // Save geometries matrix
                                cairo_save(this->stencil_path_buffer);
                                // Set transformation for alignment
                                cairo_translate(this->stencil_path_buffer, align_point.x, align_point.y + render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_y);
                                switch(rs.direction){
                                    case SSBDirection::Mode::LTR:
                                        cairo_translate(this->stencil_path_buffer,
                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x,
                                                        0);
                                        break;
                                    case SSBDirection::Mode::RTL:
                                        cairo_translate(this->stencil_path_buffer,
                                                        render_sizes[size_index.pos].lines[size_index.line].width -
                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x -
                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].width,
                                                        0);
                                        break;
                                    case SSBDirection::Mode::TTB:
                                        cairo_translate(this->stencil_path_buffer,
                                                        render_sizes[size_index.pos].width -
                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x -
                                                        render_sizes[size_index.pos].lines[size_index.line].width + (render_sizes[size_index.pos].lines[size_index.line].width - render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].width) / 2,
                                                        0);
                                        break;
                                }
                                // Draw aligned points / path
                                if(geometry->type == SSBGeometry::Type::POINTS)
                                    points_to_cairo(dynamic_cast<SSBPoints*>(geometry), rs.line_width, this->stencil_path_buffer);
                                else
                                    path_to_cairo(dynamic_cast<SSBPath*>(geometry), this->stencil_path_buffer);
                                // Restore geometries matrix
                                cairo_restore(this->stencil_path_buffer);
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
                                    while(getlineex(text, line)){
                                        // Recalculate data for new line
                                        if(++line_i > 1){
                                            align_point = calc_align_offset(rs.align, rs.direction, render_sizes[size_index.pos], ++size_index.line);
                                            size_index.geometry = 0;
                                        }
                                        // Draw line
                                        switch(rs.direction){
                                            case SSBDirection::Mode::LTR:
                                            case SSBDirection::Mode::RTL:
                                                {
                                                    std::vector<Word> words = getwords(line);
                                                    std::string merged_word;
                                                    for(Word& word : words){
                                                        merged_word = word.prespace + word.text;
                                                        // Update geometry index by newline
                                                        if(size_index.geometry >= render_sizes[size_index.pos].lines[size_index.line].geometries.size()){
                                                            align_point = calc_align_offset(rs.align, rs.direction, render_sizes[size_index.pos], ++size_index.line);
                                                            size_index.geometry = 0;
                                                            merged_word = word.text;
                                                        }
                                                        // Define path
                                                        cairo_save(this->stencil_path_buffer);
                                                        cairo_translate(this->stencil_path_buffer,
                                                                        align_point.x +
                                                                        (rs.direction == SSBDirection::Mode::LTR ?
                                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x :
                                                                        render_sizes[size_index.pos].lines[size_index.line].width - render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x - render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].width),
                                                                        align_point.y +
                                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_y +
                                                                        (render_sizes[size_index.pos].lines[size_index.line].height - render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].height));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                                                        if(rs.font_space_h != 0){
#pragma GCC diagnostic pop
                                                            std::vector<std::string> chars = utf8_chars(merged_word);
                                                            for(std::string& c: chars){
                                                                font.text_path_to_cairo(c, this->stencil_path_buffer);
                                                                cairo_translate(this->stencil_path_buffer, font.get_text_width(c) + rs.font_space_h, 0);
                                                            }
                                                        }else
                                                            font.text_path_to_cairo(merged_word, this->stencil_path_buffer);
                                                        cairo_restore(this->stencil_path_buffer);
                                                        // Increase geometry index
                                                        if(&word != &words.back())
                                                            ++size_index.geometry;
                                                    }
                                                }
                                                break;
                                            case SSBDirection::Mode::TTB:
                                                {
                                                    std::vector<Word> words = getwords(line);
                                                    std::string merged_word;
                                                    for(Word& word : words){
                                                        merged_word = word.prespace + word.text;
                                                        // Update geometry index by newline
                                                        if(size_index.geometry >= render_sizes[size_index.pos].lines[size_index.line].geometries.size()){
                                                            align_point = calc_align_offset(rs.align, rs.direction, render_sizes[size_index.pos], ++size_index.line);
                                                            size_index.geometry = 0;
                                                            merged_word = word.text;
                                                        }
                                                        // Define path
                                                        cairo_save(this->stencil_path_buffer);
                                                        cairo_translate(this->stencil_path_buffer,
                                                                        align_point.x +
                                                                        render_sizes[size_index.pos].width - render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_x - render_sizes[size_index.pos].lines[size_index.line].width,
                                                                        align_point.y +
                                                                        render_sizes[size_index.pos].lines[size_index.line].geometries[size_index.geometry].off_y);
                                                        std::vector<std::string> chars = utf8_chars(merged_word);
                                                        for(std::string& c: chars){
                                                            cairo_save(this->stencil_path_buffer);
                                                            cairo_translate(this->stencil_path_buffer,
                                                                            (render_sizes[size_index.pos].lines[size_index.line].width - font.get_text_width(c)) / 2,
                                                                            0);
                                                            font.text_path_to_cairo(c, this->stencil_path_buffer);
                                                            cairo_restore(this->stencil_path_buffer);
                                                            cairo_translate(this->stencil_path_buffer, 0, metrics.height + rs.font_space_v);
                                                        }
                                                        cairo_restore(this->stencil_path_buffer);
                                                        // Increase geometry index
                                                        if(&word != &words.back())
                                                            ++size_index.geometry;
                                                    }
                                                }
                                                break;
                                        }
                                    }
                                }
                                break;
                        }
                        // Increase geometry index
                        ++size_index.geometry;
                        // Deform geometry
                        if(!rs.deform_x.empty() || !rs.deform_y.empty())
                            path_deform(this->stencil_path_buffer, rs.deform_x, rs.deform_y, rs.deform_progress);
                        // Get original geometry dimensions (for color shifting to geometry)
                        double x1, y1, x2, y2; cairo_path_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                        int fill_x = floor(x1), fill_y = floor(y1), fill_width = ceil(x2) - fill_x, fill_height = ceil(y2) - fill_y;
                        // Transform matrix
                        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                        if(!(rs.pos_x == std::numeric_limits<decltype(rs.pos_x)>::max() && rs.pos_y == std::numeric_limits<decltype(rs.pos_y)>::max())){
#pragma GCC diagnostic pop
                            if(frame_scale_x > 0 && frame_scale_y > 0)
                                cairo_matrix_scale(&matrix, frame_scale_x, frame_scale_y);
                            cairo_matrix_translate(&matrix, rs.pos_x, rs.pos_y);
                        }else{
                            if(frame_scale_x > 0 && frame_scale_y > 0){
                                Point pos = get_auto_pos(this->width, this->height, rs.align, rs.margin_h, rs.margin_v, frame_scale_x, frame_scale_y);
                                cairo_matrix_translate(&matrix, pos.x, pos.y);
                                cairo_matrix_scale(&matrix, frame_scale_x, frame_scale_y);
                            }else{
                                Point pos = get_auto_pos(this->width, this->height, rs.align, rs.margin_h, rs.margin_v);
                                cairo_matrix_translate(&matrix, pos.x, pos.y);
                            }
                        }
                        cairo_matrix_multiply(&matrix, &rs.matrix, &matrix);
                        cairo_apply_matrix(this->stencil_path_buffer, &matrix);
                        // Get transformed geometry dimensions (for overlay image)
                        cairo_path_extents(this->stencil_path_buffer, &x1, &y1, &x2, &y2);
                        int x = floor(x1), y = floor(y1), width = ceil(x2 - x), height = ceil(y2 - y);
                        // Set line properties
                        if(frame_scale_x > 0 && frame_scale_y > 0)
                            set_line_props(this->stencil_path_buffer, rs, (frame_scale_x + frame_scale_y) / 2);
                        else
                            set_line_props(this->stencil_path_buffer, rs);
                        // Create overlay by type
                        enum class DrawType{FILL_BLURRED, FILL_WITHOUT_BLUR, BORDER, WIRE};
                        auto create_overlay = [&](DrawType draw_type) -> Renderer::ImageData{
                            /*
                                CODE FOR PERFORMANCE TESTING ON WINDOWS

                                LARGE_INTEGER freq, t1, t2;
                                QueryPerformanceFrequency(&freq);
                                QueryPerformanceCounter(&t1);
                                // INSERT CODE
                                QueryPerformanceCounter(&t2);
                                std::ostringstream s;
                                s << "Duration: " << (static_cast<double>(t2.QuadPart - t1.QuadPart) / freq.QuadPart * 1000) << "ms";
                                MessageBoxA(NULL, s.str().c_str(), "Performance", MB_OK);
                            */
                            // Create image
                            int border_h = 0, border_v = 0;
                            switch(draw_type){
                                case DrawType::WIRE:
                                case DrawType::BORDER:
                                    border_h = ceil(rs.blur_h + cairo_get_line_width(this->stencil_path_buffer) / 2),
                                    border_v = ceil(rs.blur_v + cairo_get_line_width(this->stencil_path_buffer) / 2);
                                    break;
                                case DrawType::FILL_BLURRED:
                                    border_h = ceil(rs.blur_h),
                                    border_v = ceil(rs.blur_v);
                                    break;
                                case DrawType::FILL_WITHOUT_BLUR:
                                    // Border already with zero initialized
                                    break;
                            }
                            CairoImage image(width + (border_h << 1), height + (border_v << 1), CAIRO_FORMAT_ARGB32);
                            // Anything visible?
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                            if(!((rs.alphas.size() == 1 && rs.alphas[0] == 0.0) || (rs.alphas.size() == 4 && std::count(rs.alphas.begin(), rs.alphas.end(), 0) == 4))){
#pragma GCC diagnostic pop
                                // Transfer shifted path & matrix from buffer to image
                                cairo_translate(image, -x + border_h, -y + border_v);
                                cairo_path_t* path = cairo_copy_path(this->stencil_path_buffer);
                                cairo_append_path(image, path);
                                cairo_path_destroy(path);
                                cairo_transform(image, &matrix);
                                // Set line properties
                                if(draw_type == DrawType::BORDER || draw_type == DrawType::WIRE){
                                    if(frame_scale_x > 0)
                                        set_line_props(image, rs, (frame_scale_x + frame_scale_y) / 2);
                                    else
                                        set_line_props(image, rs);
                                }
                                // Draw colored geometry on image
                                if(draw_type == DrawType::FILL_BLURRED || draw_type == DrawType::FILL_WITHOUT_BLUR){
                                    if(rs.colors.size() == 1 && rs.alphas.size() == 1)
                                        cairo_set_source_rgba(image, rs.colors[0].r, rs.colors[0].g, rs.colors[0].b, rs.alphas[0]);
                                    else{
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wnarrowing"
                                        cairo_rectangle_t color_rect = {fill_x, fill_y, fill_width, fill_height};
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
                                    cairo_set_source_rgba(image, rs.line_color.r, rs.line_color.g, rs.line_color.b, rs.line_alpha);
                                    cairo_save(image);
                                    cairo_identity_matrix(image);
                                    cairo_stroke_preserve(image);
                                    cairo_restore(image);
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
                                        cairo_copy_matrix(image, rgb_image);
                                        // Create texture pattern for color
                                        cairo_matrix_t pattern_matrix = {1, 0, 0, 1, -fill_x - rs.texture_x, -fill_y - rs.texture_y};
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
                                        cairo_save(image);
                                        cairo_identity_matrix(image);
                                        cairo_set_source_surface(image, rgb_image, 0, 0);
                                        cairo_paint(image);
                                        cairo_restore(image);
                                    }
                                }
                                // Draw karaoke over image
                                if((draw_type == DrawType::FILL_BLURRED || draw_type == DrawType::FILL_WITHOUT_BLUR) && rs.karaoke_start >= 0){
                                    int elapsed_time = start_ms - event.start_ms;
                                    cairo_set_operator(image, CAIRO_OPERATOR_ATOP);
                                    cairo_set_source_rgb(image, rs.karaoke_color.r, rs.karaoke_color.g, rs.karaoke_color.b);
                                    if(elapsed_time >= rs.karaoke_start + rs.karaoke_duration)
                                        cairo_paint(image);
                                    else if(elapsed_time >= rs.karaoke_start){
                                        double progress = static_cast<double>(elapsed_time - rs.karaoke_start) / rs.karaoke_duration;
                                        cairo_new_path(image);
                                        switch(rs.direction){
                                            case SSBDirection::Mode::LTR: cairo_rectangle(image, fill_x, fill_y, progress * fill_width, fill_height); break;
                                            case SSBDirection::Mode::RTL: cairo_rectangle(image, fill_x + (1 - progress) * fill_width, fill_y, progress * fill_width, fill_height); break;
                                            case SSBDirection::Mode::TTB: cairo_rectangle(image, fill_x, fill_y, fill_width, progress * fill_height); break;
                                        }
                                        cairo_fill(image);
                                    }
                                }
                                // Blur image
                                if(draw_type != DrawType::FILL_WITHOUT_BLUR)
                                    cairo_image_surface_blur(image, rs.blur_h, rs.blur_v);
                                // Erase filling in stroke -> create border
                                if(draw_type == DrawType::BORDER){
                                    cairo_set_source_rgba(image, 0, 0, 0, 1);
                                    cairo_set_operator(image, CAIRO_OPERATOR_DEST_OUT);
                                    cairo_fill(image);
                                }
                            }
                            // Return complete overlay data
                            return {image, -border_h + x, -border_v + y, rs.blend_mode, rs.fade_in, rs.fade_out};
                        };
                        // Create overlay
                        Renderer::ImageData overlay;
                        if(rs.mode == SSBMode::Mode::FILL){
                            if(rs.line_width > 0 && geometry->type != SSBGeometry::Type::POINTS){
                                overlay = create_overlay(DrawType::BORDER);
                                Renderer::ImageData overlay2 = create_overlay(DrawType::FILL_WITHOUT_BLUR);
                                cairo_set_operator(overlay.image, CAIRO_OPERATOR_ADD);
                                cairo_identity_matrix(overlay.image);
                                cairo_set_source_surface(overlay.image, overlay2.image, overlay2.x - overlay.x, overlay2.y - overlay.y);
                                cairo_paint(overlay.image);
                            }else
                                overlay = create_overlay(DrawType::FILL_BLURRED);
                        }else   // rs.mode == SSBMode::Mode::WIRE
                            overlay = create_overlay(DrawType::WIRE);
                        // Apply stenciling and/or blending on frame
                        switch(rs.stencil_mode){
                            case SSBStencil::Mode::OFF:
                                this->blend(create_faded_image(overlay.image, overlay.fade_in, overlay.fade_out, start_ms, event.start_ms, event.end_ms),
                                            overlay.x, overlay.y, frame, pitch, overlay.blend_mode);
                                if(event.static_tags)
                                    event_images.push_back(overlay);
                                break;
                            case SSBStencil::Mode::INSIDE:
                                cairo_set_operator(overlay.image, CAIRO_OPERATOR_DEST_IN);
                                cairo_identity_matrix(overlay.image);
                                cairo_set_source_surface(overlay.image, this->stencil_path_buffer, -overlay.x, -overlay.y);
                                cairo_paint(overlay.image);
                                this->blend(create_faded_image(overlay.image, overlay.fade_in, overlay.fade_out, start_ms, event.start_ms, event.end_ms),
                                            overlay.x, overlay.y, frame, pitch, overlay.blend_mode);
                                if(event.static_tags)
                                    event_images.push_back(overlay);
                                break;
                            case SSBStencil::Mode::OUTSIDE:
                                cairo_set_operator(overlay.image, CAIRO_OPERATOR_DEST_OUT);
                                cairo_identity_matrix(overlay.image);
                                cairo_set_source_surface(overlay.image, this->stencil_path_buffer, -overlay.x, -overlay.y);
                                cairo_paint(overlay.image);
                                this->blend(create_faded_image(overlay.image, overlay.fade_in, overlay.fade_out, start_ms, event.start_ms, event.end_ms),
                                            overlay.x, overlay.y, frame, pitch, overlay.blend_mode);
                                if(event.static_tags)
                                    event_images.push_back(overlay);
                                break;
                            case SSBStencil::Mode::SET:
                                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_ADD);
                                cairo_set_source_surface(this->stencil_path_buffer, overlay.image, overlay.x, overlay.y);
                                cairo_paint(this->stencil_path_buffer);
                                break;
                            case SSBStencil::Mode::UNSET:
                                // Invert alpha
                                cairo_set_operator(overlay.image, CAIRO_OPERATOR_XOR);
                                cairo_set_source_rgba(overlay.image, 1, 1, 1, 1);
                                cairo_paint(overlay.image);
                                // Multiply alpha
                                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_IN);
                                cairo_set_source_surface(this->stencil_path_buffer, overlay.image, overlay.x, overlay.y);
                                cairo_paint(this->stencil_path_buffer);
                                break;
                        }
                        // Clear path
                        cairo_new_path(this->stencil_path_buffer);
                    }
                // Clear stencil
                cairo_set_operator(this->stencil_path_buffer, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgba(this->stencil_path_buffer, 0, 0, 0, 0);
                cairo_paint(this->stencil_path_buffer);
                // Save event images to cache
                if(!event_images.empty())
                    this->cache.add(&event, event_images);
            }
        }
}
