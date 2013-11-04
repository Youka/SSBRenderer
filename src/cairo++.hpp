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
        CairoImage& operator=(const CairoImage& image){
            // Free old content
            if(this->context){
                cairo_destroy(this->context); this->context = nullptr;
            }
            cairo_surface_destroy(this->surface);
            // Assign new content
            if(image.context)
                this->context = cairo_reference(image.context);
            this->surface = cairo_surface_reference(image.surface);
            return *this;
        }
        CairoImage(const CairoImage& image){
            *this = image;
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
