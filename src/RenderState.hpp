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
        double off_x = 0, off_y = 0;
        SSBAlign::Align align = SSBAlign::Align::CENTER_BOTTOM;
        double margin_h = 0, margin_v = 0;
        SSBDirection::Mode direction = SSBDirection::Mode::LTR;
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
        SSBStencil::Mode stencil_mode = SSBStencil::Mode::CLEAR;
        // Karaoke
        long int karaoke_start = -1, karaoke_duration = 0;
        RGB karaoke_color = {1, 0, 0};
        // State modifier
        struct StateChange{
            bool position = false,
                stencil = false;
        };
        StateChange eval_tag(SSBTag* tag, SSBTime inner_ms, SSBTime inner_duration){
            StateChange change;
            switch(tag->type){
                case SSBTag::Type::FONT_FAMILY:
                    this->font_family = dynamic_cast<SSBFontFamily*>(tag)->family;
                    break;
                case SSBTag::Type::FONT_STYLE:
                    {
                        SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(tag);
                        this->bold = font_style->bold;
                        this->italic = font_style->italic;
                        this->underline = font_style->underline;
                        this->strikeout = font_style->strikeout;
                    }
                    break;
                case SSBTag::Type::FONT_SIZE:
                    this->font_size = dynamic_cast<SSBFontSize*>(tag)->size;
                    break;
                case SSBTag::Type::FONT_SPACE:
                    {
                        SSBFontSpace* font_space = dynamic_cast<SSBFontSpace*>(tag);
                        switch(font_space->type){
                            case SSBFontSpace::Type::HORIZONTAL: this->font_space_h = font_space->x; break;
                            case SSBFontSpace::Type::VERTICAL: this->font_space_v = font_space->y; break;
                            case SSBFontSpace::Type::BOTH: this->font_space_h = font_space->x; this->font_space_v = font_space->y; break;
                        }
                    }
                    break;
                case SSBTag::Type::LINE_WIDTH:
                    this->line_width = dynamic_cast<SSBLineWidth*>(tag)->width;
                    break;
                case SSBTag::Type::LINE_STYLE:
                    {
                        SSBLineStyle* line_style = dynamic_cast<SSBLineStyle*>(tag);
                        switch(line_style->join){
                            case SSBLineStyle::Join::MITER: this->line_join = CAIRO_LINE_JOIN_MITER; break;
                            case SSBLineStyle::Join::BEVEL: this->line_join = CAIRO_LINE_JOIN_BEVEL; break;
                            case SSBLineStyle::Join::ROUND: this->line_join = CAIRO_LINE_JOIN_ROUND; break;
                        }
                        switch(line_style->cap){
                            case SSBLineStyle::Cap::FLAT: this->line_cap = CAIRO_LINE_CAP_BUTT; break;
                            case SSBLineStyle::Cap::SQUARE: this->line_cap = CAIRO_LINE_CAP_SQUARE; break;
                            case SSBLineStyle::Cap::ROUND: this->line_cap = CAIRO_LINE_CAP_ROUND; break;
                        }
                    }
                    break;
                case SSBTag::Type::LINE_DASH:
                    {
                        SSBLineDash* line_dash = dynamic_cast<SSBLineDash*>(tag);
                        this->dash_offset = line_dash->offset;
                        this->dashes.resize(line_dash->dashes.size());
                        std::copy(line_dash->dashes.begin(), line_dash->dashes.end(), this->dashes.begin());
                    }
                    break;
                case SSBTag::Type::MODE:
                    this->mode = dynamic_cast<SSBMode*>(tag)->mode;
                    break;
                case SSBTag::Type::DEFORM:
                    {
                        SSBDeform* deform = dynamic_cast<SSBDeform*>(tag);
                        this->deform_x = deform->formula_x;
                        this->deform_y = deform->formula_y;
                        this->deform_progress = 0;
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
                            this->pos_x = std::numeric_limits<decltype(this->pos_x)>::max();
                            this->pos_y = std::numeric_limits<decltype(this->pos_y)>::max();
                        }else{
                            this->pos_x = pos->x;
                            this->pos_y = pos->y;
                        }
                        change.position = true;
                    }
                    break;
                case SSBTag::Type::ALIGN:
                    this->align = dynamic_cast<SSBAlign*>(tag)->align;
                    break;
                case SSBTag::Type::MARGIN:
                    {
                        SSBMargin* margin = dynamic_cast<SSBMargin*>(tag);
                        switch(margin->type){
                            case SSBMargin::Type::HORIZONTAL: this->margin_h = margin->x; break;
                            case SSBMargin::Type::VERTICAL: this->margin_v = margin->y; break;
                            case SSBMargin::Type::BOTH: this->margin_h = margin->x; this->margin_v = margin->y; break;
                        }
                    }
                    break;
                case SSBTag::Type::DIRECTION:
                    this->direction = dynamic_cast<SSBDirection*>(tag)->mode;
                    break;
                case SSBTag::Type::IDENTITY:
                    cairo_matrix_init_identity(&this->matrix);
                    break;
                case SSBTag::Type::TRANSLATE:
                    {
                        SSBTranslate* translation = dynamic_cast<SSBTranslate*>(tag);
                        switch(translation->type){
                            case SSBTranslate::Type::HORIZONTAL: cairo_matrix_translate(&this->matrix, translation->x, 0); break;
                            case SSBTranslate::Type::VERTICAL: cairo_matrix_translate(&this->matrix, 0, translation->y); break;
                            case SSBTranslate::Type::BOTH: cairo_matrix_translate(&this->matrix, translation->x, translation->y); break;
                        }
                    }
                    break;
                case SSBTag::Type::SCALE:
                    {
                        SSBScale* scale = dynamic_cast<SSBScale*>(tag);
                        switch(scale->type){
                            case SSBScale::Type::HORIZONTAL: cairo_matrix_scale(&this->matrix, scale->x, 1); break;
                            case SSBScale::Type::VERTICAL: cairo_matrix_scale(&this->matrix, 1, scale->y); break;
                            case SSBScale::Type::BOTH: cairo_matrix_scale(&this->matrix, scale->x, scale->y); break;
                        }
                    }
                    break;
                case SSBTag::Type::ROTATE:
                    {
                        SSBRotate* rotation = dynamic_cast<SSBRotate*>(tag);
                        switch(rotation->axis){
                            case SSBRotate::Axis::Z: cairo_matrix_rotate(&this->matrix, DEG_TO_RAD(rotation->angle1)); break;
                            case SSBRotate::Axis::XY:
                                {
                                    double rad_x = DEG_TO_RAD(rotation->angle1), rad_y = DEG_TO_RAD(rotation->angle2);
                                    cairo_matrix_t tmp_matrix = {cos(rad_y), 0, sin(rad_x) * sin(rad_y), cos(rad_x), 0, 0};
                                    cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                                }
                                break;
                            case SSBRotate::Axis::YX:
                                {
                                    double rad_y = DEG_TO_RAD(rotation->angle1), rad_x = DEG_TO_RAD(rotation->angle2);
                                    cairo_matrix_t tmp_matrix = {cos(rad_y), -sin(rad_x) * -sin(rad_y), 0, cos(rad_x), 0, 0};
                                    cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
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
                        cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                    }
                    break;
                case SSBTag::Type::TRANSFORM:
                    {
                        SSBTransform* transform = dynamic_cast<SSBTransform*>(tag);
                        cairo_matrix_t tmp_matrix = {transform->xx, transform->yx, transform->xy, transform->yy, transform->x0, transform->y0};
                        cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                    }
                    break;
                case SSBTag::Type::COLOR:
                    {
                        SSBColor* color = dynamic_cast<SSBColor*>(tag);
                        if(color->target == SSBColor::Target::FILL)
                            if(color->colors[1].r < 0){
                                this->colors.resize(1);
                                this->colors[0] = color->colors[0];
                            }else{
                                this->colors.resize(4);
                                this->colors[0] = color->colors[0];
                                this->colors[1] = color->colors[1];
                                this->colors[2] = color->colors[2];
                                this->colors[3] = color->colors[3];
                            }
                        else
                            if(color->colors[1].r < 0){
                                this->line_colors.resize(1);
                                this->line_colors[0] = color->colors[0];
                            }else{
                                this->line_colors.resize(4);
                                this->line_colors[0] = color->colors[0];
                                this->line_colors[1] = color->colors[1];
                                this->line_colors[2] = color->colors[2];
                                this->line_colors[3] = color->colors[3];
                            }
                    }
                    break;
                case SSBTag::Type::ALPHA:
                    {
                        SSBAlpha* alpha = dynamic_cast<SSBAlpha*>(tag);
                        if(alpha->target == SSBAlpha::Target::FILL)
                            if(alpha->alphas[1] < 0){
                                this->alphas.resize(1);
                                this->alphas[0] = alpha->alphas[0];
                            }else{
                                this->alphas.resize(4);
                                this->alphas[0] = alpha->alphas[0];
                                this->alphas[1] = alpha->alphas[1];
                                this->alphas[2] = alpha->alphas[2];
                                this->alphas[3] = alpha->alphas[3];
                            }
                        else
                            if(alpha->alphas[1] < 0){
                                this->line_alphas.resize(1);
                                this->line_alphas[0] = alpha->alphas[0];
                            }else{
                                this->line_alphas.resize(4);
                                this->line_alphas[0] = alpha->alphas[0];
                                this->line_alphas[1] = alpha->alphas[1];
                                this->line_alphas[2] = alpha->alphas[2];
                                this->line_alphas[3] = alpha->alphas[3];
                            }
                    }
                    break;
                case SSBTag::Type::TEXTURE:
                    {
                        SSBTexture* texture = dynamic_cast<SSBTexture*>(tag);
                        if(texture->target == SSBTexture::Target::FILL)
                            this->texture = texture->filename;
                        else
                            this->line_texture = texture->filename;
                    }
                    break;
                case SSBTag::Type::TEXFILL:
                    {
                        SSBTexFill* texfill = dynamic_cast<SSBTexFill*>(tag);
                        if(texfill->target == SSBTexFill::Target::FILL){
                            this->texture_x = texfill->x;
                            this->texture_y = texfill->y;
                            switch(texfill->wrap){
                                case SSBTexFill::WrapStyle::CLAMP: this->wrap_style = CAIRO_EXTEND_NONE; break;
                                case SSBTexFill::WrapStyle::REPEAT: this->wrap_style = CAIRO_EXTEND_REPEAT; break;
                                case SSBTexFill::WrapStyle::MIRROR: this->wrap_style = CAIRO_EXTEND_REFLECT; break;
                                case SSBTexFill::WrapStyle::FLOW: this->wrap_style = CAIRO_EXTEND_PAD; break;
                            }
                        }else{
                            this->line_texture_x = texfill->x;
                            this->line_texture_y = texfill->y;
                            switch(texfill->wrap){
                                case SSBTexFill::WrapStyle::CLAMP: this->line_wrap_style = CAIRO_EXTEND_NONE; break;
                                case SSBTexFill::WrapStyle::REPEAT: this->line_wrap_style = CAIRO_EXTEND_REPEAT; break;
                                case SSBTexFill::WrapStyle::MIRROR: this->line_wrap_style = CAIRO_EXTEND_REFLECT; break;
                                case SSBTexFill::WrapStyle::FLOW: this->line_wrap_style = CAIRO_EXTEND_PAD; break;
                            }
                        }
                    }
                    break;
                case SSBTag::Type::BLEND:
                    this->blend_mode = dynamic_cast<SSBBlend*>(tag)->mode;
                    break;
                case SSBTag::Type::BLUR:
                    {
                        SSBBlur* blur = dynamic_cast<SSBBlur*>(tag);
                        switch(blur->type){
                            case SSBBlur::Type::HORIZONTAL: this->blur_h = blur->x; break;
                            case SSBBlur::Type::VERTICAL: this->blur_v = blur->y; break;
                            case SSBBlur::Type::BOTH: this->blur_h = blur->x; this->blur_v = blur->y; break;
                        }
                    }
                    break;
                case SSBTag::Type::STENCIL:
                    this->stencil_mode = dynamic_cast<SSBStencil*>(tag)->mode;
                    change.stencil = true;
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
                            std::for_each(this->alphas.begin(), this->alphas.end(), [&progress](double& a){a *= progress;});
                            std::for_each(this->line_alphas.begin(), this->line_alphas.end(), [&progress](double& a){a *= progress;});
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
                                        this->font_family = dynamic_cast<SSBFontFamily*>(animate_tag)->family;
                                    break;
                                case SSBTag::Type::FONT_STYLE:
                                    if(progress >= threshold){
                                        SSBFontStyle* font_style = dynamic_cast<SSBFontStyle*>(animate_tag);
                                        this->bold = font_style->bold;
                                        this->italic = font_style->italic;
                                        this->underline = font_style->underline;
                                        this->strikeout = font_style->strikeout;
                                    }
                                    break;
                                case SSBTag::Type::FONT_SIZE:
                                    this->font_size += progress * (static_cast<short int>(dynamic_cast<SSBFontSize*>(animate_tag)->size) - this->font_size);
                                    break;
                                case SSBTag::Type::FONT_SPACE:
                                    {
                                        SSBFontSpace* font_space = dynamic_cast<SSBFontSpace*>(animate_tag);
                                        switch(font_space->type){
                                            case SSBFontSpace::Type::HORIZONTAL: this->font_space_h += progress * (font_space->x - this->font_space_h); break;
                                            case SSBFontSpace::Type::VERTICAL: this->font_space_v += progress * (font_space->y - this->font_space_v); break;
                                            case SSBFontSpace::Type::BOTH: this->font_space_h += progress * (font_space->x - this->font_space_h); this->font_space_v += progress * (font_space->y - this->font_space_v); break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::LINE_WIDTH:
                                    this->line_width += progress * (dynamic_cast<SSBLineWidth*>(animate_tag)->width - this->line_width);
                                    break;
                                case SSBTag::Type::LINE_STYLE:
                                    if(progress >= threshold){
                                        SSBLineStyle* line_style = dynamic_cast<SSBLineStyle*>(animate_tag);
                                        switch(line_style->join){
                                            case SSBLineStyle::Join::MITER: this->line_join = CAIRO_LINE_JOIN_MITER; break;
                                            case SSBLineStyle::Join::BEVEL: this->line_join = CAIRO_LINE_JOIN_BEVEL; break;
                                            case SSBLineStyle::Join::ROUND: this->line_join = CAIRO_LINE_JOIN_ROUND; break;
                                        }
                                        switch(line_style->cap){
                                            case SSBLineStyle::Cap::FLAT: this->line_cap = CAIRO_LINE_CAP_BUTT; break;
                                            case SSBLineStyle::Cap::SQUARE: this->line_cap = CAIRO_LINE_CAP_SQUARE; break;
                                            case SSBLineStyle::Cap::ROUND: this->line_cap = CAIRO_LINE_CAP_ROUND; break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::LINE_DASH:
                                    {
                                        SSBLineDash* line_dash = dynamic_cast<SSBLineDash*>(animate_tag);
                                        this->dash_offset += progress * (line_dash->offset - this->dash_offset);
                                        if(line_dash->dashes.size() == this->dashes.size())
                                            std::transform(this->dashes.begin(), this->dashes.end(), line_dash->dashes.begin(), this->dashes.begin(), [&progress](const double& dst, decltype(line_dash->dashes.front()) src){return dst + progress * (src - dst);});
                                    }
                                    break;
                                case SSBTag::Type::MODE:
                                    if(progress >= threshold)
                                        this->mode = dynamic_cast<SSBMode*>(animate_tag)->mode;
                                    break;
                                case SSBTag::Type::DEFORM:
                                    {
                                        SSBDeform* deform = dynamic_cast<SSBDeform*>(animate_tag);
                                        this->deform_x = deform->formula_x;
                                        this->deform_y = deform->formula_y;
                                        this->deform_progress = progress;
                                    }
                                    break;
                                case SSBTag::Type::POSITION:
                                    {
                                        SSBPosition* pos = dynamic_cast<SSBPosition*>(animate_tag);
                                        constexpr decltype(pos->x) max_pos = std::numeric_limits<decltype(pos->x)>::max();
                                        constexpr decltype(this->pos_x) rsp_max_pos = std::numeric_limits<decltype(this->pos_x)>::max();
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfloat-equal"
                                        if(this->pos_x != rsp_max_pos && this->pos_y != rsp_max_pos && pos->x != max_pos && pos->y != max_pos){
    #pragma GCC diagnostic pop
                                            this->pos_x += progress * (pos->x - this->pos_x);
                                            this->pos_y += progress * (pos->y - this->pos_y);
                                        }
                                        change.position = true;
                                    }
                                    break;
                                case SSBTag::Type::ALIGN:
                                    if(progress >= threshold)
                                        this->align = dynamic_cast<SSBAlign*>(animate_tag)->align;
                                    break;
                                case SSBTag::Type::MARGIN:
                                    {
                                        SSBMargin* margin = dynamic_cast<SSBMargin*>(animate_tag);
                                        switch(margin->type){
                                            case SSBMargin::Type::HORIZONTAL: this->margin_h += progress * (margin->x - this->margin_h); break;
                                            case SSBMargin::Type::VERTICAL: this->margin_v += progress * (margin->y - this->margin_v); break;
                                            case SSBMargin::Type::BOTH: this->margin_h += progress * (margin->x - this->margin_h); this->margin_v += progress * (margin->y - this->margin_v); break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::DIRECTION:
                                    if(progress >= threshold)
                                        this->direction = dynamic_cast<SSBDirection*>(animate_tag)->mode;
                                    break;
                                case SSBTag::Type::IDENTITY:
                                    if(progress >= threshold)
                                        cairo_matrix_init_identity(&this->matrix);
                                    break;
                                case SSBTag::Type::TRANSLATE:
                                    {
                                        SSBTranslate* translation = dynamic_cast<SSBTranslate*>(animate_tag);
                                        switch(translation->type){
                                            case SSBTranslate::Type::HORIZONTAL: cairo_matrix_translate(&this->matrix, progress * translation->x, 0); break;
                                            case SSBTranslate::Type::VERTICAL: cairo_matrix_translate(&this->matrix, 0, progress * translation->y); break;
                                            case SSBTranslate::Type::BOTH: cairo_matrix_translate(&this->matrix, progress * translation->x, progress * translation->y); break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::SCALE:
                                    {
                                        SSBScale* scale = dynamic_cast<SSBScale*>(animate_tag);
                                        switch(scale->type){
                                            case SSBScale::Type::HORIZONTAL: cairo_matrix_scale(&this->matrix, 1 + progress * (scale->x - 1), 1); break;
                                            case SSBScale::Type::VERTICAL: cairo_matrix_scale(&this->matrix, 1, 1 + progress * (scale->y-1)); break;
                                            case SSBScale::Type::BOTH: cairo_matrix_scale(&this->matrix, 1 + progress * (scale->x - 1), 1 + progress * (scale->y - 1)); break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::ROTATE:
                                    {
                                        SSBRotate* rotation = dynamic_cast<SSBRotate*>(animate_tag);
                                        switch(rotation->axis){
                                            case SSBRotate::Axis::Z: cairo_matrix_rotate(&this->matrix, progress * DEG_TO_RAD(rotation->angle1)); break;
                                            case SSBRotate::Axis::XY:
                                                {
                                                    double rad_x = progress * DEG_TO_RAD(rotation->angle1), rad_y = progress * DEG_TO_RAD(rotation->angle2);
                                                    cairo_matrix_t tmp_matrix = {cos(rad_y), 0, sin(rad_x) * sin(rad_y), cos(rad_x), 0, 0};
                                                    cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                                                }
                                                break;
                                            case SSBRotate::Axis::YX:
                                                {
                                                    double rad_y = progress * DEG_TO_RAD(rotation->angle1), rad_x = progress * DEG_TO_RAD(rotation->angle2);
                                                    cairo_matrix_t tmp_matrix = {cos(rad_y), -sin(rad_x) * -sin(rad_y), 0, cos(rad_x), 0, 0};
                                                    cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
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
                                        cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                                    }
                                    break;
                                case SSBTag::Type::TRANSFORM:
                                    {
                                        SSBTransform* transform = dynamic_cast<SSBTransform*>(animate_tag);
                                        cairo_matrix_t tmp_matrix = { 1 + progress * (transform->xx - 1), progress * transform->yx, progress * transform->xy, 1 + progress * (transform->yy - 1), transform->x0, transform->y0};
                                        cairo_matrix_multiply(&this->matrix, &tmp_matrix, &this->matrix);
                                    }
                                    break;
                                case SSBTag::Type::COLOR:
                                    {
                                        SSBColor* color = dynamic_cast<SSBColor*>(animate_tag);
                                        if(color->target == SSBColor::Target::FILL)
                                            if(this->colors.size() == 1 && color->colors[1].r < 0)
                                                this->colors[0] += (color->colors[0] - this->colors[0]) * progress;
                                            else if(this->colors.size() == 4 && color->colors[1].r >= 0){
                                                this->colors[0] += (color->colors[0] - this->colors[0]) * progress;
                                                this->colors[1] += (color->colors[1] - this->colors[1]) * progress;
                                                this->colors[2] += (color->colors[2] - this->colors[2]) * progress;
                                                this->colors[3] += (color->colors[3] - this->colors[3]) * progress;
                                            }else if(this->colors.size() == 1 && color->colors[1].r >= 0){
                                                this->colors.resize(4);
                                                std::fill(this->colors.begin(), this->colors.end(), this->colors[0]);
                                                this->colors[0] += (color->colors[0] - this->colors[0]) * progress;
                                                this->colors[1] += (color->colors[1] - this->colors[1]) * progress;
                                                this->colors[2] += (color->colors[2] - this->colors[2]) * progress;
                                                this->colors[3] += (color->colors[3] - this->colors[3]) * progress;
                                            }else{
                                                this->colors[0] += (color->colors[0] - this->colors[0]) * progress;
                                                this->colors[1] += (color->colors[0] - this->colors[1]) * progress;
                                                this->colors[2] += (color->colors[0] - this->colors[2]) * progress;
                                                this->colors[3] += (color->colors[0] - this->colors[3]) * progress;
                                            }
                                        else
                                            if(this->line_colors.size() == 1 && color->colors[1].r < 0)
                                                this->line_colors[0] += (color->colors[0] - this->line_colors[0]) * progress;
                                            else if(this->line_colors.size() == 4 && color->colors[1].r >= 0){
                                                this->line_colors[0] += (color->colors[0] - this->line_colors[0]) * progress;
                                                this->line_colors[1] += (color->colors[1] - this->line_colors[1]) * progress;
                                                this->line_colors[2] += (color->colors[2] - this->line_colors[2]) * progress;
                                                this->line_colors[3] += (color->colors[3] - this->line_colors[3]) * progress;
                                            }else if(this->line_colors.size() == 1 && color->colors[1].r >= 0){
                                                this->line_colors.resize(4);
                                                std::fill(this->line_colors.begin(), this->line_colors.end(), this->line_colors[0]);
                                                this->line_colors[0] += (color->colors[0] - this->line_colors[0]) * progress;
                                                this->line_colors[1] += (color->colors[1] - this->line_colors[1]) * progress;
                                                this->line_colors[2] += (color->colors[2] - this->line_colors[2]) * progress;
                                                this->line_colors[3] += (color->colors[3] - this->line_colors[3]) * progress;
                                            }else{
                                                this->line_colors[0] += (color->colors[0] - this->line_colors[0]) * progress;
                                                this->line_colors[1] += (color->colors[0] - this->line_colors[1]) * progress;
                                                this->line_colors[2] += (color->colors[0] - this->line_colors[2]) * progress;
                                                this->line_colors[3] += (color->colors[0] - this->line_colors[3]) * progress;
                                            }
                                    }
                                    break;
                                case SSBTag::Type::ALPHA:
                                    {
                                        SSBAlpha* alpha = dynamic_cast<SSBAlpha*>(animate_tag);
                                        if(alpha->target == SSBAlpha::Target::FILL)
                                            if(this->alphas.size() == 1 && alpha->alphas[1] < 0)
                                                this->alphas[0] += progress * (alpha->alphas[0] - this->alphas[0]);
                                            else if(this->alphas.size() == 4 && alpha->alphas[1] >= 0){
                                                this->alphas[0] += progress * (alpha->alphas[0] - this->alphas[0]);
                                                this->alphas[1] += progress * (alpha->alphas[1] - this->alphas[1]);
                                                this->alphas[2] += progress * (alpha->alphas[2] - this->alphas[2]);
                                                this->alphas[3] += progress * (alpha->alphas[3] - this->alphas[3]);
                                            }else if(this->alphas.size() == 1 && alpha->alphas[1] >= 0){
                                                this->alphas.resize(4);
                                                std::fill(this->alphas.begin(), this->alphas.end(), this->alphas[0]);
                                                this->alphas[0] += progress * (alpha->alphas[0] - this->alphas[0]);
                                                this->alphas[1] += progress * (alpha->alphas[1] - this->alphas[1]);
                                                this->alphas[2] += progress * (alpha->alphas[2] - this->alphas[2]);
                                                this->alphas[3] += progress * (alpha->alphas[3] - this->alphas[3]);
                                            }else{
                                                this->alphas[0] += progress * (alpha->alphas[0] - this->alphas[0]);
                                                this->alphas[1] += progress * (alpha->alphas[0] - this->alphas[1]);
                                                this->alphas[2] += progress * (alpha->alphas[0] - this->alphas[2]);
                                                this->alphas[3] += progress * (alpha->alphas[0] - this->alphas[3]);
                                            }
                                        else
                                            if(this->line_alphas.size() == 1 && alpha->alphas[1] < 0)
                                                this->line_alphas[0] += progress * (alpha->alphas[0] - this->line_alphas[0]);
                                            else if(this->line_alphas.size() == 4 && alpha->alphas[1] >= 0){
                                                this->line_alphas[0] += progress * (alpha->alphas[0] - this->line_alphas[0]);
                                                this->line_alphas[1] += progress * (alpha->alphas[1] - this->line_alphas[1]);
                                                this->line_alphas[2] += progress * (alpha->alphas[2] - this->line_alphas[2]);
                                                this->line_alphas[3] += progress * (alpha->alphas[3] - this->line_alphas[3]);
                                            }else if(this->line_alphas.size() == 1 && alpha->alphas[1] >= 0){
                                                this->line_alphas.resize(4);
                                                std::fill(this->line_alphas.begin(), this->line_alphas.end(), this->line_alphas[0]);
                                                this->line_alphas[0] += progress * (alpha->alphas[0] - this->line_alphas[0]);
                                                this->line_alphas[1] += progress * (alpha->alphas[1] - this->line_alphas[1]);
                                                this->line_alphas[2] += progress * (alpha->alphas[2] - this->line_alphas[2]);
                                                this->line_alphas[3] += progress * (alpha->alphas[3] - this->line_alphas[3]);
                                            }else{
                                                this->line_alphas[0] += progress * (alpha->alphas[0] - this->line_alphas[0]);
                                                this->line_alphas[1] += progress * (alpha->alphas[0] - this->line_alphas[1]);
                                                this->line_alphas[2] += progress * (alpha->alphas[0] - this->line_alphas[2]);
                                                this->line_alphas[3] += progress * (alpha->alphas[0] - this->line_alphas[3]);
                                            }
                                    }
                                    break;
                                case SSBTag::Type::TEXTURE:
                                    if(progress >= threshold){
                                        SSBTexture* texture = dynamic_cast<SSBTexture*>(animate_tag);
                                        if(texture->target == SSBTexture::Target::FILL)
                                            this->texture = texture->filename;
                                        else
                                            this->line_texture = texture->filename;
                                    }
                                    break;
                                case SSBTag::Type::TEXFILL:
                                    {
                                        SSBTexFill* texfill = dynamic_cast<SSBTexFill*>(animate_tag);
                                        if(texfill->target == SSBTexFill::Target::FILL){
                                            this->texture_x += progress * (texfill->x - this->texture_x);
                                            this->texture_y += progress * (texfill->y - this->texture_x);
                                            if(progress >= threshold)
                                                switch(texfill->wrap){
                                                    case SSBTexFill::WrapStyle::CLAMP: this->wrap_style = CAIRO_EXTEND_NONE; break;
                                                    case SSBTexFill::WrapStyle::REPEAT: this->wrap_style = CAIRO_EXTEND_REPEAT; break;
                                                    case SSBTexFill::WrapStyle::MIRROR: this->wrap_style = CAIRO_EXTEND_REFLECT; break;
                                                    case SSBTexFill::WrapStyle::FLOW: this->wrap_style = CAIRO_EXTEND_PAD; break;
                                                }
                                        }else{
                                            this->line_texture_x += progress * (texfill->x - this->line_texture_x);
                                            this->line_texture_y += progress * (texfill->y - this->line_texture_x);
                                            if(progress >= threshold)
                                                switch(texfill->wrap){
                                                    case SSBTexFill::WrapStyle::CLAMP: this->line_wrap_style = CAIRO_EXTEND_NONE; break;
                                                    case SSBTexFill::WrapStyle::REPEAT: this->line_wrap_style = CAIRO_EXTEND_REPEAT; break;
                                                    case SSBTexFill::WrapStyle::MIRROR: this->line_wrap_style = CAIRO_EXTEND_REFLECT; break;
                                                    case SSBTexFill::WrapStyle::FLOW: this->line_wrap_style = CAIRO_EXTEND_PAD; break;
                                                }
                                        }
                                    }
                                    break;
                                case SSBTag::Type::BLEND:
                                    if(progress >= threshold)
                                        this->blend_mode = dynamic_cast<SSBBlend*>(animate_tag)->mode;
                                    break;
                                case SSBTag::Type::BLUR:
                                    {
                                        SSBBlur* blur = dynamic_cast<SSBBlur*>(animate_tag);
                                        switch(blur->type){
                                            case SSBBlur::Type::HORIZONTAL: this->blur_h += progress * (blur->x - this->blur_h); break;
                                            case SSBBlur::Type::VERTICAL: this->blur_v += progress * (blur->y - this->blur_v); break;
                                            case SSBBlur::Type::BOTH: this->blur_h += progress * (blur->x - this->blur_h); this->blur_v += progress * (blur->y - this->blur_v); break;
                                        }
                                    }
                                    break;
                                case SSBTag::Type::STENCIL:
                                    if(progress >= threshold){
                                        this->stencil_mode = dynamic_cast<SSBStencil*>(animate_tag)->mode;
                                        change.stencil = true;
                                    }
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
                                case SSBTag::Type::KARAOKE_COLOR:
                                    if(progress >= threshold)
                                        this->karaoke_color = dynamic_cast<SSBKaraokeColor*>(animate_tag)->color;
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
                                if(this->karaoke_start < 0)
                                    this->karaoke_start = 0;
                                this->karaoke_start += this->karaoke_duration;
                                this->karaoke_duration = karaoke->time;
                                break;
                            case SSBKaraoke::Type::SET:
                                this->karaoke_start = karaoke->time;
                                this->karaoke_duration = 0;
                                break;
                        }
                    }
                    break;
                case SSBTag::Type::KARAOKE_COLOR:
                    this->karaoke_color = dynamic_cast<SSBKaraokeColor*>(tag)->color;
                    break;
            }
            return change;
        }
    };
}
