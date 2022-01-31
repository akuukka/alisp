#include "ConsCellObject.hpp"
#include "Machine.hpp"
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
    m.defun("split-string", [](std::string s,
                               std::optional<std::string> sep,
                               std::optional<bool> omitNulls) -> ObjectPtr {
        std::smatch m;
        std::regex e (sep ? *sep : "[ \\n\\t]+");

        if (sep && !omitNulls) {
            omitNulls = false;
        }

        ListBuilder builder;
        //std::cout << "s: " << s << std::endl;
        auto addMatch = [&](std::string m) {
            //std::cout << "add match: '" << m << "'" << std::endl;
            if (!omitNulls || *omitNulls) {
                if (m.empty()) {
                    // std::cout << " SKIP!\n";
                    return;
                }
            }
            builder.add(std::make_unique<StringObject>(std::move(m)));
        };

        auto begin = s.cbegin();
        auto end = s.cend();
        
        while (std::regex_search(begin, end, m, e)) {
            //std::cout << "left: '" << s << "'" << std::endl;
            for (auto x : m) {
                auto p = std::distance(s.cbegin(), begin);
                auto mp = p + m.position();
                //std::cout << "match pos:" << mp << " size: " << x.length() << std::endl;
                //std::cout << " => substr from " << p << " to " << (mp - p) <<  std::endl;
                addMatch(s.substr(p, mp - p));
                if (mp + x.str().length() == s.size()) {
                    addMatch("");
                }
                begin = m.suffix().first;
                break;
            }
            //std::cout << "left suffix: '" << s << "'" << std::endl;
        }
        return builder.get();
    });
}

}
