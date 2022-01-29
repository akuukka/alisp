#pragma once
#include "Object.hpp"
#include "Symbol.hpp"

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

    Symbol* getSymbol() const;
    Symbol* getSymbolOrNull() const;

    Function* resolveFunction() override;

    void reset() override { sym.reset(); }

    std::string toString() const override
    {
        // Interned symbols with empty string as name have special printed form of ##. See:
        // https://www.gnu.org/software/emacs/manual/html_node/elisp/Special-Read-Syntax.html
        std::string n = sym ? sym->name : name;
        return n.size() ? n : "##";
    }

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
