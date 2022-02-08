#pragma once
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <iostream>
#include <type_traits>
#include <variant>
#include "Template.hpp"

namespace alisp
{

struct Function;
struct ConsCellObject;
struct SymbolObject;

#ifdef ENABLE_DEBUG_REFCOUNTING

inline int changeRefCount(int i) {
    static int refcnt = 0;
    refcnt += i;
    return refcnt;
}

#endif

template<typename T>
struct ConvertibleTo
{
    struct Tag{};
    virtual T convertTo(Tag) const = 0;
    virtual bool canConvertTo(Tag) const { return true; };
};

struct Object :
        ConvertibleTo<bool>,
        ConvertibleTo<const Object&>,
        ConvertibleTo<std::unique_ptr<Object>>
{
    template<typename T>
    bool isConvertibleTo() const
    {
        if constexpr (IsInstantiationOf<std::variant, T>::value) {
            return isConvertibleToVariant<T, 0>();
        }
        else {
            auto asConvertible = dynamic_cast<const ConvertibleTo<T>*>(this);
            return asConvertible &&
                asConvertible->canConvertTo(typename ConvertibleTo<T>::Tag());
        }
    }

    template<typename T, size_t I> const
    bool isConvertibleToVariant() const
    {
        if constexpr (I == std::variant_size_v<T>) {
            return false;
        }
        else {
            return isConvertibleTo<std::variant_alternative_t<I, T>>() ||
                isConvertibleToVariant<T, I+1>();
        }
    }

    bool convertTo(ConvertibleTo<bool>::Tag) const { return !isNil(); }
    const Object& convertTo(ConvertibleTo<const Object&>::Tag) const { return *this; }
    std::unique_ptr<Object> convertTo(ConvertibleTo<std::unique_ptr<Object>>::Tag) const {
        return clone();
    }
    
#ifdef ENABLE_DEBUG_REFCOUNTING
    Object()
    {
        changeRefCount(1);
        getAllObjects().insert(this);
    }
    
    Object(const Object&)
    {
        changeRefCount(1);
        getAllObjects().insert(this);
    }
    
    virtual ~Object()
    {
        changeRefCount(-1);
        getAllObjects().erase(this);
    }
    static int getDebugRefCount() { return changeRefCount(0); }
    static bool& destructionDebug()
    {
        static bool dbg = false;
        return dbg;
    }
    static std::set<Object*>& getAllObjects()
    {
        static std::set<Object*> l;
        return l;
    }
    static void printAllObjects()
    {
        for (auto p: getAllObjects()) {
            std::cout << p->toString() << std::endl;
        }
    }
#else
    virtual ~Object() {}
#endif
    virtual std::string toString(bool aesthetic = false) const = 0;
    virtual bool isList() const { return false; }
    virtual bool isNil() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool isCharacter() const { return false; }
    virtual bool isSymbol() const { return false; }
    virtual bool isFunction() const { return false; }

    virtual bool operator!() const { return false; }
    virtual Object* trySelfEvaluate() { return nullptr; }

    virtual std::shared_ptr<Function> resolveFunction() const { return nullptr; }
    virtual std::unique_ptr<Object> clone() const = 0;
    virtual bool equals(const Object& o) const { return false; }

    virtual ConsCellObject* asList() { return nullptr; }
    virtual const ConsCellObject* asList() const { return nullptr; }
    virtual SymbolObject* asSymbol() { return nullptr; }
    virtual const SymbolObject* asSymbol() const { return nullptr; }

    template <typename T>
    T value() const
    {
        if (!isConvertibleTo<T>()) {
            throw std::runtime_error("No type conversion available.");
        }
        if constexpr (IsInstantiationOf<std::variant, T>::value) {
            return valueToVariant<T, 0>();
        }
        else {
            auto asConvertible = dynamic_cast<const ConvertibleTo<T>*>(this);
            return asConvertible->convertTo(typename ConvertibleTo<T>::Tag());
        }
    }

    template <typename T, size_t I>
    T valueToVariant() const
    {
        if constexpr (I == std::variant_size_v<T>) {
            throw std::runtime_error("Conversion to variant failed.");
        }
        else {
            if (isConvertibleTo<std::variant_alternative_t<I, T>>()) {
                return value<std::variant_alternative_t<I, T>>();
            }
            return valueToVariant<T, I+1>();
        }
    }

    template <typename T> std::optional<T> valueOrNull() const
    {
        return isConvertibleTo<T>() ? std::optional<T>(value<T>()) : std::nullopt;
    }

    virtual std::unique_ptr<Object> eval() { return clone(); }    
    virtual void traverse(const std::function<bool(const Object&)>& f) const { f(*this); }
};

using ObjectPtr = std::unique_ptr<Object>;

std::ostream &operator<<(std::ostream &os, const Object &sym);
std::ostream &operator<<(std::ostream &os, const std::unique_ptr<Object> &sym);

}
