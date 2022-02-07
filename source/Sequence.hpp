#pragma once
#include "Object.hpp"
#include <cstddef>
#include <memory>

namespace alisp {

struct Object;

struct Sequence : ConvertibleTo<const Sequence&>
{
    virtual ObjectPtr copy() const = 0;
    virtual ObjectPtr elt(std::int64_t index) const = 0;
    virtual size_t length() const = 0;
    virtual ObjectPtr reverse() const = 0;
    const Sequence& convertTo(ConvertibleTo<const Sequence&>::Tag) const override { return *this; }
};

}
