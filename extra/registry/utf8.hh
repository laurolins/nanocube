#pragma once

#include <cstdint>

namespace utf8 {

    //
    // negative numbers indicates either a
    // problem parsing the next utf-8 CodePoint or
    // the end of the input stream
    //
    using CodePoint = int32_t;

    static const CodePoint END     = -1;
    static const CodePoint PROBLEM = -2;

    struct Next {

        Next() = default;

        Next(uint32_t code_point, const char* next):
            _code_point((CodePoint)code_point), _next(next)
        {}

        Next(CodePoint code_point, const char* next):
            _code_point(code_point), _next(next)
        {}

        bool normal()  const { return _code_point >= 0; }

        bool done()    const { return _code_point == END; }

        bool problem() const { return _code_point == PROBLEM; }
        
        operator bool() const { return normal(); }

        const char* next() const { return _next; }

        CodePoint   code_point() const { return _code_point; }
        
        CodePoint   operator*() const { return _code_point; }

    public:
        // code point of a next call
        CodePoint   _code_point { PROBLEM };

        // where to continue parsing:
        //   (1) _code
        //   (1) valid pointer, in case code is valid
        // different from PROBLEM, otherwi
        const char* _next { nullptr };
    };

    //
    // A call to next returns a Next record "n" that can be
    //
    // (1) n.normal() == true
    //     valid code point and the new position to look for
    //     the next code point.
    //
    // (2) n.done() == true
    //     no code point, and n.next() is equal to the "end"
    //     input which should be the same as "it" input
    //
    // (3) n.problem() == true
    //     could not extract valid utf-8 code point
    //     n.next() == nullptr
    //
    Next next(const char* it, const char* end) noexcept;

}