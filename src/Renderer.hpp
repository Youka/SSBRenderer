/*
Project: SSBRenderer
File: Renderer.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "SSBData.hpp"
#include "cairo++.hpp"

class Renderer{
    public:
        // Supported colorspaces
        enum class Colorspace : char{BGR, BGRX, BGRA};
    private:
        // Frame data
        int width, height;
        Colorspace format;
        // SSB data
        SSBData ssb;
        // Path buffer
        CairoImage stencil_path_buffer;
        // Event images cache
        struct ImageData{
            CairoImage image;
            int x, y;
            SSBBlend::Mode blend_mode;
        };
        Cache<SSBEvent*,std::vector<ImageData>> cache;
        // Blend image on frame
        void blend(cairo_surface_t* src, int dst_x, int dst_y,
                   unsigned char* dst_data, int dst_stride,
                   SSBBlend::Mode blend_mode);
    public:
        // Frame meta informations saving + SSB parsing + path buffer creation
        Renderer(int width, int height, Colorspace format, std::string& script, bool warnings);
        // Change frame meta informations
        void set_target(int width, int height, Colorspace format);
        // Render SSB contents on frame
        void render(unsigned char* frame, int pitch, unsigned long int start_ms) noexcept;
};
