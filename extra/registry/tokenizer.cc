#include "tokenizer.hh"

//------------------------------------------------------------------------------
// Tokenizer
//------------------------------------------------------------------------------
namespace tokenizer {

    Tokenizer::Tokenizer(const std::string& text, char sep1, char sep2):
        text(text), sep1(sep1), sep2(sep2)
    {}
    
    void Tokenizer::update() {
//        int count = 0;
        indices.clear();
        auto b=text.begin();
        auto i=b;
        auto e=text.end();
        while (i!=e) {
            if (*i == sep1 || *i == sep2) {
                indices.push_back(std::distance(b,i));
            }
//            else if (*i == '"') {
//                count = (count + 1) % 2;
//            }
            ++i;
        }
        indices.push_back(std::distance(b,e));
    }
    
    int Tokenizer::tokens() const {
        return (int) indices.size();
    }
    
    int64_t Tokenizer::asInt64(int token_index)  const {
        auto txt    = std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]);
        auto result = std::stol(txt);
        return result;
    }
    
    int Tokenizer::asInt(int token_index)  const {
        auto txt    = std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]);
        auto result = std::stoi(txt);
        return result;
    }

    uint32_t Tokenizer::asUInt(int token_index)  const {
        auto txt    = std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]);
        auto result = std::stoi(txt);
        return (uint32_t) result;
    }

    int Tokenizer::asUnquoteInt(int token_index)  const {
        auto txt    = std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1)+1, text.begin() + indices[token_index]-1);
        auto result = std::stoi(txt);
        return result;
    }
    
    float Tokenizer::asFloat(int token_index)  const {
        return std::stof(std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]));
    }

    double Tokenizer::asDouble(int token_index)  const {
        auto st = std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]);
        return std::stod(st);
    }
    
    std::string Tokenizer::asStr(int token_index) const {
        return std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1), text.begin() + indices[token_index]);
    }
    
    std::string Tokenizer::asUnquoteStr(int token_index) const {
        return std::string(text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1) + 1, text.begin() + indices[token_index] - 1);
    }
    
    std::string Tokenizer::asUnquoteRobustStr(int token_index)  const {
        auto a = text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1);
        auto b = text.begin() + indices[token_index];
        if (a == b)
            return std::string();
        else
            return std::string(a+1,b-1);
    }
    
    int Tokenizer::asUnquoteIntRobust(int token_index) const  {  // return -1 if field is empty
        auto a = text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1);
        auto b = text.begin() + indices[token_index];
        if (a == b)
            return -1;
        if (*a == '"') {
            try {
                return std::stoi(std::string(a+1,b-1));
            }
            catch(...) {
                return -1;
            }
        }
        else {
            try {
                return std::stoi(std::string(a,b));
            }
            catch(...) {
                return -1;
            }
        }
    }
    
    int Tokenizer::asIntRobust(int token_index) const { // return -1 if field is empty
        auto a = text.begin() + (token_index == 0 ? 0 : indices[token_index-1] + 1);
        auto b = text.begin() + indices[token_index];
        if (a == b)
            return -1;
        auto result = std::stoi(std::string(a,b));
        return result;
    }

}

