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
    // Converts string to number pair
    template<class T>
    inline bool string_to_number_pair(std::string src, T& dst1, T& dst2){
        std::string::size_type pos;
        return (pos = src.find(',')) != std::string::npos && string_to_number(src.substr(0, pos), dst1) && string_to_number(src.substr(pos+1), dst2);
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
        enum class SSBSection{NONE, META, FRAME, STYLES, EVENTS} section = SSBSection::NONE;
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
                    else if(section_name == "EVENTS")
                        section = SSBSection::EVENTS;
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
                                }
                            }else if(line.compare(0, 8, "Height: ") == 0){
                                if(!string_to_number(line.substr(8), this->ssb.frame.height) || this->ssb.frame.height < 0){
                                    this->ssb.frame.height = -1;
                                    if(warnings)
                                        throw_parse_error(line_i, "Invalid frame height");
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
                        case SSBSection::EVENTS:
                            {
                                // Output buffer
                                SSBEvent ssb_event;
                                // Split line into tokens
                                std::istringstream event_stream(line);
                                std::string token;
                                // Get start time
                                if(!std::getline(event_stream, token, '-') || !parse_time(token, ssb_event.start_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find start time");
                                    break;
                                }
                                // Get end time
                                if(!std::getline(event_stream, token, '|') || !parse_time(token, ssb_event.end_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find end time");
                                    break;
                                }
                                // Get style content for later text insertion
                                std::string style_content;
                                if(!std::getline(event_stream, token, '|') || !this->ssb.styles.count(token)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find style");
                                    break;
                                }else
                                    style_content = this->ssb.styles[token];
                                // Skip note
                                if(!std::getline(event_stream, token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find note");
                                    break;
                                }
                                // Get text
                                if(!std::getline(event_stream, token))
                                    token = "";
                                std::string text = style_content + token;
                                // Parse text
                                std::string::size_type pos_start = 0, pos_end;
                                bool in_tags = false;
                                SSBGeometry::Type geometry_type = SSBGeometry::Type::TEXT;
                                do{
                                    // Evaluate tags
                                    if(in_tags){
                                        // Search tags end at closing bracket or cause error
                                        pos_end = text.find('}', pos_start);
                                        if(pos_end == std::string::npos){
                                            if(warnings)
                                                throw_parse_error(line_i, "Tags closing brace not found");
                                            break;
                                        }
                                        // Parse single tags
                                        std::string tags = text.substr(pos_start, pos_end - pos_start);
                                        if(!tags.empty()){
                                            std::istringstream tags_stream(tags);
                                            while(std::getline(tags_stream, token, ';'))
                                                if(token.compare(0, 12, "font-family=") == 0)
                                                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontFamily(token.substr(12))));
                                                else if(token.compare(0, 11, "font-style=") == 0){
                                                    bool bold = false, italic = false, underline = false, strikeout = false;
                                                    for(char c : token.substr(11))
                                                        if(c == 'b' && !bold)
                                                            bold = true;
                                                        else if(c == 'i' && !italic)
                                                            italic = true;
                                                        else if(c == 'u' && !underline)
                                                            underline = true;
                                                        else if(c == 's' && !strikeout)
                                                            strikeout = true;
                                                        else if(warnings)
                                                            throw_parse_error(line_i, "Invalid font style");
                                                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontStyle(bold, italic, underline, strikeout)));
                                                }else if(token.compare(0, 10, "font-size=") == 0){
                                                    unsigned int size;
                                                    if(string_to_number(token.substr(10), size))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSize(size)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid font size");
                                                }else if(token.compare(0, 11, "font-space=") == 0){
                                                    SSBCoord x, y;
                                                    if(string_to_number_pair(token.substr(11), x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid font spaces");
                                                }else if(token.compare(0, 13, "font-space-h=") == 0){
                                                    SSBCoord x;
                                                    if(string_to_number(token.substr(13), x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(SSBFontSpace::Type::HORIZONTAL, x)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid horizontal font space");
                                                }else if(token.compare(0, 13, "font-space-v=") == 0){
                                                    SSBCoord y;
                                                    if(string_to_number(token.substr(13), y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(SSBFontSpace::Type::VERTICAL, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid vertical font space");
                                                }else if(token.compare(0, 11, "line-width=") == 0){
                                                    SSBCoord width;
                                                    if(string_to_number(token.substr(11), width))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBLineWidth(width)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid line width");
                                                }else if(token.compare(0, 11, "line-style=") == 0){
                                                    std::string tag_value = token.substr(11);
                                                    std::string::size_type pos;
                                                    if((pos = tag_value.find(',')) != std::string::npos){
                                                        std::string join_string = tag_value.substr(0, pos), cap_string = tag_value.substr(pos+1);
                                                        SSBLineStyle::Join join = SSBLineStyle::Join::ROUND;
                                                        if(join_string == "miter")
                                                            join = SSBLineStyle::Join::MITER;
                                                        else if(join_string == "round")
                                                            join = SSBLineStyle::Join::ROUND;
                                                        else if(join_string == "bevel")
                                                            join = SSBLineStyle::Join::BEVEL;
                                                        else if(warnings)
                                                            throw_parse_error(line_i, "Invalid line style join");
                                                        SSBLineStyle::Cap cap = SSBLineStyle::Cap::ROUND;
                                                        if(cap_string == "flat")
                                                            cap = SSBLineStyle::Cap::FLAT;
                                                        else if(cap_string == "round")
                                                            cap = SSBLineStyle::Cap::ROUND;
                                                        else if(cap_string == "square")
                                                            cap = SSBLineStyle::Cap::SQUARE;
                                                        else if(warnings)
                                                            throw_parse_error(line_i, "Invalid line style cap");
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBLineStyle(join, cap)));
                                                    }else if(warnings)
                                                        throw_parse_error(line_i, "Invalid line style");
                                                }else if(token.compare(0, 10, "line-dash=") == 0){
                                                    SSBCoord offset;
                                                    std::istringstream dash_stream(token.substr(10));
                                                    std::string dash_token;
                                                    if(std::getline(dash_stream, dash_token, ',') && string_to_number(dash_token, offset)){
                                                        std::vector<SSBCoord> dashes;
                                                        SSBCoord dash;
                                                        while(std::getline(dash_stream, dash_token, ','))
                                                            if(string_to_number(dash_token, dash))
                                                                dashes.push_back(dash);
                                                            else if(warnings)
                                                                throw_parse_error(line_i, "Invalid line dash");
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBLineDash(offset, dashes)));
                                                    }else if(warnings)
                                                        throw_parse_error(line_i, "Invalid line dashes");
                                                }else if(token.compare(0, 9, "geometry=") == 0){
                                                    std::string tag_value = token.substr(9);
                                                    if(tag_value == "points")
                                                        geometry_type = SSBGeometry::Type::POINTS;
                                                    else if(tag_value == "path")
                                                        geometry_type = SSBGeometry::Type::PATH;
                                                    else if(tag_value == "text")
                                                        geometry_type = SSBGeometry::Type::TEXT;
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid geometry");
                                                }else if(token.compare(0, 5, "mode=") == 0){
                                                    std::string tag_value = token.substr(5);
                                                    if(tag_value == "fill")
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMode(SSBMode::Mode::FILL)));
                                                    else if(tag_value == "wire")
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMode(SSBMode::Mode::WIRE)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid mode");
                                                }else if(token.compare(0, 7, "deform=") == 0){
                                                    std::string tag_value = token.substr(7);
                                                    std::string::size_type pos;
                                                    if((pos = tag_value.find(',')) != std::string::npos){
                                                        ssb_event.static_tags = false;
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDeform(tag_value.substr(0, pos), tag_value.substr(pos+1))));
                                                    }else if(warnings)
                                                        throw_parse_error(line_i, "Invalid deform");
                                                }else if(token.compare(0, 9, "position=") == 0){
                                                    std::string tag_value = token.substr(9);
                                                    SSBCoord x, y;
                                                    if(tag_value.empty())
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPosition(std::numeric_limits<decltype(x)>::max(), std::numeric_limits<decltype(y)>::max())));
                                                    else if(string_to_number_pair(token.substr(9), x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPosition(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid position");
                                                }else if(token.compare(0, 6, "align=") == 0){
                                                    std::string tag_value = token.substr(6);
                                                    if(tag_value.length() == 1 && tag_value[0] >= '1' && tag_value[0] <= '9')
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBAlign(static_cast<SSBAlign::Align>(tag_value[0] - '0'))));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid alignment");
                                                }else if(token.compare(0, 7, "margin=")){
                                                    std::string tag_value = token.substr(7);
                                                    SSBCoord x, y;
                                                    if(string_to_number(tag_value, x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::BOTH, x)));
                                                    else if(string_to_number_pair(tag_value, x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid margin");
                                                }else if(token.compare(0, 9, "margin-h=")){
                                                    SSBCoord x;
                                                    if(string_to_number(token.substr(9), x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::HORIZONTAL, x)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid horizontal margin");
                                                }else if(token.compare(0, 9, "margin-v=")){
                                                    SSBCoord y;
                                                    if(string_to_number(token.substr(9), y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::VERTICAL, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid vertical margin");
                                                }else if(token.compare(0, 10, "direction=")){
                                                    double angle;
                                                    if(string_to_number(token.substr(10), angle))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDirection(angle)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid direction");
                                                }else if(token == "identity")
                                                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBIdentity()));
                                                else if(token.compare(0, 10, "translate=") == 0){
                                                    SSBCoord x, y;
                                                    if(string_to_number_pair(token.substr(10), x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid translation");
                                                }else if(token.compare(0, 12, "translate-x=") == 0){
                                                    SSBCoord x;
                                                    if(string_to_number(token.substr(12), x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(SSBTranslate::Type::HORIZONTAL, x)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid horizontal translation");
                                                }else if(token.compare(0, 12, "translate-y=") == 0){
                                                    SSBCoord y;
                                                    if(string_to_number(token.substr(12), y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(SSBTranslate::Type::VERTICAL, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid vertical translation");
                                                }else if(token.compare(0, 6, "scale=") == 0){
                                                    std::string tag_value = token.substr(6);
                                                    double x, y;
                                                    if(string_to_number(tag_value, x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::BOTH, x)));
                                                    else if(string_to_number_pair(tag_value, x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid scale");
                                                }else if(token.compare(0, 8, "scale-x=") == 0){
                                                    double x;
                                                    if(string_to_number(token.substr(8), x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::HORIZONTAL, x)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid horizontal scale");
                                                }else if(token.compare(0, 8, "scale-y=") == 0){
                                                    double y;
                                                    if(string_to_number(token.substr(8), y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::VERTICAL, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid vertical scale");
                                                }else if(token.compare(0, 9, "rotate-x=") == 0){
                                                    double angle;
                                                    if(string_to_number(token.substr(9), angle))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(SSBRotate::Axis::X, angle)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid rotation on x axis");
                                                }else if(token.compare(0, 9, "rotate-y=") == 0){
                                                    double angle;
                                                    if(string_to_number(token.substr(9), angle))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(SSBRotate::Axis::Y, angle)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid rotation on y axis");
                                                }else if(token.compare(0, 9, "rotate-z") == 0){
                                                    double angle;
                                                    if(string_to_number(token.substr(9), angle))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(SSBRotate::Axis::Z, angle)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid rotation on z axis");
                                                }else if(token.compare(0, 6, "shear=") == 0){
                                                    double x, y;
                                                    if(string_to_number_pair(token.substr(6), x, y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(x, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid shear");
                                                }else if(token.compare(0, 8, "shear-x=") == 0){
                                                    double x;
                                                    if(string_to_number(token.substr(8), x))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(SSBShear::Type::HORIZONTAL, x)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid horizontal shear");
                                                }else if(token.compare(0, 8, "shear-y") == 0){
                                                    double y;
                                                    if(string_to_number(token.substr(8), y))
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(SSBShear::Type::VERTICAL, y)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid vertical shear");
                                                }else if(token.compare(0, 10, "transform=") == 0){
                                                    double xx, yx, xy, yy, x0, y0;
                                                    std::istringstream matrix_stream(token.substr(10));
                                                    std::string matrix_value;
                                                    if(std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, xx) &&
                                                        std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, yx) &&
                                                        std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, xy) &&
                                                        std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, yy) &&
                                                        std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, x0) &&
                                                        std::getline(matrix_stream, matrix_value, ',') && string_to_number(matrix_value, y0) &&
                                                        matrix_stream.eof()
                                                       )
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTransform(xx, yx, xy, yy, x0, y0)));
                                                    else if(warnings)
                                                        throw_parse_error(line_i, "Invalid transform");
#pragma message "Parse SSB tags"
                                                }else if(warnings)
                                                    throw_parse_error(line_i, "Invalid tag");
                                        }
                                    // Evaluate geometry
                                    }else{
                                        // Search geometry end at tags bracket or text end
                                        pos_end = text.find('{', pos_start);
                                        if(pos_end == std::string::npos)
                                            pos_end = text.length();
                                        // Parse geometry by type
                                        std::string geometry = text.substr(pos_start, pos_end - pos_start);
                                        if(!geometry.empty())
                                            switch(geometry_type){
                                                case SSBGeometry::Type::POINTS:
                                                    {
                                                        // Points buffer
                                                        std::vector<Point> points;
                                                        // Iterate through numbers
                                                        std::istringstream points_stream(geometry);
                                                        Point point;
                                                        while(points_stream >> point.x)
                                                            if(points_stream >> point.y)
                                                                points.push_back(point);
                                                            else if(warnings)
                                                                throw_parse_error(line_i, "Points must have 2 numbers");
                                                        // Check for successfull streaming end
                                                        if((points_stream >> std::ws).eof())
                                                            ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPoints(points)));
                                                        else if(warnings)
                                                            throw_parse_error(line_i, "Points are invalid");
                                                    }
                                                    break;
                                                case SSBGeometry::Type::PATH:
                                                    {
                                                        // Path buffer
                                                        std::vector<SSBPath::Segment> path;
                                                        // Iterate through words
                                                        std::istringstream path_stream(geometry);
                                                        SSBPath::Segment segments[3];
                                                        while(path_stream >> token)
                                                            // Set segment type
                                                            if(token == "m")
                                                                segments[0].type = SSBPath::SegmentType::MOVE_TO;
                                                            else if(token == "l")
                                                                segments[0].type = SSBPath::SegmentType::LINE_TO;
                                                            else if(token == "b")
                                                                segments[0].type = segments[1].type = segments[2].type = SSBPath::SegmentType::CURVE_TO;
                                                            else if(token == "a")
                                                                segments[0].type = segments[1].type = SSBPath::SegmentType::ARC_TO;
                                                            else if(token == "c"){
                                                                segments[0].type = SSBPath::SegmentType::CLOSE;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
                                                                path.push_back({SSBPath::SegmentType::CLOSE});
#pragma GCC diagnostic pop
                                                            }
                                                            // Get complete segment
                                                            else{
                                                                // Put token back in stream for rereading
                                                                path_stream.seekg(-static_cast<long int>(token.length()), std::istringstream::cur);
                                                                // Parse segment
                                                                switch(segments[0].type){
                                                                    case SSBPath::SegmentType::MOVE_TO:
                                                                    case SSBPath::SegmentType::LINE_TO:
                                                                        if(path_stream >> segments[0].point.x &&
                                                                           path_stream >> segments[0].point.y)
                                                                            path.push_back(segments[0]);
                                                                        else if(warnings)
                                                                            throw_parse_error(line_i, segments[0].type == SSBPath::SegmentType::MOVE_TO ? "Path (move) is invalid" : "Path (line) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::CURVE_TO:
                                                                        if(path_stream >> segments[0].point.x &&
                                                                            path_stream >> segments[0].point.y &&
                                                                            path_stream >> segments[1].point.x &&
                                                                            path_stream >> segments[1].point.y &&
                                                                            path_stream >> segments[2].point.x &&
                                                                            path_stream >> segments[2].point.y){
                                                                            path.push_back(segments[0]);
                                                                            path.push_back(segments[1]);
                                                                            path.push_back(segments[2]);
                                                                        }else if(warnings)
                                                                            throw_parse_error(line_i, "Path (curve) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::ARC_TO:
                                                                        if(path_stream >> segments[0].point.x &&
                                                                           path_stream >> segments[0].point.y &&
                                                                           path_stream >> segments[1].angle){
                                                                            path.push_back(segments[0]);
                                                                            path.push_back(segments[1]);
                                                                        }else if(warnings)
                                                                            throw_parse_error(line_i, "Path (arc) is invalid");
                                                                        break;
                                                                    case SSBPath::SegmentType::CLOSE:
                                                                        if(warnings)
                                                                            throw_parse_error(line_i, "Path (close) is invalid");
                                                                        break;
                                                                }
                                                            }
                                                        // Segments collection successfull without exception -> insert SSBPath as SSBObject to SSBEvent
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPath(path)));
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
                                                        // Insert SSBText as SSBObject to SSBEvent
                                                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBText(geometry)));
                                                    }
                                                    break;
                                            }
                                    }
                                    pos_start = pos_end + 1;
                                    in_tags = !in_tags;
                                }while(pos_end < text.length());
                                // Parsing successfull without exception -> commit output
                                this->ssb.events.push_back(ssb_event);
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
