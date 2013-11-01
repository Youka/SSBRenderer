#pragma once

#include <windows.h>
#include <string>
#include <algorithm>

class FileReader{
    private:
        // File handle
        HANDLE file;
        // Read buffer
        char buffer[4096];
        char* buffer_start, *buffer_end = buffer_start;
    public:
        // Constructors
        FileReader()
        : file(INVALID_HANDLE_VALUE){}
        FileReader(std::string& filename)
        : file(CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){}
        FileReader(std::wstring& filename)
        : file(CreateFileW(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)){}
        FileReader(FileReader& reader)
        : file(reader.file){reader.file = INVALID_HANDLE_VALUE;}
        // Assign operator
        FileReader& operator =(FileReader& reader){
            if(this->file != INVALID_HANDLE_VALUE)
                CloseHandle(this->file);
            this->file = reader.file;
            this->buffer_start = this->buffer_end;
            reader.file = INVALID_HANDLE_VALUE;
            return *this;
        }
        // Destructor
        virtual ~FileReader(){
            if(this->file != INVALID_HANDLE_VALUE)
                CloseHandle(this->file);
        }
        // Check file state
        operator bool(){
            return this->file != INVALID_HANDLE_VALUE;
        }
        // Reset file pointer
        void reset(){
            if(this->file != INVALID_HANDLE_VALUE){
                SetFilePointer(this->file, 0, 0, FILE_BEGIN);
                this->buffer_start = this->buffer_end;
            }
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
};
