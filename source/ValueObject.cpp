#include "alisp.hpp"
#include "ValueObject.hpp"
#include "UTF8.hpp"

namespace alisp
{

ALISP_INLINE bool IntObject::isCharacter() const
{
    return utf8::isValidCodepoint(value);
}

}
