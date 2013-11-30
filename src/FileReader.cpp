/*
Project: SSBRenderer
File: FileReader.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "FileReader.hpp"

std::string FileReader::dir;

void FileReader::set_additional_directory(std::string dir){
    FileReader::dir = dir;
}

#ifdef _WIN32

#include "textconv.hpp"
#include <algorithm>

FileReader::FileReader(std::string& filename)
: file(CreateFileW(utf8_to_utf16(filename).c_str(), FILE_READ_DATA|STANDARD_RIGHTS_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){
    if(this->file == INVALID_HANDLE_VALUE){
        std::string filenameex = FileReader::dir + filename;
        this->file = CreateFileW(utf8_to_utf16(filenameex).c_str(), FILE_READ_DATA|STANDARD_RIGHTS_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
}

FileReader::FileReader(std::wstring& filename)
: file(CreateFileW(filename.c_str(), FILE_READ_DATA|STANDARD_RIGHTS_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){
    if(this->file != INVALID_HANDLE_VALUE)
        this->file = CreateFileW((utf8_to_utf16(FileReader::dir) + filename).c_str(), FILE_READ_DATA|STANDARD_RIGHTS_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

FileReader::~FileReader(){
    if(this->file != INVALID_HANDLE_VALUE)
        CloseHandle(this->file);
}

FileReader::operator bool() const{
    return this->file != INVALID_HANDLE_VALUE;
}

void FileReader::reset(){
    if(this->file != INVALID_HANDLE_VALUE){
        SetFilePointer(this->file, 0, 0, FILE_BEGIN);
        this->buffer_start = this->buffer_end;
    }
}

unsigned long FileReader::read(unsigned long nbytes, unsigned char* bytes){
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

bool FileReader::getline(std::string& line){
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

FileReader::FileReader(std::string& filename) : file(filename){
    if(!file)
        this->file = std::ifstream(FileReader::dir + filename);
}

FileReader::operator bool() const{
    return this->file;
}

void FileReader::reset(){
    this->file.seekg(0);
}

unsigned long FileReader::read(unsigned long nbytes, unsigned char* bytes){
    this->file.read(bytes, nbytes);
    return this->file.gcount();
}

bool FileReader::getline(std::string& line){
    return std::getline(this->file, line);
}

#endif
