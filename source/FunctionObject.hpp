#pragma once
#include "Object.hpp"
#include "FArgs.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

struct FunctionObject : Object, ConvertibleTo<const Function&>
{
    std::shared_ptr<Function> value;

    FunctionObject(std::shared_ptr<Function> func) : value(func) { }
    std::string toString(bool aesthetic) const override
    {
        if (!value->closure) {
            return "#<subr " + value->name + ">";
        }
        return value->closure->toString(aesthetic);
    }

    std::unique_ptr<Object> clone() const override 
    {
        return std::make_unique<FunctionObject>(value);
    }

    bool equals(const Object& o) const override
    {
        const FunctionObject* op = dynamic_cast<const FunctionObject*>(&o);
        if (!op) {
            return false;
        }
        return value == op->value;
    }

    const Function& convertTo(ConvertibleTo<const Function&>::Tag) const override
    {
        return *value;
    }

    std::shared_ptr<Function> resolveFunction() const override
    {
        return value;
    }

    bool isFunction() const override { return true; }
};

}
