#pragma once
#include "Object.hpp"
#include "FArgs.hpp"

namespace alisp
{

struct SubroutineObject : Object
{
    std::shared_ptr<Function> value;

    SubroutineObject(std::shared_ptr<Function> func) : value(func) { }
    std::string toString(bool aesthetic) const override
    {
        return "#<subr " + value->name + ">";
    }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<SubroutineObject>(value);
    }

    bool equals(const Object& o) const override
    {
        const SubroutineObject* op = dynamic_cast<const SubroutineObject*>(&o);
        if (!op) {
            return false;
        }
        return value == op->value;
    }

    std::shared_ptr<Function> resolveFunction() const override { return value; }
};

}
