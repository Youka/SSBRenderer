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

SSBData SSBParser::data() const {
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
        // Test for empty timestamp
        if(s.empty())
            return false;
        // Time position
        enum class TimeUnit{MS, MS_10, MS_100, MS_LIMIT, SEC, SEC_10, SEC_LIMIT, MIN, MIN_10, MIN_LIMIT, H, H_10, END} unit = TimeUnit::MS;
        // Iterate through characters
        for(auto it = s.crbegin(); it != s.crend(); ++it)
            switch(unit){
                case TimeUnit::MS:
                    if(*it >= '0' && *it <= '9'){
                        t = *it - '0';
                        unit = TimeUnit::MS_10;
                    }else
                        return false;
                    break;
                case TimeUnit::MS_10:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 10;
                        unit = TimeUnit::MS_100;
                    }else if(*it == '.')
                        unit = TimeUnit::SEC;
                    else
                        return false;
                    break;
                case TimeUnit::MS_100:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 100;
                        unit = TimeUnit::MS_LIMIT;
                    }else if(*it == '.')
                        unit = TimeUnit::SEC;
                    else
                        return false;
                    break;
                case TimeUnit::MS_LIMIT:
                    if(*it == '.')
                        unit = TimeUnit::SEC;
                    else
                        return false;
                    break;
                case TimeUnit::SEC:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 1000;
                        unit = TimeUnit::SEC_10;
                    }else
                        return false;
                    break;
                case TimeUnit::SEC_10:
                    if(*it >= '0' && *it <= '5'){
                        t += (*it - '0') * 10 * 1000;
                        unit = TimeUnit::SEC_LIMIT;
                    }else if(*it == ':')
                        unit = TimeUnit::MIN;
                    else
                        return false;
                    break;
                case TimeUnit::SEC_LIMIT:
                    if(*it == ':')
                        unit = TimeUnit::MIN;
                    else
                        return false;
                    break;
                case TimeUnit::MIN:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 60 * 1000;
                        unit = TimeUnit::MIN_10;
                    }else
                        return false;
                    break;
                case TimeUnit::MIN_10:
                    if(*it >= '0' && *it <= '5'){
                        t += (*it - '0') * 10 * 60 * 1000;
                        unit = TimeUnit::MIN_LIMIT;
                    }else if(*it == ':')
                        unit = TimeUnit::H;
                    else
                        return false;
                    break;
                case TimeUnit::MIN_LIMIT:
                    if(*it == ':')
                        unit = TimeUnit::H;
                    else
                        return false;
                    break;
                case TimeUnit::H:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 60 * 60 * 1000;
                        unit = TimeUnit::H_10;
                    }else
                        return false;
                    break;
                case TimeUnit::H_10:
                    if(*it >= '0' && *it <= '9'){
                        t += (*it - '0') * 10 * 60 * 60 * 1000;
                        unit = TimeUnit::END;
                    }else
                        return false;
                    break;
                case TimeUnit::END:
                    return false;
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
                                std::istringstream line_stream(line);
                                std::string token;
                                // Get start time
                                if(!std::getline(line_stream, token, '-')){
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
                                if(!std::getline(line_stream, token, '|')){
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
                                if(!std::getline(line_stream, token, '|')){
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
                                if(!std::getline(line_stream, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find note");
                                    break;
                                }
                                // Get text
                                if(!std::getline(line_stream, token))
                                    token = "";
                                std::string text = style_content + token;
                                // Parse text
                                std::string::size_type pos_start = 0, pos_end;
                                bool in_tags = false;
                                SSBGeometry::Type geometry_type = SSBGeometry::Type::TEXT;
                                do{
                                    // Evaluate either tags or geometry
                                    if(in_tags){
                                        // Search tags end at closing bracket or cause error
                                        pos_end = text.find('}', pos_start);
                                        if(pos_end == std::string::npos){
                                            if(warnings)
                                                throw_parse_error(line_i, "Tags closing brace not found");
                                            break;
                                        }else
                                            pos_end = pos_end - 1;
                                        // Parse single tags
                                        std::string tags = text.substr(pos_start, pos_end - pos_start);
                                        if(!tags.empty()){
                                            std::istringstream tags_stream(tags);
                                            while(std::getline(tags_stream, token, ';')){
#pragma message "Parse SSB tags"
                                            }
                                        }
                                    }else{
                                        // Search geometry end at tags bracket or text end
                                        pos_end = text.find('{', pos_start);
                                        pos_end = pos_end == std::string::npos ? text.length() - 1 : pos_end - 1;
                                        // Parse geometry by type
                                        std::string geometry = text.substr(pos_start, pos_end - pos_start);
                                        if(!geometry.empty())
                                            switch(geometry_type){
                                                case SSBGeometry::Type::POINTS:
                                                    {
                                                        // SSBPoints buffer
                                                        SSBPoints ssb_points;
                                                        // Iterate through numbers
                                                        std::istringstream points_stream(geometry);
                                                        Point point;
                                                        while(points_stream >> point.x && points_stream >> point.y)
                                                            ssb_points.points.push_back(point);
                                                        // Check for streaming error
                                                        if(!(points_stream >> std::ws).eof()){
                                                            if(warnings)
                                                                throw_parse_error(line_i, "Points are invalid");
                                                            break;
                                                        }
                                                        // Points collection successfull without exception -> insert SSBPoints as SSBObject to SSBLine
                                                        ssb_line.objects.push_back(std::shared_ptr<SSBObject>(new SSBPoints(ssb_points)));
                                                    }
                                                    break;
                                                case SSBGeometry::Type::PATH:
                                                    {
                                                        // SSBPath buffer
                                                        SSBPath ssb_path;
                                                        // Iterate through words
                                                        std::istringstream path_stream(geometry);
                                                        SSBPath::Segment segments[3];
                                                        while(path_stream >> token)
                                                            // Set segment type
                                                            if(token == "m")
                                                                segments[0].type = SSBPath::SegmentType::MOVE_TO;
                                                            else if(token == "l")
                                                                segments[0].type = SSBPath::SegmentType::LINE_TO;
                                                            else if(token == "b"){
                                                                segments[0].type = segments[1].type = segments[2].type = SSBPath::SegmentType::CURVE_TO;
                                                            }else if(token == "a"){
                                                                segments[0].type = segments[1].type = SSBPath::SegmentType::ARC_TO;
                                                            }else if(token == "c"){
                                                                segments[0].type = SSBPath::SegmentType::CLOSE;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
                                                                ssb_path.segments.push_back({SSBPath::SegmentType::CLOSE});
#pragma GCC diagnostic pop
                                                            }
                                                            // Get complete segment
                                                            else{
                                                                // Put token back in stream for new reading
                                                                path_stream.seekg(-token.length(), std::istringstream::cur);
                                                                // Parse segment
                                                                switch(segments[0].type){
                                                                    case SSBPath::SegmentType::MOVE_TO:
                                                                    case SSBPath::SegmentType::LINE_TO:
                                                                        if(path_stream >> segments[0].value.point.x &&
                                                                           path_stream >> segments[0].value.point.y)
                                                                            ssb_path.segments.push_back(segments[0]);
                                                                        else if(warnings)
                                                                            throw_parse_error(line_i, segments[0].type == SSBPath::SegmentType::LINE_TO ? "Path (move) is invalid" : "Path (line) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::CURVE_TO:
                                                                        if(path_stream >> segments[0].value.point.x &&
                                                                            path_stream >> segments[0].value.point.y &&
                                                                            path_stream >> segments[1].value.point.x &&
                                                                            path_stream >> segments[1].value.point.y &&
                                                                            path_stream >> segments[2].value.point.x &&
                                                                            path_stream >> segments[2].value.point.y){
                                                                            ssb_path.segments.push_back(segments[0]);
                                                                            ssb_path.segments.push_back(segments[1]);
                                                                            ssb_path.segments.push_back(segments[2]);
                                                                        }else if(warnings)
                                                                            throw_parse_error(line_i, "Path (curve) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::ARC_TO:
                                                                        if(path_stream >> segments[0].value.point.x &&
                                                                           path_stream >> segments[0].value.point.y &&
                                                                           path_stream >> segments[1].value.angle){
                                                                            ssb_path.segments.push_back(segments[0]);
                                                                            ssb_path.segments.push_back(segments[1]);
                                                                        }else if(warnings)
                                                                            throw_parse_error(line_i, "Path (arc) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::CLOSE:
                                                                        if(warnings)
                                                                            throw_parse_error(line_i, "Path (close) is invalid");
                                                                        break;
                                                                }
                                                            }
                                                        // Segments collection successfull without exception -> insert SSBPath as SSBObject to SSBLine
                                                        ssb_line.objects.push_back(std::shared_ptr<SSBObject>(new SSBPath(ssb_path)));
                                                    }
                                                    break;
                                                case SSBGeometry::Type::TEXT:
                                                    {
                                                        // Replace in string \n to real line breaks
                                                        std::string::size_type pos = 0;
                                                        while((pos = geometry.find("\\n", pos)) != std::string::npos){
                                                            geometry.replace(pos, 2, 1, '\n');
                                                            pos++;
                                                        }
                                                        // Insert SSBText as SSBObject to SSBLine
                                                        ssb_line.objects.push_back(std::shared_ptr<SSBObject>(new SSBText(geometry)));
                                                    }
                                                    break;
                                            }
                                    }
                                    pos_start = pos_end + 2;
                                    in_tags = !in_tags;
                                }while(pos_end < text.length() - 1);
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
