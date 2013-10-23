#include "SSBParser.hpp"
#ifdef _WIN32   // Windows
#   include "textconv.hpp"
#   include "FileReader.hpp"
#else   // Unix
#   include <fstream>
#endif
#include <sstream>

SSBParser::SSBParser(SSBData& ssb) : ssb(ssb){}

SSBParser::SSBParser(std::string& script, bool warnings){
    this->parse(script, warnings);
}

SSBData SSBParser::data(){
    return this->ssb;
}

void SSBParser::parse(std::string& script, bool warnings){
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
        // File line index (needed for warnings)
        unsigned int line_i = 0;
        // File line buffer
        std::string line;
        // Line iteration
#ifdef _WIN32   // Windows
        while(file.getline(line)){
#else   // Unix
        while(std::getline(file, line)){
#endif
            // No skippable line
            if(!line.empty() && !(line.length() >= 2 && line[0] == '/' && line[1] == '/')){
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
                    else if(warnings){
                        std::stringstream s;
                        s << line_i << ": Invalid section name";
                        throw s.str();
                    }
                // Got section value
                }else
                    switch(section){
#pragma message "Parse SSB line"
                        case SSBSection::META:
                            break;
                        case SSBSection::FRAME:
                            break;
                        case SSBSection::STYLES:
                            break;
                        case SSBSection::LINES:
                            break;
                        case SSBSection::NONE:
                            if(warnings){
                                std::stringstream s;
                                s << line_i << ": No section set";
                                throw s.str();
                            }
                            break;
                    }
            }
            // Increase line index for next one
            line_i++;
        }
    // File couldn't be read
    }else if(warnings)
        throw std::string("Script couldn't be read: ") + script;
}
