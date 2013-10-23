#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Meta information (no effect on rendering)
struct SSBMeta{
    std::string title, description, author;
    unsigned int version = 0;
};

// Destination frame (for scaling to different frame sizes than the intended one)
struct SSBFrame{
    int width = -1, height = -1;
};

// Any state or geometry for rendering
class SSBObject{
    public:
        enum class Type : char{TAG, GEOMETRY}type;
        virtual ~SSBObject(){}
    protected:
        SSBObject(Type type) : type(type){}
};

// Any state for rendering
class SSBTag : protected SSBObject{
    public:
        enum class Type : char{FONT_FAMILY}type;
        virtual ~SSBTag(){}
    protected:
        SSBTag(Type type) : SSBObject(SSBObject::Type::TAG), type(type){}
};

// Any geometry for rendering
class SSBGeometry : protected SSBObject{
    public:
        enum class Type : char{TEXT, SHAPE}type;
        virtual ~SSBGeometry(){}
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
    std::map<std::string, std::string>/*Name, Content*/ styles;
    std::vector<std::vector<std::shared_ptr<SSBObject>>>/*Line objects*/ lines;
};
