#pragma once
#ifndef ALISP_SINGLE_HEADER
#define ALISP_INLINE  
#endif

#include <cstdint>
#include <string>
#include <variant>
#include <any>
#include <limits>
#include <map>
#include <cstdio>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <iostream>
#include <cassert>
#include <functional>
#include <optional>
#include <set>
#include "Object.hpp"
#include "SharedDataObject.hpp"
#include "AtScopeExit.hpp"
#include "Template.hpp"
#include "SymbolObject.hpp"
#include "ConsCell.hpp"
#include "Exception.hpp"

namespace alisp {

class Machine;

struct FArgs;
struct ConsCellObject;

struct Function
{
    std::string name;
    int minArgs = 0;
    int maxArgs = 0xffff;
    bool isMacro = false;
    std::function<std::unique_ptr<Object>(FArgs&)> func;
};

inline int countArgs(const ConsCell* cc)
{
    if (!cc || !(*cc)) {
        return 0;
    }
    int i = 0;
    while (cc) {
        i++;
        cc = cc->next();
    }
    return i;
}

struct FArgs
{
    ConsCell* cc;
    Machine& m;
    std::vector<std::unique_ptr<Object>> argStorage;
    
    FArgs(ConsCell& cc, Machine& m) : cc(&cc), m(m) {}

    Object* get();
    
    void skip()
    {
        cc = cc->next();
    }

    bool hasNext() const
    {
        return cc;
    }

    struct Iterator
    {
        Machine* m;
        ConsCell* cc;

        bool operator!=(const Iterator& o) const
        {
            return cc != o.cc;
        }

        void operator++()
        {
            cc = cc->next();
        }

        std::unique_ptr<Object> operator*();
    };

    Iterator begin()
    {
        return Iterator{&m, cc};
    }

    Iterator end()
    {
        return Iterator{&m, nullptr};
    }
};

template<size_t I, typename ...Args>
inline typename std::enable_if<I == sizeof...(Args), void>::type writeToTuple(std::tuple<Args...>&,
                                                                              FArgs&)
{}

template <size_t I, typename... Args>
inline constexpr typename std::enable_if<I == std::tuple_size_v<std::tuple<Args...>>,
                                         bool>::type
tupleOptCheck()
{
    return true;
}

template <size_t I, typename... Args>
inline constexpr typename std::enable_if<I < std::tuple_size_v<std::tuple<Args...>>,
                                             bool>::type
tupleOptCheck()
{
    using T = typename std::tuple_element<I, std::tuple<Args...>>::type;
    constexpr bool isOptionalParam = OptCheck<T>::value;
    return isOptionalParam;
}
    
    template<size_t I, typename ...Args>
    inline typename std::enable_if<I < sizeof...(Args), void>::type writeToTuple(std::tuple<Args...>& t,
                                                                                 FArgs& args)
    {
        using T = typename std::tuple_element<I, std::tuple<Args...>>::type;
        constexpr bool isOptionalParam = OptCheck<T>::value;
        static_assert(!isOptionalParam || tupleOptCheck<I+1, Args...>(),
                      "Non-optional function param given after optional one. ");
        std::optional<typename OptCheck<T>::BaseType> opt;
        Object* arg;
        bool conversionFailed = false;
        if (args.hasNext()) {
            arg = args.get();
            opt = arg->valueOrNull<typename OptCheck<T>::BaseType>();
            if (!opt) {
                if (isOptionalParam && arg->isNil()) {
                    // nil => std::nullopt makes sense
                }
                else {
                    conversionFailed = true;
                }
            }
        }
        if (!opt && (!isOptionalParam || conversionFailed)) {
            throw exceptions::WrongTypeArgument(arg->toString());
        }
        if (opt) {
            std::get<I>(t) = std::move(*opt);
        }
        writeToTuple<I + 1>(t, args);
    }
        
        template<typename ...Args>
        std::tuple<Args...> toTuple(FArgs& args)
        {
            std::tuple<Args...> tuple;
            writeToTuple<0>(tuple, args);
            return tuple;
        }

inline bool isWhiteSpace(const char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline Object* FArgs::get()
{
    if (!cc) {
        return nullptr;
    }
    auto self = cc->car->trySelfEvaluate();
    if (self && false) {
        cc = cc->next();
        return self;
    }
    argStorage.push_back(cc->car->eval());
    cc = cc->next();
    return argStorage.back().get();
}

inline std::unique_ptr<Object> FArgs::Iterator::operator*() { return cc->car->eval(); }

}
