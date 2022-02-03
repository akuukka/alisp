#pragma once
#include "Object.hpp"
#include <iostream>
#include <memory>

namespace alisp
{

struct Stream
{
    std::istream* istream = nullptr;
    std::ostream* ostream = nullptr;

    Stream(std::istream* istream, std::ostream* ostream)
    {
        this->istream = istream;
        this->ostream = ostream;
    }
    
    bool isNilStream() const
    {
        return !istream && !ostream;
    }

    static const Stream& getEmptyStream()
    {
        static Stream s(nullptr, nullptr);
        return s;
    }
};

struct StreamObject : Object
{
    std::shared_ptr<Stream> stream;

    StreamObject(std::istream* istream, std::ostream* ostream)
    {
        stream = std::make_shared<Stream>(istream, ostream);
    }

    StreamObject(std::shared_ptr<Stream> stream) : stream(stream) {}

    std::string toString(bool aesthetic) const override
    {
        return "<stream>";
    }

    ObjectPtr clone() const override
    {
        return std::make_unique<StreamObject>(stream);
    }
};

}
