#include "SymbolObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"

namespace alisp {

ALISP_INLINE std::unique_ptr<Object> SymbolObject::eval() 
{
    const auto var = sym ? sym->variable.get() : parent->resolveVariable(name);
    if (!var) {
        throw exceptions::VoidVariable(toString());
    }
    return var->clone();
}

ALISP_INLINE Function* SymbolObject::resolveFunction()
{
    if (sym) {
        return sym->function.get();
    }
    assert(name.size());
    return parent->resolveFunction(name);
}

ALISP_INLINE Symbol* SymbolObject::getSymbolOrNull() const
{
    return sym ? sym.get() : parent->getSymbolOrNull(name).get();
}

ALISP_INLINE std::shared_ptr<Symbol> SymbolObject::getSymbol() const
{
    return sym ? sym : parent->getSymbol(name);
}

}
