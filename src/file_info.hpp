#pragma once

#define FILTER_VERSION_NUMBER 0,0,1,0
#define FILTER_VERSION_STRING "0.0.1"
#define FILTER_DESCRIPTION "Renderer for Substation Beta subtitle format"
#define FILTER_AUTHOR "Youka"
#ifdef DEBUG
#   define FILTER_NAME "SSBRenderer_debug"
#else
#   define FILTER_NAME "SSBRenderer"
#endif
#define FILTER_COPYRIGHT FILTER_AUTHOR ",© 2013"  // Author + copyright year
#define FILTER_FILENAME FILTER_NAME ".dll"    // Program title + dynamic library extension
