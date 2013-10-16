#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <avisynth_c.h>
#include "file_info.hpp"
#include <string>

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
        // Data access
        AVS_FilterInfo* info(){return this->filter_info;}
        operator AVS_Clip*(){return this->clip;}
};

// Frame filtering
AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo* filter_info, int n){
    // Get current frame
    AVS_VideoFrame* frame = avs_get_frame(filter_info->child, n);
    // Make frame writable
    avs_make_writable(filter_info->env, &frame);
    // Get frame data
    int pitch = avs_get_pitch(frame);
    BYTE* data = avs_get_write_ptr(frame);
    #pragma message "Filter frame with SSB instance"
    // Pass frame further in processing chain
    return frame;
}

// Filter call
AVS_Value AVSC_CC apply_filter(AVS_ScriptEnvironment* env, AVS_Value args, void*){
    // Get filter arguments
    AVSClip clip(env, avs_array_elt(args, 0));
    std::string script = avs_as_string(avs_array_elt(args, 1));
    bool warnings = avs_defined(avs_array_elt(args, 2)) ? avs_as_bool(avs_array_elt(args, 2)) : true;
    // Check filter arguments
    const AVS_VideoInfo* video_info = avs_get_video_info(clip);
    if(!avs_has_video(video_info))
        return avs_new_value_error("Video required!");    // Clip must have a video stream
    else if(!avs_is_rgb(video_info))
        return avs_new_value_error("Video colorspace must be RGB!");    // Video must store colors in RGB24 or RGBA32 format
    else if(script.empty())
        return avs_new_value_error("Script name required!");    // Empty script name not acceptable
    else{
        #pragma message "Create & destroy SSB instance"
        clip.info()->get_frame = get_frame; // Set callback function for frame processing
        return avs_new_value_clip(clip);    // Return filtered clip
    }
}

// Avisynth plugin interface
AVSC_EXPORT const char* avisynth_c_plugin_init(AVS_ScriptEnvironment* env){
    // Valid Avisynth interface version?
    avs_check_version(env, AVISYNTH_INTERFACE_VERSION);
    // Register functin to Avisynth scripting environment
    avs_add_function(env, SSB_NAME, "cs[warnings]b", apply_filter, nullptr);
    // Return plugin description
    return SSB_DESCRIPTION;
}
