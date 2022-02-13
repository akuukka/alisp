#pragma once
#include "Object.hpp"
#include "Sequence.hpp"
#include "SharedValueObject.hpp"
#include "String.hpp"

namespace alisp
{

struct StringObject :
        SharedValueObject<std::string>,
        Sequence,
        ConvertibleTo<String>
{
    StringObject(std::string value);
    StringObject(const StringObject& o);
    StringObject(const String& o);

    std::string toString(bool aesthetic = false) const override
    {
        return aesthetic ? *value : ("\"" + *value + "\"");
    }

    bool isString() const override { return true; }
    bool equal(const Object& obj) const override;

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<StringObject>(*this);
    }

    ObjectPtr copy() const override;
    ObjectPtr reverse() const override;
    std::unique_ptr<ConsCellObject> mapCar(const Function& func) const override;

    size_t length() const override;
    std::unique_ptr<Object> elt(std::int64_t index) const override;

    String convertTo(ConvertibleTo<String>::Tag) const override { return String(value); }
};

}
