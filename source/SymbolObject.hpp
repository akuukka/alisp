#pragma once
#include "Object.hpp"
#include "Symbol.hpp"
#include "SharedValueObject.hpp"

namespace alisp
{

struct SymbolObject :
        SharedValueObjectBase,
        ConvertibleTo<const Symbol&>,
        ConvertibleTo<Symbol&>,
        ConvertibleTo<std::shared_ptr<Symbol>>
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
    
    std::shared_ptr<Function> resolveFunction() const override;

    void reset() override { sym.reset(); }
    bool isSymbol() const override { return true; }
    std::string toString(bool aesthetic = false) const override;
    std::string typeOf() const override { return "symbol"; }

    std::unique_ptr<Object> clone() const override
    {
        return std::make_unique<SymbolObject>(parent, sym, name);
    }

    std::unique_ptr<Object> eval() override;

    bool eq(const Object& o) const override;

    SymbolObject* asSymbol() override { return this; }
    const SymbolObject* asSymbol() const override { return this; }

    const void* sharedDataPointer() const override { return sym.get(); }
    size_t sharedDataRefCount() const override { return sym.use_count(); }
    void traverse(const std::function<bool(const Object&)>& f) const override;

    const Symbol& convertTo(ConvertibleTo<const Symbol&>::Tag) const override
    {
        return *getSymbol();
    }

    Symbol& convertTo(ConvertibleTo<Symbol&>::Tag) const override { return *getSymbol(); }

    std::shared_ptr<Symbol> convertTo(ConvertibleTo<std::shared_ptr<Symbol>>::Tag) const override
    {
        return getSymbol();
    }
};

bool operator==(const Object* obj, std::shared_ptr<Symbol> sym);

}
