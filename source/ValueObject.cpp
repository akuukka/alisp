#include "ValueObject.hpp"
#include "alisp.hpp"
#include <limits>

namespace alisp
{

template<>
ALISP_INLINE std::optional<double> getValue(const Object &sym)
{
    auto s = dynamic_cast<const FloatObject*>(&sym);
    if (s) {
        return s->value;
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::int64_t> getValue(const Object &sym)
{
    auto s = dynamic_cast<const IntObject*>(&sym);
    if (s) {
        return s->value;
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::uint32_t> getValue(const Object &sym)
{
    auto s = dynamic_cast<const CharacterObject*>(&sym);
    if (s) {
        return s->value;
    }
    auto i = dynamic_cast<const IntObject*>(&sym);
    if (i && i->value >=0 && i->value <= std::numeric_limits<std::uint32_t>::max()) {
        return i->value;
    }
    return std::nullopt;
}

}
