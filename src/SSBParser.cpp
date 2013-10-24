#include "SSBParser.hpp"
#ifdef _WIN32   // Windows
#   include "textconv.hpp"
#   include "FileReader.hpp"
#else   // Unix
#   include <fstream>
#endif
#include <sstream>

SSBParser::SSBParser(SSBData& ssb) : ssb(ssb){}

SSBParser::SSBParser(std::string& script, bool warnings) throw(std::string){
    this->parse(script, warnings);
}

SSBData SSBParser::data(){
    return this->ssb;
}

// Helper functions for parsing
namespace{
    // Throws string in parse error message format
    inline void throw_parse_error(unsigned int line, const char* message){
        std::ostringstream s;
        s << line << ": " << message;
        throw s.str();
    }
    // Converts string to number
    template<class T>
    inline bool string_to_number(std::string src, T& dst){
        std::istringstream s(src);
        if(!(s >> std::noskipws >> dst) || !s.eof())
            return false;
        return true;
    }
    // Parses SSB time and converts to milliseconds
    inline bool parse_time(std::string& s, unsigned long long& t){
        // Reset accumulator
        t = 0;
        // Time position

        // Iterate through characters
        for(auto it = s.crbegin(); it != s.crend(); ++it){
            #pragma message "Implent SSB time parser"
        }
        // Success
        return true;
    }
}

void SSBParser::parse(std::string& script, bool warnings) throw(std::string){
    // File reading
#ifdef _WIN32   // Windows
    std::wstring scriptW = utf8_to_utf16(script);
    FileReader file(scriptW);
#else   // Unix
    std::ifstream file(script);
#endif
    // File valid?
    if(file){
        // Current SSB section
        enum class SSBSection{NONE, META, FRAME, STYLES, LINES} section = SSBSection::NONE;
        // File line number (needed for warnings)
        unsigned int line_i = 0;
        // File line buffer
        std::string line;
        // Line iteration
#ifdef _WIN32   // Windows
        while(file.getline(line)){
#else   // Unix
        while(std::getline(file, line)){
#endif
            // Update line number
            line_i++;
            // Remove windows carriage return at end of lines
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            // No skippable line
            if(!line.empty() && line.compare(0, 2, "//") != 0){
                // Got section
                if(line.front() == '#'){
                    std::string section_name = line.substr(1);
                    if(section_name == "META")
                        section = SSBSection::META;
                    else if(section_name == "FRAME")
                        section = SSBSection::FRAME;
                    else if(section_name == "STYLES")
                        section = SSBSection::STYLES;
                    else if(section_name == "LINES")
                        section = SSBSection::LINES;
                    else if(warnings)
                        throw_parse_error(line_i, "Invalid section name");
                // Got section value
                }else
                    switch(section){
                        case SSBSection::META:
                            if(line.compare(0, 7, "Title: ") == 0)
                                this->ssb.meta.title = line.substr(7);
                            else if(line.compare(0, 8, "Author: ") == 0)
                                this->ssb.meta.author = line.substr(8);
                            else if(line.compare(0, 13, "Description: ") == 0)
                                this->ssb.meta.description = line.substr(13);
                            else if(line.compare(0, 9, "Version: ") == 0)
                                this->ssb.meta.version = line.substr(9);
                            else if(warnings)
                                throw_parse_error(line_i, "Invalid meta field");
                            break;
                        case SSBSection::FRAME:
                            if(line.compare(0, 7, "Width: ") == 0){
                                if(!string_to_number(line.substr(7), this->ssb.frame.width) || this->ssb.frame.width < 0){
                                    this->ssb.frame.width = -1;
                                    if(warnings)
                                        throw_parse_error(line_i, "Invalid frame width");
                                    break;
                                }
                            }else if(line.compare(0, 8, "Height: ") == 0){
                                if(!string_to_number(line.substr(8), this->ssb.frame.height) || this->ssb.frame.height < 0){
                                    this->ssb.frame.height = -1;
                                    if(warnings)
                                        throw_parse_error(line_i, "Invalid frame height");
                                    break;
                                }
                            }else if(warnings)
                                throw_parse_error(line_i, "Invalid frame field");
                            break;
                        case SSBSection::STYLES:
                            {
                                std::string::size_type pos = line.find(": ");
                                if(pos != std::string::npos){
                                    this->ssb.styles[line.substr(0, pos)] = line.substr(pos+2);
                                }else if(warnings)
                                    throw_parse_error(line_i, "Invalid style");
                            }
                            break;
                        case SSBSection::LINES:
                            {
                                // Output buffer
                                SSBLine ssb_line;
                                // Split line into tokens
                                std::istringstream s(line);
                                std::string token;
                                // Get start time
                                if(!std::getline(s, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find start time");
                                    break;
                                }
                                if(!parse_time(token, ssb_line.start_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't parse start time");
                                    break;
                                }
                                // Get end time
                                if(!std::getline(s, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find end time");
                                    break;
                                }
                                if(!parse_time(token, ssb_line.end_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't parse end time");
                                    break;
                                }
                                // Get style content for later text insertion
                                if(!std::getline(s, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find style");
                                    break;
                                }
                                std::string style_content;
                                if(this->ssb.styles.count(token))
                                    style_content = this->ssb.styles[token];
                                else{
                                    if(warnings)
                                        throw_parse_error(line_i, "Style identifier not found");
                                    break;
                                }
                                // Skip note
                                if(!std::getline(s, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find note");
                                    break;
                                }
                                // Parse text
                                std::string text = style_content + s.str();
    #pragma message "Parse SSB line"
                                // Parsing successfull without exception -> commit output
                                this->ssb.lines.push_back(ssb_line);
                            }
                            break;
                        case SSBSection::NONE:
                            if(warnings)
                                throw_parse_error(line_i, "No section set");
                            break;
                    }
            }
        }
    // File couldn't be read
    }else if(warnings)
        throw std::string("Script couldn't be read: ") + script;
}
