#include "alisp.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"
#include <stdexcept>
#include "Error.hpp"
#include "UTF8.hpp"

namespace alisp {

ALISP_INLINE
size_t StringObject::length() const
{
    return value ? utf8::strlen(value->c_str()) : 0;
}

ALISP_INLINE std::unique_ptr<Object> StringObject::elt(std::int64_t index) const
{
    std::uint32_t encoded;
    size_t offset = 0;
    const char* start = value->c_str();
    for (std::int64_t i = 0; i <= index; i++) {
        if (offset >= value->size()) {
            throw std::runtime_error("Index out of range");
        }
        const size_t proceed = utf8::next(start + offset, &encoded);
        offset += proceed;
    }
    return makeInt(utf8::decode(encoded));
}

}
