#pragma once

#include "SSBData.hpp"

class SSBParser{
    private:
        // Collected SSB data
        SSBData ssb;
    public:
        // Constructors
        SSBParser() = default;
        SSBParser(SSBData& ssb);
        SSBParser(std::string& script, bool warnings) throw(std::string);
        // Get SSB data
        SSBData data() const;
        // Parse script & fill data
        void parse(std::string& script, bool warnings) throw(std::string);
};
