/*
Project: SSBRenderer
File: thread.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#error "Not implented"
#endif

class Thread{
    private:
        std::function<void()> proc;
        unsigned int* reference_counter;
#ifdef _WIN32
        HANDLE handle;
        static DWORD WINAPI run(LPVOID instance){
            reinterpret_cast<Thread*>(instance)->proc();
            return 0;
        }
#else
#error "Not implented"
#endif
    public:
#ifdef _WIN32
        // Ctor & dtor
        Thread(std::function<void()> proc) : proc(proc), reference_counter(new unsigned int), handle(CreateThread(NULL, 0, run, this, 0, NULL)){
            *this->reference_counter = 1;
        }
        ~Thread(){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                CloseHandle(this->handle);
            }
        }
        // Copy
        Thread(Thread& other){
            this->proc = other.proc;
            this->reference_counter = other.reference_counter;
            *this->reference_counter += 1;
            this->handle = other.handle;
        }
        Thread& operator=(Thread& other){
            if(--*this->reference_counter == 0){
                delete this->reference_counter;
                CloseHandle(this->handle);
            }
            this->proc = other.proc;
            this->reference_counter = other.reference_counter;
            *this->reference_counter += 1;
            this->handle = other.handle;
            return *this;
        }
        // Wait for completion
        void join(){
            WaitForSingleObject(this->handle, INFINITE);
        }
#else
#error "Not implented"
#endif
};
