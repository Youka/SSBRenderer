/*
Project: SSBRenderer
File: aegisub.cpp

Copyright (c) 2013, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

    The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    This notice may not be removed or altered from any source distribution.
*/

#include "Renderer.hpp"
#include "file_info.h"
#include <sstream>
#define CSRI_OWN_HANDLES
#define CSRIAPI extern "C" __declspec(dllexport)
typedef const char* csri_rend;
struct csri_inst{
    int height;
    Renderer* renderer;
};
#include <csri.h>

// Only renderer
csri_rend csri_ssbrenderer = FILTER_NAME;

// Converts ASS line to SSB line
std::string& cvt_ass_to_ssb(std::string& line){
    // Save frame resolution
    if(line.compare(0, 13, "[Script Info]") == 0)
        line = "#FRAME";
    else if(line.compare(0, 10, "PlayResX: ") == 0)
        line.replace(0, 10, "Width: ");
    else if(line.compare(0, 10, "PlayResY: ") == 0)
        line.replace(0, 10, "Height: ");
    // Save & reformat styles
    else if(line.compare(0, 12, "[V4+ Styles]") == 0)
        line = "#STYLES";
    else if(line.compare(0, 7, "Style: ") == 0){
        std::stringstream style_stream(line.substr(7));
        line.clear();
        // Style name
        std::string style_field;
        if(std::getline(style_stream, style_field, ',')){
            line.append(style_field).append(": ");
            // Fontname
            if(std::getline(style_stream, style_field, ',')){
                line.append("{font-family=").append(style_field);
                // Font size
                if(std::getline(style_stream, style_field, ',')){
                    line.append(";font-size=").append(style_field);
                    // Primary color
                    if(std::getline(style_stream, style_field, ',') && style_field.length() == 10){
                        line.append(";color=").append(style_field.substr(8,2) + style_field.substr(6,2) + style_field.substr(4,2)).append(";alpha=").append(style_field.substr(2,2));
                        // Secondary color
                        if(std::getline(style_stream, style_field, ',') && style_field.length() == 10){
                            line.append(";kcolor=").append(style_field.substr(8,2) + style_field.substr(6,2) + style_field.substr(4,2));
                            // Border color
                            if(std::getline(style_stream, style_field, ',') && style_field.length() == 10){
                                line.append(";line-color=").append(style_field.substr(8,2) + style_field.substr(6,2) + style_field.substr(4,2)).append(";line-alpha=").append(style_field.substr(2,2));
                                // Shadow color
                                if(std::getline(style_stream, style_field, ',') && style_field.length() == 10){
                                    // Bold
                                    if(std::getline(style_stream, style_field, ',')){
                                        line.append(";font-style=");
                                        if(style_field == "-1") line.push_back('b');
                                        // Italic
                                        if(std::getline(style_stream, style_field, ',')){
                                            if(style_field == "-1") line.push_back('i');
                                            // Underline
                                            if(std::getline(style_stream, style_field, ',')){
                                                if(style_field == "-1") line.push_back('u');
                                                // Strikeout
                                                if(std::getline(style_stream, style_field, ',')){
                                                    if(style_field == "-1") line.push_back('s');
                                                    // ScaleX
                                                    if(std::getline(style_stream, style_field, ',') && style_field.length() > 0){
                                                        line.append(";scale-x=").append(style_field.insert(1,1,'.'));
                                                        // ScaleY
                                                        if(std::getline(style_stream, style_field, ',') && style_field.length() > 0){
                                                            line.append(";scale-y=").append(style_field.insert(1,1,'.'));
                                                            // Spacing
                                                            if(std::getline(style_stream, style_field, ',')){
                                                                line.append(";font-space-h=").append(style_field);
                                                                // Angle
                                                                if(std::getline(style_stream, style_field, ',')){
                                                                    line.append(";rotate-z=").append(style_field);
                                                                    // Border style
                                                                    if(std::getline(style_stream, style_field, ',')){
                                                                        // Outline
                                                                        if(std::getline(style_stream, style_field, ',')){
                                                                            line.append(";line-width=").append(style_field);
                                                                            // Shadow
                                                                            if(std::getline(style_stream, style_field, ',')){
                                                                                // Alignment
                                                                                if(std::getline(style_stream, style_field, ',')){
                                                                                    line.append(";align=").append(style_field);
                                                                                    // MarginL
                                                                                    if(std::getline(style_stream, style_field, ',')){
                                                                                        line.append(";margin-h=").append(style_field);
                                                                                        // MarginR
                                                                                        if(std::getline(style_stream, style_field, ',')){
                                                                                            // MarginV
                                                                                            if(std::getline(style_stream, style_field, ',')){
                                                                                                line.append(";margin-v=").append(style_field);
                                                                                                // Encoding
                                                                                                if(std::getline(style_stream, style_field, ',')){
                                                                                                    line.push_back('}');
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    // Save & reformat events
    }else if(line.compare(0, 8, "[Events]") == 0)
        line = "#EVENTS";
    else if(line.compare(0, 10, "Dialogue: ") == 0){
        std::stringstream dialog_stream(line.substr(10));
        line.clear();
        // Layer
        std::string dialog_field;
        if(std::getline(dialog_stream, dialog_field, ',')){
            // Start time
            if(std::getline(dialog_stream, dialog_field, ',')){
                line.append(dialog_field).push_back('0');
                // End time
                if(std::getline(dialog_stream, dialog_field, ',')){
                    line.push_back('-');
                    line.append(dialog_field).push_back('0');
                    // Style
                    if(std::getline(dialog_stream, dialog_field, ',')){
                        line.push_back('|');
                        line.append(dialog_field);
                        // Name
                        if(std::getline(dialog_stream, dialog_field, ',')){
                            line.push_back('|');
                            // MarginL, MarginR, MarginV, Effect
                            if(std::getline(dialog_stream, dialog_field, ',') && std::getline(dialog_stream, dialog_field, ',') && std::getline(dialog_stream, dialog_field, ',') && std::getline(dialog_stream, dialog_field, ',')){
                                // Text
                                if(std::getline(dialog_stream, dialog_field)){
                                    line.push_back('|');
                                    line.append(dialog_field);
                                }
                            }
                        }
                    }
                }
            }
        }
    }else
        line.clear();
    return line;
}

// Open interface with file content
CSRIAPI csri_inst* csri_open_file(csri_rend*, const char* filename, struct csri_openflag*){
    // Get ass content in stream
    std::string filename_s(filename);
    FileReader file(filename_s);
    if(file){
        // Convert ass to ssb
        std::string ssb_content, line;
        while(file.getline(line))
            ssb_content.append(cvt_ass_to_ssb(line)).push_back('\n');
        // Create renderer
        std::istringstream ssb_stream(ssb_content);
        Renderer* renderer;
        try{
            renderer = new Renderer(0, 0, Renderer::Colorspace::BGR, ssb_stream, false);
        }catch(std::string err){
            return NULL;
        }
        return new csri_inst{0, renderer};
    }
    return NULL;
}

// Open interface with memory content
CSRIAPI csri_inst* csri_open_mem(csri_rend*, const void* data, size_t length, struct csri_openflag*){
    // Get ass content in stream
    std::stringstream ass_stream(std::string(reinterpret_cast<char*>(const_cast<void*>(data)), length));
    // Convert ass to ssb
    std::string ssb_content, line;
    while(std::getline(ass_stream, line))
        ssb_content.append(cvt_ass_to_ssb(line)).push_back('\n');
    // Create renderer
    std::istringstream ssb_stream(ssb_content);
    Renderer* renderer;
    try{
        renderer = new Renderer(1, 1, Renderer::Colorspace::BGR, ssb_stream, false);
    }catch(std::string err){
        return NULL;
    }
    return new csri_inst{0, renderer};
}

// Close interface
CSRIAPI void csri_close(csri_inst* inst){
    if(inst){
        if(inst->renderer)
            delete inst->renderer;
        delete inst;
    }
}

// Offer supported format and save him
CSRIAPI int csri_request_fmt(csri_inst* inst, const struct csri_fmt* fmt){
    if(!inst || !inst->renderer || fmt->width == 0 || fmt->height == 0)
        return -1;
    else{
        Renderer::Colorspace colorspace;
        switch(fmt->pixfmt){
            case CSRI_F_BGRA: colorspace = Renderer::Colorspace::BGRA; break;
            case CSRI_F_BGR: colorspace = Renderer::Colorspace::BGR; break;
            case CSRI_F_BGR_: colorspace = Renderer::Colorspace::BGRX; break;
            case CSRI_F_RGBA:
            case CSRI_F_ARGB:
            case CSRI_F_ABGR:
            case CSRI_F_RGB_:
            case CSRI_F__RGB:
            case CSRI_F__BGR:
            case CSRI_F_RGB:
            case CSRI_F_AYUV:
            case CSRI_F_YUVA:
            case CSRI_F_YVUA:
            case CSRI_F_YUY2:
            case CSRI_F_YV12A:
            case CSRI_F_YV12:
            default: return -1;
        }
        inst->height = fmt->height;
        inst->renderer->set_target(fmt->width, fmt->height, colorspace);
        return 0;
    }
}

// Inverts frame vertically
void frame_flip_y(unsigned char* data, long int pitch, int height){
    // Row buffer
    std::vector<unsigned char> temp_row(pitch * height);
    // Data last row
    unsigned char* data_end = data + (height - 1) * pitch;
    // Copy inverted from old to new
    for(int y = 0; y < height >> 1; ++y){
        ::memcpy(temp_row.data(), data, pitch);
        ::memcpy(data, data_end, pitch);
        ::memcpy(data_end, temp_row.data(), pitch);
        data += pitch;
        data_end -= pitch;
    }
}

// Render on frame with instance data
CSRIAPI void csri_render(csri_inst* inst, struct csri_frame* frame, double time){
    if(inst && inst->renderer){
        frame_flip_y(frame->planes[0], frame->strides[0], inst->height);
        inst->renderer->render(frame->planes[0], frame->strides[0], time * 1000);
        frame_flip_y(frame->planes[0], frame->strides[0], inst->height);
    }
}

// No extensions supported
CSRIAPI void* csri_query_ext(csri_rend*, csri_ext_id){
    return NULL;
}

// Renderer informations
static struct csri_info csri_ssbrenderer_info = {
    FILTER_NAME,
    FILTER_VERSION_STRING,
    FILTER_NAME,
    FILTER_AUTHOR,
    FILTER_COPYRIGHT
};

// Get renderer informations
CSRIAPI struct csri_info* csri_renderer_info(csri_rend*){
    return &csri_ssbrenderer_info;
}

// Just this renderer supported
CSRIAPI csri_rend* csri_renderer_byname(const char* name, const char* specific){
    return (strcmp(name, csri_ssbrenderer_info.name) || (specific && strcmp(specific, csri_ssbrenderer_info.specific))) ? NULL : &csri_ssbrenderer;
}

// Default renderer = this renderer (SSBRenderer)
CSRIAPI csri_rend* csri_renderer_default(){
    return &csri_ssbrenderer;
}

// No other renderers
CSRIAPI csri_rend* csri_renderer_next(csri_rend*){
    return NULL;
}
