#include "SSBParser.hpp"
#if defined _WIN32
    #include "textconv.hpp"
    #include "FileReader.hpp"
#else
    #include <fstream>
#endif

SSBParser::SSBParser(std::string& script, bool warnings){
    this->parse(script, warnings);
}

void SSBParser::parse(std::string& script, bool warnings){
    // File line buffer
    std::string line;
    // File reading
#if defined _WIN32   // Windows
    std::wstring scriptW = utf8_to_utf16(script);
    FileReader file(scriptW);
    if(file)
        while(file.getline(line)){
#else   // Unix
    std::ifstream file(script);
    if(file)
        while(std::getline(file, line)){
#endif
            #pragma message "Evaluate file line"
        }
    // File couldn't be read
    else if(warnings)
        throw std::string("Script couldn't be read: ") + script;
}
