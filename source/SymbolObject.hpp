#pragma once
#include "Object.hpp"
#include "Symbol.hpp"
#include "SharedDataObject.hpp"

namespace alisp
{

struct SymbolObject : SharedDataObject
{
    std::shared_ptr<Symbol> sym;
    std::string name;
    Machine* parent;

    SymbolObject(Machine* parent,
                 std::shared_ptr<Symbol> sym = nullptr,
                 std::string name = "") :
        parent(parent),
        sym(sym),
        name(name)
    {

    }

    ~SymbolObject()
    {
        if (sym) tryDestroySharedData();
    }        

    const std::string getSymbolName()
    {
        return sym ? sym->name : name;
    };

    std::shared_ptr<Symbol> getSymbol() const;
    Symbol* getSymbolOrNull() const;
    std::string typeId() const override { return "symbol"; }
    
    Function* resolveFunction() override;

    void reset() override { sym.reset(); }
    bool isSymbol() const override { return true; }
    std::string toString(bool aesthetic = false) const override;

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<SymbolObject>(parent, sym, name);
    }

    std::unique_ptr<Object> eval() override;

    bool equals(const Object& o) const override
    {
        const SymbolObject* op = dynamic_cast<const SymbolObject*>(&o);
        if (!op) {
            return false;
        }
        const Symbol* lhs = getSymbolOrNull();
        const Symbol* rhs = op->getSymbolOrNull();
        return lhs == rhs;
    }

    SymbolObject* asSymbol() override { return this; }
    const SymbolObject* asSymbol() const override { return this; }

    const void* sharedDataPointer() const override { return sym.get(); }
    size_t sharedDataRefCount() const override { return sym.use_count(); }
    void traverse(const std::function<bool(const Object&)>& f) const override
    {
        if (f(*this) && sym && sym->variable) {
            sym->variable->traverse(f);
        }
    }
};

}
