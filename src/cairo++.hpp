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

#ifdef _WIN32
#include <cairo.h>
#include <windows.h>
#else
#include <pango/pangocairo.h>
#endif
#include "Cache.hpp"
#include <vector>

class CairoImage{
    private:
        // Image + image context
        cairo_surface_t* surface;
        cairo_t* context = nullptr;
        // File image cache
        static Cache<std::string,CairoImage> cache;
    public:
        // Ctor & dtor
        CairoImage();
        CairoImage(int width, int height, cairo_format_t format);
        CairoImage(std::string png_filename);
        ~CairoImage();
        // Copy
        CairoImage(const CairoImage& image);
        CairoImage& operator=(const CairoImage& image);
        // Cast
        operator cairo_surface_t*() const;
        operator cairo_t*();
};

class NativeFont{
    private:
#ifdef _WIN32
        // Platform dependent font data
        HDC dc;
        HFONT font;
        HGDIOBJ old_font;
        // Upscale / quality / precision
#else
        // Platform dependent font data
        CairoImage dc;
        PangoLayout* layout;
#endif
        constexpr static double UPSCALE = 64;
    public:
        // Font metrics structure
        struct FontMetrics{
            double height, ascent, descent, internal_lead, external_lead;
        };
        // No copy
        NativeFont(const NativeFont&) = delete;
        NativeFont& operator=(const NativeFont&) = delete;
        // Ctor & dtor
        NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, float size, bool rtl = false);
#ifdef _WIN32
        NativeFont(std::wstring family, bool bold, bool italic, bool underline, bool strikeout, float size, bool rtl = false);
#endif
        ~NativeFont();
        // Get font metrics
        FontMetrics get_metrics();
        // Get text width
        double get_text_width(std::string& text);
#ifdef _WIN32
        double get_text_width(std::wstring& text);
#endif
        // Add text path to cairo context
        void text_path_to_cairo(std::string& text, cairo_t* ctx);
#ifdef _WIN32
        void text_path_to_cairo(std::wstring& text, cairo_t* ctx);
#endif
};

void cairo_path_filter(cairo_t* ctx, std::function<void(double&, double&)> filter);

cairo_pattern_t* cairo_pattern_create_linear_color(double x0, double y0, double x1, double y1,
                                                    double r0, double g0, double b0, double a0,
                                                    double r1, double g1, double b1, double a1);

cairo_pattern_t* cairo_pattern_create_rect_color(cairo_rectangle_t rect,
                                                        double r0, double g0, double b0, double a0,
                                                        double r1, double g1, double b1, double a1,
                                                        double r2, double g2, double b2, double a2,
                                                        double r3, double g3, double b3, double a3);

void cairo_image_surface_blur(cairo_surface_t* surface, float blur_h, float blur_v);

void cairo_apply_matrix(cairo_t* ctx, cairo_matrix_t* mat);

void cairo_copy_matrix(cairo_t* src, cairo_t* dst);
