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
#include "FileReader.hpp"
#include <cmath>
#include <algorithm>
#include "sse.hpp"
#include "thread.h"
#ifdef _WIN32
#include "textconv.hpp"
#endif

Cache<std::string,CairoImage> CairoImage::cache;

CairoImage::CairoImage() : surface(cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)){}

CairoImage::CairoImage(int width, int height, cairo_format_t format) : surface(cairo_image_surface_create(format, width, height)){}

CairoImage::CairoImage(std::string png_filename) : context(nullptr){
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

CairoImage::~CairoImage(){
    if(this->context)
        cairo_destroy(this->context);
    cairo_surface_destroy(this->surface);
}

CairoImage::CairoImage(const CairoImage& image){
    this->surface = cairo_surface_reference(image.surface);
    this->context = image.context ? cairo_reference(image.context) : nullptr;
}

CairoImage& CairoImage::operator=(const CairoImage& image){
    // Free old content
    if(this->context)
        cairo_destroy(this->context);
    cairo_surface_destroy(this->surface);
    // Assign new content
    this->surface = cairo_surface_reference(image.surface);
    this->context = image.context ? cairo_reference(image.context) : nullptr;
    return *this;
}

CairoImage::operator cairo_surface_t*() const{
        return this->surface;
}
CairoImage::operator cairo_t*(){
    if(!this->context){
        this->context = cairo_create(this->surface);
        cairo_set_tolerance(this->context, 0.05);
    }
    return this->context;
}

#ifdef _WIN32
NativeFont::NativeFont(std::wstring family, bool bold, bool italic, bool underline, bool strikeout, float size, bool rtl){
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

NativeFont::NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, float size, bool rtl)
: NativeFont(utf8_to_utf16(family), bold, italic, underline, strikeout, size, rtl){}

NativeFont::~NativeFont(){
    SelectObject(this->dc, this->old_font);
    DeleteObject(this->font);
    DeleteDC(this->dc);
}

NativeFont::FontMetrics NativeFont::get_metrics(){
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

double NativeFont::get_text_width(std::wstring& text){
    SIZE sz;
    GetTextExtentPoint32W(this->dc, text.c_str(), text.length(), &sz);
    return static_cast<double>(sz.cx) / UPSCALE;
}

double NativeFont::get_text_width(std::string& text){
    std::wstring textw = utf8_to_utf16(text);
    return this->get_text_width(textw);
}

void NativeFont::text_path_to_cairo(std::wstring& text, cairo_t* ctx){
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
        BYTE cur_type;
        for(int point_i = 0; point_i < points_n;){
            cur_type = types[point_i];
            switch(cur_type){
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
                    if(!(points[point_i].x == last_point.x && points[point_i].y == last_point.y &&
                        points[point_i+1].x == last_point.x && points[point_i+1].y == last_point.y &&
                        points[point_i+2].x == last_point.x && points[point_i+2].y == last_point.y))
                        cairo_curve_to(ctx,
                            static_cast<double>(points[point_i].x) / UPSCALE, static_cast<double>(points[point_i].y) / UPSCALE,
                            static_cast<double>(points[point_i+1].x) / UPSCALE, static_cast<double>(points[point_i+1].y) / UPSCALE,
                            static_cast<double>(points[point_i+2].x) / UPSCALE, static_cast<double>(points[point_i+2].y) / UPSCALE);
                    last_point = points[point_i+2];
                    point_i += 3;
                    break;
            }
            if(cur_type&PT_CLOSEFIGURE)
                cairo_close_path(ctx);
        }
        cairo_close_path(ctx);
    }
    // Remove path from windows context
    AbortPath(this->dc);
}

void NativeFont::text_path_to_cairo(std::string& text, cairo_t* ctx){
    std::wstring textw = utf8_to_utf16(text);
    this->text_path_to_cairo(textw, ctx);
}
#else
NativeFont::NativeFont(std::string& family, bool bold, bool italic, bool underline, bool strikeout, float size, bool rtl){
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

~NativeFont::NativeFont(){
    g_object_unref(this->layout);
}

NativeFont::FontMetrics NativeFont::get_metrics(){
    FontMetrics result;
    PangoContext* ctx = pango_layout_get_context(this->layout);
    const PangoFontDescription* desc = pango_layout_get_font_description(this->layout);
    PangoFontMetrics* metrics = pango_context_get_metrics(ctx, desc, NULL);
    result.ascent = static_cast<double>(pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE / UPSCALE;
    result.descent = static_cast<double>(pango_font_metrics_get_descent(metrics)) / PANGO_SCALE / UPSCALE;
    result.height = result.ascent + result.descent;
    result.internal_lead = result.height - result.ascent - result.descent;
    result.external_lead = static_cast<double>(pango_layout_get_spacing(this->layout)) / PANGO_SCALE / UPSCALE;
    pango_font_metrics_unref(metrics);
    return result;
}

double NativeFont::get_text_width(std::string& text){
    pango_layout_set_text(this->layout, text.c_str(), -1);
    PangoRectangle rect;
    pango_layout_get_pixel_extents(this->layout, NULL, &rect);
    return static_cast<double>(rect.width) / UPSCALE;
}

void NativeFont::text_path_to_cairo(std::string& text, cairo_t* ctx){
    pango_layout_set_text(this->layout, text.c_str(), -1);
    cairo_save(ctx);
    cairo_scale(ctx, 1 / UPSCALE, 1 / UPSCALE);
    pango_cairo_layout_path(ctx, this->layout);
    cairo_restore(ctx);
}
#endif

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

cairo_pattern_t* cairo_pattern_create_linear_color(double x0, double y0, double x1, double y1,
                                                    double r0, double g0, double b0, double a0,
                                                    double r1, double g1, double b1, double a1){
    cairo_pattern_t* gradient = cairo_pattern_create_linear(x0, y0, x1, y1);
    cairo_pattern_add_color_stop_rgba(gradient, 0.0, r0, g0, b0, a0);
    cairo_pattern_add_color_stop_rgba(gradient, 1.0, r1, g1, b1, a1);
    return gradient;
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

namespace{
    struct blur_h_thread_data{
        // Kernel data
        std::vector<float>* kernel_h;
        // Image meta data
        int width, height, stride, first_row, row_step;
        cairo_format_t format;
        // Image data
        float* src, *dst;
    };
    struct blur_v_thread_data{
        // Kernel data
        std::vector<float>* kernel_v;
        // Image meta data
        int width, height, stride, first_row, row_step;
        cairo_format_t format;
        // Image data
        float* src;
        unsigned char* dst;
    };
    THREAD_FUNC_BEGIN(blur_h_filter)
        blur_h_thread_data* tdata = reinterpret_cast<blur_h_thread_data*>(userdata);
        int kernel_radius_h = (tdata->kernel_h->size() - 1) >> 1;
        if(tdata->format == CAIRO_FORMAT_A8){
            float* row_src, *row_dst;
            float accum;
            int image_x;
            for(int y = tdata->first_row; y < tdata->height; y += tdata->row_step){
                row_src = tdata->src + y * tdata->stride;
                row_dst = tdata->dst + y * tdata->stride;
                for(int x = 0; x < tdata->width; ++x){
                    accum = 0;
                    for(decltype(tdata->kernel_h->size()) kernel_x = 0; kernel_x < tdata->kernel_h->size(); ++kernel_x){
                        image_x = x + kernel_x - kernel_radius_h;
                        if(image_x < 0 || image_x >= tdata->width)
                            continue;
                        accum += *(row_src + image_x) * tdata->kernel_h->at(kernel_x);
                    }
                    *row_dst++ = accum;
                }
            }
        }else if(tdata->format == CAIRO_FORMAT_ARGB32 || tdata->format == CAIRO_FORMAT_RGB24){
            float* row_src, *row_dst;
            __m128 accum;
            int image_x;
            for(int y = tdata->first_row; y < tdata->height; y += tdata->row_step){
                row_src = tdata->src + y * tdata->stride;
                row_dst = tdata->dst + y * tdata->stride;
                for(int x = 0; x < tdata->width; ++x){
                    accum = _mm_xor_ps(accum, accum);   // Set to zero
                    for(decltype(tdata->kernel_h->size()) kernel_x = 0; kernel_x < tdata->kernel_h->size(); ++kernel_x){
                        image_x = x + kernel_x - kernel_radius_h;
                        if(image_x < 0 || image_x >= tdata->width)
                            continue;
                        accum = _mm_add_ps(
                            accum,
                            _mm_mul_ps(
                                _mm_load_ps(row_src + (image_x << 2)),
                                _mm_set_ps1(tdata->kernel_h->at(kernel_x))
                            )
                        );
                    }
                    _mm_store_ps(row_dst, accum);
                    row_dst += 4;
                }
            }
        }
    THREAD_FUNC_END
    THREAD_FUNC_BEGIN(blur_v_filter)
        blur_v_thread_data* tdata = reinterpret_cast<blur_v_thread_data*>(userdata);
        int kernel_radius_v = (tdata->kernel_v->size() - 1) >> 1;
        if(tdata->format == CAIRO_FORMAT_A8){
            unsigned char* row_dst;
            float accum;
            int image_y;
            for(int y = tdata->first_row; y < tdata->height; y += tdata->row_step){
                row_dst = tdata->dst + y * tdata->stride;
                for(int x = 0; x < tdata->width; ++x){
                    accum = 0;
                    for(decltype(tdata->kernel_v->size()) kernel_y = 0; kernel_y < tdata->kernel_v->size(); ++kernel_y){
                        image_y = y + kernel_y - kernel_radius_v;
                        if(image_y < 0 || image_y >= tdata->height)
                            continue;
                        accum += *(tdata->src + image_y * tdata->stride + x) * tdata->kernel_v->at(kernel_y);
                    }
                    *row_dst++ = accum > 255.0f ? 255 : accum;
                }
            }
        }else if(tdata->format == CAIRO_FORMAT_ARGB32 || tdata->format == CAIRO_FORMAT_RGB24){
            unsigned char* row_dst;
            __m128 accum;
            int image_y;
            float __attribute__((aligned(16))) accum_buf[4];
            for(int y = tdata->first_row; y < tdata->height; y += tdata->row_step){
                row_dst = tdata->dst + y * tdata->stride;
                for(int x = 0; x < tdata->width; ++x){
                    accum = _mm_xor_ps(accum, accum);   // Set to zero
                    for(decltype(tdata->kernel_v->size()) kernel_y = 0; kernel_y < tdata->kernel_v->size(); ++kernel_y){
                        image_y = y + kernel_y - kernel_radius_v;
                        if(image_y < 0 || image_y >= tdata->height)
                            continue;
                        accum = _mm_add_ps(
                            accum,
                            _mm_mul_ps(
                                _mm_load_ps(tdata->src + image_y * tdata->stride + (x << 2)),
                                _mm_set_ps1(tdata->kernel_v->at(kernel_y))
                            )
                        );
                    }
                    _mm_store_ps(
                        accum_buf,
                        _mm_min_ps(
                            _mm_set_ps1(255.0f),
                            accum
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
    THREAD_FUNC_END
}
void cairo_image_surface_blur(cairo_surface_t* surface, float blur_h, float blur_v){
    // Valid blur range?
    if(blur_h >= 0 && blur_v >= 0 && (blur_h > 0 || blur_v > 0)){
        // Get surface data
        int width = cairo_image_surface_get_width(surface);
        int height = cairo_image_surface_get_height(surface);
        cairo_format_t format = cairo_image_surface_get_format(surface);
        int stride = cairo_image_surface_get_stride(surface);
        cairo_surface_flush(surface);   // Flush pending operations on surface
        unsigned char* data = cairo_image_surface_get_data(surface);
        // Create blur kernels
        int kernel_radius_h = ceil(blur_h),
            kernel_radius_v = ceil(blur_v);
        std::vector<float> kernel_h((kernel_radius_h << 1) + 1),
                            kernel_v((kernel_radius_v << 1) + 1);
        // Fill blur kernels
        float dec_h = 1.0f / (kernel_radius_h + 1), dec_v = 1.0f / (kernel_radius_v + 1);
        for(int i = 0; i < static_cast<int>(kernel_h.size()); ++i)
            kernel_h[i] = 1.0f - std::abs(i - kernel_radius_h) * dec_h;
        for(int i = 0; i < static_cast<int>(kernel_v.size()); ++i)
            kernel_v[i] = 1.0f - std::abs(i - kernel_radius_v) * dec_v;
        // Smooth kernels edges
        kernel_h.front() = kernel_h.back() *= 1.0f - (kernel_radius_h - blur_h);
        kernel_v.front() = kernel_v.back() *= 1.0f - (kernel_radius_v - blur_v);
        // Normalize kernels
        float sum_h = std::accumulate(kernel_h.begin(), kernel_h.end(), 0.0f),
                sum_v = std::accumulate(kernel_v.begin(), kernel_v.end(), 0.0f);
        std::for_each(kernel_h.begin(), kernel_h.end(), [&sum_h](float& v){v /= sum_h;});
        std::for_each(kernel_v.begin(), kernel_v.end(), [&sum_v](float& v){v /= sum_v;});
        // Create surface data in aligned float format for vector operations
        aligned_memory<float,16> fdata(height * stride),
                                fdata2(fdata.size());
        std::copy(data, data + fdata.size(), fdata.begin());
        // Get logical processors number
        int max_threads = nthread_get_processors_num();
        // Create thread data
        std::vector<blur_h_thread_data> tdata_h(max_threads);
        std::vector<blur_v_thread_data> tdata_v(max_threads);
        for(int i = 0; i < max_threads; ++i){
            tdata_h[i] = {&kernel_h, width, height, stride, i, max_threads, format, fdata, fdata2};
            tdata_v[i] = {&kernel_v, width, height, stride, i, max_threads, format, fdata2, data};
        }
        std::vector<nthread_t> threads(max_threads-1);
        // Run threads for horizontal blur
        for(int i = 0; i < max_threads-1; ++i)
            threads[i] = nthread_create(blur_h_filter, &tdata_h[i]);
        blur_h_filter(&tdata_h[max_threads-1]);
        for(nthread_t& thread : threads){
            // Wait for threads & close them
            nthread_join(thread);
            nthread_destroy(thread);
        }
        // Run threads for vertical blur
        for(int i = 0; i < max_threads-1; ++i)
            threads[i] = nthread_create(blur_v_filter, &tdata_v[i]);
        blur_v_filter(&tdata_v[max_threads-1]);
        for(nthread_t& thread : threads){
            // Wait for threads & close them
            nthread_join(thread);
            nthread_destroy(thread);
        }
        // Signal changes on surfaces
        cairo_surface_mark_dirty(surface);
    }
}

void cairo_apply_matrix(cairo_t* ctx, cairo_matrix_t* mat){
    cairo_path_t* path = cairo_copy_path(ctx);
    cairo_new_path(ctx);
    cairo_save(ctx);
    cairo_transform(ctx, mat);
    cairo_append_path(ctx, path);
    cairo_restore(ctx);
    cairo_path_destroy(path);
}

void cairo_copy_matrix(cairo_t* src, cairo_t* dst){
    cairo_matrix_t matrix;
    cairo_get_matrix(src, &matrix);
    cairo_set_matrix(dst, &matrix);
}
