#include <cstdint>
#include <iostream>
#include <string>

namespace alisp
{

namespace utf8 {

static std::uint8_t const u8_length[] = {
// 0 1 2 3 4 5 6 7 8 9 A B C D E F
   1,1,1,1,1,1,1,1,0,0,0,0,2,2,3,4
};

constexpr int u8length(const char* s)
{
    return u8_length[(((const uint8_t*)(s))[0] & 0xFF) >> 4];
}

constexpr int u8length(std::uint32_t codepoint)
{
    if (codepoint <= 0x7f) {
        return 1;
    }
    if (codepoint >= 0x0080 && codepoint <= 0x07ff) {
        return 2;
    }
    if (codepoint >= 0x0800 && codepoint <= 0xffff) {
        return 3;
    }
    if (codepoint >= 0x10000 && codepoint <= 0x10ffff) {
        return 4;
    }
    return 0;
}

inline size_t strlen(const char *s)
{
    int len=0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) len++;
        s++;
    }
    return len;
}

inline std::string encode(std::uint32_t codepoint)
{
    auto c = codepoint;
    if (codepoint > 0x7F) {
        c =  (codepoint & 0x000003F) 
            | (codepoint & 0x0000FC0) << 2 
            | (codepoint & 0x003F000) << 4
            | (codepoint & 0x01C0000) << 6;
        
        if      (codepoint < 0x0000800) c |= 0x0000C080;
        else if (codepoint < 0x0010000) c |= 0x00E08080;
        else                            c |= 0xF0808080;
    }
    std::string s;
    int len = u8length(codepoint);
    s.resize(len);
    for (int i=0;i<len;i++) {
        char* p = (char*)&c;
        s[i] = p[len-i-1];
    }
    return s;
}

}}
