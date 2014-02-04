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
#include "textconv.hpp"
#else
#include <pango/pangocairo.h>
#endif
#include "FileReader.hpp"
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
        CairoImage() : surface(cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)){}
        CairoImage(int width, int height, cairo_format_t format) : surface(cairo_image_surface_create(format, width, height)){}
        CairoImage(std::string png_filename) : context(nullptr){
            // Reuse file image
            if(this->cache.contains(png_filename))
                this->surface = cairo_surface_reference(this->cache.get(png_filename));
            // Create new file image
            else{
                FileReader file(png_filename);
                if(file){
                    this->surface = cairo_image_surface_create_from_png_stream([](void* closure, unsigned char* data, unsigned int length){
                            if(reinterpret_cast<FileReader*>(closure)->read(length, data) == length)
                                return CAIRO_STATUS_SUCCESS;
                            else
                                return CAIRO_STATUS_READ_ERROR;
                        }, &file);
                    // Add valid file image to cache
                    if(cairo_surface_status(this->surface) == CAIRO_STATUS_SUCCESS)
                        this->cache.add(png_filename, *this);
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
        // Ctor & dtor
        NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, unsigned short int size, bool rtl = false){
            this->layout = pango_cairo_create_layout(this->dc);
            PangoFontDescription *font = pango_font_description_new();
            pango_font_description_set_family(font, family.c_str());
            pango_font_description_set_weight(font, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
            pango_font_description_set_style(font, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
            pango_font_description_set_absolute_size(font, size * PANGO_SCALE * UPSCALE);
            pango_layout_set_font_description(this->layout, font);
            pango_font_description_free(font);
            PangoAttrList* attr_list = pango_attr_list_new();
            pango_attr_list_insert(attr_list, pango_attr_underline_new(underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE));
            pango_attr_list_insert(attr_list, pango_attr_strikethrough_new(strikeout));
            pango_layout_set_attributes(this->layout, attr_list);
            pango_attr_list_unref(attr_list);
            pango_layout_set_auto_dir(this->layout, rtl);
        }
        ~NativeFont(){
            g_object_unref(this->layout);
        }
        // Get font metrics
        FontMetrics get_metrics(){
            FontMetrics result;
            PangoContext* ctx = pango_layout_get_context(this->layout);
            const PangoFontDescription* desc = pango_layout_get_font_description(this->layout);
            PangoFontMetrics* metrics = pango_context_get_metrics(ctx, desc, NULL);
            result.height = static_cast<double>(pango_layout_get_baseline(this->layout)) / PANGO_SCALE / UPSCALE;
            result.ascent = static_cast<double>(pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE / UPSCALE;
            result.descent = static_cast<double>(pango_font_metrics_get_descent(metrics)) / PANGO_SCALE / UPSCALE;
            result.internal_lead = result.height - result.ascent;
            result.external_lead = static_cast<double>(pango_layout_get_spacing(this->layout)) / PANGO_SCALE / UPSCALE;
            pango_font_metrics_unref(metrics);
            return result;
        }
        // Get text width
        double get_text_width(std::string& text){
            pango_layout_set_text(this->layout, text.c_str(), -1);
            PangoRectangle rect;
            pango_layout_get_pixel_extents(this->layout, NULL, &rect);
            return static_cast<double>(rect.width) / UPSCALE;
        }
        // Add text path to cairo context
        void text_path_to_cairo(std::string& text, cairo_t* ctx){
            pango_layout_set_text(this->layout, text.c_str(), -1);
            cairo_save(ctx);
            cairo_scale(ctx, 1 / UPSCALE, 1 / UPSCALE);
            pango_cairo_layout_path(ctx, this->layout);
            cairo_restore(ctx);
        }
#endif
};

void cairo_path_filter(cairo_t* ctx, std::function<void(double&, double&)> filter);

cairo_pattern_t* cairo_pattern_create_rect_color(cairo_rectangle_t rect,
                                                        double r0, double g0, double b0, double a0,
                                                        double r1, double g1, double b1, double a1,
                                                        double r2, double g2, double b2, double a2,
                                                        double r3, double g3, double b3, double a3);

void cairo_image_surface_blur(cairo_surface_t* surface, float blur_h, float blur_v);

void cairo_apply_matrix(cairo_t* ctx, cairo_matrix_t* mat);

void cairo_copy_matrix(cairo_t* src, cairo_t* dst);
