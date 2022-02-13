#include "ConsCellObject.hpp"
#include "Error.hpp"
#include "Object.hpp"
#include "alisp.hpp"
#include "Machine.hpp"
#include "SymbolObject.hpp"
#include <memory>

namespace alisp
{

ALISP_INLINE Symbol::Symbol(Machine& parent) : parent(&parent) {}
ALISP_INLINE Symbol::~Symbol() {}

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
        if ((*it).eq(property)) {
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

ALISP_INLINE Object* get(std::unique_ptr<ConsCellObject>& plist, const Object& property)
{
    return plist ? get(*plist, property) : nullptr;
}

ALISP_INLINE void Machine::initSymbolFunctions()
{
    defun("make-symbol", [&](const std::string& name) -> ObjectPtr {
        std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>(*this);
        symbol->name = name;
        return std::make_unique<SymbolObject>(this, symbol, "");
    });
    defun("symbol-plist", [&](Symbol& symbol) { return getPlist(symbol)->clone(); });
    defun("symbol-name", [](const Symbol& sym) { return sym.name; });
    defun("symbolp", [](const Object& obj) { return obj.isSymbol(); });
    defun("put", [&](Symbol& symbol, const Object& property, const Object& value) {
        auto plist = getPlist(symbol);
        if (plist->isNil()) {
            plist->setCar(property.clone());
            plist->setCdr(makeConsCell(value.clone(), nullptr));
        }
        else {
            for (auto cc = plist->cc.get(); cc != nullptr; cc = cc->next()) {
                const Object& keyword = *cc->car;
                if (keyword.eq(property)) {
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
    defun("intern", [this](std::string name) -> std::unique_ptr<Object> {
            return std::make_unique<SymbolObject>(this, getSymbol(name));
        });
    defun("unintern", [this](const Symbol& sym) {
        const bool uninterned =
            m_syms.count(sym.name) && m_syms[sym.name].get() == &sym && m_syms.erase(sym.name);
        return uninterned;
    });
    defun("intern-soft", [this](const std::string& name) {
        std::unique_ptr<Object> r;
        if (!m_syms.count(name)) {
            r = makeNil();
        }
        else {
            r = std::make_unique<SymbolObject>(this, getSymbol(name));
        }
        return r;
    });
    defun("symbol-value", [this](const Symbol& sym) {
        if (!sym.variable) {
            throw exceptions::VoidVariable(sym.name);
        }
        return sym.variable->clone();
    });
    defun("boundp", [](const Symbol& sym) { return sym.variable ? true : false; });
    defun("makunbound", [](std::shared_ptr<Symbol> sym) {
        if (!sym || sym->constant) {
            throw exceptions::SettingConstant(sym ? sym->name : std::string("nil"));
        }
        sym->variable = nullptr;
        return sym;
    });
}

}
