/*
Project: SSBRenderer
File: FileReader.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#ifdef _WIN32
#include <string>
#include <windows.h>
#else
#include <fstream>
#endif

class FileReader{
    private:
        // Additonal directory for file searching
        static std::string dir;
#ifdef _WIN32
        // File handle
        HANDLE file;
        // Read buffer
        char buffer[4096];
        char* buffer_start = 0, *buffer_end = buffer_start;
#else
        std::ifstream file;
#endif
    public:
        // Sets an additonal search path
        static void set_additional_directory(std::string dir);
        // Constructors
        FileReader(std::string& filename);
#ifdef _WIN32
        FileReader(std::wstring& filename);
#endif
        // Copy
#ifdef _WIN32
        FileReader(const FileReader& other) = delete;
        FileReader& operator =(const FileReader& other) = delete;
#else
        FileReader(const FileReader&) = default;
        FileReader& operator =(const FileReader&) = default;
#endif
        // Destructor
#ifdef _WIN32
        ~FileReader();
#else
        ~FileReader() = default;
#endif
        // Check file state
        operator bool() const;
        // Reset file pointer
        void reset();
        // Read bytes from file
        unsigned long read(unsigned long nbytes, unsigned char* bytes);
        // Read one line from file
        bool getline(std::string& line);
};
