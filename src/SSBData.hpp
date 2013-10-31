#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Coordinate precision
using SSBCoord = float;

// Any state or geometry for rendering
class SSBObject{
    public:
        enum class Type : char{TAG, GEOMETRY} type;
        SSBObject(const SSBObject&) = delete;
        SSBObject& operator =(const SSBObject&) = delete;
        virtual ~SSBObject(){}
    protected:
        SSBObject(Type type) : type(type){}
};

// Any state for rendering
class SSBTag : public SSBObject{
    public:
        enum class Type : char{
            FONT_FAMILY,
            FONT_STYLE,
            FONT_SIZE,
            FONT_SPACE,
            LINE_WIDTH,
            LINE_STYLE,
            LINE_DASH,
            MODE,
            DEFORM,
            POSITION,
            ALIGN,
            MARGIN,
            DIRECTION,
            IDENTITY,
            TRANSLATE,
            SCALE,
            ROTATE,
            SHEAR,
            TRANSFORM
        } type;
        SSBTag(const SSBTag&) = delete;
        SSBTag& operator =(const SSBTag&) = delete;
        virtual ~SSBTag() = default;
    protected:
        SSBTag(Type type) : SSBObject(SSBObject::Type::TAG), type(type){}
};

// Any geometry for rendering
class SSBGeometry : public SSBObject{
    public:
        enum class Type : char{
            POINTS,
            PATH,
            TEXT
        } type;
        SSBGeometry(const SSBGeometry&) = delete;
        SSBGeometry& operator =(const SSBGeometry&) = delete;
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

// Font space state
class SSBFontSpace : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        SSBCoord x, y;
        SSBFontSpace(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::FONT_SPACE), type(Type::BOTH), x(x), y(y){}
        SSBFontSpace(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::FONT_SPACE), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Line width state
class SSBLineWidth : public SSBTag{
    public:
        SSBCoord width;
        SSBLineWidth(SSBCoord width) : SSBTag(SSBTag::Type::LINE_WIDTH), width(width){}
};

// Line style state
class SSBLineStyle : public SSBTag{
    public:
        enum class Join{MITER, ROUND, BEVEL} join;
        enum class Cap{FLAT, ROUND, SQUARE} cap;
        SSBLineStyle(Join join, Cap cap) : SSBTag(SSBTag::Type::LINE_STYLE), join(join), cap(cap){}
};

// Line dash pattern state
class SSBLineDash : public SSBTag{
    public:
        SSBCoord offset;
        std::vector<SSBCoord> dashes;
        SSBLineDash(SSBCoord offset, std::vector<SSBCoord> dashes) : SSBTag(SSBTag::Type::LINE_DASH), offset(offset), dashes(dashes){}
};

// Painting mode state
class SSBMode : public SSBTag{
    public:
        enum class Mode{FILL, WIRE} mode;
        SSBMode(Mode mode) : SSBTag(SSBTag::Type::MODE), mode(mode){}
};

// Deforming state
class SSBDeform : public SSBTag{
    public:
        std::string formula_x, formula_y;
        SSBDeform(std::string formula_x, std::string formula_y) : SSBTag(SSBTag::Type::DEFORM), formula_x(formula_x), formula_y(formula_y){}
};

// Position state
class SSBPosition : public SSBTag{
    public:
        SSBCoord x, y;
        SSBPosition(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::POSITION), x(x), y(y){}
};

// Alignment state
class SSBAlign : public SSBTag{
    public:
        enum Align : char{
            LEFT_BOTTOM = 1,
            CENTER_BOTTOM, // = 2
            RIGHT_BOTTOM, // = 3
            LEFT_MIDDLE, // = 4
            CENTER_MIDDLE, // = 5
            RIGHT_MIDDLE, // = 6
            LEFT_TOP, // = 7
            CENTER_TOP, // = 8
            RIGHT_TOP // = 9
        } align;
        SSBAlign(Align align) : SSBTag(SSBTag::Type::ALIGN), align(align){}
};

// Margin state
class SSBMargin : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        SSBCoord x, y;
        SSBMargin(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::MARGIN), type(Type::BOTH), x(x), y(y){}
        SSBMargin(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::MARGIN), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Direction state
class SSBDirection : public SSBTag{
    public:
        double angle;
        SSBDirection(double angle) : SSBTag(SSBTag::Type::DIRECTION), angle(angle){}
};

// Identity state
class SSBIdentity : public SSBTag{
    public:
        SSBIdentity() : SSBTag(SSBTag::Type::IDENTITY){}
};

// Translation state
class SSBTranslate : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        SSBCoord x, y;
        SSBTranslate(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::TRANSLATE), type(Type::BOTH), x(x), y(y){}
        SSBTranslate(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::TRANSLATE), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Scale state
class SSBScale : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        double x, y;
        SSBScale(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::SCALE), type(Type::BOTH), x(x), y(y){}
        SSBScale(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::SCALE), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Rotation state
class SSBRotate : public SSBTag{
    public:
        enum class Axis : char{X, Y, Z} axis;
        double angle;
        SSBRotate(Axis axis, double angle) : SSBTag(SSBTag::Type::ROTATE), axis(axis), angle(angle){}
};

// Shear state
class SSBShear : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        double x, y;
        SSBShear(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::SHEAR), type(Type::BOTH), x(x), y(y){}
        SSBShear(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::SHEAR), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Transform state
class SSBTransform : public SSBTag{
    public:
        double xx, yx, xy, yy, x0, y0;
        SSBTransform(double xx, double yx, double xy, double yy, double x0, double y0) : SSBTag(SSBTag::Type::TRANSFORM), xx(xx), yx(yx), xy(xy), yy(yy), x0(x0), y0(y0){}
};

// Point structure for geometries
struct Point{SSBCoord x,y;};

// Points geometry
class SSBPoints : public SSBGeometry{
    public:
        std::vector<Point> points;
        SSBPoints(std::vector<Point> points) : SSBGeometry(SSBGeometry::Type::POINTS), points(points){}
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
            };
        };
        std::vector<Segment> segments;
        SSBPath(std::vector<Segment> segments) : SSBGeometry(SSBGeometry::Type::PATH), segments(segments){}
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

// Event with rendering data
struct SSBEvent{
    unsigned long long start_ms = 0, end_ms = 0;
    bool static_tags = true;
    std::vector<std::shared_ptr<SSBObject>> objects;
};

// Relevant SSB data for rendering & feedback
struct SSBData{
    SSBMeta meta;
    SSBFrame frame;
    std::map<std::string, std::string>/*Name, Content*/ styles;
    std::vector<SSBEvent> events;
};
