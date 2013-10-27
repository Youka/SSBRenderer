#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Any state or geometry for rendering
class SSBObject{
    public:
        enum class Type : char{TAG, GEOMETRY}type;
        virtual ~SSBObject(){}
    protected:
        SSBObject(Type type) : type(type){}
};

// Any state for rendering
class SSBTag : public SSBObject{
    public:
        enum class Type : char{FONT_FAMILY, FONT_STYLE, FONT_SIZE}type;
        virtual ~SSBTag() = default;
    protected:
        SSBTag(Type type) : SSBObject(SSBObject::Type::TAG), type(type){}
};

// Any geometry for rendering
class SSBGeometry : public SSBObject{
    public:
        enum class Type : char{POINTS, PATH, TEXT}type;
        virtual ~SSBGeometry() = default;
    protected:
        SSBGeometry(Type type) : SSBObject(SSBObject::Type::GEOMETRY), type(type){}
};

// Font family state
class SSBFontFamily : public SSBTag{
    public:
        std::string family;
        SSBFontFamily(std::string family) : SSBTag(SSBTag::Type::FONT_FAMILY), family(family){}
};

// Font style state
class SSBFontStyle : public SSBTag{
    public:
        bool bold, italic, underline, strikeout;
        SSBFontStyle(bool bold, bool italic, bool underline, bool strikeout) : SSBTag(SSBTag::Type::FONT_STYLE), bold(bold), italic(italic), underline(underline), strikeout(strikeout){}
};

// Font size state
class SSBFontSize : public SSBTag{
    public:
        unsigned int size;
        SSBFontSize(unsigned int size) : SSBTag(SSBTag::Type::FONT_SIZE), size(size){}
};

// Point structure for geometries
struct Point{float x,y;};

// Points geometry
class SSBPoints : public SSBGeometry{
    public:
        std::vector<Point> points;
        SSBPoints() : SSBGeometry(SSBGeometry::Type::POINTS){}
};

// Path geometry
class SSBPath : public SSBGeometry{
    public:
        enum class SegmentType : char{MOVE_TO, LINE_TO, CURVE_TO, ARC_TO, CLOSE};
        struct Segment{
            SegmentType type;
            union{
                Point point;
                double angle;
            } value;
        };
        std::vector<Segment> segments;
        SSBPath() : SSBGeometry(SSBGeometry::Type::PATH){}
};

// Text geometry
class SSBText : public SSBGeometry{
    public:
        std::string text;
        SSBText(std::string text) : SSBGeometry(SSBGeometry::Type::TEXT), text(text){}
};

// Meta information (no effect on rendering)
struct SSBMeta{
    std::string title, description, author, version;
};

// Destination frame (for scaling to different frame sizes than the intended one)
struct SSBFrame{
    int width = -1, height = -1;
};

// Line with rendering data
struct SSBLine{
    unsigned long long start_ms = 0, end_ms = 0;
    bool static_tags = false;
    std::vector<std::shared_ptr<SSBObject>> objects;
};

// Relevant SSB data for rendering & feedback
struct SSBData{
    SSBMeta meta;
    SSBFrame frame;
    std::map<std::string, std::string>/*Name, Content*/ styles;
    std::vector<SSBLine> lines;
};
