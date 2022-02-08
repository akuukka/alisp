#include "Symbol.hpp"
#include "Object.hpp"
#include "FunctionObject.hpp"

namespace alisp
{

std::shared_ptr<Function> Symbol::resolveFunction() const
{
    if (function->isFunction()) {
        return dynamic_cast<const FunctionObject*>(function.get())->value;
    }
    return nullptr;
}

}
