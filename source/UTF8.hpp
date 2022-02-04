#pragma once
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace alisp
{

namespace utf8 {

constexpr int u8length(const char* s)
{
    constexpr std::uint8_t const u8_length[] = {
        // 0 1 2 3 4 5 6 7 8 9 A B C D E F
        1,1,1,1,1,1,1,1,0,0,0,0,2,2,3,4
    };
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

inline bool isValidCodepoint(std::uint32_t c)
{
    if (c <= 0x7F) return true;                 // [1]
    if (0xC280 <= c && c <= 0xDFBF)             // [2]
        return ((c & 0xE0C0) == 0xC080);
    if (0xEDA080 <= c && c <= 0xEDBFBF)         // [3]
        return 0; // Reject UTF-16 surrogates
    if (0xE0A080 <= c && c <= 0xEFBFBF)         // [4]
        return ((c & 0xF0C0C0) == 0xE08080);
    if (0xF0908080 <= c && c <= 0xF48FBFBF)     // [5]
        return ((c & 0xF8C0C0C0) == 0xF0808080);
    return false;
}

inline bool isValidCodepoint(std::int64_t i)
{
    return i >= 0 && i <= std::numeric_limits<std::uint32_t>::max()
        && isValidCodepoint(static_cast<std::uint32_t>(i));
}

inline size_t next(const char *txt, std::uint32_t* ch = nullptr)
{
    int len;
    std::uint32_t encoding = 0;
    len = u8length(txt);
    for (int i=0; i<len && txt[i] != '\0'; i++) {
        encoding = (encoding << 8) | (unsigned char)txt[i];
    }
    errno = 0;
    if (len == 0 || !isValidCodepoint(encoding)) {
        encoding = txt[0];
        len = 1;
        errno = EINVAL;
    }
    if (ch) *ch = encoding;
    return encoding ? len : 0 ;
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

inline std::uint32_t decode(std::uint32_t c)
{
    std::uint32_t mask;
    if (c > 0x7F) {
        mask = (c <= 0x00EFBFBF)? 0x000F0000 : 0x003F0000 ;
        c = ((c & 0x07000000) >> 6) |
            ((c & mask )      >> 4) |
            ((c & 0x00003F00) >> 2) |
            (c & 0x0000003F);
    }    
    return c;
}

inline std::string toUpper(const std::string& str)
{
    std::string ret = str;
    auto it = ret.c_str();
    std::uint32_t enc;
    size_t p = 0;
    for (;;) {
        const size_t proceed = next(it + p, &enc);
        if (!proceed) {
            break;
        }
        const std::uint32_t decoded = decode(enc);
        if (decoded < 128) {
            ret[p] = ::_toupper(decoded);
        }
        p += proceed;
    }
    return ret;
}

}}
