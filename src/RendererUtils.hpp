/*
Project: SSBRenderer
File: RendererUtils.hpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "RenderState.hpp"
#include "cairo++.hpp"
#include "SSBData.hpp"

namespace{
    // Applies deform filter on cairo path
    void path_deform(cairo_t* ctx, std::string& deform_x, std::string& deform_y, double progress){
        mu::Parser parser_x, parser_y;
        double x_buf, y_buf;
        parser_x.DefineConst("t", progress);
        parser_y.DefineConst("t", progress);
        parser_x.DefineVar("x", &x_buf);
        parser_y.DefineVar("x", &x_buf);
        parser_x.DefineVar("y", &y_buf);
        parser_y.DefineVar("y", &y_buf);
        parser_x.SetExpr(deform_x);
        parser_y.SetExpr(deform_y);
        cairo_path_filter(ctx,
            [&parser_x,&parser_y,&x_buf,&y_buf](double& x, double& y){
                x_buf = x;
                y_buf = y;
                try{
                    x = parser_x.Eval();
                    y = parser_y.Eval();
                }catch(...){}
            });
    }
    // Converts SSB points to cairo path
    inline void points_to_cairo(SSBPoints* points, double size, cairo_t* ctx){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(size == 1)
#pragma GCC diagnostic pop
            for(const Point& point : points->points)
                cairo_rectangle(ctx, point.x - 0.5, point.y - 0.5, 1, 1);   // Creates a move + lines + close = closed shape
        else
            for(const Point& point : points->points){
                cairo_new_sub_path(ctx);
                cairo_arc(ctx, point.x, point.y, size / 2, 0, M_PI * 2);
                cairo_close_path(ctx);
            }
    }
    // Converts SSB path to cairo path
    inline void path_to_cairo(SSBPath* path, cairo_t* ctx){
        const std::vector<SSBPath::Segment>& segments = path->segments;
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
    // Structure for line sizes
    struct LineSize{
        double width = 0, height = 0, space = 0;
    };
    // Calculates alignment offset
    inline Point calc_align_offset(SSBAlign::Align align, SSBDirection::Mode direction, std::vector<LineSize>& line_dimensions, size_t line_i){
        Point align_point;
        switch(direction){
            case SSBDirection::Mode::LTR:
            case SSBDirection::Mode::RTL:
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::RIGHT_BOTTOM:
                        align_point.y = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                            return init - line.height - line.space;
                        });
                        break;
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                        align_point.y = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                            return init - line.height - line.space;
                        }) / 2;
                        break;
                    case SSBAlign::Align::LEFT_TOP:
                    case SSBAlign::Align::CENTER_TOP:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.y = 0;
                        break;
                }
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::LEFT_TOP:
                        align_point.x = 0;
                        break;
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::CENTER_TOP:
                        {
                            double max_line_width = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                                return std::max(init, line.width);
                            });
                            align_point.x = -max_line_width / 2 + (max_line_width - line_dimensions[line_i].width) / 2;
                        }
                        break;
                    case SSBAlign::Align::RIGHT_BOTTOM:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.x = -line_dimensions[line_i].width;
                        break;
                }
                break;
            case SSBDirection::Mode::TTB:
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::RIGHT_BOTTOM:
                        align_point.y = -line_dimensions[line_i].height;
                        break;
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                        {
                            double max_line_height = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                                    return std::max(init, line.height);
                            });
                            align_point.y = -max_line_height / 2 + (max_line_height - line_dimensions[line_i].height) / 2;
                        }
                        break;
                    case SSBAlign::Align::LEFT_TOP:
                    case SSBAlign::Align::CENTER_TOP:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.y = 0;
                        break;
                }
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::LEFT_TOP:
                        align_point.x = 0;
                        break;
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::CENTER_TOP:
                        align_point.x = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                            return init - line.width - line.space;
                        }) / 2;
                        break;
                    case SSBAlign::Align::RIGHT_BOTTOM:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.x = std::accumulate(line_dimensions.begin(), line_dimensions.end(), 0.0, [](double init, LineSize& line){
                            return init - line.width - line.space;
                        });
                        break;
                }
                break;
        }
        return align_point;
    }
    // Get auto position by frame dimension, alignment and margins
    inline Point get_auto_pos(int frame_width, int frame_height, RenderState& rs, double scale_x = 0, double scale_y = 0){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
        if(scale_x > 0 && scale_y > 0){
            switch(rs.align){
                case SSBAlign::Align::LEFT_BOTTOM: return {rs.margin_h * scale_x, frame_height - rs.margin_v * scale_y};
                case SSBAlign::Align::CENTER_BOTTOM: return {frame_width / 2, frame_height - rs.margin_v * scale_y};
                case SSBAlign::Align::RIGHT_BOTTOM: return {frame_width - rs.margin_h * scale_x, frame_height - rs.margin_v * scale_y};
                case SSBAlign::Align::LEFT_MIDDLE: return {rs.margin_h * scale_x, frame_height / 2};
                case SSBAlign::Align::CENTER_MIDDLE: return {frame_width / 2, frame_height / 2};
                case SSBAlign::Align::RIGHT_MIDDLE: return {frame_width - rs.margin_h * scale_x, frame_height / 2};
                case SSBAlign::Align::LEFT_TOP: return {rs.margin_h * scale_x, rs.margin_v * scale_y};
                case SSBAlign::Align::CENTER_TOP: return {frame_width / 2, rs.margin_v * scale_y};
                case SSBAlign::Align::RIGHT_TOP: return {frame_width - rs.margin_h * scale_x, rs.margin_v * scale_y};
                default: return {0, 0};
            }
        }else{
            switch(rs.align){
                case SSBAlign::Align::LEFT_BOTTOM: return {rs.margin_h, frame_height - rs.margin_v};
                case SSBAlign::Align::CENTER_BOTTOM: return {frame_width / 2, frame_height - rs.margin_v};
                case SSBAlign::Align::RIGHT_BOTTOM: return {frame_width - rs.margin_h, frame_height - rs.margin_v};
                case SSBAlign::Align::LEFT_MIDDLE: return {rs.margin_h, frame_height / 2};
                case SSBAlign::Align::CENTER_MIDDLE: return {frame_width / 2, frame_height / 2};
                case SSBAlign::Align::RIGHT_MIDDLE: return {frame_width - rs.margin_h, frame_height / 2};
                case SSBAlign::Align::LEFT_TOP: return {rs.margin_h, rs.margin_v};
                case SSBAlign::Align::CENTER_TOP: return {frame_width / 2, rs.margin_v};
                case SSBAlign::Align::RIGHT_TOP: return {frame_width - rs.margin_h, rs.margin_v};
                default: return {0, 0};
            }
        }
#pragma GCC diagnostic pop
    }
    // Set context line properties by RenderState
    inline void set_line_props(cairo_t* ctx, RenderState& rs){
        cairo_set_line_width(ctx, rs.line_width);
        cairo_set_line_cap(ctx, rs.line_cap);
        cairo_set_line_join(ctx, rs.line_join);
        cairo_set_dash(ctx, rs.dashes.data(), rs.dashes.size(), rs.dash_offset);
    }
}
