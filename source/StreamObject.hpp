#pragma once

namespace alisp
{

struct OStreamObject : ValueObject<std::ostream*>
{
    OStreamObject(std::ostream* stream) : ValueObject<std::ostream*>(stream) {}
    std::string toString(bool) const override { return "<ostream>"; }
    ObjectPtr clone() const override { return std::make_unique<OStreamObject>(value); }
};

struct IStreamObject : ValueObject<std::istream*>
{
    IStreamObject(std::istream* stream) : ValueObject<std::istream*>(stream) {}
    std::string toString(bool) const override { return "<istream>"; }
    ObjectPtr clone() const override { return std::make_unique<IStreamObject>(value); }
};

struct IOStreamObject : Object, ConvertibleTo<std::istream*>, ConvertibleTo<std::ostream*>
{
    std::istream* istream;
    std::ostream* ostream;
    
    IOStreamObject(std::istream* istream, std::ostream* ostream) :
        istream(istream), ostream(ostream)
    {
        
    }
    std::string toString(bool) const override { return "<iostream>"; }
    ObjectPtr clone() const override { return std::make_unique<IOStreamObject>(istream, ostream); }
    std::istream* convertTo(ConvertibleTo<std::istream*>::Tag) const override { return istream; }
    std::ostream* convertTo(ConvertibleTo<std::ostream*>::Tag) const override { return ostream; }
};

}
