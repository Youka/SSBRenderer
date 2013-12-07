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
    // Get line from stream, including last empty one
    bool getlineex(std::basic_istream<char>& stream, std::string& line, char delimiter = '\n'){
        if(std::getline(stream, line, delimiter))
            return true;
        else if(stream.eof()){
            stream.clear();
            if(stream.unget() && stream.get() == delimiter){
                stream.setstate(std::ios_base::failbit);
                line = "";
                return true;
            }
        }
        return false;
    }
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
    struct GeometrySize{
        double off_x, off_y, width, height;
    };
    struct LineSize{
        double width = 0, height = 0, space = 0;
        std::vector<GeometrySize> geometries;
    };
    struct PosSize{
        double width = 0, height = 0;
        std::vector<LineSize> lines = {{}};
    };
    // Calculates alignment offset
    inline Point calc_align_offset(SSBAlign::Align align, SSBDirection::Mode direction, PosSize& pos_line_dim, size_t line_i){
        Point align_point;
        switch(direction){
            case SSBDirection::Mode::LTR:
            case SSBDirection::Mode::RTL:
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::RIGHT_BOTTOM:
                        align_point.y = -pos_line_dim.height;
                        break;
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                        align_point.y = -pos_line_dim.height / 2;
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
                        align_point.x = -pos_line_dim.width / 2 + (pos_line_dim.width - pos_line_dim.lines[line_i].width) / 2;
                        break;
                    case SSBAlign::Align::RIGHT_BOTTOM:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.x = -pos_line_dim.lines[line_i].width;
                        break;
                }
                break;
            case SSBDirection::Mode::TTB:
                switch(align){
                    case SSBAlign::Align::LEFT_BOTTOM:
                    case SSBAlign::Align::CENTER_BOTTOM:
                    case SSBAlign::Align::RIGHT_BOTTOM:
                        align_point.y = -pos_line_dim.lines[line_i].height;
                        break;
                    case SSBAlign::Align::LEFT_MIDDLE:
                    case SSBAlign::Align::CENTER_MIDDLE:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                        align_point.y = -pos_line_dim.height / 2 + (pos_line_dim.height - pos_line_dim.lines[line_i].height) / 2;
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
                        align_point.x = -pos_line_dim.width / 2;
                        break;
                    case SSBAlign::Align::RIGHT_BOTTOM:
                    case SSBAlign::Align::RIGHT_MIDDLE:
                    case SSBAlign::Align::RIGHT_TOP:
                        align_point.x = -pos_line_dim.width;
                        break;
                }
                break;
        }
        return align_point;
    }
    // Get auto position by frame dimension, alignment and margins
    inline Point get_auto_pos(int frame_width, int frame_height,
                              SSBAlign::Align align, double margin_h, double margin_v,
                              double scale_x = 0, double scale_y = 0){
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
        if(scale_x > 0 && scale_y > 0)
            switch(align){
                case SSBAlign::Align::LEFT_BOTTOM: return {margin_h * scale_x, frame_height - margin_v * scale_y};
                case SSBAlign::Align::CENTER_BOTTOM: return {frame_width / 2, frame_height - margin_v * scale_y};
                case SSBAlign::Align::RIGHT_BOTTOM: return {frame_width - margin_h * scale_x, frame_height - margin_v * scale_y};
                case SSBAlign::Align::LEFT_MIDDLE: return {margin_h * scale_x, frame_height / 2};
                case SSBAlign::Align::CENTER_MIDDLE: return {frame_width / 2, frame_height / 2};
                case SSBAlign::Align::RIGHT_MIDDLE: return {frame_width - margin_h * scale_x, frame_height / 2};
                case SSBAlign::Align::LEFT_TOP: return {margin_h * scale_x, margin_v * scale_y};
                case SSBAlign::Align::CENTER_TOP: return {frame_width / 2, margin_v * scale_y};
                case SSBAlign::Align::RIGHT_TOP: return {frame_width - margin_h * scale_x, margin_v * scale_y};
                default: return {0, 0};
            }
        else
            switch(align){
                case SSBAlign::Align::LEFT_BOTTOM: return {margin_h, frame_height - margin_v};
                case SSBAlign::Align::CENTER_BOTTOM: return {frame_width / 2, frame_height - margin_v};
                case SSBAlign::Align::RIGHT_BOTTOM: return {frame_width - margin_h, frame_height - margin_v};
                case SSBAlign::Align::LEFT_MIDDLE: return {margin_h, frame_height / 2};
                case SSBAlign::Align::CENTER_MIDDLE: return {frame_width / 2, frame_height / 2};
                case SSBAlign::Align::RIGHT_MIDDLE: return {frame_width - margin_h, frame_height / 2};
                case SSBAlign::Align::LEFT_TOP: return {margin_h, margin_v};
                case SSBAlign::Align::CENTER_TOP: return {frame_width / 2, margin_v};
                case SSBAlign::Align::RIGHT_TOP: return {frame_width - margin_h, margin_v};
                default: return {0, 0};
            }
#pragma GCC diagnostic pop
    }
    // Set line properties
    inline void set_line_props(cairo_t* ctx, RenderState& rs, double scale = 1){
        cairo_set_line_cap(ctx, rs.line_cap);
        cairo_set_line_join(ctx, rs.line_join);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(scale != 1){
#pragma GCC diagnostic pop
            cairo_set_line_width(ctx, rs.mode == SSBMode::Mode::FILL ? rs.line_width * 2 * scale : rs.line_width * scale);
            std::vector<double> dashes(rs.dashes);
            std::for_each(dashes.begin(), dashes.end(), [&scale](double& dash){dash *= scale;});
            cairo_set_dash(ctx, dashes.data(), dashes.size(), rs.dash_offset * scale);
        }else{
            cairo_set_line_width(ctx, rs.mode == SSBMode::Mode::FILL ? rs.line_width * 2 : rs.line_width);
            cairo_set_dash(ctx, rs.dashes.data(), rs.dashes.size(), rs.dash_offset);
        }
    }
}
