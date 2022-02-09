#include "Object.hpp"
#include "Error.hpp"

namespace alisp
{

std::shared_ptr<Function> Object::resolveFunction() const
{
    throw exceptions::InvalidFunction(toString());
}

std::ostream &operator<<(std::ostream &os, const Object &sym)
{
    os << sym.toString();
    return os;
}

std::ostream &operator<<(std::ostream &os, const std::unique_ptr<Object> &sym)
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
