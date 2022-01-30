#include <any>
#include "StringObject.hpp"
#include "alisp.hpp"
#include "Converter.hpp"
#include "ConsCellObject.hpp"
#include "Symbol.hpp"
#include "SymbolObject.hpp"

namespace alisp
{

template<>
ALISP_INLINE std::optional<const Symbol*> getValue(const Object& sym)
{
    if (sym.isNil()) {
        return nullptr;
    }
    auto s = dynamic_cast<const SymbolObject*>(&sym);
    if (s) {
        return s->getSymbol().get();
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::shared_ptr<Symbol>> getValue(const Object& sym)
{
    if (sym.isNil()) {
        return nullptr;
    }
    auto s = dynamic_cast<const SymbolObject*>(&sym);
    if (s) {
        return s->getSymbol();
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<const Sequence*> getValue(const Object& sym)
{
    if (sym.isString() || sym.isList()) {
        return dynamic_cast<const Sequence*>(&sym);
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::shared_ptr<Object>> getValue(const Object& sym)
{
    return std::shared_ptr<Object>(sym.clone().release());
}

template<>
ALISP_INLINE std::optional<bool> getValue(const Object &sym)
{
    return !!sym;
}

template<>
ALISP_INLINE std::optional<std::any> getValue(const Object& sym)
{
    auto s = dynamic_cast<const ConsCellObject*>(&sym);
    if (s) {
        std::shared_ptr<ConsCell> cc = s->cc;
        return cc;
    }
    return std::any();
}

template<>
ALISP_INLINE const std::string& getValueReference(const Object &sym)
{
    if (sym.isString()) {
        return *dynamic_cast<const StringObject*>(&sym)->value;
    }
    throw std::runtime_error("Failed to get string arg");
}

}
