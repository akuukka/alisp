#pragma once
#include "Object.hpp"
#include "FArgs.hpp"
#include "ConsCellObject.hpp"

namespace alisp
{

/*
 TODO: get rid of this class. funcall should support ConsCell/Symbol parameters
 */
struct FunctionObject : Object, ConvertibleTo<const Function&>
{
    std::shared_ptr<Function> value;

    FunctionObject(std::shared_ptr<Function> func) : value(func) { }
    std::string typeId() const override { return "function"; }
    std::string toString(bool aesthetic) const override
    {
        return value->closure ? value->closure->toString(aesthetic) : value->name;
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
};

}
