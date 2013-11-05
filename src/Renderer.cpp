#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <limits>
#include <muParser.h>
#define M_PI 3.14159265358979323846  // Missing in math header because of strict ANSI C

Renderer::Renderer(int width, int height, Colorspace format, std::string& script, bool warnings)
: width(width), height(height), format(format), ssb(SSBParser(script, warnings).data()){}

void Renderer::set_target(int width, int height, Colorspace format){
    this->width = width;
    this->height = height;
    this->format = format;
}

// Helper structures & functions for rendering
namespace{
    // Render state palette
    struct RenderStatePalette{
        // Font
        std::string font_family = "Arial";
        bool bold = false, italic = false, underline = false, strikeout = false;
        unsigned short int font_size = 30;
        double font_space_h = 0, font_space_v = 0;
        // Line
        double line_width = 2;
        cairo_line_join_t line_join = CAIRO_LINE_JOIN_ROUND;
        cairo_line_cap_t line_cap = CAIRO_LINE_CAP_ROUND;
        double dash_offset = 0;
        std::vector<double> dashes;
        // Geometry
        SSBMode::Mode mode = SSBMode::Mode::FILL;
        std::string deform_x, deform_y;
        // Position
        double pos_x = std::numeric_limits<double>::max(), pos_y = std::numeric_limits<double>::max();
        SSBAlign::Align align = SSBAlign::Align::CENTER_BOTTOM;
        double margin_h = 0, margin_v = 0;
        double direction_angle = 0;
        // Transformation
        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
        // Color
        std::vector<SSBColor::RGB> colors;
        std::vector<double> alphas;
        std::string texture;
        double texture_x = 0, texture_y = 0;
        cairo_extend_t wrap_style = CAIRO_EXTEND_NONE;
        std::vector<SSBColor::RGB> line_colors;
        std::vector<double> line_alphas;
        std::string line_texture;
        double line_texture_x = 0, line_texture_y = 0;
        cairo_extend_t line_wrap_style = CAIRO_EXTEND_NONE;
        // Rastering
        SSBBlend::Mode blend_mode = SSBBlend::Mode::OVER;
        double blur_h = 0, blur_v = 0;
        SSBClip::Mode clip_mode = SSBClip::Mode::CLEAR;
        // Karaoke
        long int karaoke = -1;
    };
    // Updates render state palette by SSB tag
    void tag_to_render_state_palette(SSBTag* tag, RenderStatePalette& rsp){
        switch(tag->type){
            case SSBTag::Type::FONT_FAMILY:
                rsp.font_family = dynamic_cast<SSBFontFamily*>(tag)->family;
                break;
            case SSBTag::Type::FONT_STYLE:
                {
                    SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(tag);
                    rsp.bold = font_style->bold;
                    rsp.italic = font_style->italic;
                    rsp.underline = font_style->underline;
                    rsp.strikeout = font_style->strikeout;
                }
                break;
            case SSBTag::Type::FONT_SIZE:
                rsp.font_size = dynamic_cast<SSBFontSize*>(tag)->size;
                break;
            case SSBTag::Type::FONT_SPACE:
                {
                    SSBFontSpace* font_space = dynamic_cast<SSBFontSpace*>(tag);
                    switch(font_space->type){
                        case SSBFontSpace::Type::HORIZONTAL: rsp.font_space_h = font_space->x; break;
                        case SSBFontSpace::Type::VERTICAL: rsp.font_space_v = font_space->y; break;
                        case SSBFontSpace::Type::BOTH: rsp.font_space_h = font_space->x; rsp.font_space_v = font_space->y; break;
                    }
                }
                break;
            case SSBTag::Type::LINE_WIDTH:
                rsp.line_width = dynamic_cast<SSBLineWidth*>(tag)->width;
                break;
            case SSBTag::Type::LINE_STYLE:
                {
                    SSBLineStyle* line_style = dynamic_cast<SSBLineStyle*>(tag);
                    switch(line_style->join){
                        case SSBLineStyle::Join::MITER: rsp.line_join = CAIRO_LINE_JOIN_MITER; break;
                        case SSBLineStyle::Join::BEVEL: rsp.line_join = CAIRO_LINE_JOIN_BEVEL; break;
                        case SSBLineStyle::Join::ROUND: rsp.line_join = CAIRO_LINE_JOIN_ROUND; break;
                    }
                    switch(line_style->cap){
                        case SSBLineStyle::Cap::FLAT: rsp.line_cap = CAIRO_LINE_CAP_BUTT; break;
                        case SSBLineStyle::Cap::SQUARE: rsp.line_cap = CAIRO_LINE_CAP_SQUARE; break;
                        case SSBLineStyle::Cap::ROUND: rsp.line_cap = CAIRO_LINE_CAP_ROUND; break;
                    }
                }
                break;
            case SSBTag::Type::LINE_DASH:
                {
                    SSBLineDash* line_dash = dynamic_cast<SSBLineDash*>(tag);
                    rsp.dash_offset = line_dash->offset;
                    rsp.dashes.resize(line_dash->dashes.size());
                    std::copy(line_dash->dashes.begin(), line_dash->dashes.end(), rsp.dashes.begin());
                }
                break;
            case SSBTag::Type::MODE:
                rsp.mode = dynamic_cast<SSBMode*>(tag)->mode;
                break;
            case SSBTag::Type::DEFORM:
                {
                    SSBDeform* deform = dynamic_cast<SSBDeform*>(tag);
                    rsp.deform_x = deform->formula_x;
                    rsp.deform_y = deform->formula_y;
                }
                break;
            case SSBTag::Type::POSITION:
                {
                    SSBPosition* pos = dynamic_cast<SSBPosition*>(tag);
                    rsp.pos_x = pos->x;
                    rsp.pos_y = pos->y;
                }
                break;
            case SSBTag::Type::ALIGN:
                rsp.align = dynamic_cast<SSBAlign*>(tag)->align;
                break;
            case SSBTag::Type::MARGIN:
                {
                    SSBMargin* margin = dynamic_cast<SSBMargin*>(tag);
                    switch(margin->type){
                        case SSBMargin::Type::HORIZONTAL: rsp.margin_h = margin->x; break;
                        case SSBMargin::Type::VERTICAL: rsp.margin_v = margin->y; break;
                        case SSBMargin::Type::BOTH: rsp.margin_h = margin->x; rsp.margin_v = margin->y; break;
                    }
                }
                break;
            case SSBTag::Type::DIRECTION:
                rsp.direction_angle = dynamic_cast<SSBDirection*>(tag)->angle;
                break;
            case SSBTag::Type::IDENTITY:
                cairo_matrix_init_identity(&rsp.matrix);
                break;
            case SSBTag::Type::TRANSLATE:
                {
                    SSBTranslate* translation = dynamic_cast<SSBTranslate*>(tag);
                    switch(translation->type){
                        case SSBTranslate::Type::HORIZONTAL: cairo_matrix_translate(&rsp.matrix, translation->x, 0); break;
                        case SSBTranslate::Type::VERTICAL: cairo_matrix_translate(&rsp.matrix, 0, translation->y); break;
                        case SSBTranslate::Type::BOTH: cairo_matrix_translate(&rsp.matrix, translation->x, translation->y); break;
                    }
                }
                break;
            case SSBTag::Type::SCALE:
                {
                    SSBScale* scale = dynamic_cast<SSBScale*>(tag);
                    switch(scale->type){
                        case SSBScale::Type::HORIZONTAL: cairo_matrix_scale(&rsp.matrix, scale->x, 0); break;
                        case SSBScale::Type::VERTICAL: cairo_matrix_scale(&rsp.matrix, 0, scale->y); break;
                        case SSBScale::Type::BOTH: cairo_matrix_scale(&rsp.matrix, scale->x, scale->y); break;
                    }
                }
                break;
            /*case SSBTag::Type:::
                rsp. = dynamic_cast<SSB*>(tag);
                break;*/

#pragma message "Implent updater for render state palette by tag"
        }
    }
    // Converts SSB geometry to cairo path
    void geometry_to_path(SSBGeometry* geometry, RenderStatePalette& rsp, cairo_t* ctx){
        switch(geometry->type){
            case SSBGeometry::Type::POINTS:
                if(rsp.line_width > 1)
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points){
                        cairo_new_sub_path(ctx);
                        cairo_arc(ctx, point.x, point.y, rsp.line_width / 2, 0, M_PI * 2);
                        cairo_close_path(ctx);
                    }
                else
                    for(const Point& point : dynamic_cast<SSBPoints*>(geometry)->points)
                        cairo_rectangle(ctx, point.x, point.y, rsp.line_width, rsp.line_width);   // Creates a move + lines + close = closed shape
                break;
            case SSBGeometry::Type::PATH:
                {
                    const std::vector<SSBPath::Segment>& segments = dynamic_cast<SSBPath*>(geometry)->segments;
                    for(size_t i = 0; i < segments.size();)
                        switch(segments[i].type){
                            case SSBPath::SegmentType::MOVE_TO:
                                cairo_move_to(ctx, segments[i].point.x, segments[i].point.y);
                                ++i;
                                break;
                            case SSBPath::SegmentType::LINE_TO:
                                cairo_line_to(ctx, segments[i].point.x, segments[i].point.y);
                                ++i;
                                break;
                            case SSBPath::SegmentType::CURVE_TO:
                                cairo_curve_to(ctx,
                                                segments[i].point.x, segments[i].point.y,
                                                segments[i+1].point.x, segments[i+1].point.y,
                                                segments[i+2].point.x, segments[i+2].point.y);
                                i += 3;
                                break;
                            case SSBPath::SegmentType::ARC_TO:
                                if(cairo_has_current_point(ctx)){
                                    double lx, ly; cairo_get_current_point(ctx, &lx, &ly);
                                    double xc = segments[i].point.x,
                                            yc = segments[i].point.y,
                                            r = hypot(ly - yc, lx - xc),
                                            angle1 = atan2(ly - yc, lx - xc),
                                            angle2 = angle1 + segments[i+1].angle * M_PI / 180.0L;
                                    if(angle2 > angle1)
                                        cairo_arc(ctx,
                                                    xc, yc,
                                                    r,
                                                    angle1, angle2);
                                    else
                                        cairo_arc_negative(ctx,
                                                    xc, yc,
                                                    r,
                                                    angle1, angle2);
                                }
                                i += 2;
                                break;
                            case SSBPath::SegmentType::CLOSE:
                                cairo_close_path(ctx);
                                ++i;
                                break;
                        }
                }
                break;
            case SSBGeometry::Type::TEXT:
    #pragma message "Implent SSB text paths"
                break;
        }
    }
}

void Renderer::render(unsigned char* image, int pitch, unsigned long long start_ms) noexcept{
    // Iterate through SSB events
    for(SSBEvent& event : this->ssb.events)
        // Process active SSB event
        if(start_ms >= event.start_ms && start_ms < event.end_ms){
            // Create render state palette for rendering behaviour
            RenderStatePalette rsp;
            // Process SSB objects of event
            for(std::shared_ptr<SSBObject>& obj : event.objects)
                if(obj->type == SSBObject::Type::TAG){
                    // Apply tag to render state palette
                    tag_to_render_state_palette(dynamic_cast<SSBTag*>(obj.get()), rsp);
                }else{  // obj->type == SSBObject::Type::GEOMETRY
                    // Set transformations

                    // Apply geometry to image path
                    geometry_to_path(dynamic_cast<SSBGeometry*>(obj.get()), rsp, this->path_buffer);
                    // Create image with fitting size

                    // Draw on image

                    // Clear image path

                    // Calculate image rectangle for blending on frame

                    // Blend image on frame

                }
#pragma message "Implent SSB rendering"
        }
}
