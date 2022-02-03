#pragma once
#include "Object.hpp"
#include <ios>
#include <iostream>

namespace alisp
{

struct StreamObject : Object
{
    std::istream* istream = nullptr;
    std::ostream* ostream = nullptr;

    StreamObject(std::istream* istream, std::ostream* ostream) :
        istream(istream), ostream(ostream)
    {
        
    }

    std::string toString(bool aesthetic) const override
    {
        return "<stream>";
    }

    ObjectPtr clone() const override
    {
        return std::make_unique<StreamObject>(istream, ostream);
    }
};

}
