/*
Project: SSBRenderer
File: cairo++.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "cairo++.hpp"
#include <cmath>
#include <algorithm>
#include <xmmintrin.h>
#include "thread.hpp"
#include <memory>

void cairo_path_filter(cairo_t* ctx, std::function<void(double&, double&)> filter){
    // Get flatten path
    cairo_path_t* path = cairo_copy_path_flat(ctx);
    if(path->status == CAIRO_STATUS_SUCCESS && path->num_data > 0){
        // Create new flatten path with short lines
        std::vector<cairo_path_data_t> new_path_data;
        struct{double x = 0, y = 0;} last_point;
        cairo_path_data_t* pdata;
        for(int i = 0; i < path->num_data; i += path->data[i].header.length){
            pdata = &path->data[i];
            switch(pdata->header.type){
                case CAIRO_PATH_CURVE_TO:
                    // Doesn't exist in flatten path
                    break;
                case CAIRO_PATH_CLOSE_PATH:
                    new_path_data.push_back(pdata[0]);
                    break;
                case CAIRO_PATH_MOVE_TO:
                    last_point.x = pdata[1].point.x;
                    last_point.y = pdata[1].point.y;
                    filter(pdata[1].point.x, pdata[1].point.y);
                    new_path_data.push_back(pdata[0]);
                    new_path_data.push_back(pdata[1]);
                    break;
                case CAIRO_PATH_LINE_TO:
                    {
                        double vec_x = pdata[1].point.x - last_point.x, vec_y = pdata[1].point.y - last_point.y;
                        double line_len = hypot(vec_x, vec_y);
                        constexpr double max_len = sqrt(2);
                        if(line_len > max_len){
                            double progress;
                            cairo_path_data_t path_point;
                            for(double cur_len = max_len; cur_len < line_len; cur_len += max_len){
                                progress = cur_len / line_len;
                                path_point.point.x = last_point.x + progress * vec_x;
                                path_point.point.y = last_point.y + progress * vec_y;
                                filter(path_point.point.x, path_point.point.y);
                                new_path_data.push_back(pdata[0]);
                                new_path_data.push_back(path_point);
                            }
                        }
                    }
                    last_point.x = pdata[1].point.x;
                    last_point.y = pdata[1].point.y;
                    filter(pdata[1].point.x, pdata[1].point.y);
                    new_path_data.push_back(pdata[0]);
                    new_path_data.push_back(pdata[1]);
                    break;
            }
        }
        // Replace old context path with new one
        cairo_path_t new_path = {
            CAIRO_STATUS_SUCCESS,
            new_path_data.data(),
            static_cast<int>(new_path_data.size())
        };
        cairo_new_path(ctx);
        cairo_append_path(ctx, &new_path);
    }
    // Destroy flatten path
    cairo_path_destroy(path);
}

cairo_pattern_t* cairo_pattern_create_rect_color(cairo_rectangle_t rect,
                                                        double r0, double g0, double b0, double a0,
                                                        double r1, double g1, double b1, double a1,
                                                        double r2, double g2, double b2, double a2,
                                                        double r3, double g3, double b3, double a3){
    cairo_pattern_t* mesh = cairo_pattern_create_mesh();
    cairo_mesh_pattern_begin_patch(mesh);
    cairo_mesh_pattern_move_to(mesh, rect.x, rect.y);
    cairo_mesh_pattern_line_to(mesh, rect.x + rect.width, rect.y);
    cairo_mesh_pattern_line_to(mesh, rect.x + rect.width, rect.y + rect.height);
    cairo_mesh_pattern_line_to(mesh, rect.x, rect.y + rect.height);
    cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, r0, g0, b0, a0);
    cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, r1, g1, b1, a1);
    cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, r2, g2, b2, a2);
    cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, r3, g3, b3, a3);
    cairo_mesh_pattern_end_patch(mesh);
    return mesh;
}

void cairo_image_surface_blur(cairo_surface_t* surface, double blur_h, double blur_v){
    // Valid blur range?
    if(blur_h >= 0 && blur_v >= 0 && (blur_h > 0 || blur_v > 0)){
        // Get surface data
        int width = cairo_image_surface_get_width(surface);
        int height = cairo_image_surface_get_height(surface);
        cairo_format_t format = cairo_image_surface_get_format(surface);
        int stride = cairo_image_surface_get_stride(surface);
        unsigned char* data = cairo_image_surface_get_data(surface);
        // Flush pending operations on surface
        cairo_surface_flush(surface);
        // Create data in float format for vector operations
        const unsigned long int size = height * stride;
        std::vector<float> fdata(size);
        std::copy(data, data + size, fdata.data());
        // Create blur kernel
        int kernel_radius_x = ceil(blur_h),
            kernel_radius_y = ceil(blur_v),
            kernel_width = (kernel_radius_x << 1) + 1,
            kernel_height = (kernel_radius_y << 1) + 1;
        std::vector<float> kernel_data(kernel_width * kernel_height);
        // Fill blur kernel (box blur)
        std::fill(kernel_data.begin(), kernel_data.end(), 1.0f);
        float kernel_border_x = 1 - (kernel_radius_x - blur_h),
                kernel_border_y = 1 - (kernel_radius_y - blur_v);
        if(kernel_border_x > 0)
            for(int kernel_y = 0; kernel_y < kernel_height; ++kernel_y){
                kernel_data[kernel_y * kernel_width] *= kernel_border_x;
                kernel_data[kernel_y * kernel_width + kernel_width-1] *= kernel_border_x;
            }
        if(kernel_border_y > 0)
            for(int kernel_x = 0; kernel_x < kernel_width; ++kernel_x){
                kernel_data[kernel_x] *= kernel_border_y;
                kernel_data[(kernel_height-1) * kernel_width + kernel_x] *= kernel_border_y;
            }
        // Normalize blur kernel
        float divisor = std::accumulate(kernel_data.begin(), kernel_data.end(), 0.0f);
        std::for_each(kernel_data.begin(), kernel_data.end(), [&divisor](float& v){v /= divisor;});
        // Apply kernel on image
        int max_threads = Thread<int>::get_logical_processors();
        auto filter = [&](int first_row){
            if(format == CAIRO_FORMAT_A8){
                unsigned char* row_dst;
                float accum;
                int image_x, image_y;
                for(int y = first_row; y < height; y += max_threads){
                    row_dst = data + y * stride;
                    for(int x = 0; x < width; ++x){
                        accum = 0;
                        for(int kernel_y = 0; kernel_y < kernel_height; ++kernel_y){
                            image_y = y + kernel_y - kernel_radius_y;
                            if(image_y < 0 || image_y >= height)
                                continue;
                            for(int kernel_x = 0; kernel_x < kernel_width; ++kernel_x){
                                image_x = x + kernel_x - kernel_radius_x;
                                if(image_x < 0 || image_x >= width)
                                    continue;
                                accum += fdata[image_y * stride + image_x] * kernel_data[kernel_y * kernel_width + kernel_x];
                            }
                        }
                        *row_dst++ = accum < 0.0 ? 0.0 : (accum > 255.0f ? 255.0f : accum);
                    }
                }
            }else if(format == CAIRO_FORMAT_ARGB32 || format == CAIRO_FORMAT_RGB24){
                unsigned char* row_dst;
                __m128 accum;
                float accum_buf[4];
                int image_x, image_y;
                for(int y = first_row; y < height; y += max_threads){
                    row_dst = data + y * stride;
                    for(int x = 0; x < width; ++x){
                        accum = _mm_xor_ps(accum, accum);
                        for(int kernel_y = 0; kernel_y < kernel_height; ++kernel_y){
                            image_y = y + kernel_y - kernel_radius_y;
                            if(image_y < 0 || image_y >= height)
                                continue;
                            for(int kernel_x = 0; kernel_x < kernel_width; ++kernel_x){
                                image_x = x + kernel_x - kernel_radius_x;
                                if(image_x < 0 || image_x >= width)
                                    continue;
                                accum = _mm_add_ps(
                                    accum,
                                    _mm_mul_ps(
                                        _mm_loadu_ps(&fdata[image_y * stride + (image_x << 2)]),
                                        _mm_set_ps1(kernel_data[kernel_y * kernel_width + kernel_x])
                                    )
                                );
                            }
                        }
                        _mm_storeu_ps(  // Somehow MinGW32 doesn't support 16 byte alignment
                            accum_buf,
                            _mm_max_ps(
                                _mm_setzero_ps(),
                                _mm_min_ps(
                                    _mm_set_ps1(255.0f),
                                    accum
                                )
                            )
                        );
                        row_dst[0] = accum_buf[0];
                        row_dst[1] = accum_buf[1];
                        row_dst[2] = accum_buf[2];
                        row_dst[3] = accum_buf[3];
                        row_dst += 4;
                    }
                }
            }
        };
        std::vector<std::shared_ptr<Thread<int>>> threads(max_threads);
        for(int i = 0; i < max_threads; ++i)
            threads[i] = std::shared_ptr<Thread<int>>(new Thread<int>(filter, i));
        for(std::shared_ptr<Thread<int>>& thread : threads)
            thread->join();
        // Signal changes on surfaces
        cairo_surface_mark_dirty(surface);
    }
}
