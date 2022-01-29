#include "alisp.hpp"
#include "Machine.hpp"

namespace alisp {

std::unique_ptr<Object> SymbolObject::eval() 
{
    const auto var = sym ? sym->variable.get() : parent->resolveVariable(name);
    if (!var) {
        throw exceptions::VoidVariable(toString());
    }
    return var->clone();
}

Function* SymbolObject::resolveFunction()
{
    if (sym) {
        return sym->function.get();
    }
    assert(name.size());
    return parent->resolveFunction(name);
}

Symbol* SymbolObject::getSymbolOrNull() const
{
    return sym ? sym.get() : parent->getSymbolOrNull(name).get();
}

Symbol* SymbolObject::getSymbol() const
{
    return sym ? sym.get() : parent->getSymbol(name).get();
}

}
