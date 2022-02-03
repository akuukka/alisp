#pragma once
#include "Object.hpp"
#include <cstdint>

namespace alisp {

template<typename T>
struct ValueObject : Object
{
    T value;
    ValueObject(T t) : value(t) {}
    ValueObject(const ValueObject& o) :
        Object(o),
        value(o.value)
    {}
    
    bool equals(const Object& o) const override
    {
        const ValueObject<T>* op = dynamic_cast<const ValueObject<T>*>(&o);
        return op && op->value == value;
    }
    std::string toString(bool aesthetic = false) const override { return std::to_string(value); }
    Object* trySelfEvaluate() override { return this; }
};

struct IntObject : ValueObject<std::int64_t>
{
    IntObject(std::int64_t value) : ValueObject<std::int64_t>(value) {}
    bool isInt() const override { return true; }
    std::string typeId() const override { return "integer"; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<IntObject>(value); }
};

struct FloatObject : ValueObject<double>
{
    FloatObject(double value) : ValueObject<double>(value) {}
    bool isFloat() const override { return true; }
    std::string typeId() const override { return "float"; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<FloatObject>(value); }
};

struct Number
{
    std::int64_t i;
    double f;
    bool isFloat;
};

struct CharacterObject : ValueObject<std::uint32_t>
{
    CharacterObject(std::uint32_t c) : ValueObject<std::uint32_t>(c) {}
    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<CharacterObject>(value);
    }
    std::string typeId() const override { return "char"; }
    std::string toString(bool aesthetic = false) const override;
    bool isCharacter() const override { return true; }
};

inline std::unique_ptr<IntObject> makeInt(std::int64_t value)
{
    return std::make_unique<IntObject>(value);
}

inline std::unique_ptr<FloatObject> makeFloat(double value)
{
    return std::make_unique<FloatObject>(value);
}

}
