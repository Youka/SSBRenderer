#pragma once

#include <vector>
#include <string>

struct SSBMeta{

};

struct SSBStyle{

};

struct SSBLine{

};

struct SSBData{
    SSBMeta meta;
    std::vector<SSBStyle> styles;
    std::vector<SSBLine> lines;
};
