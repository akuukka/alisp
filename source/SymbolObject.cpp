#include "SymbolObject.hpp"
#include "alisp.hpp"
#include "Machine.hpp"

namespace alisp {

ALISP_INLINE std::unique_ptr<Object> SymbolObject::eval() 
{
    const auto var = sym ? sym->variable.get() : parent->getSymbol(name)->variable.get();
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

ALISP_INLINE std::string SymbolObject::toString(bool aesthetic) const
{
    // Interned symbols with empty string as name have special printed form of ##. See:
    // https://www.gnu.org/software/emacs/manual/html_node/elisp/Special-Read-Syntax.html
    std::string n = sym ? sym->name : name;
    if (aesthetic && n.size() && n[0] == ':') {
        n = n.substr(1);
    }
    return n.size() ? n : "##";
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
