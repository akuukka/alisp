#include <any>
#include <optional>
#include "ValueObject.hpp"
#include "StringObject.hpp"
#include "alisp.hpp"
#include "Converter.hpp"
#include "ConsCellObject.hpp"
#include "Symbol.hpp"
#include "SymbolObject.hpp"
#include "Machine.hpp"

namespace alisp
{

template<>
ALISP_INLINE std::optional<std::shared_ptr<Symbol>> getValue(const Object& sym)
{
    if (sym.isNil()) {
        return sym.asList()->parent->getSymbol("nil");
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
ALISP_INLINE std::optional<std::unique_ptr<Object>> getValue(const Object& sym)
{
    return sym.clone();
}

template<>
ALISP_INLINE std::optional<bool> getValue(const Object &sym)
{
    return !!sym;
}

template<>
ALISP_INLINE std::optional<StringObject> getValue(const Object& sym)
{
    if (sym.isString()) {
        return dynamic_cast<const StringObject&>(sym);
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::uint32_t> getValue(const Object &sym)
{
    auto s = dynamic_cast<const CharacterObject*>(&sym);
    if (s) {
        return s->value;
    }
    auto i = dynamic_cast<const IntObject*>(&sym);
    if (i && i->value >=0 && i->value <= std::numeric_limits<std::uint32_t>::max()) {
        return i->value;
    }
    return std::nullopt;
}

template<>
ALISP_INLINE std::optional<std::any> getValue(const Object& sym)
{
    if (auto s = dynamic_cast<const ConsCellObject*>(&sym)) {
        return s->cc;
    }
    else if (auto s = dynamic_cast<const IntObject*>(&sym)) {
        return s->value;
    }
    else if (auto s = dynamic_cast<const FloatObject*>(&sym)) {
        return s->value;
    }
    else if (auto s = dynamic_cast<const StringObject*>(&sym)) {
        return s->value;
    }
    else if (auto s = dynamic_cast<const SymbolObject*>(&sym)) {
        return s->getSymbol();
    }
    return std::nullopt;
}

template<>
ALISP_INLINE const std::string& getValueReference(const Object &sym)
{
    if (sym.isString()) {
        return *dynamic_cast<const StringObject*>(&sym)->value;
    }
    throw std::runtime_error("Failed to get string arg");
}

template<>
ALISP_INLINE std::string& getValueReference(const Object &sym)
{
    if (sym.isString()) {
        return *dynamic_cast<const StringObject*>(&sym)->value;
    }
    throw std::runtime_error("Failed to get string arg");
}

template<>
ALISP_INLINE const Symbol& getValueReference(const Object &sym)
{
    if (sym.isNil()) {
        return *sym.asList()->parent->getSymbol("nil");
    }
    if (sym.isSymbol()) {
        return *dynamic_cast<const SymbolObject*>(&sym)->getSymbol();
    }
    throw std::runtime_error("Failed to get string arg");
}

}
