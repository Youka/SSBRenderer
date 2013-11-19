/*
Project: SSBRenderer
File: utf8.h

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string.h>

inline size_t utf8_clen(char* s, size_t pos){
    unsigned char c = s[pos];
    if(c < 192)
        return 1;
    else if(c < 224)
        return 2;
    else if(c < 240)
        return 3;
    else if(c < 248)
        return 4;
    else if(c < 252)
        return 5;
    else
        return 6;
}

inline size_t utf8_slen(char* s){
    size_t n = 0, pos = 0;
    size_t len = strlen(s);
    while(pos < len){
        pos += utf8_clen(s, pos);
        ++n;
    }
    if(pos != len) --n;
    return n;
}