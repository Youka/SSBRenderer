#pragma once

#include <string>
#include <map>
#include <vector>

// Meta information (no effect on rendering)
struct SSBMeta{
    std::string title, description, author;
    unsigned int version = 0;
};

// Destination frame (for scaling to different frame sizes than the intended one)
struct SSBFrame{
    int width = -1, height = -1;
};

// State or geometry for rendering
class SSBObject{
    public:
        enum class Type{TAG, GEOMETRY}type;
    protected:
        SSBObject(Type type) : type(type){}
};

// State for rendering
class SSBTag : protected SSBObject{
    public:
        enum class Type{FONT_FAMILY}type;
    protected:
        SSBTag(Type type) : SSBObject(SSBObject::Type::TAG), type(type){}
};

// Geometry for rendering
class SSBGeometry : protected SSBObject{
    public:
        enum class Type{TEXT}type;
    protected:
        SSBGeometry(Type type) : SSBObject(SSBObject::Type::GEOMETRY), type(type){}
};

// Font family state
class SSBFontFamily : protected SSBTag{
    public:
        std::string family;
        SSBFontFamily(std::string family) : SSBTag(SSBTag::Type::FONT_FAMILY), family(family){}
};

// Text geometry
class SSBText : protected SSBGeometry{
    public:
        std::string text;
        SSBText(std::string text) : SSBGeometry(SSBGeometry::Type::TEXT), text(text){}
};

// Relevant SSB data for rendering & feedback
struct SSBData{
    SSBMeta meta;
    SSBFrame frame;
    std::map<std::string, std::string> styles;
    std::vector<std::vector<SSBObject*>> lines;
};
