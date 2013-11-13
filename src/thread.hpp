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

template<typename T>
class Thread{
    private:
        std::function<void(T)> proc;
        T userdata;
#ifdef _WIN32
        HANDLE handle;
        static DWORD WINAPI run(LPVOID instance){
            Thread<T>* thread = reinterpret_cast<Thread<T>*>(instance);
            thread->proc(thread->userdata);
            return 0;
        }
#else
#error "Not implented"
#endif
    public:
#ifdef _WIN32
        // Ctor & dtor
        Thread(std::function<void(T)> proc, T userdata) : proc(proc), userdata(userdata), handle(CreateThread(NULL, 0, run, this, 0, NULL)){}
        ~Thread(){
            if(this->handle)
                CloseHandle(this->handle);
        }
        // No copy
        Thread(const Thread& other) = delete;
        Thread& operator=(const Thread& other) = delete;
        // Wait for thread completion
        void join(){
            if(this->handle)
                WaitForSingleObject(this->handle, INFINITE);
        }
        // Get logical processors
        static unsigned int get_logical_processors(){
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            return si.dwNumberOfProcessors;
        }
#else
#error "Not implented"
#endif
};
