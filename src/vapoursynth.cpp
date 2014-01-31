/*
Project: SSBRenderer
File: vapoursynth.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "VapourSynth.h"
#include "file_info.h"
#include "Renderer.hpp"

namespace VS{
    // Memory-safe wrapper for VS node reference
    class VSNode2{
        private:
            // Data
            VSNodeRef* clip;
            const VSAPI* vsapi;
        public:
            // Allocate & free resources
            VSNode2(VSNodeRef* clip, const VSAPI* vsapi) : clip(clip), vsapi(vsapi){}
            ~VSNode2(){this->vsapi->freeNode(this->clip);}
            // No copy
            VSNode2(const VSNode2&) = delete;
            VSNode2& operator=(const VSNode2&) = delete;
            // Data access
            const VSVideoInfo* info() const {return this->vsapi->getVideoInfo(this->clip);}
            operator VSNodeRef*() const {return this->clip;}
    };

    // Filter data to pass through filter callbacks
    struct FilterData{
        VSNode2 clip;
        Renderer* renderer;
    };

    // Filter initialization / set output video infomation(s)
    void VS_CC init_filter(VSMap*, VSMap*, void** inst_data, VSNode* node, VSCore*, const VSAPI* vsapi){
        vsapi->setVideoInfo(reinterpret_cast<FilterData*>(*inst_data)->clip.info(), 1, node);
    }

    // Frame processing
    const VSFrameRef* VS_CC get_frame(int n, int activationReason, void** inst_data, void** frameData, VSFrameContext* frame_ctx, VSCore* core, const VSAPI* vsapi){
        FilterData* data = reinterpret_cast<FilterData*>(inst_data);
        // Frame creation
        if(activationReason == arInitial)
            // Request needed input frames
            vsapi->requestFrameFilter(n, data->clip, frame_ctx);
        // Frame processing
        else if (activationReason == arAllFramesReady){
            // Get frame data
            const VSFrameRef* frame = vsapi->getFrameFilter(n, data->clip, frame_ctx);

            // TODO

            return frame;
        }
        return 0;
    }

    // Filter destruction
    void VS_CC free_filter(void* inst_data, VSCore*, const VSAPI*){
        FilterData* data = reinterpret_cast<FilterData*>(inst_data);
        // Free data from filter creation
        delete data->renderer;
        delete data;
    }

    // Filter creation
    void VS_CC apply_filter(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* vsapi){
        // Get filter arguments
        VSNode2 clip(vsapi->propGetNode(in, "clip", 0, NULL), vsapi);
        std::string script = vsapi->propGetData(in, "script", 0, NULL);
        int err;
        bool warnings = vsapi->propGetInt(in, "warnings", 0, &err);
        if(err == peUnset)
            warnings = true;
        // Check filter arguments

        // Create new filter to Vapoursynth API
        //vsapi->createFilter(in, out, FILTER_NAME, init_filter, get_frame, free_filter, fmParallel, 0, new FilterData{,}, core);
    }
}

// Plugin initialization
VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin config_func, VSRegisterFunction reg_func, VSPlugin* plugin){
    // Write filter information to Vapoursynth configuration (identifier, namespace, description, vs version, is read-only, plugin storage)
    config_func("com.subtitle.ssb", "ssb", FILTER_DESCRIPTION, VAPOURSYNTH_API_VERSION, 1, plugin);
    // Register filter to Vapoursynth with configuration in plugin storage (filter name, arguments, filter creation function, userdata, plugin storage)
    reg_func(FILTER_NAME, "clip:clip;script:data;warnings:int:opt", VS::apply_filter, 0, plugin);
}
