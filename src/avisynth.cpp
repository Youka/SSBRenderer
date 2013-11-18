/*
Project: SSBRenderer
File: avisynth.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <avisynth_c.h>
#pragma GCC diagnostic pop
#include "file_info.h"
#include "Renderer.hpp"

namespace AVS{
    // Memory-safe wrapper for AVS_Clip
    class AVSClip{
        private:
            // Data
            AVS_FilterInfo* filter_info;
            AVS_Clip* clip;
        public:
            // Allocate & free resources
            AVSClip(AVS_ScriptEnvironment* env, AVS_Value val) : clip(avs_new_c_filter(env, &this->filter_info, val, 1)){}
            ~AVSClip(){avs_release_clip(this->clip);}
            // No copy
            AVSClip(const AVSClip&) = delete;
            AVSClip& operator=(const AVSClip&) = delete;
            // Data access
            AVS_FilterInfo* info() const {return this->filter_info;}
            operator AVS_Clip*() const {return this->clip;}
    };

    // Frame filtering
    AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo* filter_info, int n){
        // Get current frame
        AVS_VideoFrame* frame = avs_get_frame(filter_info->child, n);
        // Make frame writable
        avs_make_writable(filter_info->env, &frame);
        // Render on frame
        reinterpret_cast<Renderer*>(filter_info->user_data)->render(avs_get_write_ptr(frame), avs_get_pitch(frame), n * (filter_info->vi.fps_denominator * 1000.0 / filter_info->vi.fps_numerator));
        // Pass frame further in processing chain
        return frame;
    }

    // Filter finished
    void AVSC_CC free_filter(AVS_FilterInfo* filter_info){
        // Free renderer
        delete reinterpret_cast<Renderer*>(filter_info->user_data); filter_info->user_data = nullptr;
    }

    // Filter call
    AVS_Value AVSC_CC apply_filter(AVS_ScriptEnvironment* env, AVS_Value args, void*){
        // Get filter arguments
        AVSClip clip(env, avs_array_elt(args, 0));
        std::string script = avs_as_string(avs_array_elt(args, 1));
        bool warnings = avs_defined(avs_array_elt(args, 2)) ? avs_as_bool(avs_array_elt(args, 2)) : true;
        // Check filter arguments
        const AVS_VideoInfo* video_info = avs_get_video_info(clip);
        if(!avs_has_video(video_info))  // Clip must have a video stream
            return avs_new_value_error("Video required!");
        else if(!avs_is_rgb(video_info))    // Video must store colors in RGB24 or RGBA32 format
            return avs_new_value_error("Video colorspace must be RGB!");
        else if(script.empty()) // Empty script name not acceptable
            return avs_new_value_error("Script name required!");
        else{
            AVS_FilterInfo* filter_info = clip.info();
            // Allocate renderer
            try{
                filter_info->user_data = new Renderer(video_info->width, video_info->height, avs_is_rgb32(video_info) ? Renderer::Colorspace::BGRA : Renderer::Colorspace::BGR, script, warnings);
            }catch(std::string err){
                return avs_new_value_error(err.c_str());
            }
            // Set free function for renderer
            filter_info->free_filter = free_filter;
            // Set callback function for frame processing
            filter_info->get_frame = get_frame;
            // Return filtered clip
            return avs_new_value_clip(clip);
        }
    }
}

// Avisynth plugin interface
AVSC_EXPORT const char* avisynth_c_plugin_init(AVS_ScriptEnvironment* env){
    // Valid Avisynth interface version?
    avs_check_version(env, AVISYNTH_INTERFACE_VERSION);
    // Register functin to Avisynth scripting environment
    avs_add_function(env, FILTER_NAME, "cs[warnings]b", AVS::apply_filter, nullptr);
    // Return plugin description
    return FILTER_DESCRIPTION;
}
