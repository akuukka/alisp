#include "alisp.hpp"
#include "ValueObject.hpp"
#include "UTF8.hpp"

namespace alisp
{

ALISP_INLINE bool IntObject::isCharacter() const
{
    return value >= 0 && value <= utf8::MaxChar;
}

}
