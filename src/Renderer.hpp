#pragma once

#include <string>

class Renderer{
    private:
        // Video data
        int width, height, frames;
        bool has_alpha;
        double fps;
        // SSB data
        std::string script;
        bool warnings;
    public:
        // SSB parsing & video meta informations saving
        Renderer(int width, int height, bool has_alpha, double fps, int frames, std::string script, bool warnings);
        // Render SSB contents on frame
        void Render(unsigned char* image, int pitch, int frame_index);
};
