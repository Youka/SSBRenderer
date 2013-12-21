/*
Project: SSBRenderer
File: file_info.h

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#define FILTER_VERSION_NUMBER 0,0,1,0
#define FILTER_VERSION_STRING "0.0.1b"
#define FILTER_DESCRIPTION "Renderer for Substation Beta subtitle format"
#define FILTER_AUTHOR "Youka"
#define FILTER_NAME "SSBRenderer"
#define FILTER_COPYRIGHT FILTER_AUTHOR ",© 2013"  // Author + copyright year
#ifdef _WIN32
#define FILTER_FILENAME FILTER_NAME ".dll"
#else
#define FILTER_FILENAME FILTER_NAME ".so"
#endif
