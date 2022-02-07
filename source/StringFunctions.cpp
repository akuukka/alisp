#include "ConsCell.hpp"
#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "Machine.hpp"
#include "Object.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"
#include "alisp.hpp"
#include <algorithm>
#include <cstdint>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include "UTF8.hpp"
#include "String.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

ALISP_STATIC String format(const String str, FArgs& args)
{
    String ret("");
    for (auto it = str.begin(); it != str.end(); ++it) {
        const std::uint32_t c = *it;
        if (c == '%') {
            ++it;
            assert(it != str.end());
            std::uint32_t n = *it;

            bool leadingZeros = false;
            int intWidth = 0;
            while (n == '0') {
                leadingZeros = true;
                n = *++it;
            }
            if (n >= '1' && n <= '9') {
                std::string s;
                s += n;
                n = *++it;
                while (n >= '1' && n <= '9') {
                    s += n;
                    n = *++it;
                }
                intWidth = std::atoi(s.c_str());
            }
            
            if (n == '%') {
                ret += c;
            }
            else if (n == 'd') {
                const auto nextSym = args.pop();
                std::string val;
                if (nextSym->isInt()) {
                    val = std::to_string(nextSym->value<std::int64_t>());
                }
                else if (nextSym->isFloat()) {
                    val = std::to_string(static_cast<std::int64_t>(nextSym->value<double>()));
                }
                else {
                    throw exceptions::Error("Format specifier doesn’t match argument type");
                }
                if (intWidth && val.size() < intWidth) {
                    const char filler = (leadingZeros ? '0' : ' ');
                    if (val[0] == '-' && filler == '0') {
                        val = "-" + std::string(intWidth - val.size(), filler) + val.substr(1);
                    }
                    else {
                        val = std::string(intWidth - val.size(), filler) + val;
                    }
                }
                ret += val;
            }
            else if (n == 'S') { ret += args.pop()->toString(false); }
            else if (n == 's') { ret += args.pop()->toString(true); }
            else {
                throw exceptions::Error("Invalid format operation");
            }
        }
        else {
            ret += c;
        }
    }
    return ret;
}

void Machine::initStringFunctions()
{
    defun("print", [](const Object& obj, std::optional<std::ostream*> stream){
        if (!stream) { *stream = &std::cout; }
        (**stream) << "\n" << obj.toString() << "\n";
        return obj.clone();
    });
    defun("prin1", [](const Object& obj, std::optional<std::ostream*> stream){
        if (!stream) { *stream = &std::cout; }
        (**stream) << obj.toString();
        return obj.clone();
    });
    defun("princ", [](const Object& obj, std::optional<std::ostream*> stream){
        if (!stream) { *stream = &std::cout; }
        (**stream) << obj.toString(true);
        return obj.clone();
    });
    defun("write-char", [](std::uint32_t codepoint, std::optional<std::ostream*> stream) {
        if (!stream) { *stream = &std::cout; }
        (**stream) << utf8::encode(codepoint);
        return codepoint;
    });
    defun("char-or-string-p", [](const Object& obj) { return obj.isString() || obj.isCharacter(); });
    defun("make-string", [](std::int64_t num, std::uint32_t c) {
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
    defun("stringp", [](const Object& obj) { return obj.isString(); });
    defun("string-or-null-p", [](const Object& obj) { return obj.isString() || obj.isNil(); });
    defun("string-bytes", [](const std::string& s) { return static_cast<std::int64_t>(s.size()); });
    defun("concat", [](const std::string& str1, const std::string& str2) { return str1 + str2; });
    defun("substring", [](String str,
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
    defun("string", [](Rest& rest) {
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
    defun("store-substring", [this](String sobj,
                                    std::int64_t idx,
                                    std::variant<std::string, std::uint32_t> obj)
    {
        auto& str = *sobj.sharedPointer();
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
        return StringObject(sobj);
    });
    defun("clear-string", [](std::string& str) { for (auto& c : str) { c = 0; } });
    defun("split-string", [this](std::string s,
                                 std::optional<std::string> sep,
                                 std::optional<bool> omitNulls) -> ObjectPtr {
        ListBuilder builder(*this);
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
    defun("char-equal", [](std::uint32_t c1, std::uint32_t c2) { return c1 == c2; });
    defun("string=", [](const std::string& a, const std::string& b) { return a == b; });
    defun("string-equal", [](const std::string& a, const std::string& b) { return a == b; });
    defun("string<", [](const std::string& a, const std::string& b) { return a < b; });
    defun("string-lessp", [](const std::string& a, const std::string& b) { return a < b; });
    defun("string-greaterp", [](const std::string& a, const std::string& b) { return a > b; });
    defun("number-to-string", [](std::variant<std::int64_t, std::uint32_t, double> num) {
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
    defun("char-to-string", [](std::uint32_t c1) { return utf8::encode(c1); });
    defun("string-to-number", [](std::string str) -> ObjectPtr {
        std::stringstream ss(str);
        if (str.find("e") == std::string::npos && str.find(".") == std::string::npos) {
            std::int64_t i;
            ss >> i;
            return makeInt(i);
        }
        double f;
        ss >> f;
        return makeFloat(f);
    });
    defun("parse-integer", [](std::string str) {
        std::stringstream ss(str);
        std::int64_t i;
        ss >> i;
        return i;
    });
    defun("force-output", [](std::ostream* stream) {
        assert(stream);
        return (*stream) << std::flush, false;
    });
    defun("read-line", [](std::istream* stream) {
        std::string str;
        std::getline(*stream, str);
        return str;
    });
    defun("message", [this](String str, Rest& args) {
        str = format(str, args);
        std::cout << str << std::endl;
        return str;
    });
    defun("format", [](String formatString, Rest& args) {
        return format(formatString, args);
    });

    // Common lisp format prototype...
    /*
    m.defun("format", [](Stream stream, String formatString, Rest& args) -> ObjectPtr {
        std::ostream* ostream = nullptr;
        if (stream.isNilStream()) {

        }
        else {
            ostream = stream.ostream;
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
        if (stream.isNilStream()) {
            return std::make_unique<StringObject>(s);
        }
        (*ostream) << s;
        return args.m.makeNil();
    });
    */
}

}
