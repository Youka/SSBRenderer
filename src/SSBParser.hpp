#pragma once

#include "SSBData.hpp"

class SSBParser{
    private:
        // Collected SSB data
        SSBData ssb;
    public:
        // Constructor
        SSBParser(std::string& script, bool warnings);
        // Get SSB data
        SSBData data();
        // Parse script & fill data
        void parse(std::string& script, bool warnings);

};
