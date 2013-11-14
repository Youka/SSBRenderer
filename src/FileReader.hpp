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
#include "textconv.hpp"
#include <algorithm>
#else
#include <fstream>
#endif

class FileReader{
    private:
#ifdef _WIN32
        // File handle
        HANDLE file;
        // Read buffer
        char buffer[4096];
        char* buffer_start, *buffer_end = buffer_start;
#else
        std::ifstream file;
#endif
    public:
#ifdef _WIN32
        // Constructors
        FileReader(std::string& filename)
        : file(CreateFileW(utf8_to_utf16(filename).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){}
        FileReader(std::wstring& filename)
        : file(CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){}
        // No copy
        FileReader(const FileReader& other) = delete;
        FileReader& operator =(const FileReader& other) = delete;
        // Destructor
        ~FileReader(){
            if(this->file != INVALID_HANDLE_VALUE)
                CloseHandle(this->file);
        }
        // Check file state
        operator bool() const{
            return this->file != INVALID_HANDLE_VALUE;
        }
        // Reset file pointer
        void reset(){
            if(this->file != INVALID_HANDLE_VALUE){
                SetFilePointer(this->file, 0, 0, FILE_BEGIN);
                this->buffer_start = this->buffer_end;
            }
        }
        // Read bytes from file
        unsigned long read(unsigned long nbytes, unsigned char* bytes){
            if(this->file != INVALID_HANDLE_VALUE){
                // Clear buffer (bytes back to stream)
                if(this->buffer_end != buffer_start){
                    SetFilePointer(this->file, -static_cast<int>(buffer_end-buffer_start), 0, FILE_CURRENT);
                    this->buffer_start = this->buffer_end;
                }
                // Read bytes
                DWORD readbytes;
                if(!ReadFile(this->file, bytes, nbytes, &readbytes, NULL))
                    return 0;
                else
                    return readbytes;
            }
            return 0;
        }
        // Read one line from file
        bool getline(std::string& line){
            if(this->file != INVALID_HANDLE_VALUE){
                // Clear old line content
                line.clear();
                // Fill line content
                while(true){
                    // Check for buffer content
                    if(this->buffer_start == this->buffer_end){
                        // Fill buffer
                        DWORD read;
                        if(!ReadFile(this->file, this->buffer, sizeof(this->buffer), &read, NULL) || read == 0)
                            // Error or EOF
                            break;
                        this->buffer_start = this->buffer;
                        this->buffer_end = this->buffer + read;
                    }
                    // Search newline
                    char* newline = std::find(this->buffer_start, this->buffer_end, '\n');
                    // Assign content to line
                    if(newline == this->buffer_end){
                        line.append(this->buffer_start, this->buffer_end);
                        this->buffer_start = this->buffer_end;
                    }else{
                        line.append(this->buffer_start, newline);
                        this->buffer_start = newline + 1;
                        return true;
                    }
                }
                // Read anything?
                return !line.empty();
            }
            // File handle invalid
            return false;
        }
#else
        // Constructors
        FileReader() = default;
        FileReader(std::string& filename) : file(filename){}
        FileReader(const FileReader&) = default;
        // Assign operator
        FileReader& operator =(const FileReader&) = default;
        // Destructor
        ~FileReader() = default;
        // Check file state
        operator bool() const{
            return this->file;
        }
        // Reset file pointer
        void reset(){
            this->file.seekg(0);
        }
        // Read bytes from file
        unsigned long read(unsigned long nbytes, unsigned char* bytes){
            this->file.read(bytes, nbytes);
            return this->file.gcount();
        }
        // Read one line from file
        bool getline(std::string& line){
            return std::getline(this->file, line);
        }
#endif
};
