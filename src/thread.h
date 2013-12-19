/*
Project: SSBRenderer
File: thread.h

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

typedef HANDLE nthread_t;
#define THREAD_FUNC_BEGIN(name) unsigned int WINAPI __attribute__((force_align_arg_pointer)) name(void* userdata){
#define THREAD_FUNC_END return 0;}

inline nthread_t nthread_create(unsigned (WINAPI *thread_func)(void*), void* userdata){
    return reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, thread_func, userdata, 0x0, NULL));
}
inline void nthread_join(nthread_t t){
    WaitForSingleObject(t, INFINITE);
}
inline void nthread_destroy(nthread_t t){
    CloseHandle(t);
}
inline unsigned nthread_get_processors_num(){
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors;
}

#else
#include <pthread.h>

typedef pthread_t nthread_t;
#define THREAD_FUNC_BEGIN(name) void* __attribute__((force_align_arg_pointer)) name(void* userdata){
#define THREAD_FUNC_END return NULL;}

inline nthread_t nthread_create(void* (*thread_func)(void*), void* userdata){
    pthread_t t;
    pthread_create(&t, NULL, thread_func, &userdata);
    return t;
}
inline void nthread_join(nthread_t t){
    pthread_join(t, NULL);
}
inline void nthread_destroy(nthread_t t){
    // POSIX threads need no destroy
}
inline unsigned nthread_get_processors_num(){
    return pthread_num_processors_np();
}

#endif
