#pragma once
#include <stdexcept>
#include <string>
#include <memory>
#include "UTF8.hpp"
#include <iostream>

namespace alisp {

class String {
    std::shared_ptr<std::string> m_str;
    
    size_t mapToUnderlying(size_t pos) const
    {
        return proceed(0, pos);
    }

    size_t proceed(size_t startBytes, size_t n) const
    {
        size_t it = startBytes;
        const char* str = c_str();
        for (size_t i = 0;i < n;i++) {
            const size_t proceed = utf8::next(str + it, nullptr);
            if (proceed == 0) {
                return std::string::npos;
            }
            it += proceed;
        }
        return it;
    }
public:
    static const size_t npos = std::string::npos;
    
    String(std::string s) : m_str(std::make_shared<std::string>(std::move(s))) {}
    String(const String& o) : m_str(o.m_str) {}
    String(const std::shared_ptr<std::string>& o) : m_str(o) {}

    std::shared_ptr<std::string> sharedPointer() const { return m_str; }
    const std::string& toStdString() const { return *m_str; }
    const char* c_str() const { return m_str->c_str(); }
    size_t size() const { return utf8::strlen(c_str()); }
    size_t length() const { return size(); }
    String copy() const { return String(*m_str); }
    void operator+=(std::uint32_t codepoint) { *m_str += utf8::encode(codepoint); }
    void operator+=(std::string str) { *m_str += str; }

    String substr(size_t from, size_t n = npos) const
    {
        if (n == 0) {
            return String(std::string());
        }
        from = mapToUnderlying(from);
        if (from == npos) {
            throw std::runtime_error("String index out of range");
        }
        if (n != npos) {
            const size_t to = proceed(from, n);
            n = to - from;
        }
        return String(toStdString().substr(from, n));
    }

    std::uint32_t operator[](size_t index) const
    {
        size_t to = mapToUnderlying(index);
        std::uint32_t enc;
        if (utf8::next(c_str() + to, &enc) == 0) {
            throw std::runtime_error("String index out of range");
        }
        return utf8::decode(enc);
    }

    struct ConstIterator
    {
        const String* str;
        size_t pos;
        bool operator!=(const ConstIterator& o) const { return pos != o.pos; };
        const ConstIterator& operator++()
        {
            const size_t proceed = utf8::next(str->c_str() + pos);
            pos += proceed;
            if (!utf8::next(str->c_str() + pos)) {
                pos = npos;
            }
            return *this;
        }
        std::uint32_t operator*() const
        {
            std::uint32_t enc;
            if (!utf8::next(str->c_str() + pos, &enc)) {
                throw std::runtime_error("String index out of range");
            }
            return utf8::decode(enc);
        }
        ConstIterator operator-(std::int64_t i) const
        {
            ConstIterator ni;
            ni.str = this->str;
            std::int64_t p = pos;
            p -= i;
            ni.pos = p;
            return ni;
        }
    };

    ConstIterator begin() const
    {
        if (!utf8::next(c_str())) {
            return end();
        }
        return ConstIterator{this, 0};
    }
    ConstIterator end() const { return ConstIterator{this, npos}; }

    bool operator==(const char* cstr) const { return (!m_str && !cstr[0]) || *m_str == cstr; }
};

inline std::ostream &operator<<(std::ostream &os, const String& str)
{
    os << str.toStdString();
    return os;
}

}
