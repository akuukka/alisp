#pragma once
#include "SharedValueObject.hpp"
#include "FArgs.hpp"

namespace alisp
{

struct SubroutineObject : SharedValueObject<Function>
{
    SubroutineObject(std::shared_ptr<Function> func) : SharedValueObject<Function>(func) { }
    
    std::unique_ptr<Object> clone() const override 
    {
        return std::make_unique<SubroutineObject>(value);
    }
    
    std::string toString(bool aesthetic) const override { return "#<subr " + value->name + ">"; }
    std::shared_ptr<Function> resolveFunction() const override { return value; }
};

}
