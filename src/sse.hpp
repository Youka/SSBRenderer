/*
Project: SSBRenderer
File: sse.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <xmmintrin.h>

inline bool sse_supported(){
    // Storage for cpu feature bits
    uint32_t cpu_features;
    asm(
        // Save registers
        "push %%eax\n"
        "push %%ebx\n"
        "push %%ecx\n"
        "push %%edx\n"
        // Get cpu features
        "movl $1, %%eax\n"
        "cpuid\n"
        "movl %%edx, %0\n"
        // Restore registers
        "push %%edx\n"
        "push %%ecx\n"
        "push %%ebx\n"
        "push %%eax"
        : "=r" (cpu_features)
    );
    // 25th bit marks SSE support
    return cpu_features & 0x02000000;
}

template<typename T, size_t align>
class aligned_memory{
    private:
        // Memory storage
        T* p;
        // "Copy" counter
        unsigned int* reference_counter;
    public:
        // Ctor / allocate
        aligned_memory(size_t size) : p(reinterpret_cast<T*>(_mm_malloc(sizeof(T) * size, align))), reference_counter(new unsigned int){
            *this->reference_counter = 1;
        }
        // Dtor / deallocate
        ~aligned_memory(){
            if(--*this->reference_counter == 0){
                _mm_free(this->p);
                delete this->reference_counter;
            }
        }
        // Copy
        aligned_memory(aligned_memory& other) : p(other.p), reference_counter(other.reference_counter){
            *this->reference_counter += 1;
        }
        aligned_memory& operator=(aligned_memory& other){
            if(--*this->reference_counter == 0){
                _mm_free(this->p);
                delete this->reference_counter;
            }
            this->p = other.p;
            this->reference_counter = other.reference_counter;
            this->reference_counter += 1;
            return *this;
        }
        // Data access
        operator T*(){return this->p;}
        T& operator [](const int i){return p[i];}
};
