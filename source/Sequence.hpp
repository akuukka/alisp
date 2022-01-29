#pragma once
#include <cstddef>

namespace alisp {

struct Sequence
{
    virtual size_t length() const = 0;
};

}
