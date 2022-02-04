#pragma once
#include "Object.hpp"
#include <cstdint>

namespace alisp {

template<typename T>
struct ValueObject : Object, ConvertibleTo<T>
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
    T convertTo(typename ConvertibleTo<T>::Tag) const override { return value; }
};

struct Number
{
    std::int64_t i;
    double f;
    bool isFloat;
    Number(std::int64_t i) : i(i), f(i), isFloat(false) {}
    Number(double f) : i(f), f(f), isFloat(true) {}
    Number() = default;
};

struct IntObject :
        ValueObject<std::int64_t>,
        ConvertibleTo<int>,
        ConvertibleTo<std::uint32_t>,
        ConvertibleTo<Number>
{
    IntObject(std::int64_t value) : ValueObject<std::int64_t>(value) {}
    bool isInt() const override { return true; }
    std::string typeId() const override { return "integer"; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<IntObject>(value); }
    bool isCharacter() const override;

    int convertTo(ConvertibleTo<int>::Tag) const override { return value; }
    std::uint32_t convertTo(ConvertibleTo<std::uint32_t>::Tag) const override { return value; }
    Number convertTo(ConvertibleTo<Number>::Tag) const override {
        return Number(value);
    }
};

struct FloatObject :
        ValueObject<double>,
        ConvertibleTo<Number>
{
    FloatObject(double value) : ValueObject<double>(value) {}
    bool isFloat() const override { return true; }
    std::string typeId() const override { return "float"; }
    std::unique_ptr<Object> clone() const override { return std::make_unique<FloatObject>(value); }
    Number convertTo(ConvertibleTo<Number>::Tag) const override {
        return Number(value);
    }
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
