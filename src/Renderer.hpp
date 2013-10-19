#pragma once

#include "SSBData.hpp"

class Renderer{
    private:
        // Frame data
        int width, height;
        bool has_alpha, aligned;
        // SSB data
        SSBData ssb;
    public:
        // SSB parsing & frame meta informations saving
        Renderer(int width, int height, bool has_alpha, bool aligned, std::string& script, bool warnings);
        // Render SSB contents on frame
        void render(unsigned char* image, int pitch, signed long long start_ms, signed long long end_ms);
};
