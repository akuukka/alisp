#include "ConsCellObject.hpp"
#include "Machine.hpp"
#include "Object.hpp"
#include "StringObject.hpp"
#include <algorithm>
#include <cstdint>
#include <regex>

namespace alisp
{

void initStringFunctions(Machine& m)
{
    m.defun("char-or-string-p", [](ObjectPtr obj) {
        return obj->isString() || obj->isCharacter();
    });
    m.defun("make-string", [](std::int64_t num, std::uint32_t c) {
        return std::string(num, c);
    });
    m.defun("stringp", [](ObjectPtr obj) { return obj->isString(); });
    m.defun("string-or-null-p", [](ObjectPtr obj) { return obj->isString() || obj->isNil(); });
    m.defun("concat", [](const std::string& str1, const std::string& str2) { return str1 + str2; });
    m.defun("substring", [](const std::string& str,
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
    m.defun("string", [](std::vector<std::uint32_t> chars) {
        std::string ret;
        std::transform(chars.begin(), chars.end(), std::back_inserter(ret), [](std::uint32_t c) {
            return static_cast<char>(c);
        });
        return ret;
    });
    m.defun("clear-string", [](std::string& str) {
        for (auto& c : str) {
            c = 0;
        }
        return false;
    });
    m.defun("split-string", [](std::string s,
                               std::optional<std::string> sep,
                               std::optional<bool> omitNulls) -> ObjectPtr {
        std::smatch m;
        std::regex e(sep ? *sep : "[ \\n\\t\\r\\v]+");

        if (sep && !omitNulls) {
            omitNulls = false;
        }

        ListBuilder builder;
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
}

}
