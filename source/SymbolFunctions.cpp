#include "ConsCellObject.hpp"
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

void Machine::initSymbolFunctions()
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
            bool found = false;
            for (auto cc = plist->cc.get(); cc != nullptr; cc = cc->next()) {
                const Object& keyword = *cc->car;
                if (keyword.equals(property)) {
                    cc = cc->next();
                    assert(cc);
                    cc->car = value.clone();
                    found = true;
                    break;
                }
            }
        }
        return value.clone();
    });
}

}
