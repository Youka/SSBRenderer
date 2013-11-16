/*
Project: SSBRenderer
File: RenderState.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "SSBData.hpp"
#include <cairo.h>
#include <algorithm>
#include <muParser.h>
#define M_PI 3.14159265358979323846  // Missing in math header because of strict ANSI C
#define DEG_TO_RAD(x) (x / 180.0 * M_PI)

namespace{
    // Render state
    struct RenderState{
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
        double deform_progress = 0;
        // Position
        double pos_x = std::numeric_limits<double>::max(), pos_y = std::numeric_limits<double>::max();  // 'Unset' in case of maximum values
        SSBAlign::Align align = SSBAlign::Align::CENTER_BOTTOM;
        double margin_h = 0, margin_v = 0;
        double direction_angle = 0;
        // Transformation
        cairo_matrix_t matrix = {1, 0, 0, 1, 0, 0};
        // Color
        std::vector<RGB> colors = {{1,1,1}};
        std::vector<double> alphas = {1};
        std::string texture;
        double texture_x = 0, texture_y = 0;
        cairo_extend_t wrap_style = CAIRO_EXTEND_NONE;
        std::vector<RGB> line_colors = {{0,0,0}};
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
    // Updates render state by SSB tag
    inline void tag_to_render_state(SSBTag* tag, SSBTime inner_ms, SSBTime inner_duration, RenderState& rs){
        switch(tag->type){
            case SSBTag::Type::FONT_FAMILY:
                rs.font_family = dynamic_cast<SSBFontFamily*>(tag)->family;
                break;
            case SSBTag::Type::FONT_STYLE:
                {
                    SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(tag);
                    rs.bold = font_style->bold;
                    rs.italic = font_style->italic;
                    rs.underline = font_style->underline;
                    rs.strikeout = font_style->strikeout;
                }
                break;
            case SSBTag::Type::FONT_SIZE:
                rs.font_size = dynamic_cast<SSBFontSize*>(tag)->size;
                break;
            case SSBTag::Type::FONT_SPACE:
                {
                    SSBFontSpace* font_space = dynamic_cast<SSBFontSpace*>(tag);
                    switch(font_space->type){
                        case SSBFontSpace::Type::HORIZONTAL: rs.font_space_h = font_space->x; break;
                        case SSBFontSpace::Type::VERTICAL: rs.font_space_v = font_space->y; break;
                        case SSBFontSpace::Type::BOTH: rs.font_space_h = font_space->x; rs.font_space_v = font_space->y; break;
                    }
                }
                break;
            case SSBTag::Type::LINE_WIDTH:
                rs.line_width = dynamic_cast<SSBLineWidth*>(tag)->width;
                break;
            case SSBTag::Type::LINE_STYLE:
                {
                    SSBLineStyle* line_style = dynamic_cast<SSBLineStyle*>(tag);
                    switch(line_style->join){
                        case SSBLineStyle::Join::MITER: rs.line_join = CAIRO_LINE_JOIN_MITER; break;
                        case SSBLineStyle::Join::BEVEL: rs.line_join = CAIRO_LINE_JOIN_BEVEL; break;
                        case SSBLineStyle::Join::ROUND: rs.line_join = CAIRO_LINE_JOIN_ROUND; break;
                    }
                    switch(line_style->cap){
                        case SSBLineStyle::Cap::FLAT: rs.line_cap = CAIRO_LINE_CAP_BUTT; break;
                        case SSBLineStyle::Cap::SQUARE: rs.line_cap = CAIRO_LINE_CAP_SQUARE; break;
                        case SSBLineStyle::Cap::ROUND: rs.line_cap = CAIRO_LINE_CAP_ROUND; break;
                    }
                }
                break;
            case SSBTag::Type::LINE_DASH:
                {
                    SSBLineDash* line_dash = dynamic_cast<SSBLineDash*>(tag);
                    rs.dash_offset = line_dash->offset;
                    rs.dashes.resize(line_dash->dashes.size());
                    std::copy(line_dash->dashes.begin(), line_dash->dashes.end(), rs.dashes.begin());
                }
                break;
            case SSBTag::Type::MODE:
                rs.mode = dynamic_cast<SSBMode*>(tag)->mode;
                break;
            case SSBTag::Type::DEFORM:
                {
                    SSBDeform* deform = dynamic_cast<SSBDeform*>(tag);
                    rs.deform_x = deform->formula_x;
                    rs.deform_y = deform->formula_y;
                    rs.deform_progress = 0;
                }
                break;
            case SSBTag::Type::POSITION:
                {
                    SSBPosition* pos = dynamic_cast<SSBPosition*>(tag);
                    constexpr decltype(pos->x) max_pos = std::numeric_limits<decltype(pos->x)>::max();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(pos->x == max_pos && pos->y == max_pos){
#pragma GCC diagnostic pop
                        rs.pos_x = std::numeric_limits<decltype(rs.pos_x)>::max();
                        rs.pos_y = std::numeric_limits<decltype(rs.pos_y)>::max();
                    }else{
                        rs.pos_x = pos->x;
                        rs.pos_y = pos->y;
                    }
                }
                break;
            case SSBTag::Type::ALIGN:
                rs.align = dynamic_cast<SSBAlign*>(tag)->align;
                break;
            case SSBTag::Type::MARGIN:
                {
                    SSBMargin* margin = dynamic_cast<SSBMargin*>(tag);
                    switch(margin->type){
                        case SSBMargin::Type::HORIZONTAL: rs.margin_h = margin->x; break;
                        case SSBMargin::Type::VERTICAL: rs.margin_v = margin->y; break;
                        case SSBMargin::Type::BOTH: rs.margin_h = margin->x; rs.margin_v = margin->y; break;
                    }
                }
                break;
            case SSBTag::Type::DIRECTION:
                rs.direction_angle = dynamic_cast<SSBDirection*>(tag)->angle;
                break;
            case SSBTag::Type::IDENTITY:
                cairo_matrix_init_identity(&rs.matrix);
                break;
            case SSBTag::Type::TRANSLATE:
                {
                    SSBTranslate* translation = dynamic_cast<SSBTranslate*>(tag);
                    switch(translation->type){
                        case SSBTranslate::Type::HORIZONTAL: cairo_matrix_translate(&rs.matrix, translation->x, 0); break;
                        case SSBTranslate::Type::VERTICAL: cairo_matrix_translate(&rs.matrix, 0, translation->y); break;
                        case SSBTranslate::Type::BOTH: cairo_matrix_translate(&rs.matrix, translation->x, translation->y); break;
                    }
                }
                break;
            case SSBTag::Type::SCALE:
                {
                    SSBScale* scale = dynamic_cast<SSBScale*>(tag);
                    switch(scale->type){
                        case SSBScale::Type::HORIZONTAL: cairo_matrix_scale(&rs.matrix, scale->x, 1); break;
                        case SSBScale::Type::VERTICAL: cairo_matrix_scale(&rs.matrix, 1, scale->y); break;
                        case SSBScale::Type::BOTH: cairo_matrix_scale(&rs.matrix, scale->x, scale->y); break;
                    }
                }
                break;
            case SSBTag::Type::ROTATE:
                {
                    SSBRotate* rotation = dynamic_cast<SSBRotate*>(tag);
                    switch(rotation->axis){
                        case SSBRotate::Axis::Z: cairo_matrix_rotate(&rs.matrix, DEG_TO_RAD(rotation->angle1)); break;
                        case SSBRotate::Axis::XY:
                            {
                                double rad_x = DEG_TO_RAD(rotation->angle1), rad_y = DEG_TO_RAD(rotation->angle2);
                                cairo_matrix_t tmp_matrix = {cos(rad_y), 0, sin(rad_x) * sin(rad_y), cos(rad_x), 0, 0};
                                cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                            }
                            break;
                        case SSBRotate::Axis::YX:
                            {
                                double rad_y = DEG_TO_RAD(rotation->angle1), rad_x = DEG_TO_RAD(rotation->angle2);
                                cairo_matrix_t tmp_matrix = {cos(rad_y), -sin(rad_x) * -sin(rad_y), 0, cos(rad_x), 0, 0};
                                cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
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
                    cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                }
                break;
            case SSBTag::Type::TRANSFORM:
                {
                    SSBTransform* transform = dynamic_cast<SSBTransform*>(tag);
                    cairo_matrix_t tmp_matrix = {transform->xx, transform->yx, transform->xy, transform->yy, transform->x0, transform->y0};
                    cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                }
                break;
            case SSBTag::Type::COLOR:
                {
                    SSBColor* color = dynamic_cast<SSBColor*>(tag);
                    if(color->target == SSBColor::Target::FILL)
                        if(color->colors[1].r < 0){
                            rs.colors.resize(1);
                            rs.colors[0] = color->colors[0];
                        }else{
                            rs.colors.resize(4);
                            rs.colors[0] = color->colors[0];
                            rs.colors[1] = color->colors[1];
                            rs.colors[2] = color->colors[2];
                            rs.colors[3] = color->colors[3];
                        }
                    else
                        if(color->colors[1].r < 0){
                            rs.line_colors.resize(1);
                            rs.line_colors[0] = color->colors[0];
                        }else{
                            rs.line_colors.resize(4);
                            rs.line_colors[0] = color->colors[0];
                            rs.line_colors[1] = color->colors[1];
                            rs.line_colors[2] = color->colors[2];
                            rs.line_colors[3] = color->colors[3];
                        }
                }
                break;
            case SSBTag::Type::ALPHA:
                {
                    SSBAlpha* alpha = dynamic_cast<SSBAlpha*>(tag);
                    if(alpha->target == SSBAlpha::Target::FILL)
                        if(alpha->alphas[1] < 0){
                            rs.alphas.resize(1);
                            rs.alphas[0] = alpha->alphas[0];
                        }else{
                            rs.alphas.resize(4);
                            rs.alphas[0] = alpha->alphas[0];
                            rs.alphas[1] = alpha->alphas[1];
                            rs.alphas[2] = alpha->alphas[2];
                            rs.alphas[3] = alpha->alphas[3];
                        }
                    else
                        if(alpha->alphas[1] < 0){
                            rs.line_alphas.resize(1);
                            rs.line_alphas[0] = alpha->alphas[0];
                        }else{
                            rs.line_alphas.resize(4);
                            rs.line_alphas[0] = alpha->alphas[0];
                            rs.line_alphas[1] = alpha->alphas[1];
                            rs.line_alphas[2] = alpha->alphas[2];
                            rs.line_alphas[3] = alpha->alphas[3];
                        }
                }
                break;
            case SSBTag::Type::TEXTURE:
                {
                    SSBTexture* texture = dynamic_cast<SSBTexture*>(tag);
                    if(texture->target == SSBTexture::Target::FILL)
                        rs.texture = texture->filename;
                    else
                        rs.line_texture = texture->filename;
                }
                break;
            case SSBTag::Type::TEXFILL:
                {
                    SSBTexFill* texfill = dynamic_cast<SSBTexFill*>(tag);
                    if(texfill->target == SSBTexFill::Target::FILL){
                        rs.texture_x = texfill->x;
                        rs.texture_y = texfill->y;
                        switch(texfill->wrap){
                            case SSBTexFill::WrapStyle::CLAMP: rs.wrap_style = CAIRO_EXTEND_NONE; break;
                            case SSBTexFill::WrapStyle::REPEAT: rs.wrap_style = CAIRO_EXTEND_REPEAT; break;
                            case SSBTexFill::WrapStyle::MIRROR: rs.wrap_style = CAIRO_EXTEND_REFLECT; break;
                            case SSBTexFill::WrapStyle::FLOW: rs.wrap_style = CAIRO_EXTEND_PAD; break;
                        }
                    }else{
                        rs.line_texture_x = texfill->x;
                        rs.line_texture_y = texfill->y;
                        switch(texfill->wrap){
                            case SSBTexFill::WrapStyle::CLAMP: rs.line_wrap_style = CAIRO_EXTEND_NONE; break;
                            case SSBTexFill::WrapStyle::REPEAT: rs.line_wrap_style = CAIRO_EXTEND_REPEAT; break;
                            case SSBTexFill::WrapStyle::MIRROR: rs.line_wrap_style = CAIRO_EXTEND_REFLECT; break;
                            case SSBTexFill::WrapStyle::FLOW: rs.line_wrap_style = CAIRO_EXTEND_PAD; break;
                        }
                    }
                }
                break;
            case SSBTag::Type::BLEND:
                rs.blend_mode = dynamic_cast<SSBBlend*>(tag)->mode;
                break;
            case SSBTag::Type::BLUR:
                {
                    SSBBlur* blur = dynamic_cast<SSBBlur*>(tag);
                    switch(blur->type){
                        case SSBBlur::Type::HORIZONTAL: rs.blur_h = blur->x; break;
                        case SSBBlur::Type::VERTICAL: rs.blur_v = blur->y; break;
                        case SSBBlur::Type::BOTH: rs.blur_h = blur->x; rs.blur_v = blur->y; break;
                    }
                }
                break;
            case SSBTag::Type::CLIP:
                rs.clip_mode = dynamic_cast<SSBClip*>(tag)->mode;
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
                        std::for_each(rs.alphas.begin(), rs.alphas.end(), [&progress](double& a){a *= progress;});
                        std::for_each(rs.line_alphas.begin(), rs.line_alphas.end(), [&progress](double& a){a *= progress;});
                    }
                }
                break;
            case SSBTag::Type::ANIMATE:
                {
                    SSBAnimate* animate = dynamic_cast<SSBAnimate*>(tag);
                    // Calculate start & end time
                    SSBTime animate_start, animate_end;
                    constexpr decltype(animate->start) max_duration = std::numeric_limits<decltype(animate->start)>::max();
                    if(animate->start == max_duration && animate->end == max_duration){
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
                    // Interpolate tags & set to render state
                    for(std::shared_ptr<SSBObject>& obj : animate->objects){
                        SSBTag* animate_tag = dynamic_cast<SSBTag*>(obj.get());
                        constexpr double threshold = 1;
                        switch(animate_tag->type){
                            case SSBTag::Type::FONT_FAMILY:
                                if(progress >= threshold)
                                    rs.font_family = dynamic_cast<SSBFontFamily*>(animate_tag)->family;
                                break;
                            case SSBTag::Type::FONT_STYLE:
                                if(progress >= threshold){
                                    SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(animate_tag);
                                    rs.bold = font_style->bold;
                                    rs.italic = font_style->italic;
                                    rs.underline = font_style->underline;
                                    rs.strikeout = font_style->strikeout;
                                }
                                break;
                            case SSBTag::Type::FONT_SIZE:
                                rs.font_size += progress * (static_cast<short int>(dynamic_cast<SSBFontSize*>(animate_tag)->size) - rs.font_size);
                                break;
                            case SSBTag::Type::FONT_SPACE:
                                {
                                    SSBFontSpace* font_space = dynamic_cast<SSBFontSpace*>(animate_tag);
                                    switch(font_space->type){
                                        case SSBFontSpace::Type::HORIZONTAL: rs.font_space_h += progress * (font_space->x - rs.font_space_h); break;
                                        case SSBFontSpace::Type::VERTICAL: rs.font_space_v += progress * (font_space->y - rs.font_space_v); break;
                                        case SSBFontSpace::Type::BOTH: rs.font_space_h += progress * (font_space->x - rs.font_space_h); rs.font_space_v += progress * (font_space->y - rs.font_space_v); break;
                                    }
                                }
                                break;
                            case SSBTag::Type::LINE_WIDTH:
                                rs.line_width += progress * (dynamic_cast<SSBLineWidth*>(animate_tag)->width - rs.line_width);
                                break;
                            case SSBTag::Type::LINE_STYLE:
                                if(progress >= threshold){
                                    SSBLineStyle* line_style = dynamic_cast<SSBLineStyle*>(animate_tag);
                                    switch(line_style->join){
                                        case SSBLineStyle::Join::MITER: rs.line_join = CAIRO_LINE_JOIN_MITER; break;
                                        case SSBLineStyle::Join::BEVEL: rs.line_join = CAIRO_LINE_JOIN_BEVEL; break;
                                        case SSBLineStyle::Join::ROUND: rs.line_join = CAIRO_LINE_JOIN_ROUND; break;
                                    }
                                    switch(line_style->cap){
                                        case SSBLineStyle::Cap::FLAT: rs.line_cap = CAIRO_LINE_CAP_BUTT; break;
                                        case SSBLineStyle::Cap::SQUARE: rs.line_cap = CAIRO_LINE_CAP_SQUARE; break;
                                        case SSBLineStyle::Cap::ROUND: rs.line_cap = CAIRO_LINE_CAP_ROUND; break;
                                    }
                                }
                                break;
                            case SSBTag::Type::LINE_DASH:
                                {
                                    SSBLineDash* line_dash = dynamic_cast<SSBLineDash*>(animate_tag);
                                    rs.dash_offset += progress * (line_dash->offset - rs.dash_offset);
                                    if(line_dash->dashes.size() == rs.dashes.size())
                                        std::transform(rs.dashes.begin(), rs.dashes.end(), line_dash->dashes.begin(), rs.dashes.begin(), [&progress](const double& dst, decltype(line_dash->dashes.front()) src){return dst + progress * (src - dst);});
                                }
                                break;
                            case SSBTag::Type::MODE:
                                if(progress >= threshold)
                                    rs.mode = dynamic_cast<SSBMode*>(animate_tag)->mode;
                                break;
                            case SSBTag::Type::DEFORM:
                                {
                                    SSBDeform* deform = dynamic_cast<SSBDeform*>(animate_tag);
                                    rs.deform_x = deform->formula_x;
                                    rs.deform_y = deform->formula_y;
                                    rs.deform_progress = progress;
                                }
                                break;
                            case SSBTag::Type::POSITION:
                                {
                                    SSBPosition* pos = dynamic_cast<SSBPosition*>(animate_tag);
                                    constexpr decltype(pos->x) max_pos = std::numeric_limits<decltype(pos->x)>::max();
                                    constexpr decltype(rs.pos_x) rsp_max_pos = std::numeric_limits<decltype(rs.pos_x)>::max();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                                    if(rs.pos_x != rsp_max_pos && rs.pos_y != rsp_max_pos && pos->x != max_pos && pos->y != max_pos){
#pragma GCC diagnostic pop
                                        rs.pos_x += progress * (pos->x - rs.pos_x);
                                        rs.pos_y += progress * (pos->y - rs.pos_y);
                                    }
                                }
                                break;
                            case SSBTag::Type::ALIGN:
                                if(progress >= threshold)
                                    rs.align = dynamic_cast<SSBAlign*>(animate_tag)->align;
                                break;
                            case SSBTag::Type::MARGIN:
                                {
                                    SSBMargin* margin = dynamic_cast<SSBMargin*>(animate_tag);
                                    switch(margin->type){
                                        case SSBMargin::Type::HORIZONTAL: rs.margin_h += progress * (margin->x - rs.margin_h); break;
                                        case SSBMargin::Type::VERTICAL: rs.margin_v += progress * (margin->y - rs.margin_v); break;
                                        case SSBMargin::Type::BOTH: rs.margin_h += progress * (margin->x - rs.margin_h); rs.margin_v += progress * (margin->y - rs.margin_v); break;
                                    }
                                }
                                break;
                            case SSBTag::Type::DIRECTION:
                                rs.direction_angle += progress * (dynamic_cast<SSBDirection*>(animate_tag)->angle - rs.direction_angle);
                                break;
                            case SSBTag::Type::IDENTITY:
                                if(progress >= threshold)
                                    cairo_matrix_init_identity(&rs.matrix);
                                break;
                            case SSBTag::Type::TRANSLATE:
                                {
                                    SSBTranslate* translation = dynamic_cast<SSBTranslate*>(animate_tag);
                                    switch(translation->type){
                                        case SSBTranslate::Type::HORIZONTAL: cairo_matrix_translate(&rs.matrix, progress * translation->x, 0); break;
                                        case SSBTranslate::Type::VERTICAL: cairo_matrix_translate(&rs.matrix, 0, progress * translation->y); break;
                                        case SSBTranslate::Type::BOTH: cairo_matrix_translate(&rs.matrix, progress * translation->x, progress * translation->y); break;
                                    }
                                }
                                break;
                            case SSBTag::Type::SCALE:
                                {
                                    SSBScale* scale = dynamic_cast<SSBScale*>(animate_tag);
                                    switch(scale->type){
                                        case SSBScale::Type::HORIZONTAL: cairo_matrix_scale(&rs.matrix, 1 + progress * (scale->x - 1), 1); break;
                                        case SSBScale::Type::VERTICAL: cairo_matrix_scale(&rs.matrix, 1, 1 + progress * (scale->y-1)); break;
                                        case SSBScale::Type::BOTH: cairo_matrix_scale(&rs.matrix, 1 + progress * (scale->x - 1), 1 + progress * (scale->y - 1)); break;
                                    }
                                }
                                break;
                            case SSBTag::Type::ROTATE:
                                {
                                    SSBRotate* rotation = dynamic_cast<SSBRotate*>(animate_tag);
                                    switch(rotation->axis){
                                        case SSBRotate::Axis::Z: cairo_matrix_rotate(&rs.matrix, progress * DEG_TO_RAD(rotation->angle1)); break;
                                        case SSBRotate::Axis::XY:
                                            {
                                                double rad_x = progress * DEG_TO_RAD(rotation->angle1), rad_y = progress * DEG_TO_RAD(rotation->angle2);
                                                cairo_matrix_t tmp_matrix = {cos(rad_y), 0, sin(rad_x) * sin(rad_y), cos(rad_x), 0, 0};
                                                cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                                            }
                                            break;
                                        case SSBRotate::Axis::YX:
                                            {
                                                double rad_y = progress * DEG_TO_RAD(rotation->angle1), rad_x = progress * DEG_TO_RAD(rotation->angle2);
                                                cairo_matrix_t tmp_matrix = {cos(rad_y), -sin(rad_x) * -sin(rad_y), 0, cos(rad_x), 0, 0};
                                                cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                                            }
                                            break;
                                    }
                                }
                                break;
                            case SSBTag::Type::SHEAR:
                                {
                                    SSBShear* shear = dynamic_cast<SSBShear*>(animate_tag);
                                    cairo_matrix_t tmp_matrix;
                                    switch(shear->type){
                                        case SSBShear::Type::HORIZONTAL: tmp_matrix = {1, 0, progress * shear->x, 1, 0, 0}; break;
                                        case SSBShear::Type::VERTICAL: tmp_matrix = {1, progress * shear->y, 0, 1, 0, 0}; break;
                                        case SSBShear::Type::BOTH: tmp_matrix = {1, progress * shear->y, progress * shear->x, 1, 0, 0}; break;
                                    }
                                    cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                                }
                                break;
                            case SSBTag::Type::TRANSFORM:
                                {
                                    SSBTransform* transform = dynamic_cast<SSBTransform*>(animate_tag);
                                    cairo_matrix_t tmp_matrix = { 1 + progress * (transform->xx - 1), progress * transform->yx, progress * transform->xy, 1 + progress * (transform->yy - 1), transform->x0, transform->y0};
                                    cairo_matrix_multiply(&rs.matrix, &tmp_matrix, &rs.matrix);
                                }
                                break;
                            case SSBTag::Type::COLOR:
                                {
                                    SSBColor* color = dynamic_cast<SSBColor*>(animate_tag);
                                    if(color->target == SSBColor::Target::FILL)
                                        if(rs.colors.size() == 1 && color->colors[1].r < 0)
                                            rs.colors[0] += (color->colors[0] - rs.colors[0]) * progress;
                                        else if(rs.colors.size() == 4 && color->colors[1].r >= 0){
                                            rs.colors[0] += (color->colors[0] - rs.colors[0]) * progress;
                                            rs.colors[1] += (color->colors[1] - rs.colors[1]) * progress;
                                            rs.colors[2] += (color->colors[2] - rs.colors[2]) * progress;
                                            rs.colors[3] += (color->colors[3] - rs.colors[3]) * progress;
                                        }else if(rs.colors.size() == 1 && color->colors[1].r >= 0){
                                            rs.colors.resize(4);
                                            std::fill(rs.colors.begin(), rs.colors.end(), rs.colors[0]);
                                            rs.colors[0] += (color->colors[0] - rs.colors[0]) * progress;
                                            rs.colors[1] += (color->colors[1] - rs.colors[1]) * progress;
                                            rs.colors[2] += (color->colors[2] - rs.colors[2]) * progress;
                                            rs.colors[3] += (color->colors[3] - rs.colors[3]) * progress;
                                        }else{
                                            rs.colors[0] += (color->colors[0] - rs.colors[0]) * progress;
                                            rs.colors[1] += (color->colors[0] - rs.colors[1]) * progress;
                                            rs.colors[2] += (color->colors[0] - rs.colors[2]) * progress;
                                            rs.colors[3] += (color->colors[0] - rs.colors[3]) * progress;
                                        }
                                    else
                                        if(rs.line_colors.size() == 1 && color->colors[1].r < 0)
                                            rs.line_colors[0] += (color->colors[0] - rs.line_colors[0]) * progress;
                                        else if(rs.line_colors.size() == 4 && color->colors[1].r >= 0){
                                            rs.line_colors[0] += (color->colors[0] - rs.line_colors[0]) * progress;
                                            rs.line_colors[1] += (color->colors[1] - rs.line_colors[1]) * progress;
                                            rs.line_colors[2] += (color->colors[2] - rs.line_colors[2]) * progress;
                                            rs.line_colors[3] += (color->colors[3] - rs.line_colors[3]) * progress;
                                        }else if(rs.line_colors.size() == 1 && color->colors[1].r >= 0){
                                            rs.line_colors.resize(4);
                                            std::fill(rs.line_colors.begin(), rs.line_colors.end(), rs.line_colors[0]);
                                            rs.line_colors[0] += (color->colors[0] - rs.line_colors[0]) * progress;
                                            rs.line_colors[1] += (color->colors[1] - rs.line_colors[1]) * progress;
                                            rs.line_colors[2] += (color->colors[2] - rs.line_colors[2]) * progress;
                                            rs.line_colors[3] += (color->colors[3] - rs.line_colors[3]) * progress;
                                        }else{
                                            rs.line_colors[0] += (color->colors[0] - rs.line_colors[0]) * progress;
                                            rs.line_colors[1] += (color->colors[0] - rs.line_colors[1]) * progress;
                                            rs.line_colors[2] += (color->colors[0] - rs.line_colors[2]) * progress;
                                            rs.line_colors[3] += (color->colors[0] - rs.line_colors[3]) * progress;
                                        }
                                }
                                break;
                            case SSBTag::Type::ALPHA:
                                {
                                    SSBAlpha* alpha = dynamic_cast<SSBAlpha*>(animate_tag);
                                    if(alpha->target == SSBAlpha::Target::FILL)
                                        if(rs.alphas.size() == 1 && alpha->alphas[1] < 0)
                                            rs.alphas[0] += progress * (alpha->alphas[0] - rs.alphas[0]);
                                        else if(rs.alphas.size() == 4 && alpha->alphas[1] >= 0){
                                            rs.alphas[0] += progress * (alpha->alphas[0] - rs.alphas[0]);
                                            rs.alphas[1] += progress * (alpha->alphas[1] - rs.alphas[1]);
                                            rs.alphas[2] += progress * (alpha->alphas[2] - rs.alphas[2]);
                                            rs.alphas[3] += progress * (alpha->alphas[3] - rs.alphas[3]);
                                        }else if(rs.alphas.size() == 1 && alpha->alphas[1] >= 0){
                                            rs.alphas.resize(4);
                                            std::fill(rs.alphas.begin(), rs.alphas.end(), rs.alphas[0]);
                                            rs.alphas[0] += progress * (alpha->alphas[0] - rs.alphas[0]);
                                            rs.alphas[1] += progress * (alpha->alphas[1] - rs.alphas[1]);
                                            rs.alphas[2] += progress * (alpha->alphas[2] - rs.alphas[2]);
                                            rs.alphas[3] += progress * (alpha->alphas[3] - rs.alphas[3]);
                                        }else{
                                            rs.alphas[0] += progress * (alpha->alphas[0] - rs.alphas[0]);
                                            rs.alphas[1] += progress * (alpha->alphas[0] - rs.alphas[1]);
                                            rs.alphas[2] += progress * (alpha->alphas[0] - rs.alphas[2]);
                                            rs.alphas[3] += progress * (alpha->alphas[0] - rs.alphas[3]);
                                        }
                                    else
                                        if(rs.line_alphas.size() == 1 && alpha->alphas[1] < 0)
                                            rs.line_alphas[0] += progress * (alpha->alphas[0] - rs.line_alphas[0]);
                                        else if(rs.line_alphas.size() == 4 && alpha->alphas[1] >= 0){
                                            rs.line_alphas[0] += progress * (alpha->alphas[0] - rs.line_alphas[0]);
                                            rs.line_alphas[1] += progress * (alpha->alphas[1] - rs.line_alphas[1]);
                                            rs.line_alphas[2] += progress * (alpha->alphas[2] - rs.line_alphas[2]);
                                            rs.line_alphas[3] += progress * (alpha->alphas[3] - rs.line_alphas[3]);
                                        }else if(rs.line_alphas.size() == 1 && alpha->alphas[1] >= 0){
                                            rs.line_alphas.resize(4);
                                            std::fill(rs.line_alphas.begin(), rs.line_alphas.end(), rs.line_alphas[0]);
                                            rs.line_alphas[0] += progress * (alpha->alphas[0] - rs.line_alphas[0]);
                                            rs.line_alphas[1] += progress * (alpha->alphas[1] - rs.line_alphas[1]);
                                            rs.line_alphas[2] += progress * (alpha->alphas[2] - rs.line_alphas[2]);
                                            rs.line_alphas[3] += progress * (alpha->alphas[3] - rs.line_alphas[3]);
                                        }else{
                                            rs.line_alphas[0] += progress * (alpha->alphas[0] - rs.line_alphas[0]);
                                            rs.line_alphas[1] += progress * (alpha->alphas[0] - rs.line_alphas[1]);
                                            rs.line_alphas[2] += progress * (alpha->alphas[0] - rs.line_alphas[2]);
                                            rs.line_alphas[3] += progress * (alpha->alphas[0] - rs.line_alphas[3]);
                                        }
                                }
                                break;
                            case SSBTag::Type::TEXTURE:
                                if(progress >= threshold){
                                    SSBTexture* texture = dynamic_cast<SSBTexture*>(animate_tag);
                                    if(texture->target == SSBTexture::Target::FILL)
                                        rs.texture = texture->filename;
                                    else
                                        rs.line_texture = texture->filename;
                                }
                                break;
                            case SSBTag::Type::TEXFILL:
                                {
                                    SSBTexFill* texfill = dynamic_cast<SSBTexFill*>(animate_tag);
                                    if(texfill->target == SSBTexFill::Target::FILL){
                                        rs.texture_x += progress * (texfill->x - rs.texture_x);
                                        rs.texture_y += progress * (texfill->y - rs.texture_x);
                                        if(progress >= threshold)
                                            switch(texfill->wrap){
                                                case SSBTexFill::WrapStyle::CLAMP: rs.wrap_style = CAIRO_EXTEND_NONE; break;
                                                case SSBTexFill::WrapStyle::REPEAT: rs.wrap_style = CAIRO_EXTEND_REPEAT; break;
                                                case SSBTexFill::WrapStyle::MIRROR: rs.wrap_style = CAIRO_EXTEND_REFLECT; break;
                                                case SSBTexFill::WrapStyle::FLOW: rs.wrap_style = CAIRO_EXTEND_PAD; break;
                                            }
                                    }else{
                                        rs.line_texture_x += progress * (texfill->x - rs.line_texture_x);
                                        rs.line_texture_y += progress * (texfill->y - rs.line_texture_x);
                                        if(progress >= threshold)
                                            switch(texfill->wrap){
                                                case SSBTexFill::WrapStyle::CLAMP: rs.line_wrap_style = CAIRO_EXTEND_NONE; break;
                                                case SSBTexFill::WrapStyle::REPEAT: rs.line_wrap_style = CAIRO_EXTEND_REPEAT; break;
                                                case SSBTexFill::WrapStyle::MIRROR: rs.line_wrap_style = CAIRO_EXTEND_REFLECT; break;
                                                case SSBTexFill::WrapStyle::FLOW: rs.line_wrap_style = CAIRO_EXTEND_PAD; break;
                                            }
                                    }
                                }
                                break;
                            case SSBTag::Type::BLEND:
                                if(progress >= threshold)
                                    rs.blend_mode = dynamic_cast<SSBBlend*>(animate_tag)->mode;
                                break;
                            case SSBTag::Type::BLUR:
                                {
                                    SSBBlur* blur = dynamic_cast<SSBBlur*>(animate_tag);
                                    switch(blur->type){
                                        case SSBBlur::Type::HORIZONTAL: rs.blur_h += progress * (blur->x - rs.blur_h); break;
                                        case SSBBlur::Type::VERTICAL: rs.blur_v += progress * (blur->y - rs.blur_v); break;
                                        case SSBBlur::Type::BOTH: rs.blur_h += progress * (blur->x - rs.blur_h); rs.blur_v += progress * (blur->y - rs.blur_v); break;
                                    }
                                }
                                break;
                            case SSBTag::Type::CLIP:
                                if(progress >= threshold)
                                    rs.clip_mode = dynamic_cast<SSBClip*>(animate_tag)->mode;
                                break;
                            case SSBTag::Type::FADE:
                                // Doesn't exist in an animation
                                break;
                            case SSBTag::Type::ANIMATE:
                                // Doesn't exist in an animation
                                break;
                            case SSBTag::Type::KARAOKE:
                                // Doesn't exist in an animation
                                break;
                        }
                    }
                }
                break;
            case SSBTag::Type::KARAOKE:
                {
                    SSBKaraoke* karaoke = dynamic_cast<SSBKaraoke*>(tag);
                    switch(karaoke->type){
                        case SSBKaraoke::Type::DURATION:
                            if(rs.karaoke_start < 0)
                                rs.karaoke_start = 0;
                            rs.karaoke_start += rs.karaoke_duration;
                            rs.karaoke_duration = karaoke->time;
                            break;
                        case SSBKaraoke::Type::SET:
                            rs.karaoke_start = karaoke->time;
                            rs.karaoke_duration = 0;
                            break;
                    }
                }
                break;
        }
    }
}
