#include "SymbolObject.hpp"
#include "Error.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "ConsCellObject.hpp"

namespace alisp {

ALISP_INLINE bool operator==(const Object* obj, std::shared_ptr<Symbol> sym)
{
    return obj && obj->isSymbol() && obj->asSymbol()->getSymbol() == sym;
}

ALISP_INLINE void SymbolObject::traverse(const std::function<bool(const Object&)>& f) const
{
    if (!f(*this) || !sym) {
        return;
    }
    if (sym->variable) {
        sym->variable->traverse(f);
    }
    if (sym->plist) {
        sym->plist->traverse(f);
    }
}

ALISP_INLINE bool SymbolObject::equals(const Object& o) const
{
    const SymbolObject* op = dynamic_cast<const SymbolObject*>(&o);
    if (!op) {
        return false;
    }
    const Symbol* lhs = getSymbol().get();
    const Symbol* rhs = op->getSymbol().get();
    return lhs == rhs;
}

ALISP_INLINE std::unique_ptr<Object> SymbolObject::eval() 
{
    const auto var = sym ? sym->variable.get() : parent->getSymbol(name)->variable.get();
    if (!var) {
        throw exceptions::VoidVariable(toString());
    }
    return var->clone();
}

ALISP_INLINE std::shared_ptr<Function> SymbolObject::resolveFunction() const
{
    if (sym) {
        return sym->function->resolveFunction();
    }
    auto sym = parent->getSymbol(name);
    if (sym && !sym->function && sym->local) {
        sym = parent->getSymbol(name, true);
    }
    if (!sym->function) {
        throw exceptions::VoidFunction(toString());
    }
    return sym->function->resolveFunction();
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
