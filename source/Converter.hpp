#pragma once
#include <optional>
#include <variant>
#include "Template.hpp"

namespace alisp
{

struct Object;

template <typename T>
std::optional<T> getValue(const Object &sym);

template <typename T, typename T2 = void>
struct Converter
{
    std::optional<T> operator()(const Object& obj)
    {
        return getValue<T>(obj);
    }
};

template <typename T>
struct Converter<T, typename std::enable_if<IsInstantiationOf<std::variant, T>::value>::type>
{
    template<size_t I>
    typename std::enable_if<I == std::variant_size_v<T>, void>::type tryGetValue(const Object& o,
                                                                                 std::optional<T>& v)
    {
        
    }

    template<size_t I>
    typename std::enable_if<I < std::variant_size_v<T>, void>::type tryGetValue(const Object& o,
                                                                                std::optional<T>& v)
    {
        using TT = std::variant_alternative_t<I, T>;
        std::optional<TT> opt = getValue<TT>(o);
        if (opt) {
            T variant;
            variant = *opt;
            v = variant;
            return;
        }
        tryGetValue<I+1>(o, v);
    }
    
        std::optional<T> operator()(const Object& obj)
    {
        std::optional<T> t;
        tryGetValue<0>(obj, t);
        return t;
    }
};

}
