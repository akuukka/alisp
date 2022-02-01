#pragma once
#include <cstdint>
#include <limits>
#include <type_traits>
#include <optional>

namespace alisp {

struct FArgs;

template < template <typename...> class Template, typename T >
struct IsInstantiationOf : std::false_type {};

template < template <typename...> class Template, typename... Args >
struct IsInstantiationOf< Template, Template<Args...> > : std::true_type {};




template <typename T>
struct OptCheck : std::false_type
{
    using BaseType = T;
};

template <typename T>
struct OptCheck<std::optional<T>> : std::true_type
{
    using BaseType = T;
};



template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
{
    using result_type = ReturnType;
    using arg_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);
};

template <typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)>
{
    using result_type = Ret;
    using arg_tuple = std::tuple<Args...>;
    static constexpr auto arity = sizeof...(Args);
};

template <class F, std::size_t ... Is, class T>
auto lambda_to_func_impl(F f, std::index_sequence<Is...>, T) {
    return std::function<typename T::result_type(std::tuple_element_t<Is, typename T::arg_tuple>...)>(f);
}

template <class F>
auto lambda_to_func(F f) {
    using traits = function_traits<F>;
    return lambda_to_func_impl(f, std::make_index_sequence<traits::arity>{}, traits{});
}


template<int N, typename... Ts>
using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

template <size_t I, typename... Args>
inline typename std::enable_if<I == sizeof...(Args), size_t>::type countNonOpts()
{
    return 0;
}

template <size_t I, typename... Args>
inline typename std::enable_if<I < sizeof...(Args), size_t>::type countNonOpts()
{
    using ThisType = NthTypeOf<I, Args...>;
    if (OptCheck<ThisType>::value) {
        return 0;
    }
    if constexpr (std::is_same_v<FArgs&, ThisType>) {
        static_assert(I + 1 == sizeof...(Args), "Rest function params must be last");
        return 0;
    }
    return 1 + countNonOpts<I+1, Args...>();
}

template <size_t I, typename... Args>
inline typename std::enable_if<I == sizeof...(Args), size_t>::type countMaxArgs()
{
    return sizeof...(Args);
}

template <size_t I, typename... Args>
inline typename std::enable_if<I < sizeof...(Args), size_t>::type countMaxArgs()
{
    using ThisType = NthTypeOf<I, Args...>;
    if (std::is_same<FArgs&, ThisType>::value) {
        return static_cast<size_t>(std::numeric_limits<std::int32_t>::max());
    }
    return countMaxArgs<I+1, Args...>();
}

}
