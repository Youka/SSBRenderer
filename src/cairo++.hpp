#pragma once

#include <cairo.h>

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
