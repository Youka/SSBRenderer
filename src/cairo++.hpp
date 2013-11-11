/*
Project: SSBRenderer
File: cairo++.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <cairo.h>
#include <vector>
#include <cmath>

class CairoImage{
    private:
        // Image + image context
        cairo_surface_t* surface;
        cairo_t* context;
    public:
        // Ctor & dtor
        CairoImage() : surface(cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)), context(nullptr){}
        CairoImage(int width, int height, cairo_format_t format) : surface(cairo_image_surface_create(format, width, height)), context(nullptr){}
        ~CairoImage(){
            if(this->context)
                cairo_destroy(this->context);
            cairo_surface_destroy(this->surface);
        }
        // Copy
        CairoImage(const CairoImage& image){
            this->surface = cairo_surface_reference(image.surface);
            this->context = image.context ? cairo_reference(image.context) : nullptr;
        }
        CairoImage& operator=(const CairoImage& image){
            // Free old content
            if(this->context)
                cairo_destroy(this->context);
            cairo_surface_destroy(this->surface);
            // Assign new content
            this->surface = cairo_surface_reference(image.surface);
            this->context = image.context ? cairo_reference(image.context) : nullptr;
            return *this;
        }
        // Cast
        operator cairo_surface_t*() const{
            return this->surface;
        }
        operator cairo_t*(){
            if(!this->context)
                this->context = cairo_create(this->surface);
            return this->context;
        }
};

inline void cairo_path_filter(cairo_t* ctx, std::function<void(double&, double&)> filter){
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

// SSE2 vector type for parallel single-precision floating-point arithmetric
union v4sf{
    float __attribute__ ((vector_size(16))) x;
    float v[4];
};

inline void cairo_image_surface_blur(cairo_surface_t* surface, double blur_h, double blur_v){
    // Valid blur range?
    if(blur_h > 0 || blur_v > 0){
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
        unsigned int blur_rx = blur_h < 0 ? 0 : ceil(blur_h),
                    blur_ry = blur_v < 0 ? 0 : ceil(blur_v),
                    kernel_width = (blur_rx << 1) + 1,
                    kernel_height = (blur_ry << 1) + 1;
#pragma message "Implent cairo surface blurring"
        // Run threads

        // Signal changes on surfaces
        cairo_surface_mark_dirty(surface);
    }
}

#ifdef _WIN32
#include "textconv.hpp"

class NativeFont{
    private:
        // Platform dependent font data
        HDC dc;
        HFONT font;
        HGDIOBJ old_font;
        // Upscale / quality / precision
        constexpr static double UPSCALE = 64;
    public:
        // Ctor & dtor
        NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, unsigned short int size){
            this->dc = CreateCompatibleDC(NULL);
            SetMapMode(this->dc, MM_TEXT);
            SetBkMode(this->dc, TRANSPARENT);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
            LOGFONTW lf = {0};
#pragma GCC diagnostic pop
            lf.lfHeight = size * UPSCALE;
            lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
            lf.lfItalic = italic;
            lf.lfUnderline = underline;
            lf.lfStrikeOut = strikeout;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfOutPrecision = OUT_TT_PRECIS;
            lf.lfQuality = ANTIALIASED_QUALITY;
            lf.lfFaceName[utf8_to_utf16(family).copy(lf.lfFaceName, LF_FACESIZE - 1)] = L'\0';
            this->font = CreateFontIndirectW(&lf);
            this->old_font = SelectObject(this->dc, this->font);
        }
        ~NativeFont(){
            SelectObject(this->dc, this->old_font);
            DeleteObject(this->font);
            DeleteDC(this->dc);
        }
        // No copy
        NativeFont(const NativeFont&) = delete;
        NativeFont& operator=(const NativeFont&) = delete;
        // Font metrics structure
        struct FontMetrics{
            double height, ascent, descent, internal_lead, external_lead;
        };
        // Get font metrics
        FontMetrics get_metrics(){
            TEXTMETRICW metrics;
            GetTextMetricsW(this->dc, &metrics);
            return {
                static_cast<double>(metrics.tmHeight) / UPSCALE,
                static_cast<double>(metrics.tmAscent) / UPSCALE,
                static_cast<double>(metrics.tmDescent) / UPSCALE,
                static_cast<double>(metrics.tmInternalLeading) / UPSCALE,
                static_cast<double>(metrics.tmExternalLeading) / UPSCALE
            };
        }
        // Get text width
        double get_text_width(std::string& text){
            std::wstring textw = utf8_to_utf16(text);
            SIZE sz;
            GetTextExtentPoint32W(this->dc, textw.c_str(), textw.length(), &sz);
            return static_cast<double>(sz.cx) / UPSCALE;
        }
        // Add text path to cairo context
        void text_path_to_cairo(std::string& text, cairo_t* ctx){
            std::wstring textw = utf8_to_utf16(text);
            // Add path to windows context
            BeginPath(this->dc);
            ExtTextOutW(this->dc, 0, 0, 0x0, NULL, textw.c_str(), textw.length(), NULL);
            EndPath(this->dc);
            // Get windows path
            int points_n = GetPath(this->dc, NULL, NULL, 0);
            if(points_n > 0){
                std::vector<POINT> points(points_n);
                std::vector<BYTE> types(points_n);
                GetPath(this->dc, points.data(), types.data(), points_n);
                // Transfers windows path to cairo context
                POINT last_point = {-1,-1};
                for(int point_i = 0; point_i < points_n;)
                    switch(types[point_i]){
                        case PT_MOVETO:
                            cairo_close_path(ctx);
                            cairo_move_to(ctx, static_cast<double>(points[point_i].x) / UPSCALE, static_cast<double>(points[point_i].y) / UPSCALE);
                            last_point = points[point_i];
                            ++point_i;
                            break;
                        case PT_LINETO:
                        case PT_LINETO|PT_CLOSEFIGURE:
                            if(!(points[point_i].x == last_point.x && points[point_i].y == last_point.y)){
                                cairo_line_to(ctx, static_cast<double>(points[point_i].x) / UPSCALE, static_cast<double>(points[point_i].y) / UPSCALE);
                                last_point = points[point_i];
                            }
                            ++point_i;
                            break;
                        case PT_BEZIERTO:
                        case PT_BEZIERTO|PT_CLOSEFIGURE:
                            if(points[point_i].x == last_point.x && points[point_i].y == last_point.y)
                                points[point_i] = points[point_i+1];
                            cairo_curve_to(ctx,
                                static_cast<double>(points[point_i].x) / UPSCALE, static_cast<double>(points[point_i].y) / UPSCALE,
                                static_cast<double>(points[point_i+1].x) / UPSCALE, static_cast<double>(points[point_i+1].y) / UPSCALE,
                                static_cast<double>(points[point_i+2].x) / UPSCALE, static_cast<double>(points[point_i+2].y) / UPSCALE);
                            last_point = points[point_i+2];
                            point_i += 3;
                            break;
                    }
                cairo_close_path(ctx);
            }
            // Remove path from windows context
            AbortPath(this->dc);
        }
};
#else
#   error "Not implented"
#endif
