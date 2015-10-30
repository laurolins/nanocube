#pragma once

#include <string>
#include <vector>

namespace tokenizer {

//------------------------------------------------------------------------------
// Tokenizer
//------------------------------------------------------------------------------

    struct Tokenizer {
    
        Tokenizer(const std::string& text, char sep1=',', char sep2=',');
    
        void update();
    
        int tokens() const;
    
        int64_t asInt64(int token_index) const;
    
        int asInt(int token_index) const;

        uint32_t asUInt(int token_index) const;

        int asUnquoteInt(int token_index) const;
    
        float asFloat(int token_index) const;
    
        double asDouble(int token_index) const;

        std::string asStr(int token_index) const;
    
        std::string asUnquoteStr(int token_index) const;
    
        std::string asUnquoteRobustStr(int token_index) const;
    
        int asUnquoteIntRobust(int token_index) const;
    
        int asIntRobust(int token_index) const;
    
        const std::string& text;
        char sep1;
        char sep2;
        std::vector<std::string::size_type>   indices;
    };


}

