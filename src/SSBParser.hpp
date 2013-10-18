#pragma once

#include <string>

class SSBParser{
    private:

    public:
        // Constructor
        SSBParser(std::string& script, bool warnings);
        // Parse & fill data
        void parse(std::string& script, bool warnings);
};
