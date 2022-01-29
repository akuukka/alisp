#include "alisp.hpp"
#include "StringObject.hpp"

namespace alisp {

template<>
ALISP_INLINE std::optional<std::string> getValue(const Object& sym)
{
    auto s = dynamic_cast<const StringObject*>(&sym);
    if (s) {
        return *s->value;
    }
    return std::nullopt;
}

}
