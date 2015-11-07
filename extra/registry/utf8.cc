#include "utf8.hh"

namespace utf8 {

    static uint32_t f(const char *p) { return *((const uint8_t*) p); }
    
    //
    // http://en.wikipedia.org/wiki/UTF-8
    //
    //  7 	U+0000 	U+007F 	 1 	0xxxxxxx
    // 11 	U+0080 	U+07FF 	 2 	110xxxxx 	10xxxxxx
    // 16 	U+0800 	U+FFFF 	 3 	1110xxxx 	10xxxxxx 	10xxxxxx
    // 21 	U+10000 U+1FFFFF 4 	11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
    //
    Next next(const char* it, const char* end)  noexcept
    {
        static const uint32_t MASK_R[]  = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff }; 
        static const uint32_t MASK_L[]  = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff }; 

        if (it != end) {
            uint32_t c1 = f(it);
            if( (c1 & MASK_L[1]) == 0 ) {
                return Next(c1, it + 1);
            }
            else { // multibyte
                auto bytes_available = end - it;
                if( (c1 & MASK_L[3]) == 0xc0 && bytes_available >= 2) {
                    return Next(
                        ( (c1      & MASK_R[5]) << 6 ) + 
                        ( (f(it+1) & MASK_R[6])      ),
                        it + 2
                        );
                }
                else if( (c1 & MASK_L[4]) == 0xe0 && bytes_available >= 3) {
                    return Next(
                        ( (c1      & MASK_R[5]) << 12 ) + 
                        ( (f(it+1) & MASK_R[6]) <<  6 ) + 
                        ( (f(it+2) & MASK_R[6])       ),
                        it + 3);
                }
                else if( (c1 & MASK_L[5]) == 0xf0  && bytes_available >= 4) {
                    return Next(
                        ( (c1      & MASK_R[5]) << 18 ) + 
                        ( (f(it+1) & MASK_R[6]) << 12 ) + 
                        ( (f(it+2) & MASK_R[6]) <<  6 ) + 
                        ( (f(it+3) & MASK_R[6]) ),
                        it + 4 );
                }
                else {
                    return Next(PROBLEM, nullptr);
                }
            }
        }
        else {
            return Next(END, end);
        }
    }

    
}