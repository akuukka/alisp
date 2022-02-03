#include "ConsCell.hpp"
#include "ConsCellObject.hpp"
#include "Exception.hpp"
#include "Machine.hpp"
#include "Object.hpp"
#include "StreamObject.hpp"
#include "StringObject.hpp"
#include "alisp.hpp"
#include <algorithm>
#include <cstdint>
#include <ostream>
#include <regex>
#include "UTF8.hpp"
#include "String.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

void initStringFunctions(Machine& m)
{
    m.defun("char-or-string-p", [](ObjectPtr obj) {
        return obj->isString() || obj->isCharacter();
    });
    m.defun("make-string", [](std::int64_t num, std::uint32_t c) {
        if (c < 128) {
            return std::string(num, c);
        }
        std::string str;
        if (num == 0) {
            return str;
        }
        const std::string encoded = utf8::encode(c);
        str = utf8::encode(c);
        std::int64_t i = 1;
        while (i < num) {
            if (i < num/2) {
                str = str + str;
                i = i * 2;
            }
            else {
                str += encoded;
                i++;
            }
        }
        return str;
    });
    m.defun("stringp", [](ObjectPtr obj) { return obj->isString(); });
    m.defun("string-or-null-p", [](ObjectPtr obj) { return obj->isString() || obj->isNil(); });
    m.defun("string-bytes", [](const std::string& s) {
        return static_cast<std::int64_t>(s.size());
    });
    m.defun("concat", [](const std::string& str1, const std::string& str2) { return str1 + str2; });
    m.defun("substring", [](String str,
                            std::optional<std::int64_t> start,
                            std::optional<std::int64_t> end) {
        if (start && *start < 0) {
            *start = static_cast<std::int64_t>(str.size()) + *start;
        }
        if (end && *end < 0) {
            *end = static_cast<std::int64_t>(str.size()) + *end;
        }
        return !start ? str : (!end ? str.substr(*start) : str.substr(*start, *end - *start));
    });
    m.defun("string", [](Rest& rest) {
        std::string ret;
        while (rest.hasNext()) {
            auto arg = rest.pop();
            const std::optional<std::uint32_t> codepoint =
                arg->valueOrNull<std::uint32_t>();
            if (!codepoint) {
                throw exceptions::WrongTypeArgument(arg->toString());
            }
            ret += utf8::encode(*codepoint);
        }
        return ret;
    });
    m.defun("store-substring", [](StringObject sobj,
                                  std::int64_t idx,
                                  std::variant<std::string, std::uint32_t> obj)
    {
        auto& str = *sobj.value;
        std::string s;
        try {
            const std::uint32_t c = std::get<std::uint32_t>(obj);
            s = utf8::encode(c);
        }
        catch (std::bad_variant_access&) {
            s = std::get<std::string>(obj);
        }
        if (idx < 0 || idx + s.size() > str.size()) {
            throw exceptions::Error("Index out bounds. Can't grow string");
        }
        for (size_t i = 0; i < s.length(); i++) {
            str[idx+i] = s[i];
        }
        return sobj;
    });
    m.defun("clear-string", [](std::string& str) { for (auto& c : str) { c = 0; } });
    m.defun("split-string", [&m](std::string s,
                               std::optional<std::string> sep,
                               std::optional<bool> omitNulls) -> ObjectPtr {
        ListBuilder builder(m);
        std::smatch m;
        std::regex e(sep ? *sep : "[ \\n\\t\\r\\v]+");

        if (sep && !omitNulls) {
            omitNulls = false;
        }

        auto addMatch = [&](std::string m) {
            if (!omitNulls || *omitNulls) {
                if (m.empty()) {
                    return;
                }
            }
            builder.append(std::make_unique<StringObject>(std::move(m)));
        };
        std::regex_iterator<std::string::iterator> rit(s.begin(), s.end(), e);
        std::regex_iterator<std::string::iterator> rend;
        size_t from = 0;
        bool lastWasEmpty = false;
        while (rit != rend) {
            auto m = s.substr(from, rit->position() - from);
            addMatch(m);
            from = rit->position() + rit->length();
            lastWasEmpty = rit->position() == s.length() && m.empty();
            ++rit;
        }
        if (!lastWasEmpty) {
            addMatch(s.substr(from));
        }
        return builder.get();
    });
    m.defun("char-equal", [](std::uint32_t c1, std::uint32_t c2) { return c1 == c2; });
    m.defun("string=", [](const std::string& a, const std::string& b) { return a == b; });
    m.defun("string-equal", [](const std::string& a, const std::string& b) { return a == b; });
    m.defun("string<", [](const std::string& a, const std::string& b) { return a < b; });
    m.defun("string-lessp", [](const std::string& a, const std::string& b) { return a < b; });
    m.defun("string-greaterp", [](const std::string& a, const std::string& b) { return a > b; });
    m.defun("number-to-string", [](std::variant<std::int64_t, std::uint32_t, double> num)
    {
        try {
            const std::uint32_t val = std::get<std::uint32_t>(num);
            return std::to_string(val);
        }
        catch (std::bad_variant_access&) {}
        try {
            const std::int64_t val = std::get<std::int64_t>(num);
            return std::to_string(val);
        }
        catch (std::bad_variant_access&) {}
        const double val = std::get<double>(num);
        return std::to_string(val);
    });
    m.defun("char-to-string", [](std::uint32_t c1) { return utf8::encode(c1); });

    m.defun("format", [](ObjectPtr stream, String formatString, Rest& args) -> ObjectPtr {
        std::ostream* ostream = nullptr;
        if (stream->isNil()) {

        }
        else if (stream->isSymbol()) {
            if (stream->asSymbol()->getSymbol() == args.m.getSymbol(TName)) {
                ostream = &std::cout;
            }
        }
        else if (auto streamObj = dynamic_cast<StreamObject*>(stream.get())) {
            ostream = streamObj->ostream;
        }
        else {
            throw exceptions::WrongTypeArgument(stream->toString());
        }
        
        std::vector<std::pair<ConsCell*, String::ConstIterator>> getFrom
            = { std::make_pair(args.cc, formatString.begin()) };
        auto pop = [&]() {
            ObjectPtr ptr = getFrom.size() == 1 ? getFrom.back().first->car->eval() :
                getFrom.back().first->car->clone();
            getFrom.back().first = getFrom.back().first->next();
            return ptr;
        };
        
        std::string s;
        int col = 0;
        for (auto it = formatString.begin(); it != formatString.end();) {
            const std::uint32_t ch = *it;
            if (ch == '~') {
                ++it;
                const std::uint32_t n = *it;
                if (n == '%') {
                    s += "\n";
                    col = 0;
                }
                else if (n >= '0' && n <= '9') {
                    std::string num;
                    while (*it >= '0' && *it <= '9') {
                        num += (char)*it;
                        ++it;
                    }
                    int i = std::atoi(num.c_str());
                    if (*it == 't') {
                        while (col < i) {
                            s += " ";
                            col++;
                        }
                    }
                    else {
                        assert(false && "Unknown stuff after number");
                    }
                }
                else if (n == '{') {
                    auto arg = pop();
                    assert(arg->asList());
                    getFrom.emplace_back(arg->asList()->cc.get(), it);
                }
                else if (n == '}') {
                    if (getFrom.back().first) {
                        it = getFrom.back().second;
                        ++it;
                        continue;
                    }
                    getFrom.pop_back();
                }
                else if (n == 'a') {
                    const std::string add = pop()->toString(true);
                    col += utf8::strlen(add.c_str());
                    s += add;
                }
            }
            else {
                s += utf8::encode(ch);
                col++;
            }
            ++it;
        }
        if (stream->isNil()) {
            return std::make_unique<StringObject>(s);
        }
        (*ostream) << s;
        return args.m.makeNil();
    });
}

}
