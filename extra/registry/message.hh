#pragme once

#include "utf8.h"




//
// A message is a sequence of key value pairs based encoded in utf-8
//
// Search for first occurrence
//
//

namespace message {

    struct Token {
        Token() = default;
        Token(const char* begin, const char* end):
            _begin(begin), _end(end)
        {}
        Token& lstrip(CodePoint p); // left strip
        const char* _begin { nullptr };
        const char* _end   { nullptr };
    }

    


    struct Next {
        Token& key()   { return _key; }
        Token& value() { return _value; }
        
        Token _key;
        Token _value;
        const char* _next;
    };


    struct Iter {
        Iter(const char* begin,
             const char* end,
             CodePoint key_value_separator = ' ',
             CodePoint item_separator = '\n'):
            _it(begin), _end(end), 
        {}
            
        const char* _it;
        const char* _end;
        CodePoint   _key_value_separator { ' ' };
        CodePoint   _item_separator      { '\n' }
    };


    
    
}




namespace message {


}