#include "alisp.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"
#include <stdexcept>
#include "Exception.hpp"

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

ALISP_INLINE std::unique_ptr<Object> StringObject::elt(std::int64_t index) const
{
    if (static_cast<size_t>(index) >= value->size()) {
        throw alisp::exceptions::Error("Index out of range");
    }
    return std::make_unique<CharacterObject>(value->at(index));
}

}
