/*
Project: SSBRenderer
File: SSBParser.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "SSBParser.hpp"
#include "FileReader.hpp"
#include <algorithm>
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
    template<typename Line, typename Message>
    inline void throw_parse_error(Line line, Message message){
        std::ostringstream s;
        s << line << ": " << message;
        throw s.str();
    }
    // Converts string to number
    template<typename T>
    inline bool string_to_number(std::string src, T& dst){
        std::istringstream s(src);
        if(!(s >> std::noskipws >> dst) || !s.eof())
            return false;
        return true;
    }
    // Converts string to number pair
    template<typename T>
    inline bool string_to_number_pair(std::string src, T& dst1, T& dst2){
        std::string::size_type pos;
        return (pos = src.find(',')) != std::string::npos &&
                string_to_number(src.substr(0, pos), dst1) &&
                string_to_number(src.substr(pos+1), dst2);
    }
    // Converts hex string to number
    template<typename T>
    inline bool hex_string_to_number(std::string src, T& dst){
        std::istringstream s(src);
        if(!(s >> std::noskipws >> std::hex >> dst) || !s.eof())
            return false;
        return true;
    }
    // Converts hex string to three numbers
    template<typename T>
    inline bool hex_string_to_number_quadruple(std::string src, T& dst1, T& dst2, T& dst3, T& dst4){
        std::string::size_type pos1, pos2;
        return (pos1 = src.find(',')) != std::string::npos &&
                hex_string_to_number(src.substr(0, pos1), dst1) &&
                (pos2 = src.find(',', pos1+1)) != std::string::npos &&
                hex_string_to_number(src.substr(pos1+1, pos2-(pos1+1)), dst2) &&
                (pos1 = src.find(',', pos2+1)) != std::string::npos &&
                hex_string_to_number(src.substr(pos2+1, pos1-(pos2+1)), dst3) &&
                hex_string_to_number(src.substr(pos1+1), dst4);
    }
    // Replaces text
    inline std::string& string_replace(std::string& s, std::string find, std::string repl){
        for(std::string::size_type pos = 0; (pos = s.find(find, pos)) != std::string::npos; pos+=repl.length())
            s.replace(pos, find.length(), repl);
        return s;
    }
    // Find character in string which isn't escaped by character '\'
    inline std::string::size_type find_non_escaped_character(std::string& s, const char c, const std::string::size_type pos_start = 0){
        std::string::size_type pos_end;
        for(auto search_pos_start = pos_start;
            (pos_end = s.find(c, search_pos_start)) != std::string::npos && pos_end > 0 && s[pos_end-1] == '\\';
            search_pos_start = pos_end + 1);
        return pos_end;
    }
    // Parses SSB time and converts to milliseconds
    template<typename T>
    inline bool parse_time(std::string& s, T& t){
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
    // Parses tags and adds to SSB event object
    void parse_tags(std::string& tags, SSBEvent& ssb_event, SSBGeometry::Type& geometry_type, unsigned long int line_i, bool warnings) throw(std::string){
        std::istringstream tags_stream(tags);
        std::string tags_token;
        while(std::getline(tags_stream, tags_token, ';'))
            if(tags_token.compare(0, 12, "font-family=") == 0)
                ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontFamily(tags_token.substr(12))));
            else if(tags_token.compare(0, 11, "font-style=") == 0){
                bool bold = false, italic = false, underline = false, strikeout = false;
                for(char c : tags_token.substr(11))
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
            }else if(tags_token.compare(0, 10, "font-size=") == 0){
                decltype(SSBFontSize::size) size;
                if(string_to_number(tags_token.substr(10), size))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSize(size)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid font size");
            }else if(tags_token.compare(0, 11, "font-space=") == 0){
                decltype(SSBFontSpace::x) x, y;
                if(string_to_number_pair(tags_token.substr(11), x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid font spaces");
            }else if(tags_token.compare(0, 13, "font-space-h=") == 0){
                decltype(SSBFontSpace::x) x;
                if(string_to_number(tags_token.substr(13), x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(SSBFontSpace::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal font space");
            }else if(tags_token.compare(0, 13, "font-space-v=") == 0){
                decltype(SSBFontSpace::y) y;
                if(string_to_number(tags_token.substr(13), y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFontSpace(SSBFontSpace::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical font space");
            }else if(tags_token.compare(0, 11, "line-width=") == 0){
                decltype(SSBLineWidth::width) width;
                if(string_to_number(tags_token.substr(11), width) && width >= 0)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBLineWidth(width)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid line width");
            }else if(tags_token.compare(0, 11, "line-style=") == 0){
                std::string tag_value = tags_token.substr(11);
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
            }else if(tags_token.compare(0, 10, "line-dash=") == 0){
                decltype(SSBLineDash::offset) offset;
                std::istringstream dash_stream(tags_token.substr(10));
                std::string dash_token;
                if(std::getline(dash_stream, dash_token, ',') && string_to_number(dash_token, offset) && offset >= 0){
                    decltype(SSBLineDash::dashes) dashes;
                    decltype(SSBLineDash::offset) dash;
                    while(std::getline(dash_stream, dash_token, ','))
                        if(string_to_number(dash_token, dash) && dash >= 0)
                            dashes.push_back(dash);
                        else if(warnings)
                            throw_parse_error(line_i, "Invalid line dash");
                    if(static_cast<size_t>(std::count(dashes.begin(), dashes.end(), 0)) != dashes.size())
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBLineDash(offset, dashes)));
                    else if(warnings)
                        throw_parse_error(line_i, "Dashes must not be only 0");
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid line dashes");
            }else if(tags_token.compare(0, 9, "geometry=") == 0){
                std::string tag_value = tags_token.substr(9);
                if(tag_value == "points")
                    geometry_type = SSBGeometry::Type::POINTS;
                else if(tag_value == "path")
                    geometry_type = SSBGeometry::Type::PATH;
                else if(tag_value == "text")
                    geometry_type = SSBGeometry::Type::TEXT;
                else if(warnings)
                    throw_parse_error(line_i, "Invalid geometry");
            }else if(tags_token.compare(0, 5, "mode=") == 0){
                std::string tag_value = tags_token.substr(5);
                if(tag_value == "fill")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMode(SSBMode::Mode::FILL)));
                else if(tag_value == "wire")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMode(SSBMode::Mode::WIRE)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid mode");
            }else if(tags_token.compare(0, 7, "deform=") == 0){
                std::string tag_value = tags_token.substr(7);
                std::string::size_type pos;
                if((pos = tag_value.find(',')) != std::string::npos && tag_value.find(',', pos+1) == std::string::npos)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDeform(tag_value.substr(0, pos), tag_value.substr(pos+1))));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid deform");
            }else if(tags_token.compare(0, 9, "position=") == 0){
                std::string tag_value = tags_token.substr(9);
                decltype(SSBPosition::x) x, y;
                constexpr decltype(x) max_pos = std::numeric_limits<decltype(x)>::max();
                if(tag_value.empty())
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPosition(max_pos, max_pos)));
                else if(string_to_number_pair(tags_token.substr(9), x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBPosition(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid position");
            }else if(tags_token.compare(0, 6, "align=") == 0){
                std::string tag_value = tags_token.substr(6);
                if(tag_value.length() == 1 && tag_value[0] >= '1' && tag_value[0] <= '9')
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBAlign(static_cast<SSBAlign::Align>(tag_value[0] - '0'))));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid alignment");
            }else if(tags_token.compare(0, 7, "margin=") == 0){
                std::string tag_value = tags_token.substr(7);
                decltype(SSBMargin::x) x, y;
                if(string_to_number(tag_value, x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::BOTH, x)));
                else if(string_to_number_pair(tag_value, x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid margin");
            }else if(tags_token.compare(0, 9, "margin-h=") == 0){
                decltype(SSBMargin::x) x;
                if(string_to_number(tags_token.substr(9), x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal margin");
            }else if(tags_token.compare(0, 9, "margin-v=") == 0){
                decltype(SSBMargin::y) y;
                if(string_to_number(tags_token.substr(9), y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBMargin(SSBMargin::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical margin");
            }else if(tags_token.compare(0, 10, "direction=") == 0){
                std::string tag_value = tags_token.substr(10);
                if(tag_value == "ltr")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDirection(SSBDirection::Mode::LTR)));
                else if(tag_value == "rtl")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDirection(SSBDirection::Mode::RTL)));
                else if(tag_value == "ttb")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBDirection(SSBDirection::Mode::TTB)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid direction");
            }else if(tags_token == "identity")
                ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBIdentity()));
            else if(tags_token.compare(0, 10, "translate=") == 0){
                decltype(SSBTranslate::x) x, y;
                if(string_to_number_pair(tags_token.substr(10), x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid translation");
            }else if(tags_token.compare(0, 12, "translate-x=") == 0){
                decltype(SSBTranslate::x) x;
                if(string_to_number(tags_token.substr(12), x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(SSBTranslate::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal translation");
            }else if(tags_token.compare(0, 12, "translate-y=") == 0){
                decltype(SSBTranslate::y) y;
                if(string_to_number(tags_token.substr(12), y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTranslate(SSBTranslate::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical translation");
            }else if(tags_token.compare(0, 6, "scale=") == 0){
                std::string tag_value = tags_token.substr(6);
                decltype(SSBScale::x) x, y;
                if(string_to_number(tag_value, x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::BOTH, x)));
                else if(string_to_number_pair(tag_value, x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid scale");
            }else if(tags_token.compare(0, 8, "scale-x=") == 0){
                decltype(SSBScale::x) x;
                if(string_to_number(tags_token.substr(8), x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal scale");
            }else if(tags_token.compare(0, 8, "scale-y=") == 0){
                decltype(SSBScale::y) y;
                if(string_to_number(tags_token.substr(8), y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBScale(SSBScale::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical scale");
            }else if(tags_token.compare(0, 10, "rotate-xy=") == 0){
                decltype(SSBRotate::angle1) angle1, angle2;
                if(string_to_number_pair(tags_token.substr(10), angle1, angle2))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(SSBRotate::Axis::XY, angle1, angle2)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid rotation on x axis");
            }else if(tags_token.compare(0, 10, "rotate-yx=") == 0){
                decltype(SSBRotate::angle1) angle1, angle2;
                if(string_to_number_pair(tags_token.substr(10), angle1, angle2))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(SSBRotate::Axis::YX, angle1, angle2)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid rotation on y axis");
            }else if(tags_token.compare(0, 9, "rotate-z=") == 0){
                decltype(SSBRotate::angle1) angle;
                if(string_to_number(tags_token.substr(9), angle))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBRotate(angle)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid rotation on z axis");
            }else if(tags_token.compare(0, 6, "shear=") == 0){
                decltype(SSBShear::x) x, y;
                if(string_to_number_pair(tags_token.substr(6), x, y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid shear");
            }else if(tags_token.compare(0, 8, "shear-x=") == 0){
                decltype(SSBShear::x) x;
                if(string_to_number(tags_token.substr(8), x))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(SSBShear::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal shear");
            }else if(tags_token.compare(0, 8, "shear-y") == 0){
                decltype(SSBShear::y) y;
                if(string_to_number(tags_token.substr(8), y))
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBShear(SSBShear::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical shear");
            }else if(tags_token.compare(0, 10, "transform=") == 0){
                decltype(SSBTransform::xx) xx, yx, xy, yy, x0, y0;
                std::istringstream matrix_stream(tags_token.substr(10));
                std::string matrix_token;
                if(std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, xx) &&
                        std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, yx) &&
                        std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, xy) &&
                        std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, yy) &&
                        std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, x0) &&
                        std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, y0) &&
                        matrix_stream.eof()
                  )
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTransform(xx, yx, xy, yy, x0, y0)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid transform");
            }else if(tags_token.compare(0, 6, "color=") == 0 || tags_token.compare(0, 11, "line-color=") == 0){
                std::string tag_value;
                SSBColor::Target target;
                if(tags_token[0] == 'l'){
                    tag_value = tags_token.substr(11);
                    target = SSBColor::Target::LINE;
                }else{
                    tag_value = tags_token.substr(6);
                    target = SSBColor::Target::FILL;
                }
                unsigned long int rgb[4];
                if(hex_string_to_number(tag_value, rgb[0]) &&
                        rgb[0] <= 0xffffff)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBColor(target,
                                                static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff
                                                                                       )));
                else if(hex_string_to_number_quadruple(tag_value, rgb[0], rgb[1], rgb[2], rgb[3]) &&
                        rgb[0] <= 0xffffff && rgb[1] <= 0xffffff && rgb[2] <= 0xffffff && rgb[3] <= 0xffffff)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBColor(target,
                                                static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff,
                                                static_cast<decltype(RGB::r)>(rgb[1] >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb[1] & 0xff) / 0xff,
                                                static_cast<decltype(RGB::r)>(rgb[2] >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb[2] >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb[2] & 0xff) / 0xff,
                                                static_cast<decltype(RGB::r)>(rgb[3] >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb[3] >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb[3] & 0xff) / 0xff
                                                                                       )));
                else if(warnings)
                    throw_parse_error(line_i, tags_token[0] == 'l' ? "Invalid line color" : "Invalid color");
            }else if(tags_token.compare(0, 6, "alpha=") == 0 || tags_token.compare(0, 11, "line-alpha=") == 0){
                std::string tag_value;
                SSBAlpha::Target target;
                if(tags_token[0] == 'l'){
                    tag_value = tags_token.substr(11);
                    target = SSBAlpha::Target::LINE;
                }else{
                    tag_value = tags_token.substr(6);
                    target = SSBAlpha::Target::FILL;
                }
                unsigned short int a[4];
                if(hex_string_to_number(tag_value, a[0]) &&
                        a[0] <= 0xff)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBAlpha(target, static_cast<decltype(RGB::r)>(a[0]) / 0xff)));
                else if(hex_string_to_number_quadruple(tag_value, a[0], a[1], a[2], a[3]) &&
                        a[0] <= 0xff && a[1] <= 0xff && a[2] <= 0xff && a[3] <= 0xff)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBAlpha(target,
                                                static_cast<decltype(RGB::r)>(a[0]) / 0xff,
                                                static_cast<decltype(RGB::r)>(a[1]) / 0xff,
                                                static_cast<decltype(RGB::r)>(a[2]) / 0xff,
                                                static_cast<decltype(RGB::r)>(a[3]) / 0xff
                                                                                       )));
                else if(warnings)
                    throw_parse_error(line_i, tags_token[0] == 'l' ? "Invalid line alpha" : "Invalid alpha");
            }else if(tags_token.compare(0, 8, "texture=") == 0 || tags_token.compare(0, 13, "line-texture=") == 0){
                if(tags_token[0] == 'l')
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexture(SSBTexture::Target::LINE, tags_token.substr(13))));
                else
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexture(SSBTexture::Target::FILL, tags_token.substr(8))));
            }else if(tags_token.compare(0, 8, "texfill=") == 0 || tags_token.compare(0, 13, "line-texfill=") == 0){
                std::string tag_value;
                SSBTexFill::Target target;
                if(tags_token[0] == 'l'){
                    tag_value = tags_token.substr(13);
                    target = SSBTexFill::Target::LINE;
                }else{
                    tag_value = tags_token.substr(8);
                    target = SSBTexFill::Target::FILL;
                }
                decltype(SSBTexFill::x) x, y;
                std::string::size_type pos1, pos2;
                if((pos1 = tag_value.find(',')) != std::string::npos &&
                        string_to_number(tag_value.substr(0, pos1), x) &&
                        (pos2 = tag_value.find(',', pos1+1)) != std::string::npos &&
                        string_to_number(tag_value.substr(pos1+1, pos2-(pos1+1)), y)){
                    std::string wrap = tag_value.substr(pos2+1);
                    if(wrap == "clamp")
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexFill(target, x, y, SSBTexFill::WrapStyle::CLAMP)));
                    else if(wrap == "repeat")
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexFill(target, x, y, SSBTexFill::WrapStyle::REPEAT)));
                    else if(wrap == "mirror")
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexFill(target, x, y, SSBTexFill::WrapStyle::MIRROR)));
                    else if(wrap == "flow")
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBTexFill(target, x, y, SSBTexFill::WrapStyle::FLOW)));
                    else if(warnings)
                        throw_parse_error(line_i, "Invalid texture filling wrap style");
                }else if(warnings)
                    throw_parse_error(line_i, tags_token[0] == 'l' ? "Invalid line texture filling" : "Invalid texture filling");
            }else if(tags_token.compare(0, 6, "blend=") == 0){
                std::string tag_value = tags_token.substr(6);
                if(tag_value == "over")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::OVER)));
                else if(tag_value == "add")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::ADDITION)));
                else if(tag_value == "sub")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::SUBTRACT)));
                else if(tag_value == "mult")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::MULTIPLY)));
                else if(tag_value == "screen")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::SCREEN)));
                else if(tag_value == "differ")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlend(SSBBlend::Mode::DIFFERENT)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid blending");
            }else if(tags_token.compare(0, 5, "blur=") == 0){
                std::string tag_value = tags_token.substr(5);
                decltype(SSBBlur::x) x, y;
                if(string_to_number(tag_value, x) && x >= 0)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlur(SSBBlur::Type::BOTH, x)));
                else if(string_to_number_pair(tag_value, x, y) && x >= 0 && y >= 0)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlur(x, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid blur");
            }else if(tags_token.compare(0, 7, "blur-h=") == 0){
                decltype(SSBBlur::x) x;
                if(string_to_number(tags_token.substr(7), x) && x >= 0)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlur(SSBBlur::Type::HORIZONTAL, x)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid horizontal blur");
            }else if(tags_token.compare(0, 7, "blur-v=") == 0){
                decltype(SSBBlur::y) y;
                if(string_to_number(tags_token.substr(7), y) && y >= 0)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBBlur(SSBBlur::Type::VERTICAL, y)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid vertical blur");
            }else if(tags_token.compare(0, 8, "stencil=") == 0){
                std::string tag_value = tags_token.substr(8);
                if(tag_value == "off")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBStencil(SSBStencil::Mode::OFF)));
                else if(tag_value == "set")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBStencil(SSBStencil::Mode::SET)));
                else if(tag_value == "unset")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBStencil(SSBStencil::Mode::UNSET)));
                else if(tag_value == "in")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBStencil(SSBStencil::Mode::INSIDE)));
                else if(tag_value == "out")
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBStencil(SSBStencil::Mode::OUTSIDE)));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid stencil mode");
            }else if(tags_token.compare(0, 5, "fade=") == 0){
                std::string tag_value = tags_token.substr(5);
                decltype(SSBFade::in) in, out;
                if(string_to_number(tag_value, in)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFade(SSBFade::Type::BOTH, in)));
                }else if(string_to_number_pair(tag_value, in, out)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFade(in, out)));
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid fade");
            }else if(tags_token.compare(0, 8, "fade-in=") == 0){
                decltype(SSBFade::in) in;
                if(string_to_number(tags_token.substr(8), in)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFade(SSBFade::Type::INFADE, in)));
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid infade");
            }else if(tags_token.compare(0, 9, "fade-out=") == 0){
                decltype(SSBFade::out) out;
                if(string_to_number(tags_token.substr(9), out)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBFade(SSBFade::Type::OUTFADE, out)));
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid outfade");
            }else if(tags_token.compare(0, 8, "animate=") == 0){
                // Collect animation tokens (maximum: 4)
                std::vector<std::string> animate_tokens;
                std::istringstream animate_stream(tags_token.substr(8));
                std::string animate_token;
                for(unsigned char i = 0; std::getline(animate_stream, animate_token, ',') && i < 4; ++i)
                    if(!animate_token.empty() && animate_token.front() == '('){
                        if(animate_stream.unget() && animate_stream.get() == ','){
                            std::string animate_rest;
                            std::getline(animate_stream, animate_rest);
                            animate_token += ',' + animate_rest;
                        }
                        while(animate_token.back() != ')' && std::getline(tags_stream, tags_token, ';'))
                            animate_token += ';' + tags_token;
                        animate_tokens.push_back(animate_token);
                        break;
                    }else
                        animate_tokens.push_back(animate_token);
                // Check last animation token for brackets
                if(animate_tokens.size() > 0 && animate_tokens.back().length() >= 2 && animate_tokens.back().front() == '(' && animate_tokens.back().back() == ')'){
                    // Get animation values
                    constexpr decltype(SSBAnimate::start) max_duration = std::numeric_limits<decltype(SSBAnimate::start)>::max();
                    decltype(SSBAnimate::start) start_time = max_duration, end_time = max_duration;
                    std::string progress_formula;
                    SSBEvent buffer_event;
                    bool success = true;
                    try{
                        switch(animate_tokens.size()){
                            case 1:
                                {
                                    auto tags = animate_tokens[0].substr(1, animate_tokens[0].size()-2);
                                    parse_tags(tags, buffer_event, geometry_type, line_i, warnings);
                                }
                                break;
                            case 2:
                                {
                                    progress_formula = animate_tokens[0];
                                    auto tags = animate_tokens[1].substr(1, animate_tokens[1].size()-2);
                                    parse_tags(tags, buffer_event, geometry_type, line_i, warnings);
                                }
                                break;
                            case 3:
                                if(string_to_number(animate_tokens[0], start_time) && string_to_number(animate_tokens[1], end_time)){
                                    auto tags = animate_tokens[2].substr(1, animate_tokens[2].size()-2);
                                    parse_tags(tags, buffer_event, geometry_type, line_i, warnings);
                                }else
                                    success = false;
                                break;
                            case 4:
                                if(string_to_number(animate_tokens[0], start_time) && string_to_number(animate_tokens[1], end_time)){
                                    progress_formula = animate_tokens[2];
                                    auto tags = animate_tokens[3].substr(1, animate_tokens[3].size()-2);
                                    parse_tags(tags, buffer_event, geometry_type, line_i, warnings);
                                }else
                                    success = false;
                                break;
                        }
                    }catch(...){
                        success = false;
                    }
                    // Validate animation
                    if(success && buffer_event.static_tags){
                        ssb_event.static_tags = false;
                        ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBAnimate(start_time, end_time, progress_formula, buffer_event.objects)));
                    }else if(warnings)
                        throw_parse_error(line_i, "Animation values incorrect");
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid animate");
            }else if(tags_token.compare(0, 2, "k=") == 0){
                decltype(SSBKaraoke::time) time;
                if(string_to_number(tags_token.substr(2), time)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBKaraoke(SSBKaraoke::Type::DURATION, time)));
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid karaoke");
            }else if(tags_token.compare(0, 5, "kset=") == 0){
                decltype(SSBKaraoke::time) time;
                if(string_to_number(tags_token.substr(5), time)){
                    ssb_event.static_tags = false;
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBKaraoke(SSBKaraoke::Type::SET, time)));
                }else if(warnings)
                    throw_parse_error(line_i, "Invalid karaoke set");
            }else if(tags_token.compare(0, 7, "kcolor=") == 0){
                unsigned long int rgb;
                if(hex_string_to_number(tags_token.substr(7), rgb) && rgb <= 0xffffff)
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBKaraokeColor(
                                                static_cast<decltype(RGB::r)>(rgb >> 16) / 0xff,
                                                static_cast<decltype(RGB::g)>(rgb >> 8 & 0xff) / 0xff,
                                                static_cast<decltype(RGB::b)>(rgb & 0xff) / 0xff
                                                                                       )));
                else if(warnings)
                    throw_parse_error(line_i, "Invalid karaoke color");
            }else if(warnings)
                throw_parse_error(line_i, "Invalid tag");
	}
    // Parse geometry and adds to SSB event object
    void parse_geometry(std::string& geometry, SSBGeometry::Type geometry_type, SSBEvent& ssb_event, unsigned long int line_i, bool warnings) throw(std::string){
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
                    std::string path_token;
                    SSBPath::Segment segments[3];
                    while(path_stream >> path_token)
                        // Set segment type
                        if(path_token == "m")
                            segments[0].type = SSBPath::SegmentType::MOVE_TO;
                        else if(path_token == "l")
                            segments[0].type = SSBPath::SegmentType::LINE_TO;
                        else if(path_token == "b")
                            segments[0].type = segments[1].type = segments[2].type = SSBPath::SegmentType::CURVE_TO;
                        else if(path_token == "a")
                            segments[0].type = segments[1].type = SSBPath::SegmentType::ARC_TO;
                        else if(path_token == "c"){
                            segments[0].type = SSBPath::SegmentType::CLOSE;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
                            path.push_back({SSBPath::SegmentType::CLOSE});
#pragma GCC diagnostic pop
                        }
                    // Get complete segment
                        else{
                            // Put token back in stream for rereading
                            path_stream.seekg(-static_cast<long int>(path_token.length()), std::istringstream::cur);
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
                    // Replace in string \t to 4 spaces
                    string_replace(geometry, "\t", "    ");
                    // Replace in string \n to real line breaks
                    string_replace(geometry, "\\n", "\n");
                    // Replace in string \{ to single {
                    string_replace(geometry, "\\{", "{");
                    // Insert SSBText as SSBObject to SSBEvent
                    ssb_event.objects.push_back(std::shared_ptr<SSBObject>(new SSBText(geometry)));
                }
                break;
		}
    }
}

void SSBParser::parse(std::string& script, bool warnings) throw(std::string){
    // File reading
    FileReader file(script);
    // File valid?
    if(file){
        // Current SSB section
        enum class SSBSection{NONE, META, FRAME, STYLES, EVENTS} section = SSBSection::NONE;
        // File line number (needed for warnings)
        unsigned long int line_i = 0;
        // File line buffer
        std::string line;
        // Line iteration
        while(file.getline(line)){
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
                                decltype(SSBFrame::width) width;
                                if(string_to_number(line.substr(7), width))
                                    this->ssb.frame.width = width;
                                else if(warnings)
                                    throw_parse_error(line_i, "Invalid frame width");
                            }else if(line.compare(0, 8, "Height: ") == 0){
                                decltype(SSBFrame::height) height;
                                if(string_to_number(line.substr(8), height))
                                    this->ssb.frame.height = height;
                                else if(warnings)
                                    throw_parse_error(line_i, "Invalid frame height");
                            }else if(warnings)
                                throw_parse_error(line_i, "Invalid frame field");
                            break;
                        case SSBSection::STYLES:
                            {
                                auto pos = line.find(": ");
                                if(pos != std::string::npos){
                                    this->ssb.styles[line.substr(0, pos)] = line.substr(pos+2);
                                }else if(warnings)
                                    throw_parse_error(line_i, "Invalid style format");
                            }
                            break;
                        case SSBSection::EVENTS:
                            {
                                // Output buffer
                                SSBEvent ssb_event;
                                // Split line into tokens
                                std::istringstream event_stream(line);
                                std::string event_token;
                                // Get start time
                                if(!std::getline(event_stream, event_token, '-') || !parse_time(event_token, ssb_event.start_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find start time");
                                    break;
                                }
                                // Get end time
                                if(!std::getline(event_stream, event_token, '|') || !parse_time(event_token, ssb_event.end_ms)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find end time");
                                    break;
                                }
                                // Check times
                                if(ssb_event.end_ms <= ssb_event.start_ms){
                                    if(warnings)
                                        throw_parse_error(line_i, "Invalid time range");
                                    break;
                                }
                                // Get style content for later text insertion
                                std::string style_content;
                                if(!std::getline(event_stream, event_token, '|') || !this->ssb.styles.count(event_token)){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find style");
                                    break;
                                }else
                                    style_content = this->ssb.styles[event_token];
                                // Skip note
                                if(!std::getline(event_stream, event_token, '|')){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find note");
                                    break;
                                }
                                // Get text
                                if(!event_stream.unget() || event_stream.get() != '|'){
                                    if(warnings)
                                        throw_parse_error(line_i, "Couldn't find text");
                                    break;
                                }
                                std::string text = std::getline(event_stream, event_token) ? style_content + event_token : style_content;
                                // Add inline styles to text
                                uint8_t macro_insert_count = 64;
                                std::string::size_type pos_start = 0, pos_end;
                                while(macro_insert_count && (pos_start = text.find("\\\\", pos_start)) != std::string::npos && (pos_end = text.find("\\\\", pos_start+2)) != std::string::npos){
                                    std::string macro = text.substr(pos_start + 2, pos_end - (pos_start + 2));
                                    if(this->ssb.styles.count(macro)){
                                        text.replace(pos_start, macro.length() + 4, this->ssb.styles[macro]);
                                        macro_insert_count--;  // Blocker to avoid infinite recursive macros
                                    }else
                                        pos_start = pos_end + 2;
                                }
                                // Parse text
                                pos_start = 0;
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
                                        if(!tags.empty())
                                            parse_tags(tags, ssb_event, geometry_type, line_i, warnings);
                                    // Evaluate geometry
                                    }else{
                                        // Search geometry end at tags bracket (unescaped) or text end
                                        pos_end = find_non_escaped_character(text, '{', pos_start);
                                        if(pos_end == std::string::npos)
                                            pos_end = text.length();
                                        // Parse geometry by type
                                        std::string geometry = text.substr(pos_start, pos_end - pos_start);
                                        if(!geometry.empty())
                                            parse_geometry(geometry, geometry_type, ssb_event, line_i, warnings);
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
