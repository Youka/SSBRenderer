#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <vd2/vdvideofilt.h>
#pragma GCC diagnostic pop
#include "file_info.hpp"
#include "Renderer.hpp"
#include <cstdio>   // _snprintf for description filling
#include "virtualdub_dialog.hpp"
#include <windows.h>    // Dialog presentation
#include "module.hpp" // Module needed for windows

namespace VDub{
    // Filter instance data
    struct Userdata{
        Renderer *renderer;
        std::string *script;
        bool warnings;
    };
    // Filter initialization
    int initProc(VDXFilterActivation* fdata, const VDXFilterFunctions*){
        Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
        inst_data->renderer = nullptr;
        inst_data->script = new std::string;
        inst_data->warnings = true;
        // Success
        return 0;
    }
    // Filter deinitialization
    void deinitProc(VDXFilterActivation* fdata, const VDXFilterFunctions*){
        Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
        delete inst_data->renderer; inst_data->renderer = nullptr;
        delete inst_data->script; inst_data->script = nullptr;
    }
    // Filter run/frame processing
    int runProc(const VDXFilterActivation* fdata, const VDXFilterFunctions*){
        Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
        inst_data->renderer->Render(reinterpret_cast<unsigned char*>(fdata->src.data), fdata->src.pitch, fdata->src.mFrameTimestampStart / 10, fdata->src.mFrameTimestampEnd / 10);
        // Success
        return 0;
    }
    // Filter video format
    long paramProc(VDXFilterActivation*, const VDXFilterFunctions*){
        // Use default format (RGB, bottom-up)
        return 0;
    }
    // Filter configuration
    INT_PTR CALLBACK config_message_handler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam){
        // Evaluate message
        switch(msg){
            #pragma message "Implent config message handling"
            // Closed dialog with 'X' button
            case WM_CLOSE:
                // Unsuccessful end
                EndDialog(wnd, 1);
                break;
            // Message not handled (continue with default behaviour)
            default:
                return FALSE;
        }
        // Message handled
        return TRUE;
    }
    int configProc(VDXFilterActivation* fdata, const VDXFilterFunctions*, VDXHWND wnd){
        // Show modal dialog for filter configuration
        DialogBoxParamW(reinterpret_cast<HINSTANCE>(module), MAKEINTRESOURCEW(VDUB_DIALOG), reinterpret_cast<HWND>(wnd), config_message_handler, reinterpret_cast<LPARAM>(fdata->filter_data));
        char err_msg[128];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, err_msg, sizeof(err_msg), NULL);
        MessageBoxA(NULL, err_msg, "TEST", MB_OK);
        return 0;
    }
    // Filter description
    void fill_description(Userdata* inst_data, char* buf, int maxlen = 128){
        // Fill description buffer with script and warnings information
        _snprintf(buf, maxlen, " Script:\"%s\" - Warnings:%s", inst_data->script->c_str(), inst_data->warnings ? "On" : "Off");
    }
    void stringProc(const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf){
        fill_description(reinterpret_cast<Userdata*>(fdata->filter_data), buf);
    }
    void stringProc2(const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf, int maxlen){
        fill_description(reinterpret_cast<Userdata*>(fdata->filter_data), buf, maxlen);
    }
    // Filter start running
    int startProc(VDXFilterActivation* fdata, const VDXFilterFunctions* ffuncs){
        Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
        // All video informations available?
        if(fdata->pfsi == nullptr)
            ffuncs->Except("Video informations are missing!");
        // Free previous renderer (in case of buggy twice start)
        delete inst_data->renderer; inst_data->renderer = nullptr;
        // Allocate renderer
        try{
            inst_data->renderer = new Renderer(fdata->src.w, fdata->src.h, false, static_cast<double>(fdata->src.mFrameRateHi) / fdata->src.mFrameRateLo, *inst_data->script, inst_data->warnings);
        }catch(std::string err){
            ffuncs->Except(err.c_str());
            return 1;
        }
        // Success
        return 0;
    }
    // Filter end running
    int endProc(VDXFilterActivation* fdata, const VDXFilterFunctions*){
        Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
        delete inst_data->renderer; inst_data->renderer = nullptr;
        // Success
        return 0;
    }
    // Filter definition
    VDXFilterDefinition filter_definition = {
        nullptr,	// _next
        nullptr,	// _prev
        nullptr,	// _module

        FILTER_NAME,	// name
        FILTER_DESCRIPTION,	// desc
        FILTER_AUTHOR,	// maker
        nullptr,	// private data
        sizeof(Userdata),	// inst_data_size

        initProc,	// initProc
        deinitProc,	// deinitProc
        runProc,	// runProc
        paramProc,	// paramProc
        configProc,	// configProc
        stringProc,	// stringProc
        startProc,	// startProc
        endProc,	// endProc

        nullptr,	// script_obj
        nullptr,	// fssProc

        stringProc2,	// stringProc2
        nullptr,	// serializeProc
        nullptr,	// deserializeProc
        nullptr,	// copyProc

        nullptr,	// prefetchProc

        nullptr,	// copyProc2
        nullptr,	// prefetchProc2
        nullptr     // eventProc
    };
    // VirtualDub version
    int version;
}

// VirtualDub plugin interface - register
VDXFilterDefinition *registered_filter_definition;
extern "C" __declspec(dllexport) int VirtualdubFilterModuleInit2(struct VDXFilterModule* fmodule, const VDXFilterFunctions* ffuncs, int& vdfd_ver, int& vdfd_compat){
	// Create register definition
	registered_filter_definition = ffuncs->addFilter(fmodule, &VDub::filter_definition, sizeof(VDXFilterDefinition));
	// Version & compatibility definition
	VDub::version = vdfd_ver;
	vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE_COPYCTOR;
	// Success
	return 0;
}

// VirtualDub plugin interface - unregister
extern "C" __declspec(dllexport) void VirtualdubFilterModuleDeinit(struct VDXFilterModule*, const VDXFilterFunctions *ffuncs){
	// Remove register definition
	ffuncs->removeFilter(registered_filter_definition);
}
