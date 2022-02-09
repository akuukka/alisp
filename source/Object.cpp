#include "Object.hpp"
#include "Error.hpp"
#include "alisp.hpp"

namespace alisp
{

ALISP_INLINE std::shared_ptr<Function> Object::resolveFunction() const
{
    throw exceptions::InvalidFunction(toString());
}

ALISP_INLINE std::ostream &operator<<(std::ostream &os, const Object &sym)
{
    os << sym.toString();
    return os;
}

ALISP_INLINE std::ostream &operator<<(std::ostream &os, const std::unique_ptr<Object> &sym)
{
    if (sym) {
        os << sym->toString();
    }
    else {
        os << "nullptr";
    }
    return os;
}

}
