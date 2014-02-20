/*
Project: SSBRenderer
File: SSBData.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Coordinate precision
using SSBCoord = double;
// Time precision
using SSBTime = unsigned long int;
using SSBDuration = long int;

// Any state or geometry for rendering
class SSBObject{
    public:
        enum class Type : char{TAG, GEOMETRY} const type;
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
            TRANSFORM,
            COLOR,
            LINE_COLOR,
            ALPHA,
            LINE_ALPHA,
            TEXTURE,
            TEXFILL,
            BLEND,
            BLUR,
            STENCIL,
            FADE,
            ANIMATE,
            KARAOKE,
            KARAOKE_COLOR,
            KARAOKE_MODE
        } const type;
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
        } const type;
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
        float size;
        SSBFontSize(float size) : SSBTag(SSBTag::Type::FONT_SIZE), size(size){}
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
        enum class Join{ROUND, BEVEL} join;
        enum class Cap{ROUND, FLAT} cap;
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
        enum class Mode{FILL, WIRE, BOXED} mode;
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
        SSBCoord x, y;  // 'Unset' in case of maximum values
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
        enum class Mode{LTR, RTL, TTB} mode;
        SSBDirection(Mode mode) : SSBTag(SSBTag::Type::DIRECTION), mode(mode){}
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
        enum class Axis : char{XY, YX, Z} axis;
        double angle1, angle2;
        SSBRotate(Axis axis, double angle1, double angle2) : SSBTag(SSBTag::Type::ROTATE), axis(axis), angle1(angle1), angle2(angle2){}
        SSBRotate(double angle) : SSBTag(SSBTag::Type::ROTATE), axis(Axis::Z), angle1(angle){}
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

// RGB structure for colors
struct RGB{
    double r, g, b;
    RGB operator-(const RGB& value){
        return {this->r - value.r, this->g - value.g, this->b - value.b};
    }
    RGB operator*(const double& value){
        return {this->r * value, this->g * value, this->b * value};
    }
    RGB& operator+=(const RGB& value){
        this->r += value.r;
        this->g += value.g;
        this->b += value.b;
        return *this;
    }
    bool operator==(const RGB& value){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return this->r == value.r && this->g == value.g && this->b == value.b;
#pragma GCC diagnostic pop
    }
};

// Color state
class SSBColor : public SSBTag{
    public:
        RGB colors[4];
        SSBColor(double r, double g, double b) : SSBTag(SSBTag::Type::COLOR), colors({{r, g, b}, {r, g, b}, {r, g, b}, {r, g, b}}){}
        SSBColor(double r0, double g0, double b0, double r1, double g1, double b1) : SSBTag(SSBTag::Type::COLOR), colors({{r0, g0, b0}, {r1, g1, b1}, {r1, g1, b1}, {r0, g0, b0}}){}
        SSBColor(double r0, double g0, double b0, double r1, double g1, double b1, double r2, double g2, double b2, double r3, double g3, double b3) : SSBTag(SSBTag::Type::COLOR), colors({{r0, g0, b0}, {r1, g1, b1}, {r2, g2, b2}, {r3, g3, b3}}){}
};
class SSBLineColor : public SSBTag{
    public:
        RGB color;
        SSBLineColor(double r, double g, double b) : SSBTag(SSBTag::Type::LINE_COLOR), color({r, g, b}){}
};

// Alpha state
class SSBAlpha : public SSBTag{
    public:
        double alphas[4];
        SSBAlpha(double a) : SSBTag(SSBTag::Type::ALPHA), alphas{a, a, a, a}{}
        SSBAlpha(double a0, double a1) : SSBTag(SSBTag::Type::ALPHA), alphas{a0, a1, a1, a0}{}
        SSBAlpha(double a0, double a1, double a2, double a3) : SSBTag(SSBTag::Type::ALPHA), alphas{a0, a1, a2, a3}{}
};
class SSBLineAlpha : public SSBTag{
    public:
        double alpha;
        SSBLineAlpha(double a) : SSBTag(SSBTag::Type::LINE_ALPHA), alpha(a){}
};

// Texture state
class SSBTexture : public SSBTag{
    public:
        std::string filename;
        SSBTexture(std::string filename) : SSBTag(SSBTag::Type::TEXTURE), filename(filename){}
};

// Texture fill state
class SSBTexFill : public SSBTag{
    public:
        SSBCoord x, y;
        enum class WrapStyle{CLAMP, REPEAT, MIRROR, FLOW} wrap;
        SSBTexFill(SSBCoord x, SSBCoord y, WrapStyle wrap) : SSBTag(SSBTag::Type::TEXFILL), x(x), y(y), wrap(wrap){}
};

// Blend state
class SSBBlend : public SSBTag{
    public:
        enum class Mode{OVER, ADDITION, SUBTRACT, MULTIPLY, SCREEN, DIFFERENCES} mode;
        SSBBlend(Mode mode) : SSBTag(SSBTag::Type::BLEND), mode(mode){}
};

// Blur state
class SSBBlur : public SSBTag{
    public:
        enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
        SSBCoord x, y;
        SSBBlur(SSBCoord x, SSBCoord y) : SSBTag(SSBTag::Type::BLUR), type(Type::BOTH), x(x), y(y){}
        SSBBlur(Type type, SSBCoord xy) : SSBTag(SSBTag::Type::BLUR), type(type){
            switch(type){
                case Type::HORIZONTAL: this->x = xy; break;
                case Type::VERTICAL: this->y = xy; break;
                case Type::BOTH: this->x = this->y = xy; break;
            }
        }
};

// Stencil state
class SSBStencil : public SSBTag{
    public:
        enum class Mode{OFF, SET, UNSET, INSIDE, OUTSIDE} mode;
        SSBStencil(Mode mode) : SSBTag(SSBTag::Type::STENCIL), mode(mode){}
};

// Fade state
class SSBFade : public SSBTag{
    public:
        enum class Type : char{INFADE, OUTFADE, BOTH} type;
        SSBTime in, out;
        SSBFade(SSBTime in, SSBTime out) : SSBTag(SSBTag::Type::FADE), type(Type::BOTH), in(in), out(out){}
        SSBFade(Type type, SSBTime inout) : SSBTag(SSBTag::Type::FADE), type(type){
            switch(type){
                case Type::INFADE: this->in = inout; break;
                case Type::OUTFADE: this->out = inout; break;
                case Type::BOTH: this->in = this->out = inout; break;
            }
        }
};

// Animation state
class SSBAnimate : public SSBTag{
    public:
        SSBDuration start, end; // 'Unset' in case of maximum values
        std::string progress_formula;   // 'Unset' in case of emtpiness
        std::vector<std::shared_ptr<SSBObject>> objects;
        SSBAnimate(SSBDuration start, SSBDuration end, std::string progress_formula, std::vector<std::shared_ptr<SSBObject>> objects) : SSBTag(SSBTag::Type::ANIMATE), start(start), end(end), progress_formula(progress_formula), objects(objects){}
};

// Karaoke time state
class SSBKaraoke : public SSBTag{
    public:
        enum class Type{DURATION, SET} type;
        SSBTime time;
        SSBKaraoke(Type type, SSBTime time) : SSBTag(SSBTag::Type::KARAOKE), type(type), time(time){}
};

// Karaoke color state
class SSBKaraokeColor : public SSBTag{
    public:
        RGB color;
        SSBKaraokeColor(double r, double g, double b) : SSBTag(SSBTag::Type::KARAOKE_COLOR), color({r, g, b}){}
};

// Karaoke filling mode
class SSBKaraokeMode : public SSBTag{
    public:
        enum class Mode{FILL, SOLID, GLOW} mode;
        SSBKaraokeMode(Mode mode) : SSBTag(SSBTag::Type::KARAOKE_MODE), mode(mode){}
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
    unsigned int width = 0, height = 0;
};

// Event with rendering data
struct SSBEvent{
    SSBTime start_ms = 0, end_ms = 0;
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
