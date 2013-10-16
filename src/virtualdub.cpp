#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <vd2/vdvideofilt.h>
#pragma GCC diagnostic pop
#include "file_info.hpp"

namespace VDub{
    #pragma message "Fill filter definition"
    // Filter definition
    VDXFilterDefinition filter_definition = {
        nullptr,	// _next
        nullptr,	// _prev
        nullptr,	// _module

        SSB_NAME,	// name
        SSB_DESCRIPTION,	// desc
        SSB_AUTHOR,	// maker
        nullptr,	// private data
        0,	// inst_data_size

        nullptr,	// initProc
        nullptr,	// deinitProc
        nullptr,	// runProc
        nullptr,	// paramProc
        nullptr,	// configProc
        nullptr,	// stringProc
        nullptr,	// startProc
        nullptr,	// endProc

        nullptr,	// script_obj
        nullptr,	// fssProc

        nullptr,	// stringProc2
        nullptr,	// serializeProc
        nullptr,	// deserializeProc
        nullptr,	// copyProc

        nullptr,	// prefetchProc

        nullptr,	// copyProc2
        nullptr,	// prefetchProc2
        nullptr	// eventProc
    };
    // VirtualDub version
    int version;
}

// VirtualDub plugin interface - register
VDXFilterDefinition *registered_filter_definition;
extern "C" __declspec(dllexport) int __cdecl VirtualdubFilterModuleInit2(struct VDXFilterModule* fmodule, const VDXFilterFunctions* ffuncs, int& vdfd_ver, int& vdfd_compat){
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
extern "C" __declspec(dllexport) void __cdecl VirtualdubFilterModuleDeinit(struct VDXFilterModule*, const VDXFilterFunctions *ffuncs){
	// Remove register definition
	ffuncs->removeFilter(registered_filter_definition);
}
