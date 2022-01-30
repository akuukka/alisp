#pragma once
#include <cstddef>
#include <memory>

namespace alisp {

struct Object;

struct Sequence
{
    virtual size_t length() const = 0;
    virtual std::unique_ptr<Object> elt(std::int64_t index) const = 0;
};

}
