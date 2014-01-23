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
#include <cstdlib>

/*static bool sse2_supported(){
    // Storage for cpu feature bits
    static uint32_t cpu_features = 0x0;
    if(cpu_features == 0x0){
        asm(
            // Get cpu features
            "movl $1, %%eax\n"
            "cpuid\n"
            "movl %%edx, %0\n"
            : "=r" (cpu_features)
            : // No input
            : "%eax", "%ebx", "%ecx", "%edx"
        );
    }
    // 26th bit marks SSE2 support
    return cpu_features & 0x04000000;
}*/

template<typename T, size_t align>
class aligned_memory{
    private:
        // "Copy" counter
        unsigned int* reference_counter;
        // Memory storage
        void* p;
        T* aligned_p;
        // Memory size
        size_t msize;
    public:
        // Ctor / allocate
        aligned_memory(size_t size) : reference_counter(new unsigned int){
            *this->reference_counter = 1;
            if(size == 0 || align == 0){
                this->p = this->aligned_p = nullptr;
                this->msize = 0;
            }else{
                this->p = malloc(sizeof(T) * size + align);
                this->aligned_p = this->p ? reinterpret_cast<T*>(reinterpret_cast<size_t>(this->p) + (align - reinterpret_cast<size_t>(this->p) % align)) : nullptr;
                this->msize = this->aligned_p ? size : 0;
            }
        }
        // Dtor / deallocate
        ~aligned_memory(){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                if(this->p) free(this->p);
            }
        }
        // Copy
        aligned_memory(aligned_memory& other) : reference_counter(other.reference_counter), p(other.p), aligned_p(other.aligned_p), msize(other.msize){
            *this->reference_counter += 1;
        }
        aligned_memory& operator=(aligned_memory& other){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                if(this->p) free(this->p);
            }
            this->reference_counter = other.reference_counter;
            *this->reference_counter += 1;
            this->p = other.p;
            this->aligned_p = other.aligned_p;
            this->msize = other.msize;
            return *this;
        }
        // Data access
        T& operator [](const int i){return aligned_p[i];}
        operator T*() const{return this->aligned_p;}
        size_t size() const{return this->msize;}
        T* begin() const{return this->aligned_p;}
        T* end() const{return this->aligned_p + this->msize;}
};
