#pragma once

#include "SSBData.hpp"

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
    public:
        // SSB parsing & frame meta informations saving
        Renderer(int width, int height, Colorspace format, std::string& script, bool warnings);
        // Change frame meta informations
        void set_target(int width, int height, Colorspace format);
        // Render SSB contents on frame
        void render(unsigned char* image, int pitch, unsigned long long start_ms, unsigned long long end_ms) noexcept;
};
