#pragma once
#include "Object.hpp"
#include "Sequence.hpp"
#include "String.hpp"

namespace alisp
{

struct StringObject : Object, Sequence
{
    std::shared_ptr<std::string> value;

    StringObject(std::string value) : value(std::make_shared<std::string>(value)) {}
    StringObject(const StringObject& o) : value(o.value) {}
    StringObject(const String& o) : value(o.sharedPointer()) {}

    std::string toString(bool aesthetic = false) const override
    {
        return aesthetic ? *value : ("\"" + *value + "\"");
    }

    bool isString() const override { return true; }
    std::string typeId() const override { return "string"; }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<StringObject>(*this);
    }

    bool equals(const Object& o) const override
    {
        const StringObject* str = dynamic_cast<const StringObject*>(&o);
        return str && str->value == value;
    }

    size_t length() const override;

    std::unique_ptr<Object> elt(std::int64_t index) const override;
};

}
