#include "alisp.hpp"
#include "FArgs.hpp"
#include "ConsCellObject.hpp"
#include "SharedValueObject.hpp"
#include "StringObject.hpp"
#include "ValueObject.hpp"
#include <stdexcept>
#include "Error.hpp"
#include "UTF8.hpp"

namespace alisp {

ALISP_INLINE StringObject::StringObject(std::string value) :
    SharedValueObject<std::string>(std::make_shared<std::string>(std::move(value)))
{

}

ALISP_INLINE StringObject::StringObject(const StringObject& o) :
    SharedValueObject<std::string>(o.value)
{

}

ALISP_INLINE StringObject::StringObject(const String& o) :
    SharedValueObject<std::string>(o.sharedPointer())
{

}

ALISP_INLINE
size_t StringObject::length() const
{
    return value ? utf8::strlen(value->c_str()) : 0;
}

ALISP_INLINE ObjectPtr StringObject::copy() const
{
    return std::make_unique<StringObject>(*value);
}

ALISP_INLINE ObjectPtr StringObject::reverse() const
{
    std::string reversed;
    for (const std::uint32_t codepoint : String(value)) {
        reversed = utf8::encode(codepoint) + reversed;
    }
    return std::make_unique<StringObject>(reversed);
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

ListPtr StringObject::mapCar(const Function& func) const
{
    ListBuilder builder(func.parent);
    std::unique_ptr<IntObject> integer = makeInt(0);
    IntObject* ptr = integer.get();
    ConsCell cc;
    cc.car = std::move(integer);
    for (const std::uint32_t codepoint : String(value)) {
        ptr->value = codepoint;
        FArgs args(cc, func.parent);
        builder.append(func.func(args));
    }
    return builder.get();

}

}
