#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "Object.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

ALISP_STATIC ConsCellObject* getPlist(Symbol& symbol)
{
    if (!symbol.plist) {
        symbol.plist = symbol.parent->makeConsCell(nullptr, nullptr);
    }
    return symbol.plist.get();
}

ALISP_INLINE Object* get(const ConsCell& plist, const Object& property)
{
    for (auto it = plist.begin(); it != plist.end();) {
        if ((*it).equals(property)) {
            if ((it + 1) == plist.end()) {
                return nullptr;
            }
            ++it;
            return &(*it);
        }
        ++it;
        if (it == plist.end()) break;
        ++it;
    }
    return nullptr;
}

ALISP_INLINE Object* get(const ConsCellObject& plist, const Object& property)
{
    return plist.cc ? get(*plist.cc, property) : nullptr;
}

ALISP_INLINE void Machine::initSymbolFunctions()
{
    defun("make-symbol", [&](const std::string& name) -> ObjectPtr {
        std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>();
        symbol->name = name;
        return std::make_unique<SymbolObject>(this, symbol, "");
    });
    defun("symbol-plist", [&](Symbol& symbol) {
        return getPlist(symbol)->clone();
    });
    defun("put", [&](Symbol& symbol, const Object& property, const Object& value) {
        auto plist = getPlist(symbol);
        if (plist->isNil()) {
            plist->setCar(property.clone());
            plist->setCdr(makeConsCell(value.clone(), nullptr));
        }
        else {
            for (auto cc = plist->cc.get(); cc != nullptr; cc = cc->next()) {
                const Object& keyword = *cc->car;
                if (keyword.equals(property)) {
                    cc = cc->next();
                    assert(cc);
                    cc->car = value.clone();
                    break;
                }
                if (!cc->next()) {
                    throw exceptions::WrongTypeArgument("Not a proper plist.");
                }
                cc = cc->next();
                if (!cc->next()) {
                    cc->cdr = makeConsCell(property.clone(), makeConsCell(value.clone()));
                }
            }
        }
        return value.clone();
    });
}

}
