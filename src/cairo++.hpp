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
#include "FileReader.hpp"
#include <algorithm>
#include <deque>
#include <vector>

class CairoImage{
    private:
        // Image + image context
        cairo_surface_t* surface;
        cairo_t* context = nullptr;
        // File image cache
        static std::deque<std::pair<std::string,CairoImage>> cache;
    public:
        // Ctor & dtor
        CairoImage() : surface(cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)){}
        CairoImage(int width, int height, cairo_format_t format) : surface(cairo_image_surface_create(format, width, height)){}
        CairoImage(std::string png_filename) : context(nullptr){
            // File image in cache?
            auto it = std::find_if(this->cache.begin(), this->cache.end(), [&png_filename](std::pair<std::string,CairoImage>& image){
                return image.first == png_filename;
            });
            // Reuse file image
            if(it != this->cache.end()){
                this->surface = cairo_surface_reference(it->second);
                // Refresh lifetime in cache
                auto elem = *it;
                this->cache.erase(it);
                this->cache.push_front(elem);
            // Create new file image
            }else{
                FileReader file(png_filename);
                if(file){
                    this->surface = cairo_image_surface_create_from_png_stream([](void* closure, unsigned char* data, unsigned int length){
                            if(reinterpret_cast<FileReader*>(closure)->read(length, data) == length)
                                return CAIRO_STATUS_SUCCESS;
                            else
                                return CAIRO_STATUS_READ_ERROR;
                        }, &file);
                    // Add valid file image to cache
                    if(cairo_surface_status(this->surface) == CAIRO_STATUS_SUCCESS){
                        this->cache.push_front({png_filename, *this});
                        // Limit cache size to max. 32 elements
                        if(this->cache.size() > 32)
                            this->cache.pop_back();
                    }
                }else
                    this->surface = cairo_image_surface_create(CAIRO_FORMAT_INVALID, 1, 1);
            }
        }
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

class CairoPath{
    private:
        unsigned int* reference_counter;
        cairo_path_t* path;
    public:
        // Ctor & dtor
        CairoPath() : reference_counter(new unsigned int), path(nullptr){
            *this->reference_counter = 1;
        }
        CairoPath(cairo_t* ctx) : reference_counter(new unsigned int){
            *this->reference_counter = 1;
            this->path = cairo_copy_path(ctx);
            if(this->path->status != CAIRO_STATUS_SUCCESS)
                this->path = nullptr;
        }
        ~CairoPath(){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                if(this->path) cairo_path_destroy(this->path);
            }
        }
        // Copy
        CairoPath(const CairoPath& other) : reference_counter(other.reference_counter), path(other.path){
            *this->reference_counter += 1;
        }
        CairoPath& operator=(const CairoPath& other){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                if(this->path) cairo_path_destroy(this->path);
            }
            this->reference_counter = other.reference_counter;
            *this->reference_counter += 1;
            this->path = other.path;
            return *this;
        }
        // Access
        operator cairo_path_t*() const {
            return this->path;
        }
};

#ifdef _WIN32
#include "textconv.hpp"
#else
#error "Not implented"
#endif

class NativeFont{
    private:
#ifdef _WIN32
        // Platform dependent font data
        HDC dc;
        HFONT font;
        HGDIOBJ old_font;
        // Upscale / quality / precision
        constexpr static double UPSCALE = 64;
#else
#error "Not implented"
#endif
    public:
#ifdef _WIN32
        // Ctor & dtor
        NativeFont(std::wstring family, bool bold, bool italic, bool underline, bool strikeout, unsigned short int size, bool rtl = false){
            this->dc = CreateCompatibleDC(NULL);
            SetMapMode(this->dc, MM_TEXT);
            SetBkMode(this->dc, TRANSPARENT);
            if(rtl)
                SetTextAlign(this->dc, TA_RTLREADING);
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
            lf.lfFaceName[family.copy(lf.lfFaceName, LF_FACESIZE - 1)] = L'\0';
            this->font = CreateFontIndirectW(&lf);
            this->old_font = SelectObject(this->dc, this->font);
        }
        NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, unsigned short int size, bool rtl = false)
        : NativeFont(utf8_to_utf16(family), bold, italic, underline, strikeout, size, rtl){}
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
        double get_text_width(std::wstring& text){
            SIZE sz;
            GetTextExtentPoint32W(this->dc, text.c_str(), text.length(), &sz);
            return static_cast<double>(sz.cx) / UPSCALE;
        }
        double get_text_width(std::string& text){
            std::wstring textw = utf8_to_utf16(text);
            return this->get_text_width(textw);
        }
        // Add text path to cairo context
        void text_path_to_cairo(std::wstring& text, cairo_t* ctx){
            // Add path to windows context
            BeginPath(this->dc);
            ExtTextOutW(this->dc, 0, 0, 0x0, NULL, text.c_str(), text.length(), NULL);
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
        void text_path_to_cairo(std::string& text, cairo_t* ctx){
            std::wstring textw = utf8_to_utf16(text);
            this->text_path_to_cairo(textw, ctx);
        }
#else
#error "Not implented"
#endif
};

void cairo_path_filter(cairo_t* ctx, std::function<void(double&, double&)> filter);

cairo_pattern_t* cairo_pattern_create_rect_color(cairo_rectangle_t rect,
                                                        double r0, double g0, double b0, double a0,
                                                        double r1, double g1, double b1, double a1,
                                                        double r2, double g2, double b2, double a2,
                                                        double r3, double g3, double b3, double a3);

void cairo_image_surface_blur(cairo_surface_t* surface, double blur_h, double blur_v);

void cairo_apply_matrix(cairo_t* ctx, cairo_matrix_t* mat);

void cairo_copy_matrix(cairo_t* src, cairo_t* dst);
