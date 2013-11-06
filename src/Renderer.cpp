#include "Renderer.hpp"
#include "SSBParser.hpp"
#include <limits>
#include <algorithm>
#include <muParser.h>
#define M_PI 3.14159265358979323846  // Missing in math header because of strict ANSI C
#define DEG_TO_RAD(x) (x / 180.0L * M_PI)

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
        double pos_x = std::numeric_limits<double>::max(), pos_y = std::numeric_limits<double>::max();  // 'Unset' in case of maximum values
        SSBAlign::Align align = SSBAlign::Align::CENTER_BOTTOM;
        double margin_h = 0, margin_v = 0;
        double direction_angle = 0;
        // Transformation
        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
        // Color
        std::vector<SSBColor::RGB> colors = {{1,1,1}};
        std::vector<double> alphas = {1};
        std::string texture;
        double texture_x = 0, texture_y = 0;
        cairo_extend_t wrap_style = CAIRO_EXTEND_NONE;
        std::vector<SSBColor::RGB> line_colors = {{0,0,0}};
        std::vector<double> line_alphas = {1};
        std::string line_texture;
        double line_texture_x = 0, line_texture_y = 0;
        cairo_extend_t line_wrap_style = CAIRO_EXTEND_NONE;
        // Rastering
        SSBBlend::Mode blend_mode = SSBBlend::Mode::OVER;
        double blur_h = 0, blur_v = 0;
        SSBClip::Mode clip_mode = SSBClip::Mode::CLEAR;
        // Karaoke
        long int karaoke_start = -1, karaoke_duration = 0;
    };
    // Updates render state palette by SSB tag
    inline void tag_to_render_state_palette(SSBTag* tag, SSBTime inner_ms, SSBTime inner_duration, RenderStatePalette& rsp){
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
            case SSBTag::Type::ROTATE:
                {
                    SSBRotate* rotation = dynamic_cast<SSBRotate*>(tag);
                    switch(rotation->axis){
                        case SSBRotate::Axis::Z: cairo_matrix_rotate(&rsp.matrix, DEG_TO_RAD(rotation->angle1)); break;
                        case SSBRotate::Axis::XY:
                        {
                            double rad_x = DEG_TO_RAD(rotation->angle1), rad_y = DEG_TO_RAD(rotation->angle2);
                            cairo_matrix_t tmp_matrix = {cos(rad_y), 0, sin(rad_x) * sin(rad_y), cos(rad_x), 0, 0};
                            cairo_matrix_multiply(&rsp.matrix, &tmp_matrix, &rsp.matrix);
                        }
                        break;
                        case SSBRotate::Axis::YX:
                        {
                            double rad_y = DEG_TO_RAD(rotation->angle1), rad_x = DEG_TO_RAD(rotation->angle2);
                            cairo_matrix_t tmp_matrix = {cos(rad_y), -sin(rad_x) * -sin(rad_y), 0, cos(rad_x), 0, 0};
                            cairo_matrix_multiply(&rsp.matrix, &tmp_matrix, &rsp.matrix);
                        }
                        break;
                    }
                }
                break;
            case SSBTag::Type::SHEAR:
                {
                    SSBShear* shear = dynamic_cast<SSBShear*>(tag);
                    cairo_matrix_t tmp_matrix;
                    switch(shear->type){
                        case SSBShear::Type::HORIZONTAL: tmp_matrix = {1, 0, shear->x, 1, 0, 0}; break;
                        case SSBShear::Type::VERTICAL: tmp_matrix = {1, shear->y, 0, 1, 0, 0}; break;
                        case SSBShear::Type::BOTH: tmp_matrix = {1, shear->y, shear->x, 1, 0, 0}; break;
                    }
                    cairo_matrix_multiply(&rsp.matrix, &tmp_matrix, &rsp.matrix);
                }
                break;
            case SSBTag::Type::TRANSFORM:
                {
                    SSBTransform* transform = dynamic_cast<SSBTransform*>(tag);
                    cairo_matrix_t tmp_matrix = {transform->xx, transform->yx, transform->xy, transform->yy, transform->x0, transform->y0};
                    cairo_matrix_multiply(&rsp.matrix, &tmp_matrix, &rsp.matrix);
                }
                break;
            case SSBTag::Type::COLOR:
                {
                    SSBColor* color = dynamic_cast<SSBColor*>(tag);
                    if(color->target == SSBColor::Target::FILL)
                        if(color->colors[1].r < 0){
                            rsp.colors.resize(1);
                            rsp.colors[0] = color->colors[0];
                        }else{
                            rsp.colors.resize(4);
                            rsp.colors[0] = color->colors[0];
                            rsp.colors[1] = color->colors[1];
                            rsp.colors[2] = color->colors[2];
                            rsp.colors[3] = color->colors[3];
                        }
                    else
                        if(color->colors[1].r < 0){
                            rsp.line_colors.resize(1);
                            rsp.line_colors[0] = color->colors[0];
                        }else{
                            rsp.line_colors.resize(4);
                            rsp.line_colors[0] = color->colors[0];
                            rsp.line_colors[1] = color->colors[1];
                            rsp.line_colors[2] = color->colors[2];
                            rsp.line_colors[3] = color->colors[3];
                        }
                }
                break;
            case SSBTag::Type::ALPHA:
                {
                    SSBAlpha* alpha = dynamic_cast<SSBAlpha*>(tag);
                    if(alpha->target == SSBAlpha::Target::FILL)
                        if(alpha->alphas[1] < 0){
                            rsp.alphas.resize(1);
                            rsp.alphas[0] = alpha->alphas[0];
                        }else{
                            rsp.alphas.resize(4);
                            rsp.alphas[0] = alpha->alphas[0];
                            rsp.alphas[1] = alpha->alphas[1];
                            rsp.alphas[2] = alpha->alphas[2];
                            rsp.alphas[3] = alpha->alphas[3];
                        }
                    else
                        if(alpha->alphas[1] < 0){
                            rsp.line_alphas.resize(1);
                            rsp.line_alphas[0] = alpha->alphas[0];
                        }else{
                            rsp.line_alphas.resize(4);
                            rsp.line_alphas[0] = alpha->alphas[0];
                            rsp.line_alphas[1] = alpha->alphas[1];
                            rsp.line_alphas[2] = alpha->alphas[2];
                            rsp.line_alphas[3] = alpha->alphas[3];
                        }
                }
                break;
            case SSBTag::Type::TEXTURE:
                {
                    SSBTexture* texture = dynamic_cast<SSBTexture*>(tag);
                    if(texture->target == SSBTexture::Target::FILL)
                        rsp.texture = texture->filename;
                    else
                        rsp.line_texture = texture->filename;
                }
                break;
            case SSBTag::Type::TEXFILL:
                {
                    SSBTexFill* texfill = dynamic_cast<SSBTexFill*>(tag);
                    if(texfill->target == SSBTexFill::Target::FILL){
                        rsp.texture_x = texfill->x;
                        rsp.texture_y = texfill->y;
                        switch(texfill->wrap){
                            case SSBTexFill::WrapStyle::CLAMP: rsp.wrap_style = CAIRO_EXTEND_NONE; break;
                            case SSBTexFill::WrapStyle::REPEAT: rsp.wrap_style = CAIRO_EXTEND_REPEAT; break;
                            case SSBTexFill::WrapStyle::MIRROR: rsp.wrap_style = CAIRO_EXTEND_REFLECT; break;
                            case SSBTexFill::WrapStyle::FLOW: rsp.wrap_style = CAIRO_EXTEND_PAD; break;
                        }
                    }else{
                        rsp.line_texture_x = texfill->x;
                        rsp.line_texture_y = texfill->y;
                        switch(texfill->wrap){
                            case SSBTexFill::WrapStyle::CLAMP: rsp.line_wrap_style = CAIRO_EXTEND_NONE; break;
                            case SSBTexFill::WrapStyle::REPEAT: rsp.line_wrap_style = CAIRO_EXTEND_REPEAT; break;
                            case SSBTexFill::WrapStyle::MIRROR: rsp.line_wrap_style = CAIRO_EXTEND_REFLECT; break;
                            case SSBTexFill::WrapStyle::FLOW: rsp.line_wrap_style = CAIRO_EXTEND_PAD; break;
                        }
                    }
                }
                break;
            case SSBTag::Type::BLEND:
                rsp.blend_mode = dynamic_cast<SSBBlend*>(tag)->mode;
                break;
            case SSBTag::Type::BLUR:
                {
                    SSBBlur* blur = dynamic_cast<SSBBlur*>(tag);
                    switch(blur->type){
                        case SSBBlur::Type::HORIZONTAL: rsp.blur_h = blur->x; break;
                        case SSBBlur::Type::VERTICAL: rsp.blur_v = blur->y; break;
                        case SSBBlur::Type::BOTH: rsp.blur_h = blur->x; rsp.blur_v = blur->y; break;
                    }
                }
                break;
            case SSBTag::Type::CLIP:
                rsp.clip_mode = dynamic_cast<SSBClip*>(tag)->mode;
                break;
            case SSBTag::Type::KARAOKE:
                {
                    SSBKaraoke* karaoke = dynamic_cast<SSBKaraoke*>(tag);
                    switch(karaoke->type){
                        case SSBKaraoke::Type::DURATION:
                            if(rsp.karaoke_start < 0)
                                rsp.karaoke_start = 0;
                            rsp.karaoke_start += rsp.karaoke_duration;
                            rsp.karaoke_duration = karaoke->time;
                            break;
                        case SSBKaraoke::Type::SET:
                            rsp.karaoke_start = karaoke->time;
                            rsp.karaoke_duration = 0;
                            break;
                    }
                }
                break;
            case SSBTag::Type::FADE:
                {
                    SSBFade* fade = dynamic_cast<SSBFade*>(tag);
                    double progress = -1;
                    if(inner_ms < fade->in)
                        progress = static_cast<double>(inner_ms) / fade->in;
                    else{
                        decltype(inner_ms) inv_inner_ms = inner_duration - inner_ms;
                        if(inv_inner_ms < fade->out)
                            progress = static_cast<double>(inv_inner_ms) / fade->out;
                    }
                    if(progress >= 0){
                        std::for_each(rsp.alphas.begin(), rsp.alphas.end(), [&progress](double& a){a *= progress;});
                        std::for_each(rsp.line_alphas.begin(), rsp.line_alphas.end(), [&progress](double& a){a *= progress;});
                    }
                }
                break;
            case SSBTag::Type::ANIMATE:
                {
                    SSBAnimate* animate = dynamic_cast<SSBAnimate*>(tag);
                    // Calculate start & end time
                    SSBTime animate_start, animate_end;
                    if(animate->start == std::numeric_limits<decltype(animate->start)>::max()){
                        animate_start = 0;
                        animate_end = inner_duration;
                    }else{
                        animate_start = animate->start >= 0 ? animate->start : inner_duration + animate->start;
                        animate_end = animate->end > 0 ? animate->end : inner_duration + animate->end;
                    }
                    // Calculate progress
                    double progress = inner_ms < animate_start ? 0 : (inner_ms > animate_end ? 1 : static_cast<double>(inner_ms - animate_start) / (animate_end - animate_start));
                    // Recalulate progress by formula
                    if(!animate->progress_formula.empty()){
                        mu::Parser parser;
                        parser.SetExpr(animate->progress_formula);
                        parser.DefineConst("t", progress);
                        try{
                            progress = parser.Eval();
                        }catch(...){}
                    }
                    // Interpolate tags & set to render state palette
                    for(std::shared_ptr<SSBObject>& obj : animate->objects){
                        SSBTag* animate_tag = dynamic_cast<SSBTag*>(obj.get());
                        switch(animate_tag->type){
                            case SSBTag::Type::FONT_FAMILY:
                                if(progress > 0)
                                    rsp.font_family = dynamic_cast<SSBFontFamily*>(animate_tag)->family;
                                break;
                            case SSBTag::Type::FONT_STYLE:
                                if(progress > 0){
                                    SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(animate_tag);
                                    rsp.bold = font_style->bold;
                                    rsp.italic = font_style->italic;
                                    rsp.underline = font_style->underline;
                                    rsp.strikeout = font_style->strikeout;
                                }
                                break;
                            case SSBTag::Type::FONT_SIZE:
                                rsp.font_size = rsp.font_size + progress * (static_cast<short int>(dynamic_cast<SSBFontSize*>(animate_tag)->size) - rsp.font_size);
                                break;
#pragma message "Implent updater for render state palette by tag"
                        }
                    }
                }
                break;
        }
    }
    // Converts SSB geometry to cairo path
    inline void geometry_to_path(SSBGeometry* geometry, RenderStatePalette& rsp, cairo_t* ctx){
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
                                            angle2 = angle1 + DEG_TO_RAD(segments[i+1].angle);
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

void Renderer::render(unsigned char* image, int pitch, unsigned long int start_ms) noexcept{
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
                    tag_to_render_state_palette(dynamic_cast<SSBTag*>(obj.get()), start_ms - event.start_ms, event.end_ms - event.start_ms, rsp);
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
