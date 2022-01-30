#pragma once
#include <set>
#include <string>
#include <iostream>
#include <type_traits>
#include "Converter.hpp"

namespace alisp
{

struct Function;
struct ConsCellObject;

#ifdef ENABLE_DEBUG_REFCOUNTING

inline int changeRefCount(int i) {
    static int refcnt = 0;
    refcnt += i;
    return refcnt;
}

#endif

struct Object
{
#ifdef ENABLE_DEBUG_REFCOUNTING
    Object()
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
    virtual std::string toString() const = 0;
    virtual bool isList() const { return false; }
    virtual bool isNil() const { return false; }
    virtual bool isInt() const { return false; }
    virtual bool isFloat() const { return false; }
    virtual bool isString() const { return false; }
    virtual bool operator!() const { return false; }
    virtual Object* trySelfEvaluate() { return nullptr; }

    virtual Function* resolveFunction() { return nullptr; }
    virtual std::unique_ptr<Object> clone() const = 0;
    virtual bool equals(const Object& o) const { return false; }

    virtual ConsCellObject* asList() { return nullptr; }
    virtual const ConsCellObject* asList() const { return nullptr; }

    template <typename T>
    typename std::enable_if<!std::is_reference<T>::value, T>::type value() const
    {
        const std::optional<T> opt = Converter<T>()(*this);
        if (opt) {
            return *opt;
        }
        throw std::runtime_error("Unable to convert object to desired type.");
    }

    template <typename T>
    typename std::enable_if<std::is_reference<T>::value, T>::type value() const
    {
        return Converter<T>()(*this);
    }

    template <typename T> std::optional<T> valueOrNull() const
    {
        Converter<T> conv;
        return conv(*this);
    }

    virtual std::unique_ptr<Object> eval() { return clone(); }    
    virtual void traverse(const std::function<bool(const Object&)>& f) const { f(*this); }
};

using ObjectPtr = std::unique_ptr<Object>;

std::ostream &operator<<(std::ostream &os, const Object &sym);
std::ostream &operator<<(std::ostream &os, const std::unique_ptr<Object> &sym);

}
